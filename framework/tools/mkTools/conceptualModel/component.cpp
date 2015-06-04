//--------------------------------------------------------------------------------------------------
/**
 * @file component.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Map of file paths to pointers to Component objects.
 *
 * This is used to keep a single, unique component object for each unique component directory.
 *
 * The key is the cannonical path to the component directory.  The value is a pointer to an object.
 **/
//--------------------------------------------------------------------------------------------------
std::map<std::string, Component_t*> Component_t::ComponentMap;



//--------------------------------------------------------------------------------------------------
/**
 * Get a pre-existing Component object for the component found at a given directory path.
 *
 * @return Pointer to the object or NULL if not found.
 **/
//--------------------------------------------------------------------------------------------------
Component_t* Component_t::GetComponent
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    std::string canonicalPath = path::MakeCanonical(path);

    auto i = ComponentMap.find(canonicalPath);

    if (i == ComponentMap.end())
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
 * Create a new Component object for the component found at a given directory path.
 *
 * @return Pointer to the object.
 *
 * @throw model::Exception_t if already exists.
 **/
//--------------------------------------------------------------------------------------------------
Component_t* Component_t::CreateComponent
(
    const parseTree::CdefFile_t* filePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string canonicalPath = path::MakeCanonical(path::GetContainingDir(filePtr->path));

    auto i = ComponentMap.find(canonicalPath);

    if (i == ComponentMap.end())
    {
        auto componentPtr = new Component_t(filePtr);
        ComponentMap[canonicalPath] = componentPtr;
        return componentPtr;
    }
    else
    {
        throw mk::Exception_t("Internal error: Attempt to create duplicate Component object"
                                   " for '" + canonicalPath + "' (" + filePtr->path + ").");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor.
 **/
//--------------------------------------------------------------------------------------------------
Component_t::Component_t
(
    const parseTree::CdefFile_t* filePtr
)
//--------------------------------------------------------------------------------------------------
:   defFilePtr(filePtr),
    dir(path::GetContainingDir(defFilePtr->path)),
    name(path::GetIdentifierSafeName(path::GetLastNode(dir))),
    workingDir("component/" + name)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Constructor.
 **/
//--------------------------------------------------------------------------------------------------
ComponentInstance_t::ComponentInstance_t
(
    Exe_t* ePtr,
    Component_t* cPtr
)
//--------------------------------------------------------------------------------------------------
:   exePtr(ePtr),
    componentPtr(cPtr)
//--------------------------------------------------------------------------------------------------
{
}



} // namespace model
