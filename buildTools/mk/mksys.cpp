//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mksys" functionality of the "mk" tool.
 *
 *  Run 'mksys --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
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
#include "mksys.h"
#include "InterfaceBuilder.h"
#include "ComponentBuilder.h"
#include "ExecutableBuilder.h"
#include "ApplicationBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Builder objects when they are created.
static legato::BuildParams_t BuildParams;

/// Path to the directory into which the final, built system file should be placed.
static std::string OutputDir;

/// The root object for this system's object model.
static legato::System System;



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

    std::string cFlags;  // C compiler flags.
    std::string ldFlags; // Linker flags.

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

    // Lambda function that gets called once for each occurence of the source search path
    // argument on the command line.
    auto sourcePathPush = [&](const char* path)
        {
            BuildParams.AddSourceDir(legato::DoEnvVarSubstitution(path));
        };

    // Lambda function that gets called once for each occurence of a .sdef file name on the
    // command line.
    auto sdefFileNameSet = [&](const char* param)
            {
                static bool matched = false;
                if (matched)
                {
                    throw legato::Exception("Only one system definition (.sdef) file allowed.");
                }
                matched = true;
                System.DefFilePath(legato::DoEnvVarSubstitution(param));
            };

    le_arg_AddOptionalString(&OutputDir,
                             ".",
                             'o',
                             "output-dir",
                             "Specify the directory into which the final, built system file"
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

    le_arg_AddMultipleString('L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables.",
                             ldFlagsPush);

    // Any remaining parameters on the command-line are treated as the .sdef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    le_arg_SetLooseParamHandler(sdefFileNameSet);

    le_arg_Scan(argc, argv);

    // Were we given an system definition?
    if (System.DefFilePath() == "")
    {
        throw std::runtime_error("A system definition must be supplied.");
    }

    // If we were not given an object file directory (intermediate build output directory) path,
    // use a subdirectory of the current working directory.
    if (objectFilesDir == "")
    {
        objectFilesDir = "./_build_" + System.Name() + "/" + target;
    }
    BuildParams.ObjOutputDir(objectFilesDir);

    // Add the directory containing the .sdef file to the list of source search directories
    // and the list of interface search directories.
    std::string systemDefFileDir = legato::GetContainingDir(System.DefFilePath());
    BuildParams.AddSourceDir(systemDefFileDir);
    BuildParams.AddInterfaceDir(systemDefFileDir);

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
 * Generate a configuration file for settings that are outside of any single app.
 * E.g., user-to-user bindings or user-to-app bindings appear in the /users branch of the system
 * configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSystemConfig
(
    const std::string& stagingDirPath
)
//--------------------------------------------------------------------------------------------------
{
    // Open the bindings file for writing
    std::string path = stagingDirPath + "/bindings";

    if (BuildParams.IsVerbose())
    {
        std::cout << "Writing non-app bindings to file '" << path << "'."
                  << std::endl;
    }

    std::ofstream cfgStream(path, std::ofstream::trunc);

    // For each binding in the System object's list,
    for (auto bindIter : System.ApiBinds())
    {
        auto& bind = bindIter.second;

        // If the client is a non-app user,
        // Write an entry into the bindings file for this binding.
        if (! bind.IsClientAnApp())
        {
            cfgStream << '<' << bind.ClientUserName() << ">."
                      << bind.ClientInterfaceName() << " -> ";

            if (bind.IsServerAnApp())
            {
                cfgStream << bind.ServerAppName() << ".";
            }
            else
            {
                cfgStream << "<" << bind.ServerUserName() << ">.";
            }

            cfgStream << bind.ServerInterfaceName() << std::endl;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Build the system.
 */
//--------------------------------------------------------------------------------------------------
static void Build
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Construct the working directory structure, which consists of an "obj" directory and
    // a "staging" directory.  Application bundles will be put inside the "staging" directory.
    // The "staging" directory will get tarred to become the actual system bundle.
    // The "obj" directory is for intermediate build output, like generated .c
    // files and .o files.  Under the "obj" directory each app has its own subdirectory to work in.
    if (BuildParams.IsVerbose())
    {
        std::cout << "Creating working directories under '" << BuildParams.ObjOutputDir() << "'."
                  << std::endl;
    }
    std::string objDirPath = BuildParams.ObjOutputDir() + "/obj";
    std::string stagingDirPath = BuildParams.ObjOutputDir() + "/staging";

    // Clean the staging area.
    legato::CleanDir(stagingDirPath);

    // Create the staging and working directories.
    legato::MakeDir(objDirPath);
    legato::MakeDir(stagingDirPath);

    // For each app in the system,
    for (auto& mapEntry : System.Apps())
    {
        auto& app = mapEntry.second;

        // Create an Application Builder object to use to build this app.
        // Give it the appropriate build parameters.
        legato::BuildParams_t appBuildParams(BuildParams);
        appBuildParams.ObjOutputDir(legato::CombinePath(objDirPath, app.Name()));
        appBuildParams.StagingDir(legato::CombinePath(appBuildParams.ObjOutputDir(), "staging"));
        ApplicationBuilder_t appBuilder(appBuildParams);

        // Build the app.  This should result in an application bundle appearing in the
        // staging directory.
        appBuilder.Build(app, stagingDirPath);
    }

    // TODO: Copy in metadata for use by Developer Studio.

    // Generate a configuration data file containing user-to-app and user-to-user bindings.
    GenerateSystemConfig(stagingDirPath);

    // Create the tarball file name.
    std::string outputPath = legato::CombinePath(OutputDir, System.Name());
    outputPath += "." + BuildParams.Target() + "_sys";  // Add the file name extension.
    if (!legato::IsAbsolutePath(outputPath))
    {
        outputPath = legato::GetWorkingDir() + "/" + outputPath;
    }

    // Create the tarball.
    std::string tarCommandLine = "tar cf \"" + outputPath + "\" -C \"" + stagingDirPath + "\" .";
    if (BuildParams.IsVerbose())
    {
        std::cout << "Packaging system into '" << outputPath << "'." << std::endl;
        std::cout << std::endl << "$ "<< tarCommandLine << std::endl << std::endl;
    }
    mk::ExecuteCommandLine(tarCommandLine);
}



//--------------------------------------------------------------------------------------------------
/**
 * Implements the mksys functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeSystem
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    // Parse the .sdef file, populating the System object with the results.
    legato::parser::ParseSystem(&System, BuildParams);

    Build();
}
