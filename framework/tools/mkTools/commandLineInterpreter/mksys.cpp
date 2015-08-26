//--------------------------------------------------------------------------------------------------
/**
 *  Implements the "mksys" functionality of the "mk" tool.
 *
 *  Run 'mksys --help' for command-line options and usage help.
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
/// This is passed to the Builder objects when they are created.
static mk::BuildParams_t BuildParams;

/// Path to the directory into which the final, built system file should be placed.
static std::string OutputDir;

/// The root object for this system's object model.
static std::string SdefFilePath;

/// The system's name.
static std::string SystemName;


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
    auto cFlagsPush = [&](const char* arg)
        {
            BuildParams.cFlags += " ";
            BuildParams.cFlags += arg;
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

    // Lambda function that gets called once for each occurence of a .sdef file name on the
    // command line.
    auto sdefFileNameSet = [&](const char* param)
            {
                if (SdefFilePath != "")
                {
                    throw mk::Exception_t("Only one system definition (.sdef) file allowed.");
                }
                SdefFilePath = param;
            };

    args::AddOptionalString(&OutputDir,
                           ".",
                            'o',
                            "output-dir",
                            "Specify the directory into which the final, built system file"
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

    args::AddMultipleString('s',
                            "source-search",
                            "Add a directory to the source search path.",
                            sourcePathPush);

    args::AddOptionalString(&BuildParams.target,
                            "localhost",
                            't',
                            "target",
                            "Set the compile target (e.g., localhost or ar7).");

    args::AddOptionalFlag(&BuildParams.beVerbose,
                          'v',
                          "verbose",
                          "Set into verbose mode for extra diagnostic information.");

    args::AddMultipleString('C',
                            "cflags",
                            "Specify extra flags to be passed to the C compiler.",
                            cFlagsPush);

    args::AddMultipleString('L',
                            "ldflags",
                            "Specify extra flags to be passed to the linker when linking "
                            "executables.",
                            ldFlagsPush);

    args::AddOptionalFlag(&BuildParams.codeGenOnly,
                          'g',
                          "generate-code",
                          "Only generate code, but don't compile, link, or bundle anything."
                          " The interface definition (include) files will be generated, along"
                          " with component and executable main files and configuration files."
                          " This is useful for supporting context-sensitive auto-complete and"
                          " related features in source code editors, for example.");

    // Any remaining parameters on the command-line are treated as the .sdef file path.
    // Note: there should only be one parameter not prefixed by an argument identifier.
    args::SetLooseArgHandler(sdefFileNameSet);

    args::Scan(argc, argv);

    // Were we given an system definition?
    if (SdefFilePath == "")
    {
        throw mk::Exception_t("A system definition must be supplied.");
    }

    // Compute the system name from the .sdef file path.
    SystemName = path::RemoveSuffix(path::GetLastNode(SdefFilePath), ".sdef");

    // If we were not given a working directory (intermediate build output directory) path,
    // use a subdirectory of the current directory, and use a different working dir for
    // different systems and for the same system built for different targets.
    if (BuildParams.workingDir == "")
    {
        BuildParams.workingDir = "./_build_" + SystemName + "/" + BuildParams.target;
    }

    // Add the directory containing the .sdef file to the list of source search directories
    // and the list of interface search directories.
    std::string sdefFileDir = path::GetContainingDir(SdefFilePath);
    BuildParams.sourceDirs.push_back(sdefFileDir);
    BuildParams.interfaceDirs.push_back(sdefFileDir);
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
    envVars::SetTargetSpecific(BuildParams.target);

    // Parse the .sdef file.
    parser::sdef::Parse(SdefFilePath, BuildParams.beVerbose);

    // Check that all client-side interfaces have been bound to something.
}



} // namespace cli
