//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkapp" functionality of the "mk" tool.
 *
 *  Run 'mkapp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    bool isVerbose = false;

    // Path to the directory where intermediate build output files (such as generated
    // source code and object code files) should be put.
    std::string objectFilesDir;

    std::string cFlags;    // C compiler flags.
    std::string cxxFlags;  // C++ compiler flags.
    std::string ldFlags;   // Linker flags.

    // Lambda function that gets called once for each occurence of the --append-to-version (or -a)
    // argument on the command line.
    auto versionPush = [&](const char* arg) { VersionSuffix += arg; };

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg) { cFlags += " ";  cFlags += arg; };

    // Lambda function that gets called for each occurence of the --cxxflags, (or -X) argument on
    // the command line.
    auto cxxFlagsPush = [&](const char* arg) { cxxFlags += " ";  cxxFlags += arg; };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg) { ldFlags += " ";  ldFlags += arg; };

    // Lambda function that gets called once for each occurence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            BuildParams.AddInterfaceDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of the source search path
    // argument on the command line.
    auto sourcePathPush = [&](const char* path)
        {
            BuildParams.AddSourceDir(legato::DoEnvVarSubstitution(path));
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
                             "(DEPRECATED) Add a directory to the source search path (same as -s).",
                             sourcePathPush);

    le_arg_AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path.",
                             sourcePathPush);

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

    le_arg_AddMultipleString('X',
                             "cxxflags",
                             "Specify extra flags to be passed to the C++ compiler.",
                             cxxFlagsPush);

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

    // Add the directory containing the .adef file to the list of source search directories
    // and the list of interface search directories.
    std::string appDefFileDir = legato::GetContainingDir(App.DefFilePath());
    BuildParams.AddSourceDir(appDefFileDir);
    BuildParams.AddInterfaceDir(appDefFileDir);

    // Store other build params specified on the command-line.
    if (isVerbose)
    {
        BuildParams.SetVerbose();
    }
    BuildParams.SetTarget(target);
    BuildParams.CCompilerFlags(cFlags);
    BuildParams.CxxCompilerFlags(cxxFlags);
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
