//--------------------------------------------------------------------------------------------------
/**
 * @file api.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"



namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Map of file paths to pointers to API File objects.
 *
 * This is used to keep a single, unique API File object for each unique .api file.
 *
 * The key is the cannonical path to the .api file.  The value is a pointer to an object.
 **/
//--------------------------------------------------------------------------------------------------
std::map<std::string, ApiFile_t*> ApiFile_t::ApiFileMap;


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the client-side .h file that would be generated for this .api file with a
 * given internal alias.
 *
 * @return The path.
 */
//--------------------------------------------------------------------------------------------------
std::string ApiFile_t::GetClientInterfaceFile
(
    const std::string& internalName
)
const
//--------------------------------------------------------------------------------------------------
{
    return path::Combine(codeGenDir, "client/") + internalName + "_interface.h";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the generated (synchronous mode) server-side .h file for this .api file.
 *
 * @return The path.
 */
//--------------------------------------------------------------------------------------------------
std::string ApiFile_t::GetServerInterfaceFile
(
    const std::string& internalName
)
const
//--------------------------------------------------------------------------------------------------
{
    return path::Combine(codeGenDir, "server/") + internalName + "_server.h";
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated Java files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
std::string ApiFile_t::GetJavaInterfaceFile
(
    const std::string& internalName
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(codeGenDir, "src/io/legato/api/");

    return srcDir + internalName + ".java";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the generated (async mode) server-side .h file for this .api file.
 *
 * @return The path.
 */
//--------------------------------------------------------------------------------------------------
std::string ApiFile_t::GetAsyncServerInterfaceFile
(
    const std::string& internalName
)
const
//--------------------------------------------------------------------------------------------------
{
    return path::Combine(codeGenDir, "async_server/") + internalName + "_server.h";
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the path to generated file for RPC exported reference in config tree.
 */
//--------------------------------------------------------------------------------------------------
std::string ApiFile_t::GetRpcReferenceFile
(
    const std::string& internalName
) const
{
    return path::Combine(codeGenDir, internalName + "_ref.cfg");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pre-existing API file object for the .api file at a given path.
 *
 * @return A pointer to the API file object or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiFile_t* ApiFile_t::GetApiFile
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    std::string canonicalPath = path::MakeCanonical(path);

    auto i = ApiFileMap.find(canonicalPath);

    if (i == ApiFileMap.end())
    {
        return NULL;
    }
    else
    {
        return i->second;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new API file object for the .api file at a given path.
 *
 * @return A pointer to the API file object.
 *
 * @throw model::Exception_t if already exists.
 */
//--------------------------------------------------------------------------------------------------
ApiFile_t* ApiFile_t::CreateApiFile
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    std::string canonicalPath = path::MakeCanonical(path);

    auto i = ApiFileMap.find(canonicalPath);

    if (i == ApiFileMap.end())
    {
        auto apiFilePtr = new ApiFile_t(canonicalPath);
        ApiFileMap[canonicalPath] = apiFilePtr;
        return apiFilePtr;
    }
    else
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Internal error: Attempt to create duplicate API File object"
                               " for '%s' (%s)."), canonicalPath, path)
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for .api file objects.
 */
//--------------------------------------------------------------------------------------------------
ApiFile_t::ApiFile_t
(
    const std::string& p
)
//--------------------------------------------------------------------------------------------------
:   path(p),
    defaultPrefix(path::RemoveSuffix(path::GetLastNode(p), ".api")),
    isIncluded(false),
    codeGenDir(path::Combine("api", md5(path)))
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Get paths for all common interface .h files which must be generated whenever
 * this API is used, but are not associated with a specific API binding.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
void ApiFile_t::GetCommonInterfaceFiles
(
    InterfaceCFiles_t& cFiles
) const
//--------------------------------------------------------------------------------------------------
{
    cFiles.interfaceFile = codeGenDir + "/"  + defaultPrefix + "_common.h";
    cFiles.internalHFile = codeGenDir + "/" + defaultPrefix + "_messages.h";
    cFiles.sourceFile    = codeGenDir + "/" + defaultPrefix + "_commonclient.c";
    cFiles.objectFile    = cFiles.sourceFile + ".o";
}


//--------------------------------------------------------------------------------------------------
/**
 * Add to a given set the paths for all the client-side interface .h files generated for all
 * .api files that a given .api file includes through USETYPES statements.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
void ApiFile_t::GetClientUsetypesApiHeaders
(
    std::set<std::string>& results  ///< [OUT] Set to add results to.
) const
//--------------------------------------------------------------------------------------------------
{
    for (const auto includedApiPtr : includes)
    {
        results.insert(includedApiPtr->GetClientInterfaceFile(includedApiPtr->defaultPrefix) );

        // Recurse.
        includedApiPtr->GetClientUsetypesApiHeaders(results);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add to a given set the paths for all the server-side interface .h files generated for all
 * .api files that a given .api file includes through USETYPES statements.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
void ApiFile_t::GetServerUsetypesApiHeaders
(
    std::set<std::string>& results  ///< [OUT] Set to add results to.
) const
//--------------------------------------------------------------------------------------------------
{
    for (const auto &includedApiPtr : includes)
    {
        results.insert(includedApiPtr->GetServerInterfaceFile(includedApiPtr->defaultPrefix) );

        // Recurse.
        includedApiPtr->GetServerUsetypesApiHeaders(results);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get paths for all common interface.h files generate for all
 * .api files included by this one.  Results are added to the set provided.
 *
 * @note These paths are all relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
void ApiFile_t::GetCommonUsetypesApiHeaders
(
    std::set<std::string>& results  ///< [OUT] Set to add results to.
) const
//--------------------------------------------------------------------------------------------------
{
    for (const auto &includedApiPtr : includes)
    {
        InterfaceCFiles_t cFiles;

        includedApiPtr->GetCommonInterfaceFiles(cFiles);
        results.insert( cFiles.interfaceFile );

        // Recurse.
        includedApiPtr->GetCommonUsetypesApiHeaders(results);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * If a given .api has any USETYPES statements in it, add those to a given set of
 * USETYPES included .api files.
 */
//--------------------------------------------------------------------------------------------------
void ApiFile_t::GetUsetypesApis
(
    std::set<const model::ApiFile_t*>& results ///< Set to add the USETYPES-included .api files to.
) const
{
    for (auto usetypesApiFilePtr : this->includes)
    {
        results.insert(usetypesApiFilePtr);

        usetypesApiFilePtr->GetUsetypesApis(results); // Recurse.
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Constructor for the .api reference objects' base class.
 */
//--------------------------------------------------------------------------------------------------
ApiRef_t::ApiRef_t
(
    const parseTree::TokenList_t* itemPtr, ///< Ptr to the reference in the parse tree.
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   itemPtr(itemPtr),
    apiFilePtr(aPtr),
    componentPtr(cPtr),
    internalName(iName)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to generated file for RPC exported reference in config tree.
 */
//--------------------------------------------------------------------------------------------------
std::string ApiRef_t::GetRpcReferenceFile
(
    void
) const
{
    return apiFilePtr->GetRpcReferenceFile(internalName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for types-only interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiTypesOnlyInterface_t::ApiTypesOnlyInterface_t
(
    const parseTree::TokenList_t* itemPtr,  ///< Ptr to the reference in the parse tree.
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(itemPtr, aPtr, cPtr, iName)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated C files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiTypesOnlyInterface_t::GetInterfaceFiles
(
    InterfaceCFiles_t& cFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string codeGenDir = path::Combine(apiFilePtr->codeGenDir, "client/");

    cFiles.interfaceFile = codeGenDir + apiFilePtr->defaultPrefix + "_interface.h";
    cFiles.internalHFile = "";
    cFiles.sourceFile = "";
    cFiles.objectFile = "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated Java files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiTypesOnlyInterface_t::GetInterfaceFiles
(
    InterfaceJavaFiles_t& javaFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "src/io/legato/api/");

    javaFiles.interfaceSourceFile = srcDir + internalName + ".java";
    javaFiles.implementationSourceFile = "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated Python files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiTypesOnlyInterface_t::GetInterfaceFiles
(
    InterfacePythonFiles_t& pythonFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "");

    pythonFiles.cExtensionBinaryFile = internalName + "_native.so";
    pythonFiles.wrapperSourceFile = internalName + ".py";
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiClientInterface_t::ApiClientInterface_t
(
    const parseTree::TokenList_t* itemPtr, ///< Ptr to the reference in the parse tree.
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(itemPtr, aPtr, cPtr, iName),
    manualStart(false),
    optional(false)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated client C files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiClientInterface_t::GetInterfaceFiles
(
    InterfaceCFiles_t& cFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string codeGenDir = path::Combine(apiFilePtr->codeGenDir, "client/");

    cFiles.interfaceFile = codeGenDir + internalName + "_interface.h";
    cFiles.internalHFile = codeGenDir + internalName + "_service.h";
    cFiles.sourceFile = codeGenDir + internalName + "_client.c";
    cFiles.objectFile = codeGenDir + internalName + "_client.c.o";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated client Java files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiClientInterface_t::GetInterfaceFiles
(
    InterfaceJavaFiles_t& javaFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "src/io/legato/api/");

    javaFiles.interfaceSourceFile = srcDir + internalName + ".java";
    javaFiles.implementationSourceFile = srcDir + "implementation/" + internalName + "Client.java";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated Python files of an interface.
 * NOTE: Python server code not yet implemented.
 **/
//--------------------------------------------------------------------------------------------------
void ApiClientInterface_t::GetInterfaceFiles
(
    InterfacePythonFiles_t& pythonFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "");

    pythonFiles.cExtensionBinaryFile = internalName + "_native.so";
    pythonFiles.cExtensionObjectFile = internalName + "_native.o";
    pythonFiles.wrapperSourceFile = internalName + ".py";
    pythonFiles.cExtensionSourceFile = internalName + "_native.c";
    pythonFiles.cdefSourceFile = internalName + "_cdef.h";
}

//--------------------------------------------------------------------------------------------------
/**
 * Constructor for server-side interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiServerInterface_t::ApiServerInterface_t
(
    const parseTree::TokenList_t* itemPtr, ///< Ptr to the reference in the parse tree.
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName,   ///< The internal name used inside the component.
    bool isAsync                ///< true if async server-side interface code should be generated.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(itemPtr, aPtr, cPtr, iName),
    async(isAsync),
    manualStart(false)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated server C files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiServerInterface_t::GetInterfaceFiles
(
    InterfaceCFiles_t& cFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string codeGenDir;

    if (async)
    {
        codeGenDir = path::Combine(apiFilePtr->codeGenDir, "async_server/");
    }
    else
    {
        codeGenDir = path::Combine(apiFilePtr->codeGenDir, "server/");
    }

    cFiles.interfaceFile = codeGenDir + internalName + "_server.h";
    cFiles.internalHFile = codeGenDir + internalName + "_service.h";
    cFiles.sourceFile = codeGenDir + internalName + "_server.c";
    cFiles.objectFile = codeGenDir + internalName + "_server.o";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated server Java files of an interface.
 **/
//--------------------------------------------------------------------------------------------------
void ApiServerInterface_t::GetInterfaceFiles
(
    InterfaceJavaFiles_t& javaFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "src/io/legato/api/");

    javaFiles.interfaceSourceFile = srcDir + internalName + ".java";
    javaFiles.implementationSourceFile = srcDir + "implementation/" + internalName + "Server.java";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the set of paths for the generated Python files of an interface.
 * NOTE: Python server code not yet implemented.
 **/
//--------------------------------------------------------------------------------------------------
void ApiServerInterface_t::GetInterfaceFiles
(
    InterfacePythonFiles_t& pythonFiles
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string srcDir = path::Combine(componentPtr->workingDir, "");

    pythonFiles.cExtensionBinaryFile = internalName + "_native.so";
    pythonFiles.wrapperSourceFile = internalName + ".py";
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interface instances.
 **/
//--------------------------------------------------------------------------------------------------
ApiInterfaceInstance_t::ApiInterfaceInstance_t
(
    ComponentInstance_t* cInstPtr,
    const std::string& internalName ///< Component's internal name for this interface.
)
//--------------------------------------------------------------------------------------------------
:   componentInstancePtr(cInstPtr),
    externMarkPtr(NULL),
    systemExtern(false)
//--------------------------------------------------------------------------------------------------
{
    if (componentInstancePtr == NULL)
    {
        name = internalName;
    }
    else
    {
        name = componentInstancePtr->exePtr->name
             + '.' + componentInstancePtr->componentPtr->name
             + '.' + internalName;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interface instances.
 **/
//--------------------------------------------------------------------------------------------------
ApiClientInterfaceInstance_t::ApiClientInterfaceInstance_t
(
    ComponentInstance_t* cInstPtr,  ///< Component instance this i/f instance belongs to (or NULL).
    ApiClientInterface_t* p
)
//--------------------------------------------------------------------------------------------------
:   ApiInterfaceInstance_t(cInstPtr, p->internalName),
    ifPtr(p),
    bindingPtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interface instances.
 **/
//--------------------------------------------------------------------------------------------------
ApiServerInterfaceInstance_t::ApiServerInterfaceInstance_t
(
    ComponentInstance_t* cInstPtr,  ///< Component instance this interface instance belongs to.
    ApiServerInterface_t* p
)
//--------------------------------------------------------------------------------------------------
:   ApiInterfaceInstance_t(cInstPtr, p->internalName),
    ifPtr(p)
//--------------------------------------------------------------------------------------------------
{
}



} // namespace model
