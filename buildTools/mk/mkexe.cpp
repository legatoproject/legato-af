//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkexe" functionality of the "mk" tool.
 *
 *  Run 'mkexe --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mkexe.h"
#include "ComponentBuilder.h"
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

    // Non-zero = say what we are doing on stdout.
    int isVerbose = 0;

    // Path to the directory where generated runtime libs should be put.
    std::string libOutputDir = ".";

    // Path to the directory where intermediate build output files (such as generated
    // source code and object code files) should be put.
    std::string objOutputDir = ".";

    std::string cFlags;  // C compiler flags.
    std::string ldFlags; // Linker flags.

    // Lambda functions for handling arguments that can appear more than once on the
    // command line.
    auto interfaceDirPush = [&](const char* path) { BuildParams.AddInterfaceDir(path); };
    auto componentDirPush = [&](const char* path) { BuildParams.AddComponentDir(path); };
    auto contentPush = [&](const char* param) { ContentNames.push_back(param); };

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
                             "Add a directory to the component search path.",
                             componentDirPush);

    le_arg_AddOptionalFlag(&isVerbose,
                           'v',
                           "verbose",
                           "Set into verbose mode for extra diagnostic information.");

    le_arg_AddOptionalString(&cFlags,
                             "",
                             'C',
                             "cflags",
                             "Specify extra flags to be passed to the C compiler.");

    le_arg_AddOptionalString(&ldFlags,
                             "",
                             'L',
                             "ldflags",
                             "Specify extra flags to be passed to the linker when linking "
                             "executables and libraries.");


    // Any remaining parameters on the command-line are treated as content items to be included
    // in the executable.
    le_arg_SetLooseParamHandler(contentPush);

    // Scan the arguments now.
    le_arg_Scan(argc, argv);

    // Add the current working directory to the list of component search directories and the
    // list of interface search directories.
    BuildParams.AddComponentDir(".");
    BuildParams.AddInterfaceDir(".");

    // Add the Legato framework include directory to the include path so people don't have
    // to keep doing it themselves.
    BuildParams.AddInterfaceDir("$LEGATO_ROOT/framework/c/inc");

    // Store other build params specified on the command-line.
    if (isVerbose)
    {
        BuildParams.SetVerbose();
    }
    BuildParams.SetTarget(target);
    BuildParams.LibOutputDir(libOutputDir);
    BuildParams.ObjOutputDir(objOutputDir);
    BuildParams.CCompilerFlags(cFlags);
    BuildParams.LinkerFlags(ldFlags);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a Component object for a given component path.
 *
 * @return reference to the Component object.
 */
//--------------------------------------------------------------------------------------------------
static legato::Component& GetComponent
(
    const std::string& name     ///< Name of the component.
)
//--------------------------------------------------------------------------------------------------
{
    auto& map = App.ComponentMap();

    // Check if we have already parsed a component with the same name.
    // If so, it's an error!
    if (map.find(name) != map.end())
    {
        throw legato::Exception("Multiple components with the same name (" + name + ").");
    }

    // Tell the App to create a new Component object for us and then get the parser to populate it.
    legato::Component& component = App.CreateComponent(name);
    legato::parser::ParseComponent(component, BuildParams);

    return component;
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
                  << "\t(using exe name '" << exe.CName() << "')" << std::endl
                  << "\tcontaining:" << std::endl;
    }

    // For each item of content, we have to figure out what type of content it is and
    // handle it accordingly.
    for (auto contentName: ContentNames)
    {
        const char* contentType;

        if (legato::IsCSource(contentName))
        {
            contentType = "C source code";

            // Add the source code file to the default component.
            exe.AddCSourceFile(legato::FindFile(contentName, BuildParams.ComponentDirs()));
        }
        else if (legato::IsLibrary(contentName))
        {
            contentType = "library";

            // Add the library file to the list of libraries to be linked with the default
            // component.
            exe.AddLibrary(contentName);
        }
        else if (legato::IsComponent(contentName, BuildParams.ComponentDirs()))
        {
            contentType = "component";

            // Find the component and add it to the executable's list of component instances.
            // NOTE: For now, we only support one instance of a component per executable, and it is
            //       identified by the file system path to that component (relative to a directory
            //       somewhere in the component search path).
            legato::Component& component = GetComponent(contentName);
            exe.AddComponentInstance(legato::ComponentInstance(component));
        }
        else
        {
            contentType = "** unknown **";

            std::cerr << "** ERROR: Couldn't identify content item '"
                      << contentName << "'." << std::endl;

            errorFound = true;
        }

        if (BuildParams.IsVerbose())
        {
            std::cout << "\t\t'" << contentName << "' (" << contentType << ")" << std::endl;
        }
    }

    if (errorFound)
    {
        throw std::runtime_error("Unable to identify requested content.");
    }

    return exe;
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
    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    // Auto-generate the source code file containing main() and add it to the default component.
    ExecutableBuilder_t exeBuilder(BuildParams);
    exeBuilder.GenerateMain(exe);

    // Build all the components.
    ComponentBuilder_t componentBuilder(BuildParams);
    for (auto componentInstance : exe.ComponentInstanceList())
    {
        // Generate the IPC import/export code.
        componentBuilder.GenerateInterfaceCode(componentInstance.GetComponent());

        // Build the component.
        componentBuilder.Build(componentInstance.GetComponent());
    }

    // Do the final build step for the executable.
    // Note: All the components need to be built before this.
    exeBuilder.Build(exe);
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

    Build(ConstructObjectModel());
}
