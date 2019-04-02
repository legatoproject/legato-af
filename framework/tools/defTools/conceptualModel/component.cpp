//--------------------------------------------------------------------------------------------------
/**
 * @file component.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

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
        throw mk::Exception_t(
            mk::format(LE_I18N("Internal error: Attempt to create duplicate Component "
                               "object for '%s' (%s)."),
                       canonicalPath, filePtr->path)
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Build a list of bundled files that are of the same type.
 **/
//--------------------------------------------------------------------------------------------------
void Component_t::GetBundledFilesOfType
(
    BundleAccess_t access,             ///< Are we searching the source or dest paths?
    const std::string& fileType,       ///< What kind of file are we looking for?
    std::list<std::string>& fileList   ///< Add the found files to this list.
)
//--------------------------------------------------------------------------------------------------
{
    for (const auto& bundledDir : bundledDirs)
    {
        const auto& dirPath = bundledDir->GetBundledPath(access);
        auto bundledFiles = file::ListFiles(dirPath);

        for (const auto& file : bundledFiles)
        {
            if (path::GetFileNameExtension(file) == fileType)
            {
                fileList.push_back(file);
            }
        }
    }

    for (const auto& bundledFile : bundledFiles)
    {
        const auto& filePath = bundledFile->GetBundledPath(access);
        auto extension = path::GetFileNameExtension(filePath);

        if (extension == fileType)
        {
            fileList.push_back(filePath);
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Throw error message about incompatible source or build methods.
 */
//--------------------------------------------------------------------------------------------------
void Component_t::ThrowIncompatibleLanguageException
(
    const parseTree::CompoundItem_t* conflictSectionPtr
) const
//--------------------------------------------------------------------------------------------------
{
    if (HasExternalBuild())
    {
        conflictSectionPtr->ThrowException(LE_I18N("A component with an external build step cannot"
                                                   " have source files."));
    }
    else
    {
        conflictSectionPtr->ThrowException(LE_I18N("A component can only use one source file"
                                                   " language."));
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
    workingDir("component/" + defFilePtr->pathMd5),
    isStandAloneComp(false)
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


//--------------------------------------------------------------------------------------------------
/**
 * Searches the list of server-side interface instances for one with a given name.
 *
 * @return Pointer to the interface instance object or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiServerInterfaceInstance_t* ComponentInstance_t::FindServerInterface
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    for (auto ifInstancePtr : serverApis)
    {
        if (ifInstancePtr->ifPtr->internalName == name)
        {
            return ifInstancePtr;
        }
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the list of client-side interface instances for one with a given name.
 *
 * @return Pointer to the interface instance object or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
ApiClientInterfaceInstance_t* ComponentInstance_t::FindClientInterface
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    for (auto ifInstancePtr : clientApis)
    {
        if (ifInstancePtr->ifPtr->internalName == name)
        {
            return ifInstancePtr;
        }
    }

    return NULL;
}



} // namespace model
