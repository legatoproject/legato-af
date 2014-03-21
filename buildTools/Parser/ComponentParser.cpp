//--------------------------------------------------------------------------------------------------
/**
 * Main file for the Component Parser.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
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
 *          on the component search path in the buildParams.
 */
//--------------------------------------------------------------------------------------------------
void ParseComponent
(
    Component& component,               ///< The Component object to populate.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
)
//--------------------------------------------------------------------------------------------------
{
    ComponentPtr = &component;
    BuildParamsPtr = &buildParams;

    // Open the component's Component.cdef file for reading.
    std::string path = FindComponent(component.Name(), buildParams.ComponentDirs());
    if (path == "")
    {
        throw Exception("Couldn't find component '" + component.Name() + "'.");
    }
    component.Path(path);
    path += "/Component.cdef";
    FILE* file = fopen(path.c_str(), "r");
    if (file == NULL)
    {
        int error = errno;
        std::stringstream errorMessage;
        errorMessage << "Failed to open file '" << path << "'." <<
                        " Errno = " << error << "(" << strerror(error) << ").";
        throw Exception(errorMessage.str());
    }

    if (buildParams.IsVerbose())
    {
        std::cout << "Parsing '" << path << "'\n";
    }

    // Tell the parser to reset itself and connect to the new file stream for future parsing.
    cyy_FileName = path.c_str();
    cyy_IsVerbose = (BuildParamsPtr->IsVerbose() ? 1 : 0);
    cyy_EndOfFile = 0;
    cyy_ErrorCount = 0;
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
        throw Exception("Errors encountered while parsing '" + path + "'.");
    }

    ComponentPtr = NULL;

    if (buildParams.IsVerbose())
    {
        std::cout << "Finished parsing '" << path << "'\n";
    }
}


}

}

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
        ComponentPtr->AddSourceFile(filePath);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a file mapping to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddFileMapping
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the target file system, outside sandbox.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (cyy_IsVerbose)
    {
        printf("  Importing '%s' from the target file system to '%s' %s inside the sandbox.\n",
               sourcePath,
               destPath,
               permissions);
    }

    try
    {
        ComponentPtr->AddImportedFile( {    yy_GetPermissionFlags(permissions),
                                            yy_StripQuotes(sourcePath),
                                            yy_StripQuotes(destPath)    } );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an included file to a Component.  This is a file that should be bundled into any app that
 * this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddIncludedFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (cyy_IsVerbose)
    {
        printf("Any app that includes this component will include '%s' (at '%s' %s in the sandbox).\n",
               sourcePath,
               destPath,
               permissions);
    }

    try
    {
        ComponentPtr->AddIncludedFile( {    yy_GetPermissionFlags(permissions),
                                            yy_StripQuotes(sourcePath),
                                            yy_StripQuotes(destPath)    } );
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an imported interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddImportedInterface
(
    const char* instanceName,
    const char* apiFile
)
//--------------------------------------------------------------------------------------------------
{
    if (cyy_IsVerbose)
    {
        printf("  Importing API '%s' with local instance name '%s'\n", apiFile, instanceName);
    }

    try
    {
        std::string apiFilePath = legato::FindFile(apiFile, BuildParamsPtr->InterfaceDirs());
        if (apiFilePath.empty())
        {
            throw legato::Exception("Couldn't find API file '" + std::string(apiFile) + "'.");
        }

        ComponentPtr->AddImportedInterface(instanceName, apiFilePath);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an exported interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddExportedInterface
(
    const char* instanceName,
    const char* apiFile
)
//--------------------------------------------------------------------------------------------------
{
    if (cyy_IsVerbose)
    {
        printf("  Exporting API '%s' with local instance name '%s'\n", apiFile, instanceName);
    }

    try
    {
        std::string apiFilePath = legato::FindFile(apiFile, BuildParamsPtr->InterfaceDirs());
        if (apiFilePath.empty())
        {
            throw legato::Exception("Couldn't find API file '" + std::string(apiFile) + "'.");
        }

        ComponentPtr->AddExportedInterface(instanceName, apiFilePath);
    }
    catch (legato::Exception e)
    {
        cyy_error(e.what());
    }
}
