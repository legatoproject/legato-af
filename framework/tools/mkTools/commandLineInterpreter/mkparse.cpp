
//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <string>
#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{



using DefType_t = parseTree::DefFile_t::Type_t;



using DefTypeMap_t = std::unordered_map<std::string, DefType_t>;



struct CommandArgs_t
{
    mk::BuildParams_t params;
    std::string defFilePath;
    DefType_t defType;
};




//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static std::string ExtensionsToStr
(
    const DefTypeMap_t& allowedExtensions
)
//--------------------------------------------------------------------------------------------------
{
    std::string extensionStr = " ";

    for (const auto& extension : allowedExtensions)
    {
        extensionStr += extension.first + " ";
    }

    return extensionStr;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
[[noreturn]]
static void ThrowMissingDefinitionError
(
    const DefTypeMap_t& allowedExtensions
)
//--------------------------------------------------------------------------------------------------
{
    throw mk::Exception_t(mk::format(LE_I18N("A definition is required to parse.  "
                                             "Allowed definition files: (%s)."),
                                     ExtensionsToStr(allowedExtensions)));
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
[[noreturn]]
static void ThrowMultipleDefinitionError
(
    const std::string& filePath
)
//--------------------------------------------------------------------------------------------------
{
    throw mk::Exception_t(mk::format(LE_I18N("Only one definition (.sdef) file allowed.  "
                                             "Duplicate definition found: %s."),
                                     filePath));
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
[[noreturn]]
static void ThrowUnrecognizedExtension
(
    const DefTypeMap_t& allowedExtensions,
    const std::string& foundPath
)
//--------------------------------------------------------------------------------------------------
{
    throw mk::Exception_t(mk::format(LE_I18N("Unrecognized file extension on file %s.  "
                                             "Allowed extensions: (%s)"),
                                     foundPath,
                                     ExtensionsToStr(allowedExtensions)));
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static DefType_t TestDefinitionExtension
(
    const DefTypeMap_t& allowedExtensions,
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    const auto extension = path::GetFileNameExtension(path);
    const auto found = allowedExtensions.find(extension);

    if (found == allowedExtensions.end())
    {
        ThrowUnrecognizedExtension(allowedExtensions, path);
    }

    return found->second;
}



//--------------------------------------------------------------------------------------------------
/**
 * Parse the command-line arguments and update the static operating parameters variables.
 *
 * Throws a std::runtime_error exception on failure.
 **/
//--------------------------------------------------------------------------------------------------
static CommandArgs_t GetCommandLineArgs
(
    int argc,
    const char** argv
)
//--------------------------------------------------------------------------------------------------
{
    static const DefTypeMap_t allowedExtensions =
        {
            { ".cdef", DefType_t::CDEF },
            { ".adef", DefType_t::ADEF },
            { ".sdef", DefType_t::SDEF },
            { ".mdef", DefType_t::MDEF }
        };

    mk::BuildParams_t buildParams;
    std::string sourceDefinition;
    DefType_t sourceType;

    buildParams.argc = argc;
    buildParams.argv = argv;

    args::AddOptionalString(&buildParams.outputDir,
                            ".",
                            'o',
                            "output-dir",
                            LE_I18N("Specify the directory into which the generated json model "
                                    "should be written.  Specify a dash, -, to write to standard "
                                    "out instead."));

    args::AddMultipleString('i',
                            "interface-search",
                            LE_I18N("Add a directory to the interface search path."),
                            [&buildParams](const char* arg)
                            {
                                buildParams.interfaceDirs.push_back(path::MakeAbsolute(arg));
                            });

    args::AddOptionalString(&buildParams.target,
                            "localhost",
                            't',
                            "target",
                            LE_I18N("Set the compile target (e.g., localhost or ar7)."));

    args::AddMultipleString('s',
                            "source-search",
                            LE_I18N("Add a directory to the source search path."),
                            [&buildParams](const char* arg)
                            {
                                std::string path = path::MakeAbsolute(arg);

                                // In order to preserve original command line functionality, we push
                                // this new path into all of the various search paths.
                                buildParams.moduleDirs.push_back(path);
                                buildParams.appDirs.push_back(path);
                                buildParams.componentDirs.push_back(path);
                                buildParams.sourceDirs.push_back(path);
                            });

    args::AddMultipleString('C',
                            "cflags",
                            LE_I18N("Specify extra flags to be passed to the C compiler."),
                            [&buildParams](const char* arg)
                            {
                                buildParams.cFlags += " ";
                                buildParams.cFlags += arg;
                            });

    args::AddMultipleString('X',
                            "cxxflags",
                            LE_I18N("Specify extra flags to be passed to the C++ compiler."),
                            [&buildParams](const char* arg)
                            {
                                buildParams.cxxFlags += " ";
                                buildParams.cxxFlags += arg;
                            });

    args::AddMultipleString('L',
                            "ldflags",
                            LE_I18N("Specify extra flags to be passed to the linker when linking "
                                    "executables."),
                            [&buildParams](const char* arg)
                            {
                                buildParams.ldFlags += " ";
                                buildParams.ldFlags += arg;
                            });

    args::SetLooseArgHandler([&](const char* arg)
        {
            const std::string path(arg);

            if (sourceDefinition != "")
            {
                ThrowMultipleDefinitionError(path);
            }

            sourceType = TestDefinitionExtension(allowedExtensions, path);
            sourceDefinition = path::MakeAbsolute(path);
        });

    // Process the command-line arguments and our registered call-backs will handle the details.
    args::Scan(argc, argv);


    // Make sure we were given a source file to process.
    if (sourceDefinition.empty())
    {
        sourceDefinition = envVars::Get("LEGATO_DEF_FILE");

        if (sourceDefinition.empty())
        {
            ThrowMissingDefinitionError(allowedExtensions);
        }

        sourceType = TestDefinitionExtension(allowedExtensions, sourceDefinition);
        sourceDefinition = path::MakeAbsolute(sourceDefinition);
    }


    // Normalize the output directory and return our completed arguments structure.
    buildParams.outputDir = path::MakeAbsolute(buildParams.outputDir);

    // Add the directory containing the .sdef file to the list of source search directories
    // and the list of interface search directories.
    std::string defFileDir = path::GetContainingDir(sourceDefinition);
    buildParams.moduleDirs.push_back(defFileDir);
    buildParams.appDirs.push_back(defFileDir);
    buildParams.componentDirs.push_back(defFileDir);
    buildParams.sourceDirs.push_back(defFileDir);
    buildParams.interfaceDirs.push_back(defFileDir);

    return { buildParams, sourceDefinition, sourceType };
}



//--------------------------------------------------------------------------------------------------
/**
 * Load the requested model object and model it in JSON.
 */
//--------------------------------------------------------------------------------------------------
template <typename modelTypePtr, typename buildParamType>
void GenerateJsonModel
(
    std::ostream& out,
    CommandArgs_t& processedArgs,
    modelTypePtr (*getter)(const std::string&, buildParamType&)
)
//--------------------------------------------------------------------------------------------------
{
    json::GenerateModel(out,
                        getter(processedArgs.defFilePath, processedArgs.params),
                        processedArgs.params);
}



//--------------------------------------------------------------------------------------------------
/**
 * Implements the mkparse functionality.
 */
//--------------------------------------------------------------------------------------------------
void MakeParsedModel
(
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    // Process our command line arguments and figure out what the user is asking us to do.
    auto processedArgs = GetCommandLineArgs(argc, argv);

    FindToolChain(processedArgs.params);
    mk::BuildParams_t& buildParams = processedArgs.params;
    //buildParams.readOnly = true;

    // Set the target-specific environment variables (e.g., LEGATO_TARGET).
    envVars::SetTargetSpecific(buildParams);

    std::ostream& output = std::cout;

    switch (processedArgs.defType)
    {
        case DefType_t::CDEF:
            GenerateJsonModel(output, processedArgs, modeller::GetComponent);
            break;

        case DefType_t::ADEF:
            //GenerateJsonModel(output, processedArgs, modeller::GetApp);
            break;

        case DefType_t::SDEF:
            GenerateJsonModel(output, processedArgs, modeller::GetSystem);
            break;

        case DefType_t::MDEF:
            GenerateJsonModel(output, processedArgs, modeller::GetModule);
            break;
    }
}


}
