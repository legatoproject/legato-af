//--------------------------------------------------------------------------------------------------
/**
 * Main file for the Component Parser.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc., Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "../ComponentModel/LegatoObjectModel.h"
#include "Parser.h"

extern "C"
{
    #include "ComponentParser.tab.h"
    #include "ParserCommonInternals.h"
    #include "ComponentParserInternals.h"
    #include <string.h>
    #include <stdio.h>
    #include "lex.cyy.h"
}


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
int cyy_IsVerbose = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Build parameters received via the command line.
 */
//--------------------------------------------------------------------------------------------------
static const legato::BuildParams_t* BuildParamsPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the Component object for the component that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::Component* ComponentPtr;


namespace legato
{

namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Parses a Component Definition (Component.cdef) and populates a Component object with the
 * information garnered.
 *
 * @note    Expects the component's name to be set.  Will search for the Component.cdef based
 *          on the source search path in the buildParams.
 */
//--------------------------------------------------------------------------------------------------
void ParseComponent
(
    Component* componentPtr,            ///< The Component object to populate.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
)
//--------------------------------------------------------------------------------------------------
{
    ComponentPtr = componentPtr;
    BuildParamsPtr = &buildParams;

    // Open the component's Component.cdef file for reading.
    std::string path = FindComponent(componentPtr->Path(), buildParams.SourceDirs());
    if (path == "")
    {
        throw Exception("Couldn't find component '" + componentPtr->Path() + "'.");
    }
    componentPtr->Path(path);

    std::string cdefFilePath = CombinePath(path, "/Component.cdef");
    FILE* file = fopen(cdefFilePath.c_str(), "r");
    if (file == NULL)
    {
        int error = errno;
        std::stringstream errorMessage;
        errorMessage << "Failed to open file '" << cdefFilePath << "'." <<
                        " Errno = " << error << "(" << strerror(error) << ").";
        throw Exception(errorMessage.str());
    }

    if (buildParams.IsVerbose())
    {
        std::cout << "Parsing '" << cdefFilePath << "'\n";
    }

    // Tell the parser to reset itself and connect to the new file stream for future parsing.
    cyy_FileName = cdefFilePath.c_str();
    cyy_IsVerbose = (BuildParamsPtr->IsVerbose() ? 1 : 0);
    cyy_EndOfFile = 0;
    cyy_ErrorCount = 0;
    cyy_set_lineno(1);
    cyy_restart(file);

    // Until the parsing is done,
    int parsingResult;
    do
    {
        // Start parsing.
        parsingResult = cyy_parse();
    }
    while ((parsingResult != 0) && (!cyy_EndOfFile));

    // Close the file
    fclose(file);

    // Halt if there were errors.
    if (cyy_ErrorCount > 0)
    {
        throw Exception("Errors encountered while parsing '" + cdefFilePath + "'.");
    }

    ComponentPtr = NULL;

    if (buildParams.IsVerbose())
    {
        std::cout << "Finished parsing '" << cdefFilePath << "'\n";
    }

    // Recursively, for each of the new component's sub-components,
    for (auto& mapEntry : componentPtr->SubComponents())
    {
        mapEntry.second = Component::FindComponent(mapEntry.first);

        // If the sub-component has not yet been parsed, create an object for it and parse it now.
        if (mapEntry.second == NULL)
        {
            mapEntry.second = Component::CreateComponent(mapEntry.first);

            ParseComponent(mapEntry.second, buildParams);
        }
    }
}


} // namespace parser

} // namespace legato

//--------------------------------------------------------------------------------------------------
// NOTE: The following functions are called from C code inside the bison-generated parser code.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Add a source code file to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddSourceFile
(
    const char* filePath    ///< The file path.  Storage will never be freed.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string path = legato::DoEnvVarSubstitution(filePath);

        // If env var substitution happened.
        if (path != filePath)
        {
            if (cyy_IsVerbose)
            {
                std::cout << "Environment variable substitution of '" << filePath
                          << "' resulted in '" << path << "'." << std::endl;
            }

            // If the result was an empty string, ignore it.
            if (path == "")
            {
                return;
            }
        }

        ComponentPtr->AddSourceFile(path);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a C compiler command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddCFlag
(
    const char* arg ///< The command-line argument.
)
//--------------------------------------------------------------------------------------------------
{
    ComponentPtr->AddCFlag(arg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a C++ compiler command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddCxxFlag
(
    const char* arg ///< The command-line argument.
)
//--------------------------------------------------------------------------------------------------
{
    ComponentPtr->AddCxxFlag(arg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a linker command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddLdFlag
(
    const char* arg ///< The command-line argument.
)
//--------------------------------------------------------------------------------------------------
{
    ComponentPtr->AddLdFlag(arg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a required file to a Component.  This is a file that is expected to exist outside the
 * application's sandbox in the target file system and that the component needs access to.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredFile
(
    const char* sourcePath, ///< The file path in the target file system, outside sandbox.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        ComponentPtr->AddRequiredFile(yy_CreateRequiredFileMapping(sourcePath,
                                                                   destPath,
                                                                   *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a required directory to a Component.  This is a directory that is expected to exist outside
 * the application's sandbox in the target file system and that the component needs access to.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredDir
(
    const char* sourcePath, ///< The directory path in the target file system, outside sandbox.
    const char* destPath    ///< The directory path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        ComponentPtr->AddRequiredDir(yy_CreateRequiredDirMapping(sourcePath,
                                                                 destPath,
                                                                 *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a required library to a Component.  This is a library that is expected to exist outside
 * the application's sandbox in the target file system and that the component needs access to.
 *
 * Furthermore, this library will be linked with the component library (if it has source files) and
 * any executable that this component is a part of.
 *
 * At link time, the library search path will be searched for the library in the build host file
 * system.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredLib
(
    const char* libName     ///< The name of the library.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string libraryPath = legato::DoEnvVarSubstitution(libName);

        // If env var substitution happened.
        if (libraryPath != libName)
        {
            if (cyy_IsVerbose)
            {
                std::cout << "Environment variable substitution of '" << libName
                          << "' resulted in '" << libraryPath << "'." << std::endl;
            }

            // If the result was an empty string, ignore it.
            if (libraryPath == "")
            {
                return;
            }
        }

        ComponentPtr->AddRequiredLib(libraryPath);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a required component to a component.  This is another component that is used by the
 * component that is currently being parsed.
 *
 * This will add that component to the component's list of subcomponents. Any executable
 * that includes a component also includes all of that component's subcomponents and their
 * subcomponents, etc.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredComponent
(
    const char* path        ///< The path to the component.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string componentPath = legato::DoEnvVarSubstitution(path);

        // If env var substitution happened.
        if (componentPath != path)
        {
            if (cyy_IsVerbose)
            {
                std::cout << "Environment variable substitution of '" << path
                          << "' resulted in '" << componentPath << "'." << std::endl;
            }

            // If the result was an empty string, ignore it.
            if (componentPath == "")
            {
                return;
            }
        }

        std::string dirPath = legato::FindComponent(componentPath,
                                                    BuildParamsPtr->SourceDirs());

        if (dirPath == "")
        {
            throw legato::Exception("Subcomponent '" + componentPath + "' not found.");
        }

        // Add the component to the list of sub-components.  We leave a NULL pointer in
        // the list of sub-components for now.  It will get resolved later when we are done
        // parsing this component.
        ComponentPtr->AddSubComponent(dirPath, NULL);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add to a Component a file from the build host file system that should be bundled into any
 * app that this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddBundledFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        ComponentPtr->AddBundledFile(yy_CreateBundledFileMapping(permissions,
                                                                 sourcePath,
                                                                 destPath,
                                                                 *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add to a Component a directory from the build host file system that should be bundled into
 * any app that this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddBundledDir
(
    const char* permissions,///< String representing permissions to be applied to files in the dir.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        ComponentPtr->AddBundledDir(yy_CreateBundledDirMapping(permissions,
                                                               sourcePath,
                                                               destPath,
                                                               *BuildParamsPtr) );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a given API file in the build host file system.
 *
 * @return The path to the file.
 *
 * @throw legato::Exception if the file can't be found.
 **/
//--------------------------------------------------------------------------------------------------
static std::string FindApiFile
(
    const char* apiFile
)
//--------------------------------------------------------------------------------------------------
{
    std::string apiFilePath(legato::DoEnvVarSubstitution(apiFile));

    if (apiFilePath.rfind(".api") != apiFilePath.length() - 4)
    {
        throw legato::Exception("File name '" + apiFilePath + "'"
                                " doesn't look like a .api file.");
    }

    apiFilePath = legato::FindFile(apiFile, BuildParamsPtr->InterfaceDirs());
    if (apiFilePath.empty())
    {
        throw legato::Exception("Couldn't find API file '" + std::string(apiFile) + "'.");
    }

    return legato::AbsolutePath(apiFilePath);
}



//--------------------------------------------------------------------------------------------------
/**
 * Generate a default IPC interface instance name from a .api file path.
 *
 * @return The interface instance name.
 **/
//--------------------------------------------------------------------------------------------------
static std::string InterfaceInstanceFromFilePath
(
    const char* apiFile
)
//--------------------------------------------------------------------------------------------------
{
    std::string instanceStr = legato::GetLastPathNode(apiFile);

    size_t suffixPos = instanceStr.rfind('.');

    return instanceStr.substr(0, suffixPos);
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a required (client-side) IPC API interface to a Component.
 *
 * @return Reference to the Imported Interface object created.
 **/
//--------------------------------------------------------------------------------------------------
static legato::ClientInterface& AddRequiredApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    std::string apiFilePath = FindApiFile(apiFile);

    std::string instanceStr;
    if (instanceName == NULL)
    {
        instanceStr = InterfaceInstanceFromFilePath(apiFile);
    }
    else
    {
        instanceStr = instanceName;
    }

    return ComponentPtr->AddRequiredApi(instanceStr,
                                        legato::parser::GetApiObject(apiFilePath, *BuildParamsPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a required (client-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddRequiredApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Client of API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a types-only required (client-side) IPC API interface to a Component.
 *
 * This only imports the type definitions from the .api file without generating the client-side IPC
 * library or automatically calling the client-side IPC initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddTypesOnlyRequiredApi
(
    const char* instanceName,   ///< Prefix to apply to the definitions, or
                                ///  NULL if prefix should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddRequiredApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Using data types from API defined in '" << interface.Api().FilePath()
                      << "' with local prefix '" << interface.InternalName() << "_'"
                      << std::endl;
        }

        interface.MarkTypesOnly();
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start required (client-side) IPC API interface to a Component.
 *
 * The client-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartRequiredApi
(
    const char* instanceName,   ///< Prefix to apply to the definitions, or
                                ///  NULL if prefix should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddRequiredApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Client of API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }

        interface.MarkManualStart();
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Add a provided (server-side) IPC API interface to a Component.
 *
 * @return Reference to the newly created Exported Interface object.
 **/
//--------------------------------------------------------------------------------------------------
static legato::ServerInterface& AddProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    std::string apiFilePath = FindApiFile(apiFile);

    std::string instanceStr;
    if (instanceName == NULL)
    {
        instanceStr = InterfaceInstanceFromFilePath(apiFile);
    }
    else
    {
        instanceStr = instanceName;
    }

    return ComponentPtr->AddProvidedApi(instanceStr,
                                        legato::parser::GetApiObject(apiFilePath, *BuildParamsPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a provided (server-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddProvidedApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Serving API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an asynchronous provided (server-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddAsyncProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddProvidedApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Serving (asynchronously) API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }

        interface.MarkAsync();
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start provided (server-side) IPC API interface to a Component.
 *
 * The server-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddProvidedApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Serving API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }

        interface.MarkManualStart();
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start, asynchronous provided (server-side) IPC API interface to a Component.
 *
 * The server-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartAsyncProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto& interface = AddProvidedApi(instanceName, apiFile);

        if (cyy_IsVerbose)
        {
            std::cout << "  Serving (asynchronously) API defined in '" << interface.Api().FilePath()
                      << "' with local interface name '" << interface.InternalName() << "'"
                      << std::endl;
        }

        interface.MarkAsync();
        interface.MarkManualStart();
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


