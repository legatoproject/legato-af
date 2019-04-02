//--------------------------------------------------------------------------------------------------
/**
 * @file system.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
System_t::System_t
(
    parseTree::SdefFile_t* filePtr  ///< Pointer to the root of the parse tree for the .sdef file.
)
//--------------------------------------------------------------------------------------------------
:   defFilePtr(filePtr),
    dir(path::MakeAbsolute(path::GetContainingDir(filePtr->path))),
    name(path::RemoveSuffix(path::GetLastNode(filePtr->path), ".sdef"))
//--------------------------------------------------------------------------------------------------
{

}


//--------------------------------------------------------------------------------------------------
/**
 * Find an app in the system.
 *
 * @return A pointer to the app object.
 *
 * @throw  mk::Exception_t if not found.
 */
//--------------------------------------------------------------------------------------------------
App_t* System_t::FindApp
(
    const parseTree::Token_t* appTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& appName = appTokenPtr->text;

    auto appIter = apps.find(appName);

    if (appIter == apps.end())
    {
        appTokenPtr->ThrowException(
            mk::format(LE_I18N("No such app '%s' in the system."), appName)
        );
    }

    return appIter->second;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a server-side interface on an app in the system.
 *
 * @return A pointer to the interface instance object, or NULL if no matching interface is found.
 */
//--------------------------------------------------------------------------------------------------
ApiServerInterfaceInstance_t* System_t::FindServerInterface
(
    const parseTree::Token_t* appTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& interfaceName = interfaceTokenPtr->text;

    auto appPtr = FindApp(appTokenPtr);

    auto interfaceIter = appPtr->externServerInterfaces.find(interfaceName);

    if (interfaceIter == appPtr->externServerInterfaces.end())
    {
        // Didn't find in extern server interface, now search in preBuilt interfaces.
        interfaceIter = appPtr->preBuiltServerInterfaces.find(interfaceName);

        if (interfaceIter == appPtr->preBuiltServerInterfaces.end())
        {
            return NULL;
        }
    }

    return interfaceIter->second;
}

} // namespace modeller
