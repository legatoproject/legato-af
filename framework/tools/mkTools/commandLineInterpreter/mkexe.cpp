//--------------------------------------------------------------------------------------------------
/**
 * @file mkexe.cpp  Implements the "mkexe" functionality of the "mk" tool.
 *
 * Run 'mkexe --help' for command-line options and usage help.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{


/// Object that stores build parameters that we gather.
static mk::BuildParams_t BuildParams;

/// List of names of content items (specified on the command line) that are to be
/// included in this executable.   These could be source file names, component names,
/// or library names.
static std::list<std::string> ContentNames;

/// Path to the executable to be built.
static std::string ExePath;

/// Object representing the executable we are building.
static model::Exe_t* ExePtr = NULL;

// true if the build.ninja file should be ignored and everything should be regenerated, including
// a new build.ninja.
static bool DontRunNinja = false;


/// Steps to run to generate a Linux executable
static generator::ExeGenerator_t LinuxSteps[] =
{
    generator::ForAllComponents<GenerateLinuxCode>,
    code::GenerateLinuxExeMain,
    ninja::GenerateLinux,
    NULL
};

/// Steps to run to generate a RTOS "executable"
static generator::ExeGenerator_t RtosSteps[] =
{
    generator::ForAllComponents<GenerateRtosCode>,
    code::GenerateRtosExeMain,
    ninja::GenerateRtos,
    NULL
};

/// List of steps for each OS type
static const std::map<std::string, generator::ExeGenerator_t*> OSTypeSteps
{
    std::pair<std::string, generator::ExeGenerator_t*> { "linux", LinuxSteps },
    std::pair<std::string, generator::ExeGenerator_t*> { "rtos", RtosSteps }
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
    // Lambda function that gets called once for each occurrence of the --cflags (or -C)
    // option on the command line.
    auto cFlagsPush = [&](const char* arg)
        {
            BuildParams.cFlags += " ";
            BuildParams.cFlags += arg;
        };

    // Lambda function that gets called for each occurrence of the --cxxflags, (or -X) option on
    // the command line.
    auto cxxFlagsPush = [&](const char* arg)
        {
            BuildParams.cxxFlags += " ";
            BuildParams.cxxFlags += arg;
        };

    // Lambda function that gets called once for each occurrence of the --ldflags (or -L)
    // option on the command line.
    auto ldFlagsPush = [&](const char* arg)
        {
            BuildParams.ldFlags += " ";
            BuildParams.ldFlags += arg;
        };

    // Lambda function that gets called once for each occurrence of the --interface-search (or -i)
    // option on the command line.
    auto interfaceDirPush = [&](const char* path)
        {
            BuildParams.interfaceDirs.push_back(path);
        };

    // Lambda function that gets called once for each occurrence of the --source-search (or -s)
    // option on the command line.
    auto sourceDirPush = [&](const char* path)
        {
            // In order to preserve original command line functionality, we push this new path into
            // all of the various search paths.
            BuildParams.moduleDirs.push_back(path);
            BuildParams.appDirs.push_back(path);
            BuildParams.componentDirs.push_back(path);
            BuildParams.sourceDirs.push_back(path);
        };

    // Lambda function that gets called once for each content item on the command line.
    auto contentPush = [&](const char* param)
        {
            ContentNames.push_back(param);
        };

    // Register all the command-line options with the argument parser.
    args::AddString(&ExePath,
                    'o',
                    "output",
                    LE_I18N("The path of the executable file to generate."));

    args::AddOptionalString(&BuildParams.libOutputDir,
                            ".",
                            'l',
                            "lib-output-dir",
                            LE_I18N("Specify the directory into which any generated runtime"
                                    " libraries should be put."));

    args::AddOptionalString(&BuildParams.workingDir,
                            "./_build",
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
                            LE_I18N("Specify the target device to build for (localhost | ar7)."));

    args::AddOptionalString(&BuildParams.osType,
                            "linux",
                            'T',
                            "os-type",
                            LE_I18N("Specify the OS type to build for."
                                    "  Options are: linux (default) or rtos."));

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
                                  " along with component and executable main files."
                                  " This is useful for supporting context-sensitive auto-complete"
                                  " and related features in source code editors, for example."));

    args::AddOptionalFlag(&BuildParams.noPie,
                          'p',
                          "no-pie",
                          LE_I18N("Do not build executable as a position independent executable."));

    // Any remaining parameters on the command-line are treated as content items to be included
    // in the executable.
    args::SetLooseArgHandler(contentPush);

    // Scan the arguments now.
    args::Scan(argc, argv);

    // Tell build params configuration is finished.
    BuildParams.FinishConfig();

    // Add the current working directory to the list of source search directories and the
    // list of interface search directories.
    BuildParams.moduleDirs.push_back(".");
    BuildParams.appDirs.push_back(".");
    BuildParams.componentDirs.push_back(".");
    BuildParams.sourceDirs.push_back(".");
    BuildParams.interfaceDirs.push_back(".");

    // Make the ExePath absolute, if it isn't already.
    ExePath = path::MakeAbsolute(ExePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a component's .cdef, construct a conceptual model for the component and add it to
 * the exe.
 */
//--------------------------------------------------------------------------------------------------
static void AddComponentToExe
(
    const std::string& componentPath
)
//--------------------------------------------------------------------------------------------------
{
    // Get the component object.
    model::Component_t* componentPtr = modeller::GetComponent(componentPath, BuildParams);

    // Add an instance of the component to the executable.
    modeller::AddComponentInstance(ExePtr, componentPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove the executable name and component name parts from the service instance names of all
 * IPC API interfaces (both client and server).
 */
//--------------------------------------------------------------------------------------------------
static void MakeAllInterfacesExternal
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    for (auto componentInstancePtr : ExePtr->componentInstances)
    {
        for (auto ifInstancePtr : componentInstancePtr->clientApis)
        {
            ifInstancePtr->name = ifInstancePtr->ifPtr->internalName;
        }
        for (auto ifInstancePtr : componentInstancePtr->serverApis)
        {
            ifInstancePtr->name = ifInstancePtr->ifPtr->internalName;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check that there's at least one source code file in the executable.
 *
 * @throw mk::Exception_t if there are no source code files in the executable.
 */
//--------------------------------------------------------------------------------------------------
static void VerifyAtLeastOneSourceFile
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Check for C or C++ source files being built directly into the exe (outside of components).
    if (   (ExePtr->hasCOrCppCode == false)
        && (ExePtr->hasPythonCode == false)
        && (ExePtr->hasJavaCode == false))
    {
        throw mk::Exception_t(LE_I18N("Executable doesn't contain any source code files."));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Identify content items and construct the object model.
 */
//--------------------------------------------------------------------------------------------------
static void ConstructObjectModel
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    bool errorFound = false;
    std::string componentPath;

    ExePtr = new model::Exe_t(ExePath, NULL, BuildParams.workingDir);

    if (BuildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Making executable '%s'"), ExePtr->path)
                  << std::endl;
    }

    // For each item of content, we have to figure out what type of content it is and
    // handle it accordingly.
    for (auto const &contentName: ContentNames)
    {
        // Is it a C source code file path?
        if (path::IsCSource(contentName))
        {
            if (BuildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("Adding C source file '%s' to executable."),
                                        contentName)
                          << std::endl;
            }

            auto sourceFilePath = file::FindFile(contentName, BuildParams.sourceDirs);
            if (sourceFilePath == "")
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Can't find file: '%s'."), contentName)
                );
            }
            sourceFilePath = path::MakeAbsolute(sourceFilePath);

            // Compute the relative directory
            auto objFilePath = "obj/" + md5(path::MakeCanonical(sourceFilePath)) + ".o";

            // Create an object file object for this source file.
            auto objFilePtr = new model::ObjectFile_t(objFilePath, sourceFilePath);

            // Add the object file to the exe's list of C object files.
            ExePtr->AddCObjectFile(objFilePtr);
        }
        // Is it a C++ source code file path?
        else if (path::IsCxxSource(contentName))
        {
            if (BuildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("Adding C++ source file '%s' to executable."),
                                        contentName)
                          << std::endl;
            }

            auto sourceFilePath = file::FindFile(contentName, BuildParams.sourceDirs);
            if (sourceFilePath == "")
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Can't find file: '%s'."), contentName)
                );
            }
            sourceFilePath = path::MakeAbsolute(sourceFilePath);

            // Compute the relative directory
            auto objFilePath = "obj/" + md5(path::MakeCanonical(sourceFilePath)) + ".o";

            // Create an object file object for this source file.
            auto objFilePtr = new model::ObjectFile_t(objFilePath, sourceFilePath);

            // Add the object file to the exe's list of C++ object files.
            ExePtr->AddCppObjectFile(objFilePtr);
        }
        // Is it a library file path?
        else if (path::IsLibrary(contentName))
        {
            if (BuildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("Adding library '%s' to executable."), contentName)
                          << std::endl;
            }

            BuildParams.ldFlags += " " + contentName;
        }
        // Is it a path to a component directory?
        else if ((componentPath = file::FindComponent(contentName, BuildParams.componentDirs)) != "")
        {
            componentPath = path::MakeAbsolute(componentPath);

            if (BuildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("Adding component '%s' to executable."),
                                        componentPath)
                          << std::endl;
            }

            AddComponentToExe(componentPath);
        }
        // It's none of the above.
        else
        {
            std::cerr << mk::format(LE_I18N("** ERROR: Couldn't identify content item '%s'."),
                                    contentName)
                      << std::endl;

            std::cerr << LE_I18N("Searched in the following locations:") << std::endl;
            for (auto const &path : BuildParams.sourceDirs)
            {
                std::cerr << "    " << path << std::endl;
            }

            errorFound = true;
        }
    }

    if (errorFound)
    {
        throw mk::Exception_t(LE_I18N("Unable to identify one or more requested content items."));
    }

    // Make all interfaces "external", because the executable is outside of any app.
    // Effectively, this means remove the "exe.component." prefix from the service instance
    // names of all interfaces.
    MakeAllInterfacesExternal();

    // Check that there's at least one source code file in the executable.
    VerifyAtLeastOneSourceFile();
}


//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkexe functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeExecutable
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

    ConstructObjectModel();

    // Run appropriate generator
    generator::RunAllGenerators(OSTypeSteps, ExePtr, BuildParams);

    // If we haven't been asked not to, run ninja.
    if (!DontRunNinja)
    {
        RunNinja(BuildParams);
    }
}


} // namespace cli
