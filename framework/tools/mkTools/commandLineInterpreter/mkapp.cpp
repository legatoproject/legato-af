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

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{



/// Object that stores build parameters that we gather.
/// This is passed to the Component Builder and the Executable Builder objects
/// when they are created.
static mk::BuildParams_t BuildParams;

/// Path to the directory into which the final, built application file should be placed.
static std::string OutputDir;

/// Suffix to append to the application version.
static std::string VersionSuffix;

/// Path to the application's .adef file.
static std::string AdefFilePath;

/// The application's name.
static std::string AppName;

/// Pointer to the application object.
static model::App_t* AppPtr;

// true if the build.ninja file should be ignored and everything should be regenerated, including
// a new build.ninja.
static bool DontRunNinja = false;


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
    // Lambda function that gets called once for each occurence of the --append-to-version (or -a)
    // argument on the command line.
    auto versionPush = [&](const char* arg)
        {
            VersionSuffix += arg;
        };

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg)
        {
            BuildParams.cFlags += " ";
            BuildParams.cFlags += arg;
        };

    // Lambda function that gets called for each occurence of the --cxxflags, (or -X) argument on
    // the command line.
    auto cxxFlagsPush = [&](const char* arg)
        {
            BuildParams.cxxFlags += " ";
            BuildParams.cxxFlags += arg;
        };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg)
        {
            BuildParams.ldFlags += " ";
            BuildParams.ldFlags += arg;
        };

    // Lambda function that gets called once for each occurence of the interface search path
    // argument on the command line.
    auto ifPathPush = [&](const char* path)
        {
            BuildParams.interfaceDirs.push_back(path);
        };

    // Lambda function that gets called once for each occurence of the source search path
    // argument on the command line.
    auto sourcePathPush = [&](const char* path)
        {
            BuildParams.sourceDirs.push_back(path);
        };

    // Lambda function that gets called once for each occurence of a .adef file name on the
    // command line.
    auto adefFileNameSet = [&](const char* param)
        {
            if (AdefFilePath != "")
            {
                throw mk::Exception_t("Only one app definition (.adef) file allowed.");
            }
            AdefFilePath = param;
        };

    args::AddMultipleString('a',
                            "append-to-version",
                            "Specify a suffix to append to the application version specified"
                            " in the .adef file.  Will automatically insert a '.' between the"
                            " .adef's version string and any version strings specified on the"
                            " command-line.  Multiple occurences of this argument will be"
                            " combined into a single string.",
                            versionPush);

    args::AddOptionalString(&OutputDir,
                            ".",
                            'o',
                            "output-dir",
                            "Specify the directory into which the final, built application file"
                            "(ready to be installed on the target) should be put.");

    args::AddOptionalString(&BuildParams.workingDir,
                            "",
                            'w',
                            "object-dir",
                            "Specify the directory into which any intermediate build artifacts"
                            " (such as .o files and generated source code files) should be put.");

    args::AddMultipleString('i',
                            "interface-search",
                            "Add a directory to the interface search path.",
                            ifPathPush);

    args::AddMultipleString('c',
                            "component-search",
                            "(DEPRECATED) Add a directory to the source search path (same as -s).",
                            sourcePathPush);

    args::AddMultipleString('s',
                            "source-search",
                            "Add a directory to the source search path.",
                            sourcePathPush);

    args::AddOptionalString(&BuildParams.target,
                            "localhost",
                            't',
                            "target",
                            "Set the compile target (localhost|ar7).");

    args::AddOptionalFlag(&BuildParams.beVerbose,
                          'v',
                          "verbose",
                          "Set into verbose mode for extra diagnostic information.");

    args::AddMultipleString('C',
                            "cflags",
                            "Specify extra flags to be passed to the C compiler.",
                            cFlagsPush);

    args::AddMultipleString('X',
                            "cxxflags",
                            "Specify extra flags to be passed to the C++ compiler.",
                            cxxFlagsPush);

    args::AddMultipleString('L',
                            "ldflags",
                            "Specify extra flags to be passed to the linker when linking "
                            "executables.",
                            ldFlagsPush);

    args::AddOptionalFlag(&DontRunNinja,
                           'n',
                           "dont-run-ninja",
                           "Even if a build.ninja file exists, ignore it, delete the staging area,"
                           " parse all inputs, and generate all output files, including a new copy"
                           " of the build.ninja, then exit without running ninja.  This is used by"
                           " the build.ninja to to regenerate itself and any other files that need"
                           " to be regenerated when the build.ninja finds itself out of date.");

    args::AddOptionalFlag(&BuildParams.codeGenOnly,
                          'g',
                          "generate-code",
                          "Only generate code, but don't compile, link, or bundle anything."
                          " The interface definition (include) files will be generated, along"
                          " with component and executable main files and configuration files."
                          " This is useful for supporting context-sensitive auto-complete and"
                          " related features in source code editors, for example.");

    // Any remaining parameters on the command-line are treated as the .adef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    args::SetLooseArgHandler(adefFileNameSet);

    args::Scan(argc, argv);

    // Were we given an application definition file path?
    if (AdefFilePath == "")
    {
        throw std::runtime_error("An application definition must be supplied.");
    }

    // Make sure we have the .adef file's absolute path (for improved error reporting).
    AdefFilePath = path::MakeAbsolute(AdefFilePath);

    // Compute the app name from the .adef file path.
    AppName = path::RemoveSuffix(path::GetLastNode(AdefFilePath), ".adef");

    // If we were not given a working directory (intermediate build output directory) path,
    // use a subdirectory of the current directory, and use a different working dir for
    // different apps and for the same app built for different targets.
    if (BuildParams.workingDir == "")
    {
        BuildParams.workingDir = "./_build_" + AppName + "/" + BuildParams.target;
    }

    // Generated libraries should be put under '/lib' under the staging directory.
    BuildParams.libOutputDir = path::Combine(BuildParams.workingDir, "staging/lib");

    // Add the directory containing the .adef file to the list of source search directories
    // and the list of interface search directories.
    std::string aDefFileDir = path::GetContainingDir(AdefFilePath);
    BuildParams.sourceDirs.push_back(aDefFileDir);
    BuildParams.interfaceDirs.push_back(aDefFileDir);
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
    envVars::SetTargetSpecific(BuildParams.target);

    // If we have been asked not to run Ninja, then delete the staging area because it probably
    // will contain some of the wrong files now that .Xdef file have changed.
    if (DontRunNinja)
    {
        file::DeleteDir(path::Combine(BuildParams.workingDir, "staging"));
    }
    // If we have not been asked to ignore any already existing build.ninja, and the command-line
    // arguments and environment variables we were given are the same as last time, just run ninja.
    else if (args::MatchesSaved(BuildParams, argc, argv) && envVars::MatchesSaved(BuildParams))
    {
        RunNinja(BuildParams);
        // NOTE: If build.ninja exists, RunNinja() will not return.  If it doesn't it will.
    }

    // Construct a model of the application.
    AppPtr = modeller::GetApp(AdefFilePath, BuildParams);
    if (AppPtr->version.empty())
    {
        AppPtr->version = VersionSuffix;
    }

    // Append a "." and the VersionSuffix if the user provides a
    // "--append or -a" argument in the command line
    else if (VersionSuffix.empty() == false)
    {
        AppPtr->version += '.' + VersionSuffix;
    }

    // Ensure that all client-side interfaces have either been bound to something or declared
    // external.
    modeller::EnsureClientInterfacesSatisfied(AppPtr);

    if (BuildParams.beVerbose)
    {
        modeller::PrintSummary(AppPtr);
    }

    // Since this app is the only app, it won't be under the "apps" subdirectory,
    // reset the application's relative working directory path.
    AppPtr->workingDir = "";

    // Create the working directory, if it doesn't already exist.
    file::MakeDir(BuildParams.workingDir);

    // Generate the configuration data file.
    config::Generate(AppPtr, BuildParams);

    // Generate the build script for the application.
    ninja::Generate(AppPtr, BuildParams, OutputDir, argc, argv);

    // For each executable in the application,
    for (auto exePtr : AppPtr->executables)
    {
        // Generate _main.c.
        code::GenerateExeMain(exePtr, BuildParams);
    }

    // For each component in the application,
    for (auto componentPtr : AppPtr->components)
    {
        // Create a working directory to build the component in.
        file::MakeDir(path::Combine(BuildParams.workingDir, componentPtr->workingDir));

        // Generate a custom "interfaces.h" file for this component.
        code::GenerateInterfacesHeader(componentPtr, BuildParams);

        // Generate a custom "_componentMain.c" file for this component.
        code::GenerateComponentMainFile(componentPtr, BuildParams, false);
    }

    // Generate the manifest.app file for Air Vantage.
    airVantage::GenerateManifest(AppPtr, BuildParams);

    // If we haven't been asked not to run ninja,
    if (!DontRunNinja)
    {
        // Save the command-line arguments and environment variables for future comparison.
        // Note: we don't need to do this if we have been asked not to run ninja, because
        // that only happens when ninja is already running and asking us to regenerate its
        // script for us, and that only happens if we just saved the args and env vars and
        // ran ninja.
        args::Save(BuildParams, argc, argv);
        envVars::Save(BuildParams);

        RunNinja(BuildParams);
    }
}


} // namespace cli
