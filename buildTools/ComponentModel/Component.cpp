//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Component class.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{

// Global map of all components seen.  Key is the canonical path to the component.
// Value is the Component object.
static std::map<std::string, Component> ComponentMap;


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new Component object for a given file system path.
 *
 * @return  Pointer to the Component object.
 *
 * @throw   legato::Exception if a duplicate is detected.
 */
//--------------------------------------------------------------------------------------------------
Component* Component::CreateComponent
(
    const std::string& path ///< Cannonical path to the component.
)
//--------------------------------------------------------------------------------------------------
{
    // Convert the path to its cannonical form so we can detect duplicates even if they
    // are found via different relative paths or symlinks.
    std::string realPath = CanonicalPath(path);

    auto iter = ComponentMap.find(realPath);
    if (iter != ComponentMap.end())
    {
        throw legato::Exception("Internal error: Duplicate component '" + realPath + "'"
                                " (" + path + ").");
    }

    // No match found, create a new entry in the map for this component and return that.
    Component& component = ComponentMap[realPath];
    component.Path(realPath);

    return &component;
}


//--------------------------------------------------------------------------------------------------
/**
 * Finds an existing Component object for a given file system path.
 *
 * @return  Pointer to the Component object or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
Component* Component::FindComponent
(
    const std::string& path ///< Cannonical path to the component.
)
//--------------------------------------------------------------------------------------------------
{
    // Convert the path to its cannonical form so we can detect duplicates even if they
    // are found via different relative paths or symlinks.
    std::string realPath = CanonicalPath(path);

    auto iter = ComponentMap.find(realPath);
    if (iter == ComponentMap.end())
    {
        return NULL;
    }
    else
    {
        return &(iter->second);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 **/
//--------------------------------------------------------------------------------------------------
Component::Component
(
    Component&& component
)
//--------------------------------------------------------------------------------------------------
:   m_Name(std::move(component.m_Name)),
    m_CName(std::move(component.m_CName)),
    m_Path(std::move(component.m_Path)),
    m_Library(std::move(component.m_Library)),
    m_CSourcesList(std::move(component.m_CSourcesList)),
    m_LibraryList(std::move(component.m_LibraryList)),
    m_IncludePath(std::move(component.m_IncludePath)),
    m_HasCSources(std::move(component.m_HasCSources)),
    m_HasCppSources(std::move(component.m_HasCppSources)),
    m_ImportedInterfaces(std::move(component.m_ImportedInterfaces)),
    m_ExportedInterfaces(std::move(component.m_ExportedInterfaces)),
    m_BundledFiles(std::move(component.m_BundledFiles)),
    m_BundledDirs(std::move(component.m_BundledDirs)),
    m_RequiredFiles(std::move(component.m_RequiredFiles)),
    m_RequiredDirs(std::move(component.m_RequiredDirs)),
    m_SubComponents(std::move(component.m_SubComponents)),
//    m_PoolList(std::move(component.m_PoolList)),
//    m_ConfigItemList(std::move(component.m_ConfigItemList))
    m_BeingProcessed(std::move(component.m_BeingProcessed))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator (=)
 **/
//--------------------------------------------------------------------------------------------------
Component& Component::operator =
(
    Component&& component
)
//--------------------------------------------------------------------------------------------------
{
    if (&component != this)
    {
        m_Name = std::move(component.m_Name);
        m_CName = std::move(component.m_CName);
        m_Path = std::move(component.m_Path);
        m_Library = std::move(component.m_Library);
        m_CSourcesList = std::move(component.m_CSourcesList);
        m_LibraryList = std::move(component.m_LibraryList);
        m_IncludePath = std::move(component.m_IncludePath);
        m_HasCSources = std::move(component.m_HasCSources);
        m_HasCppSources = std::move(component.m_HasCppSources);
        m_ImportedInterfaces = std::move(component.m_ImportedInterfaces);
        m_ExportedInterfaces = std::move(component.m_ExportedInterfaces);
        m_BundledFiles = std::move(component.m_BundledFiles);
        m_BundledDirs = std::move(component.m_BundledDirs);
        m_RequiredFiles = std::move(component.m_RequiredFiles);
        m_RequiredDirs = std::move(component.m_RequiredDirs);
        m_SubComponents = std::move(component.m_SubComponents);
    //    m_PoolList = std::move(component.m_PoolList);
    //    m_ConfigItemList = std::move(component.m_ConfigItemList);
        m_BeingProcessed = std::move(component.m_BeingProcessed);
    }

    return *this;
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the name of the component.
 */
//--------------------------------------------------------------------------------------------------
void Component::Name(const std::string& name)
{
    m_Name = name;

    if (m_CName == "")
    {
        CName(GetCSafeName(m_Name));
    }

    m_Library.ShortName(m_Name);
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the name of the component.
 */
//--------------------------------------------------------------------------------------------------
void Component::Name(std::string&& name)
{
    m_Name = std::move(name);

    if (m_CName == "")
    {
        CName(GetCSafeName(m_Name));
    }

    m_Library.ShortName(m_Name);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the component.
 */
//--------------------------------------------------------------------------------------------------
const std::string& Component::Name() const
{
    return m_Name;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the name to be used for the component inside C identifiers.
 */
//--------------------------------------------------------------------------------------------------
void Component::CName(const std::string& name)
{
    m_CName = name;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the name to be used for the component inside C identifiers.
 */
//--------------------------------------------------------------------------------------------------
void Component::CName(std::string&& name)
{
    m_CName = std::move(name);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the name to be used for the component inside C identifiers.
 */
//--------------------------------------------------------------------------------------------------
const std::string& Component::CName() const
{
    return m_CName;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the component's directory.
 */
//--------------------------------------------------------------------------------------------------
void Component::Path
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    m_Path = path;

    // Remove the trailing slash, if there is one.
    if (m_Path.back() == '/')
    {
        m_Path.erase(m_Path.end() - 1);
    }

    if (m_Name == "")
    {
        Name(GetLastPathNode(m_Path));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the component's directory.
 */
//--------------------------------------------------------------------------------------------------
void Component::Path
(
    std::string&& path
)
//--------------------------------------------------------------------------------------------------
{
    m_Path = std::move(path);

    // Remove the trailing slash, if there is one.
    if (m_Path.back() == '/')
    {
        m_Path.erase(m_Path.end() - 1);
    }

    if (m_Name == "")
    {
        Name(GetLastPathNode(m_Path));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the file system path of the component's directory.
 */
//--------------------------------------------------------------------------------------------------
const std::string& Component::Path
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return m_Path;
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a source code file to the component.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddSourceFile
(
    const std::string& path ///< The source file path.
)
//--------------------------------------------------------------------------------------------------
{
    if (legato::IsCSource(path))
    {
        m_CSourcesList.push_back(path);
        m_HasCSources = true;

        return;
    }
    else if (legato::IsCppSource(path))
    {
        m_CSourcesList.push_back(path);
        m_HasCppSources = true;

        return;
    }

    throw Exception("File '" + path + "' is an unknown type of source code file.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a library file to the component.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddLibrary
(
    const std::string& path ///< The library file path.
)
//--------------------------------------------------------------------------------------------------
{
    m_LibraryList.push_back(path);
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a directory to the include path.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddIncludeDir
(
    const std::string& path ///< The library file path.
)
//--------------------------------------------------------------------------------------------------
{
    m_IncludePath.push_back(path);
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a file from the build host's file system to an application (bundles it into the app),
 * making it appear at a specific location in the application sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddBundledFile
(
    FileMapping&& mapping   ///< The source path is in the build host file system.  Dest path is
                            ///  in the application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    // If the imported file path is not absolute, then we need to prefix it with the
    // component directory path, because it is relative to that directory.
    if (!legato::IsAbsolutePath(mapping.m_SourcePath))
    {
        mapping.m_SourcePath = legato::CombinePath(m_Path, mapping.m_SourcePath);
    }

    // Find the file in the host file system.
    if (!legato::FileExists(mapping.m_SourcePath))
    {
        throw legato::Exception("File '" + mapping.m_SourcePath + "' not found.");
    }

    m_BundledFiles.push_back(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a directory from the build host's file system to an application (bundles it into the app),
 * making it appear at a specific location in the application sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddBundledDir
(
    FileMapping&& mapping   ///< The source path is in the build host file system.  Dest path is
                            ///  in the application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    // If the imported file path is not absolute, then we need to prefix it with the
    // component directory path, because it is relative to that directory.
    if (!legato::IsAbsolutePath(mapping.m_SourcePath))
    {
        mapping.m_SourcePath = legato::CombinePath(m_Path, mapping.m_SourcePath);
    }

    // Find the directory in the host file system.
    if (!legato::DirectoryExists(mapping.m_SourcePath))
    {
        throw legato::Exception("Directory '" + mapping.m_SourcePath + "' not found.");
    }

    m_BundledDirs.push_back(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a file from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddRequiredFile
(
    FileMapping&& mapping   ///< Source path is outside the sandbox (if relative, then relative to
                            ///  the application's install directory).  Dest path is inside the
                            ///  application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    m_RequiredFiles.push_back(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a directory from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddRequiredDir
(
    FileMapping&& mapping   ///< Source path is outside the sandbox (if relative, then relative to
                            ///  the application's install directory).  Dest path is inside the
                            ///  application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    m_RequiredDirs.push_back(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a component path to the list of paths to sub-components of this component.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddSubComponent
(
    const std::string& path,            ///< File system path to the component.
    Component* componentPtr             ///< Pointer to the component object (or NULL).
)
//--------------------------------------------------------------------------------------------------
{
    if (m_SubComponents.find(path) != m_SubComponents.end())
    {
        throw legato::Exception("Component '" + m_Name + "'"
                                " has duplicate sub-component "
                                " '" + path + "'.");
    }
    else
    {
        m_SubComponents[path] = componentPtr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an interface to the Component's collection of imported interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ImportedInterface& Component::AddRequiredApi
(
    const std::string& name,    ///< Friendly name of the interface instance.
    Api_t* apiPtr               ///< Pointer to the API object.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_ImportedInterfaces.find(name) != m_ImportedInterfaces.end())
        || (m_ExportedInterfaces.find(name) != m_ExportedInterfaces.end()) )
    {
        throw Exception("Interfaces must have unique names. '" + name + "' is used more than once"
                        + "for component '" + m_Name + "'.'");
    }

    return m_ImportedInterfaces[name] = ImportedInterface(name, apiPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an interface to the Component's collection of imported interfaces.
 **/
//--------------------------------------------------------------------------------------------------
ExportedInterface& Component::AddProvidedApi
(
    const std::string& name,    ///< Friendly name of the interface instance.
    Api_t* apiPtr               ///< Pointer to the API object.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_ImportedInterfaces.find(name) != m_ImportedInterfaces.end())
        || (m_ExportedInterfaces.find(name) != m_ExportedInterfaces.end()) )
    {
        throw Exception("Interfaces must have unique names. '" + name + "' is used more than once.");
    }

    return m_ExportedInterfaces[name] = ExportedInterface(name, apiPtr);
}



}
