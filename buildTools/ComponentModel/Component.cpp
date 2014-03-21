//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Component class.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{



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
    m_CSourcesList(std::move(component.m_CSourcesList)),
    m_LibraryList(std::move(component.m_LibraryList)),
    m_IncludePath(std::move(component.m_IncludePath)),
    m_ImportedInterfaces(std::move(component.m_ImportedInterfaces)),
    m_ExportedInterfaces(std::move(component.m_ExportedInterfaces)),
    m_IncludedFiles(std::move(component.m_IncludedFiles)),
    m_ImportedFiles(std::move(component.m_ImportedFiles))
//    m_PoolList(std::move(component.m_PoolList)),
//    m_ConfigItemList(std::move(component.m_ConfigItemList))
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
        m_CSourcesList = std::move(component.m_CSourcesList);
        m_LibraryList = std::move(component.m_LibraryList);
        m_IncludePath = std::move(component.m_IncludePath);
        m_ImportedInterfaces = std::move(component.m_ImportedInterfaces);
        m_ExportedInterfaces = std::move(component.m_ExportedInterfaces);
        m_IncludedFiles = std::move(component.m_IncludedFiles);
        m_ImportedFiles = std::move(component.m_ImportedFiles);
    //    m_PoolList = std::move(component.m_PoolList);
    //    m_ConfigItemList = std::move(component.m_ConfigItemList);
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
    // If it ends in a .c, then add it to the C Sources List.
    auto ri = path.crbegin();

    if ((ri != path.crend()) && (*ri == 'c'))
    {
        ri++;

        if ((ri != path.crend()) && (*ri == '.'))
        {
            m_CSourcesList.push_back(path);
            return;
        }
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
void Component::AddIncludedFile
(
    FileMapping&& mapping   ///< The source path is in the build host file system.  Dest path is
                            ///  in the application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (!IsValidPath(mapping.m_DestPath))
    {
        throw Exception("'" + mapping.m_DestPath + "' is not a valid path.");
    }
    if (!IsAbsolutePath(mapping.m_DestPath))
    {
        throw Exception("Included files must be mapped to an absolute path ('"
                        + mapping.m_DestPath + "' is not).");
    }

    // If the imported file path is not absolute, then we need to prefix it with the
    // component directory path, because it is relative to that directory.
    if (mapping.m_SourcePath[0] != '/')
    {
        mapping.m_SourcePath = m_Path + '/' + mapping.m_SourcePath;
    }

    m_IncludedFiles.push_back(mapping);
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a file from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void Component::AddImportedFile
(
    FileMapping&& mapping   ///< Source path is outside the sandbox (if relative, then relative to
                            ///  the application's install directory).  Dest path is inside the
                            ///  application sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (!IsValidPath(mapping.m_DestPath))
    {
        throw Exception("'" + mapping.m_DestPath + "' is not a valid path.");
    }
    if (!IsAbsolutePath(mapping.m_DestPath))
    {
        throw Exception("Imported file system objects must be mapped to an absolute path ('"
                        + mapping.m_DestPath + "' is not).");
    }
    if (!IsValidPath(mapping.m_SourcePath))
    {
        throw Exception("'" + mapping.m_SourcePath + "' is not a valid path.");
    }

    m_ImportedFiles.push_back(mapping);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an interface to the Component's collection of imported interfaces.
 **/
//--------------------------------------------------------------------------------------------------
void Component::AddImportedInterface
(
    const std::string& name,        ///< Friendly name of the interface instance.
    const std::string& apiFilePath  ///< .api file path.  Can be either absolute or relative to
                                    ///  any directory in the interface search path.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_ImportedInterfaces.find(name) != m_ImportedInterfaces.end())
        || (m_ExportedInterfaces.find(name) != m_ExportedInterfaces.end()) )
    {
        throw Exception("Interfaces must have unique names. '" + name + "' is used more than once"
                        + "for component '" + m_Name + "'.'");
    }

    ImportedInterface x = {name, apiFilePath};
    m_ImportedInterfaces[name] = x;
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an interface to the Component's collection of imported interfaces.
 **/
//--------------------------------------------------------------------------------------------------
void Component::AddExportedInterface
(
    const std::string& name,        ///< Friendly name of the interface instance.
    const std::string& apiFilePath  ///< .api file path.  Can be either absolute or relative to
                                    ///  any directory in the interface search path.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_ImportedInterfaces.find(name) != m_ImportedInterfaces.end())
        || (m_ExportedInterfaces.find(name) != m_ExportedInterfaces.end()) )
    {
        throw Exception("Interfaces must have unique names. '" + name + "' is used more than once.");
    }

    m_ExportedInterfaces[name] = {name, apiFilePath};
}



}
