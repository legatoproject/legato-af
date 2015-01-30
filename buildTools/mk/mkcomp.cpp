//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkcomp" functionality of the "mk" tool.
 *
 *  Run 'mkcomp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mkcomp.h"
#include "ComponentBuilder.h"
#include "InterfaceBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Builder objects when they are created.
static legato::BuildParams_t BuildParams;

/// The one and only Component object.
static legato::Component Component;

/// true if interface instance libraries should be built and linked with the component library
/// so that the component library can be linked to and used in more traditional ways, without
/// the use of mkexe or mkapp.
static bool IsStandAlone = false;


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
    bool isVerbose = false;

    // Full path of the library file to be generated. "" = use default file name.
    std::string buildOutputPath = "";

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
    auto cFlagsPush = [&](const char* arg) { cFlags += " ";  cFlags += arg; };

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
            BuildParams.AddInterfaceDir(path);
        };
    auto sourceDirPush = [&](const char* path)
        {
            BuildParams.AddSourceDir(path);
        };

    // Lambda function that gets called once for each occurence of a component path on the
    // command line.
    auto componentPathSet = [&](const char* param)
    {
        static bool matched = false;
        if (matched)
        {
            throw legato::Exception("Only one component allowed. First is '" + Component.Path()
                                    + "'.  Second is '" + param + "'.");
        }
        matched = true;

        Component.Path(param);
    };

    // Register all our arguments with the argument parser.
    le_arg_AddOptionalString(&buildOutputPath,
                             "",
                             'o',
                             "output-path",
                             "Specify the complete path name of the component library to be built.");

    le_arg_AddOptionalString(&libOutputDir,
                             ".",
                             'l',
                             "lib-output-dir",
                             "Specify the directory into which any generated runtime libraries"
                             " should be put.  (This option ignored if -o specified.)");

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

    le_arg_AddOptionalFlag(&IsStandAlone,
                           'a',
                           "stand-alone",
                           "Create IPC interface instance libraries for APIs required by"
                           " the component and link the component library with those interface"
                           " libraries, so that the component library can be loaded and run"
                           " without the help of mkexe or mkapp.  This is useful when integrating"
                           " with third-party code that uses some other build system." );


    // Any remaining parameters on the command-line are treated as a component path.
    // Note: there should only be one.
    le_arg_SetLooseParamHandler(componentPathSet);

    // Scan the arguments now.
    le_arg_Scan(argc, argv);

    // Were we given a component?
    if (Component.Name() == "")
    {
        throw std::runtime_error("A component must be supplied on the command line.");
    }

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
    if (buildOutputPath != "")
    {
        Component.Lib().BuildOutputPath(buildOutputPath);
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
    legato::parser::ParseComponent(&Component, BuildParams);
}



//--------------------------------------------------------------------------------------------------
/**
 * Build the component.
 *
 * @note This builds only the component's .so file.  Fully qualified names of interfaces cannot
 *       be determined until the component is built into an executable, so the interface code
 *       cannot be generated at this time.
 */
//--------------------------------------------------------------------------------------------------
static void Build
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create a Component Builder object and use it to build the component library.
    ComponentBuilder_t componentBuilder(BuildParams);
    componentBuilder.Build(Component, BuildParams.ObjOutputDir());
}



//--------------------------------------------------------------------------------------------------
/**
 * Build the component so it can be used as a regular library.
 *
 * @note This builds interface instance libraries for all of the interfaces required or provided
 *       by the component, then builds the component's .so file, linking it with the interface
 *       instance libraries.  Initialization of the interfaces must be done manually using their
 *       ConnectService() or AdvertiseService() functions.  Initialization of the library itself
 *       must also be done manually.  The COMPONENT_INIT function will NOT be called automatically.
 */
//--------------------------------------------------------------------------------------------------
static void BuildStandAlone
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create an Interface Builder object.
    InterfaceBuilder_t interfaceBuilder(BuildParams);

    // Build the IPC API libs.
    for (auto& mapEntry : Component.ProvidedApis())
    {
        auto& interface = mapEntry.second;

        // We want the generated code and other intermediate output files to go into a separate
        // interface-specific directory to avoid confusion.
        interfaceBuilder.Build(interface, legato::CombinePath(BuildParams.ObjOutputDir(),
                                                              interface.InternalName()));

        // Add the interface instance library to the list of libraries to link the component
        // library with.
        Component.AddRequiredLib(interface.Lib().ShortName());
    }

    for (auto& mapEntry : Component.RequiredApis())
    {
        auto& interface = mapEntry.second;

        if (!interface.TypesOnly()) // If only using types, don't need a library.
        {
            // We want the generated code and other intermediate output files to go into a separate
            // interface-specific directory to avoid confusion.
            interfaceBuilder.Build(interface, legato::CombinePath(BuildParams.ObjOutputDir(),
                                                                  interface.InternalName()));

            // Add the interface instance library to the list of libraries to link the component
            // library with.
            Component.AddRequiredLib(interface.Lib().ShortName());
        }
    }

    // Build the component library.
    Build();
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

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    mk::SetTargetSpecificEnvVars(BuildParams.Target());

    ConstructObjectModel();

    if (IsStandAlone)
    {
        BuildStandAlone();
    }
    else
    {
        Build();
    }
}
