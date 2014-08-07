//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mkcomp" functionality of the "mk" tool.
 *
 *  Run 'mkcomp --help' for command-line options and usage help.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <iostream>
#include "Args.h"
#include "LegatoObjectModel.h"
#include "Parser.h"
#include "mkcomp.h"
#include "ComponentBuilder.h"
#include "Utilities.h"


/// Object that stores build parameters that we gather.
/// This is passed to the Builder objects when they are created.
static legato::BuildParams_t BuildParams;

/// The root object for the object model.
static legato::Component Component;



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

    // Lambda function that gets called once for each occurence of the --cflags (or -C)
    // argument on the command line.
    auto cFlagsPush = [&](const char* arg) { cFlags += " ";  cFlags += arg; };

    // Lambda function that gets called once for each occurence of the --ldflags (or -L)
    // argument on the command line.
    auto ldFlagsPush = [&](const char* arg) { ldFlags += " ";  ldFlags += arg; };

    // Lambda functions for handling arguments that can appear more than once on the
    // command line.
    auto interfaceDirPush = [&](const char* path)
        {
            BuildParams.AddInterfaceDir(path);
        };
    auto componentDirPush = [&](const char* path)
        {
            BuildParams.AddComponentDir(path);
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
                             "Add a directory to the component search path (same as -s).",
                             componentDirPush);

    le_arg_AddMultipleString('s',
                             "source-search",
                             "Add a directory to the source search path (same as -c).",
                             componentDirPush);

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

    // Add the current working directory to the list of component search directories and the
    // list of interface search directories.
    BuildParams.AddComponentDir(".");
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
    BuildParams.LinkerFlags(ldFlags);
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
    // Create a Component Builder object and use it to build the component.
    ComponentBuilder_t componentBuilder(BuildParams);
    componentBuilder.Build(Component);
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

    Build();
}
