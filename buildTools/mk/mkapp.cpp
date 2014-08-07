//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkapp" functionality of the "mk" tool.
 *
 *  Run 'mkapp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/sendfile.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mkapp.h"
#include "ApplicationBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Component Builder and the Executable Builder objects
/// when they are created.
static legato::BuildParams_t BuildParams;

/// Path to the directory into which the final, built application file should be placed.
static std::string OutputDir;

/// Suffix to append to the application version.
static std::string VersionSuffix;

/// The root object for this application's object model.
static legato::App App;

/// Map of process names to debug port numbers.
static std::map<std::string, uint16_t> DebugProcMap;

/// Set of debug port numbers that are in use.  This is used to check for duplicates,
/// since multiple processes can't share the same port number.
static std::set<uint16_t> DebugPortSet;



//--------------------------------------------------------------------------------------------------
/**
 * Process a --debug (-g) command-line argument.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleDebugArg
(
    void*       contextPtr, ///< Not used.
    const char* procAndPort ///< [in] Optional string containing a process name and a port number,
                            ///       separated by a colon (e.g., "foo:1234").
)
//--------------------------------------------------------------------------------------------------
{
    // Put the app into debug mode.
    App.SetDebug();

    // If the argument is not empty, then parse out the process name and the port number and
    // store them in the DebugProcessMap.
    std::string arg(procAndPort);
    if (!arg.empty())
    {
        size_t colonPos = arg.find(':');
        if ((colonPos == arg.npos) || (colonPos == 0))
        {
            std::cerr << "**ERROR: Invalid content for --debug (-g) argument.  Expected process"
                         " name and port number, separated by a colon (:)." << std::endl;
            exit(EXIT_FAILURE);
        }

        std::string procName = arg.substr(0, colonPos);

        std::string portNumStr(arg.substr(colonPos + 1));
        std::istringstream portNumStream(portNumStr);
        long int portNum;
        portNumStream >> portNum;   // Convert the string into a number.
        // If the conversion failed or there are unconverted bytes left at the end, then
        // it's an error.
        if (portNumStream.fail() || (portNumStream.eof() == false))
        {
            std::cerr << "**ERROR: Invalid port number '" << portNumStr
                      << "' for --debug (-g) argument." << std::endl;
            exit(EXIT_FAILURE);
        }
        // Port numbers must be greater than zero and no larger than 64K.
        if ((portNum <= 0) || (portNum > 65535))
        {
            std::cerr << "**ERROR: Port number " << portNum
                      << " out of range in --debug (-g) argument." << std::endl;
            exit(EXIT_FAILURE);
        }
        // Port numbers lower than 1024 are reserved for special services, like HTTP, NFS, etc.
        if (portNum < 1024)
        {
            std::cerr << "WARNING: Port number (" << portNum
                      << ") less than 1024 in --debug (-g) argument may not be permitted"
                         " by the target OS." << std::endl;
        }

        DebugProcMap[procName] = portNum;

        // Check for duplicate port numbers.
        if (DebugPortSet.insert(portNum).second == false)
        {
            std::cerr << "**ERROR: Debug port number " << portNum << " is used more than once."
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};


//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments and update the static operating parameters variables.
 *
 * Throws a std::runtime_error exception on failure.
 **/
//--------------------------------------------------------------------------------------------------
static void GetCommandLineArgs
(
    int argc,
    const char** argv
)
//--------------------------------------------------------------------------------------------------
{
    std::string target;
    int isVerbose = 0;  // 0 = not verbose, non-zero = verbose.

    // Path to the directory where intermediate build output files (such as generated
    // source code and object code files) should be put.
    std::string objectFilesDir;

    std::string cFlags;  // C compiler flags.
    std::string ldFlags; // Linker flags.

    // Lambda function that gets called once for each occurence of the --append-to-version (or -a)
    // argument on the command line.
    auto versionPush = [&](const char* arg) { VersionSuffix += arg; };

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg) { cFlags += " ";  cFlags += arg; };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg) { ldFlags += " ";  ldFlags += arg; };

    // Lambda function that gets called once for each occurence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            BuildParams.AddInterfaceDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of the source/component search path
    // argument on the command line.
    auto compPathPush = [&](const char* path)
        {
            BuildParams.AddComponentDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of a .adef file name on the
    // command line.
    auto adefFileNameSet = [&](const char* param)
            {
                static bool matched = false;
                if (matched)
                {
                    throw legato::Exception("Only one app definition (.adef) file allowed.");
                }
                matched = true;
                App.DefFilePath(legato::DoEnvVarSubstitution(param));
            };

    // TODO: Change the Args module to display a "USAGE: ..." line and optionally, a SYNOPSIS.
    // TODO: le_arg_AddHelpSynopsis("Blah blah blah...");

    le_arg_AddMultipleString('a',
                             "append-to-version",
                             "Specify a suffix to append to the application version specified"
                             " in the .adef file.  Will automatically insert a '.' between the"
                             " .adef's version string and any version strings specified on the"
                             " command-line.  Multiple occurences of this argument will be"
                             " combined into a single string.",
                             versionPush);

    le_arg_AddOptionalString(&OutputDir,
                             ".",
                             'o',
                             "output-dir",
                             "Specify the directory into which the final, built application file"
                             "(ready to be installed on the target) should be put.");

    le_arg_AddOptionalString(&objectFilesDir,
                             "",
                             'w',
                             "object-dir",
                             "Specify the directory into which any intermediate build artifacts"
                             " (such as .o files and generated source code files) should be put.");

    le_arg_AddMultipleString('i',
                             "interface-search",
                             "Add a directory to the interface search path.",
                             ifPathPush);

    le_arg_AddMultipleString('c',
                             "component-search",
                             "Add a directory to the component search path (same as -s).",
                             compPathPush);

    le_arg_AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path (same as -c).",
                             compPathPush);

    le_arg_AddMultipleString('g',
                             "debug",
                             "Start the app in debug mode and specify a process to debug and a port"
                             " number for gdbserver to use.  e.g., '--debug=myProc:1234'.",
                             HandleDebugArg,
                             NULL);

    le_arg_AddOptionalString(&target,
                             "localhost",
                             't',
                             "target",
                             "Set the compile target (localhost|ar7).");

    le_arg_AddOptionalFlag(&isVerbose,
                           'v',
                           "verbose",
                           "Set into verbose mode for extra diagnostic information.");

    le_arg_AddMultipleString('C',
                             "cflags",
                             "Specify extra flags to be passed to the C compiler.",
                             cFlagsPush);

    le_arg_AddMultipleString('L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables.",
                             ldFlagsPush);

    // Any remaining parameters on the command-line are treated as the .adef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    le_arg_SetLooseParamHandler(adefFileNameSet);

    le_arg_Scan(argc, argv);

    // Were we given an application definition?
    if (App.DefFilePath() == "")
    {
        throw std::runtime_error("An application definition must be supplied.");
    }

    // If we were not given an object file directory (intermediate build output directory) path,
    // use a subdirectory of the current working directory.
    if (objectFilesDir == "")
    {
        objectFilesDir = "./_build_" + App.Name() + "/" + target;
    }
    BuildParams.ObjOutputDir(objectFilesDir);
    BuildParams.StagingDir(legato::CombinePath(objectFilesDir, "staging"));

    // Add the directory containing the .adef file to the list of component search directories
    // and the list of interface search directories.
    std::string appDefFileDir = legato::GetContainingDir(App.DefFilePath());
    BuildParams.AddComponentDir(appDefFileDir);
    BuildParams.AddInterfaceDir(appDefFileDir);

    // Store other build params specified on the command-line.
    if (isVerbose)
    {
        BuildParams.SetVerbose();
    }
    BuildParams.SetTarget(target);
    BuildParams.CCompilerFlags(cFlags);
    BuildParams.LinkerFlags(ldFlags);
}



//--------------------------------------------------------------------------------------------------
/**
 * Construct the object model.
 */
//--------------------------------------------------------------------------------------------------
static void ConstructObjectModel
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Parse the .adef file and any Component.cdef files that it refers to.
    // This constructs the object model under the App object that we give it.
    legato::parser::ParseApp(&App, BuildParams);

    // Append the version suffix to the App's version.
    if (VersionSuffix != "")
    {
        // If the app has a version string already (from the .adef file), then make sure there's
        // a '.' separating the .adef version from the suffix.
        if (App.Version() != "")
        {
            if ((App.Version().back() != '.') && (VersionSuffix.front() != '.'))
            {
                App.Version() += ".";
            }
        }

        App.Version() += VersionSuffix;
    }

    // For every process in every process environment in the app, check if debugging has been
    // enabled (via command-line arguments), and if so, store the port number in the Process
    // object and flag the Executable object to be built for debugging.
    for (auto& procEnv : App.ProcEnvironments())
    {
        for (auto& process : procEnv.ProcessList())
        {
            const auto debugMapIter = DebugProcMap.find(process.Name());
            if (debugMapIter != DebugProcMap.end())
            {
                if (BuildParams.IsVerbose())
                {
                    std::cout << "Process '" << process.Name() << "' will be started in debug mode"
                              " using port number " << debugMapIter->second << "." << std::endl;
                }

                process.EnableDebugging(debugMapIter->second);

                auto exeMapIter = App.Executables().find(process.ExePath());
                if (exeMapIter != App.Executables().end())
                {
                    if (BuildParams.IsVerbose())
                    {
                        std::cout << "Executable '" << process.ExePath() << "' will be built for"
                                  " debugging (debug symbols included and optimization turned off)."
                                  << std::endl;
                    }

                    exeMapIter->second.EnableDebugging();
                }
                else
                {
                    std::cerr << "WARNING: executable '" << process.ExePath()
                              << "' is not built by this tool.  Please ensure that it has been"
                              " built with debug symbols." << std::endl;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Build the application.
 */
//--------------------------------------------------------------------------------------------------
static void Build
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create an Application Builder object.
    ApplicationBuilder_t appBuilder(BuildParams);

    // Tell it to build the app.
    appBuilder.Build(App, OutputDir);
}



//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkapp functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeApp
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    ConstructObjectModel();

    Build();
}
