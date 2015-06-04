//--------------------------------------------------------------------------------------------------
/**
 * @file api.cpp
 *
 * (C) Copyright, Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"



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
        throw mk::Exception_t("Internal error: Attempt to create duplicate API File object"
                                   " for '" + canonicalPath + "' (" + path + ").");
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
 * Constructor for the .api reference objects' base class.
 */
//--------------------------------------------------------------------------------------------------
ApiRef_t::ApiRef_t
(
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   apiFilePtr(aPtr),
    componentPtr(cPtr),
    internalName(iName)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for types-only interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiTypesOnlyInterface_t::ApiTypesOnlyInterface_t
(
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(aPtr, cPtr, iName)
//--------------------------------------------------------------------------------------------------
{
    std::string codeGenDir = path::Combine(apiFilePtr->codeGenDir, "client/");

    interfaceFile = codeGenDir + apiFilePtr->defaultPrefix + "_interface.h";
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiClientInterface_t::ApiClientInterface_t
(
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName    ///< The internal name used inside the component.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(aPtr, cPtr, iName),
    manualStart(false)
//--------------------------------------------------------------------------------------------------
{
    std::string codeGenDir = path::Combine(apiFilePtr->codeGenDir, "client/");

    interfaceFile = codeGenDir + internalName + "_interface.h";
    internalHFile = codeGenDir + internalName + "_local.h";
    sourceFile = codeGenDir + internalName + "_client.c";
    objectFile = codeGenDir + internalName + "_client.c.o";
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor for server-side interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ApiServerInterface_t::ApiServerInterface_t
(
    ApiFile_t* aPtr,            ///< Ptr to the .api file object.
    Component_t* cPtr,          ///< Ptr to the component.
    const std::string& iName,   ///< The internal name used inside the component.
    bool isAsync                ///< true if async server-side interface code should be generated.
)
//--------------------------------------------------------------------------------------------------
:   ApiRef_t(aPtr, cPtr, iName),
    async(isAsync),
    manualStart(false)
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

    interfaceFile = codeGenDir + internalName + "_server.h";
    internalHFile = codeGenDir + internalName + "_local.h";
    sourceFile = codeGenDir + internalName + "_server.c";
    objectFile = codeGenDir + internalName + "_server.o";
}



//--------------------------------------------------------------------------------------------------
/**
 * Constructor for client-side interface instances.
 **/
//--------------------------------------------------------------------------------------------------
ApiClientInterfaceInstance_t::ApiClientInterfaceInstance_t
(
    ComponentInstance_t* cInstPtr,  ///< Component instance this interface instance belongs to.
    ApiClientInterface_t* p
)
//--------------------------------------------------------------------------------------------------
:   componentInstancePtr(cInstPtr),
    ifPtr(p),
    bindingPtr(NULL),
    isExternal(false)
//--------------------------------------------------------------------------------------------------
{
    name = componentInstancePtr->exePtr->name
         + '.' + componentInstancePtr->componentPtr->name
         + '.' + ifPtr->internalName;
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
:   componentInstancePtr(cInstPtr),
    ifPtr(p),
    isExternal(false)
//--------------------------------------------------------------------------------------------------
{
    name = componentInstancePtr->exePtr->name
         + '.' + componentInstancePtr->componentPtr->name
         + '.' + ifPtr->internalName;
}



} // namespace model
