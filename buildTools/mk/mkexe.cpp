//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkexe" functionality of the "mk" tool.
 *
 *  Run 'mkexe --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mkexe.h"
#include "InterfaceBuilder.h"
#include "ComponentBuilder.h"
#include "ComponentInstanceBuilder.h"
#include "ExecutableBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Component Builder and the Executable Builder objects
/// when they are created.
static legato::BuildParams_t BuildParams;

/// Path to the executable to be built.
static std::string ExePath;

/// List of names of content items (specified on the command line) that are to be
/// included in this executable.   These could be source file names, component names,
/// or library names.
static std::list<std::string> ContentNames;

/// The root object for the object model.
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
    // The target device (e.g., "ar7").
    std::string target = "localhost";

    // true = say what we are doing on stdout.
    bool isVerbose = false;

    // Path to the directory where generated runtime libs should be put.
    std::string libOutputDir = ".";

    // Path to the directory where intermediate build output files (such as generated
    // source code and object code files) should be put.
    std::string objOutputDir = ".";

    std::string cFlags;    // C compiler flags.
    std::string cxxFlags;  // C++ compiler flags.
    std::string ldFlags;   // Linker flags.

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg) { cFlags += " "; cFlags += arg; };

    // Lambda function that gets called for each occurence of the --cxxflags, (or -X) argument on
    // the command line.
    auto cxxFlagsPush = [&](const char* arg) { cxxFlags += " ";  cxxFlags += arg; };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg) { ldFlags += " ";  ldFlags += arg; };

    // Lambda functions for handling arguments that can appear more than once on the
    // command line.
    auto interfaceDirPush = [&](const char* path)
        {
            BuildParams.AddInterfaceDir(legato::DoEnvVarSubstitution(path));
        };
    auto sourceDirPush = [&](const char* path)
        {
            BuildParams.AddSourceDir(legato::DoEnvVarSubstitution(path));
        };
    auto contentPush = [&](const char* param)
        {
            ContentNames.push_back(legato::DoEnvVarSubstitution(param));
        };

    // Register all our arguments with the argument parser.
    le_arg_AddString(&ExePath,
                     'o',
                     "output",
                     "The path of the executable file to generate.");

    le_arg_AddOptionalString(&libOutputDir,
                             ".",
                             'l',
                             "lib-output-dir",
                             "Specify the directory into which any generated runtime libraries"
                             " should be put.");

    le_arg_AddOptionalString(&objOutputDir,
                             "./_build",
                             'w',
                             "object-dir",
                             "Specify the directory into which any intermediate build artifacts"
                             " (such as .o files and generated source code files) should be put.");

    le_arg_AddOptionalString(&target,
                             "localhost",
                             't',
                             "target",
                             "Specify the target device to build for (localhost | ar7).");

    le_arg_AddMultipleString('i',
                             "interface-search",
                             "Add a directory to the interface search path.",
                             interfaceDirPush);

    le_arg_AddMultipleString('c',
                             "component-search",
                             "(DEPRECATED) Add a directory to the source search path (same as -s).",
                             sourceDirPush);

    le_arg_AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path.",
                             sourceDirPush);

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


    // Any remaining parameters on the command-line are treated as content items to be included
    // in the executable.
    le_arg_SetLooseParamHandler(contentPush);

    // Scan the arguments now.
    le_arg_Scan(argc, argv);

    // Add the current working directory to the list of source search directories and the
    // list of interface search directories.
    BuildParams.AddSourceDir(".");
    BuildParams.AddInterfaceDir(".");

    // Store other build params specified on the command-line.
    if (isVerbose)
    {
        BuildParams.SetVerbose();
    }
    BuildParams.SetTarget(target);
    BuildParams.LibOutputDir(libOutputDir);
    BuildParams.ObjOutputDir(objOutputDir);
    BuildParams.CCompilerFlags(cFlags);
    BuildParams.CxxCompilerFlags(cxxFlags);
    BuildParams.LinkerFlags(ldFlags);
}



//--------------------------------------------------------------------------------------------------
/**
 * Turn all of the executable's interfaces into external interfaces, using the internal name
 * as the external name.
 **/
//--------------------------------------------------------------------------------------------------
static void MakeAllInterfacesExternal
(
    legato::Executable& exe
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& instance : exe.ComponentInstances())
    {
        for (auto& mapEntry : instance.RequiredApis())
        {
            auto& interface = mapEntry.second;

            interface.MakeExternalToApp(interface.InternalName());
        }

        for (auto& mapEntry : instance.ProvidedApis())
        {
            auto& interface = mapEntry.second;

            interface.MakeExternalToApp(interface.InternalName());
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Identify content items and construct the object model.
 */
//--------------------------------------------------------------------------------------------------
static legato::Executable& ConstructObjectModel
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    bool errorFound = false;

    // Create a new Executable object.
    legato::Executable& exe = App.CreateExecutable(ExePath);

    if (BuildParams.IsVerbose())
    {
        std::cout << "Making executable '" << exe.OutputPath() << "'" << std::endl
                  << "\t(using exe name '" << exe.CName() << "')." << std::endl;
    }

    // For each item of content, we have to figure out what type of content it is and
    // handle it accordingly.
    for (auto contentName: ContentNames)
    {
        if (legato::IsCSource(contentName))
        {
            if (BuildParams.IsVerbose())
            {
                std::cout << "Adding C source file '" << contentName << "' to executable."
                          << std::endl;
            }

            // Add the source code file to the default component.
            exe.AddSourceFile(legato::FindFile(contentName, BuildParams.SourceDirs()));
        }
        else if (legato::IsCxxSource(contentName))
        {
            if (BuildParams.IsVerbose())
            {
                std::cout << "Adding C++ source file '" << contentName << "' to executable."
                          << std::endl;
            }

            // Add the source code file to the default component.
            exe.AddSourceFile(legato::FindFile(contentName, BuildParams.SourceDirs()));
        }
        else if (legato::IsLibrary(contentName))
        {
            if (BuildParams.IsVerbose())
            {
                std::cout << "Adding library '" << contentName << "' to executable." << std::endl;
            }

            // Add the library file to the list of libraries to be linked with the default
            // component.
            exe.AddLibrary(contentName);
        }
        else if (legato::IsComponent(contentName, BuildParams.SourceDirs()))
        {
            if (BuildParams.IsVerbose())
            {
                std::cout << "Adding component '" << contentName << "' to executable." << std::endl;
            }

            // Find the component and add it to the executable's list of component instances.
            // NOTE: For now, we only support one instance of a component per executable, and it is
            //       identified by the file system path to that component (relative to a directory
            //       somewhere in the source search path).
            legato::parser::AddComponentToExe(&App, &exe, contentName, BuildParams);
        }
        else
        {
            std::cerr << "*** ERROR: Couldn't identify content item '"
                      << contentName << "'." << std::endl;

            std::cerr << "Searched in the followind locations:" << std::endl;
            for (auto path : BuildParams.SourceDirs())
            {
                std::cerr << "    " << path << std::endl;
            }

            errorFound = true;
        }
    }

    if (errorFound)
    {
        throw legato::Exception("Unable to identify one or more requested content items.");
    }

    // Make all interfaces external, because the executable is outside of any app.
    MakeAllInterfacesExternal(exe);

    return exe;
}



//--------------------------------------------------------------------------------------------------
/**
 * Build a component and all its sub-components.
 **/
//--------------------------------------------------------------------------------------------------
static void BuildComponent
(
    legato::Component& component,
    ComponentBuilder_t componentBuilder,
    const std::string& workingDir
)
//--------------------------------------------------------------------------------------------------
{
    if (component.IsBuilt() == false)
    {
        // Do sub-components first.
        for (auto& mapEntry : component.SubComponents())
        {
            auto subComponentPtr = mapEntry.second;

            BuildComponent(*subComponentPtr, componentBuilder, workingDir);
        }

        // Each component gets its own object file dir.
        std::string objOutputDir = legato::CombinePath(workingDir, "component/" + component.Name());

        // Build the component.
        // NOTE: This will detect if the component doesn't actually need to be built, either because
        //       it doesn't have any source files that need to be compiled, or because they have
        //       already been compiled.
        componentBuilder.Build(component, objOutputDir);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Build the executable.
 */
//--------------------------------------------------------------------------------------------------
static void Build
(
    legato::Executable& exe
)
//--------------------------------------------------------------------------------------------------
{
    // Build all the components.
    // NOTE: This has to be done recursively, with sub-components first, so that components can
    //       be linked with the libraries from their sub-components.
    ComponentBuilder_t componentBuilder(BuildParams);
    for (auto& mapEntry : legato::Component::GetComponentMap())
    {
        legato::Component& component = mapEntry.second;

        BuildComponent(component, componentBuilder, BuildParams.ObjOutputDir());
    };

    // Build the executable.
    ExecutableBuilder_t exeBuilder(BuildParams);
    exeBuilder.GenerateMain(exe, BuildParams.ObjOutputDir());
    exeBuilder.Build(exe, BuildParams.ObjOutputDir());
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

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    Build(ConstructObjectModel());
}
