//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkcomp" functionality of the "mk" tool.
 *
 *  Run 'mkcomp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include <string.h>
#include <unistd.h>

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{



/// Object that stores build parameters that we gather.
static mk::BuildParams_t BuildParams;

/// Path to the directory containing the component to be built.
std::string ComponentPath;

/// Full path of the library file to be generated. "" = use default file name.
std::string BuildOutputPath = "";

// true if the build.ninja file should be ignored and everything should be regenerated, including
// a new build.ninja.
static bool DontRunNinja = false;

/// Steps required to build a component for Linux
static generator::ComponentGenerator_t LinuxSteps[] =
{
    code::GenerateInterfacesHeader,
    code::GenerateLinuxComponentMainFile,
    ninja::GenerateLinux,
    NULL
};

/// All supported OS types, and the steps needed to build them.
static const std::map<std::string, generator::ComponentGenerator_t*> OSTypeSteps
{
    std::pair<std::string, generator::ComponentGenerator_t*> { "linux", LinuxSteps },
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
    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [](const char* arg)
    {
        BuildParams.cFlags += " ";
        BuildParams.cFlags += arg;
    };

    // Lambda function that gets called for each occurence of the --cxxflags, (or -X) argument on
    // the command line.
    auto cxxFlagsPush = [](const char* arg)
    {
        BuildParams.cxxFlags += " ";
        BuildParams.cxxFlags += arg;
    };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [](const char* arg)
    {
        BuildParams.ldFlags += " ";
        BuildParams.ldFlags += arg;
    };

    // Lambda functions for handling arguments that can appear more than once on the
    // command line.
    auto interfaceDirPush = [](const char* path)
    {
        BuildParams.interfaceDirs.push_back(path);
    };

    auto sourceDirPush = [](const char* path)
    {
        // In order to preserve original command line functionality, we push this new path into
        // all of the various search paths.
        BuildParams.moduleDirs.push_back(path);
        BuildParams.appDirs.push_back(path);
        BuildParams.componentDirs.push_back(path);
        BuildParams.sourceDirs.push_back(path);
    };

    // Lambda function that gets called once for each occurence of a component path on the
    // command line.
    auto componentPathSet = [](const char* param)
    {
        static bool matched = false;
        if (matched)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Only one component allowed. First is '%s'.  Second is '%s'."),
                           ComponentPath, param)
            );
        }
        matched = true;

        ComponentPath = param;
    };

    // Register all our arguments with the argument parser.
    args::AddOptionalString(&BuildOutputPath,
                             "",
                             'o',
                             "output-path",
                            LE_I18N("Specify the complete path name of the component library"
                                    " to be built."));

    args::AddOptionalString(&BuildParams.libOutputDir,
                             ".",
                             'l',
                             "lib-output-dir",
                            LE_I18N("Specify the directory into which any generated runtime"
                                    " libraries should be put. "
                                    " (This option ignored if -o specified.)"));

    args::AddOptionalString(&BuildParams.workingDir,
                             "_build",
                             'w',
                             "object-dir",
                            LE_I18N("Specify the directory into which any intermediate build"
                                    " artifacts (such as .o files and generated source code files)"
                                    " should be put."));

    args::AddOptionalString(&BuildParams.debugDir,
                            "",
                            'd',
                            "debug-dir",
                            LE_I18N("Generate debug symbols and place them in the specified"
                                    " directory.  Debug symbol files will be named with build-id"));

    args::AddOptionalString(&BuildParams.target,
                             "localhost",
                             't',
                             "target",
                            LE_I18N("Specify the target device to build for"
                                    " (e.g., localhost or ar7)."));

    args::AddMultipleString('i',
                            "interface-search",
                            LE_I18N("Add a directory to the interface search path."),
                            interfaceDirPush);

    args::AddMultipleString('c',
                            "component-search",
                            LE_I18N("(DEPRECATED) Add a directory to the source search path"
                                    " (same as -s)."),
                            sourceDirPush);

    args::AddMultipleString('s',
                            "source-search",
                            LE_I18N("Add a directory to the source search path."),
                            sourceDirPush);

    args::AddOptionalFlag(&BuildParams.beVerbose,
                          'v',
                          "verbose",
                          LE_I18N("Set into verbose mode for extra diagnostic information."));

    args::AddOptionalInt(&BuildParams.jobCount,
                         0,
                         'j',
                         "jobs",
                         LE_I18N("Run N jobs in parallel (default derived from CPUs available)"));

    args::AddMultipleString('C',
                            "cflags",
                            LE_I18N("Specify extra flags to be passed to the C compiler."),
                            cFlagsPush);

    args::AddMultipleString('X',
                            "cxxflags",
                            LE_I18N("Specify extra flags to be passed to the C++ compiler."),
                            cxxFlagsPush);

    args::AddMultipleString('L',
                            "ldflags",
                            LE_I18N("Specify extra flags to be passed to the linker when linking "
                                    "executables."),
                            ldFlagsPush);

    args::AddOptionalFlag(&BuildParams.isStandAloneComp,
                          'a',
                          "stand-alone",
                          LE_I18N("Build the component library and all its sub-components'"
                                  " libraries such that the component library can be loaded and run"
                                  " without the help of mkexe or mkapp.  This is useful when"
                                  " integrating with third-party code that is built using some"
                                  " other build system."));

    args::AddOptionalFlag(&DontRunNinja,
                          'n',
                          "dont-run-ninja",
                          LE_I18N("Even if a build.ninja file exists, ignore it, parse all inputs,"
                                  " and generate all output files, including a new copy of the"
                                  " build.ninja, then exit without running ninja.  This is used by"
                                  " the build.ninja to to regenerate itself and any other files"
                                  " that need to be regenerated when the build.ninja finds itself"
                                  " out of date."));

    args::AddOptionalFlag(&BuildParams.codeGenOnly,
                          'g',
                          "generate-code",
                          LE_I18N("Only generate code, but don't compile or link anything."
                                  " The interface definition (include) files will be generated,"
                                  " along with component main files."
                                  " This is useful for supporting context-sensitive auto-complete"
                                  " and related features in source code editors, for example."));

    // Any remaining parameters on the command-line are treated as a component path.
    // Note: there should only be one.
    args::SetLooseArgHandler(componentPathSet);

    // Scan the arguments now.
    args::Scan(argc, argv);

    // Tell build params configuration is finished.
    BuildParams.FinishConfig();

    // Were we given a component?
    if (ComponentPath == "")
    {
        throw mk::Exception_t(LE_I18N("A component must be supplied on the command line."));
    }

    // Add the current working directory to the list of source search directories and the
    // list of interface search directories.
    BuildParams.moduleDirs.push_back(".");
    BuildParams.appDirs.push_back(".");
    BuildParams.componentDirs.push_back(".");
    BuildParams.sourceDirs.push_back(".");
    BuildParams.interfaceDirs.push_back(".");

    // Add the Legato framework's "interfaces" directory to the list of interface search dirs.
    BuildParams.interfaceDirs.push_back(path::Combine(envVars::Get("LEGATO_ROOT"), "interfaces"));
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkcomp functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeComponent
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    GetCommandLineArgs(argc, argv);

    BuildParams.argc = argc;
    BuildParams.argv = argv;

    // Get tool chain info from environment variables.
    // (Must be done after command-line args parsing and before setting target-specific env vars.)
    FindToolChain(BuildParams);

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    envVars::SetTargetSpecific(BuildParams);

    // If we have not been asked to ignore any already existing build.ninja, and the command-line
    // arguments and environment variables we were given are the same as last time, just run ninja.
    if (!DontRunNinja)
    {
        if (args::MatchesSaved(BuildParams) && envVars::MatchesSaved(BuildParams))
        {
            RunNinja(BuildParams);
            // NOTE: If build.ninja exists, RunNinja() will not return.  If it doesn't it will.
        }
        // If we have not been asked to ignore any already existing build.ninja and there has
        // been a change in either the argument list or the environment variables,
        // save the command-line arguments and environment variables for future comparison.
        // Note: we don't need to do this if we have been asked not to run ninja, because
        // that only happens when ninja is already running and asking us to regenerate its
        // script for us, and that only happens if the args and env vars have already been saved.
        else
        {
            // Save the command line arguments.
            args::Save(BuildParams);

            // Save the environment variables.
            // Note: we must do this before we parse the definition file, because parsing the file
            // will result in the CURDIR environment variable being set.
            envVars::Save(BuildParams);
        }
    }

    // Locate the component.
    auto foundPath = file::FindComponent(ComponentPath, BuildParams.componentDirs);
    if (foundPath == "")
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Couldn't find component '%s'."), ComponentPath)
        );
    }
    ComponentPath = path::MakeAbsolute(foundPath);

    // Generate the conceptual object model.
    model::Component_t* componentPtr = modeller::GetComponent(ComponentPath, BuildParams);

    // Set build output depending on build type
    if (!BuildOutputPath.empty())
    {
        if (BuildParams.osType == "linux")
        {
            // Add Linux info
            componentPtr->SetTargetInfo(
                new target::LinuxComponentInfo_t(componentPtr, BuildParams));

            componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib = BuildOutputPath;
        }
        else if (BuildParams.osType == "rtos")
        {
            // Add RTOS info
            componentPtr->SetTargetInfo(new target::RtosComponentInfo_t(componentPtr, BuildParams));

            componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->staticlib = BuildOutputPath;
        }
    }

    // Run all steps to generate a component
    generator::RunAllGenerators(OSTypeSteps, componentPtr, BuildParams);

    // If we haven't been asked not to, run ninja.
    if (!DontRunNinja)
    {
        RunNinja(BuildParams);
    }
}


} // namespace cli
