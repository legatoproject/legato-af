//--------------------------------------------------------------------------------------------------
/**
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Extract the name of an application from the path of its .adef file.
 *
 * @return the app name.
 */
//--------------------------------------------------------------------------------------------------
static std::string AppNameFromDefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto endPos = path.rfind(".");
    auto startPos = path.rfind("/");

    if ((endPos == std::string::npos) || (path.substr(endPos) != ".adef"))
    {
        throw Exception("'" + path + "' does not appear to be an application file path.");
    }

    if (startPos == std::string::npos)
    {
        startPos = 0;
    }
    else
    {
        startPos++;
    }

    return path.substr(startPos, endPos - startPos);
}



//--------------------------------------------------------------------------------------------------
/**
 *  Default constructor, create a new blank app object.
 */
//--------------------------------------------------------------------------------------------------
App::App
(
)
//--------------------------------------------------------------------------------------------------
:   m_Name("untitled"),
    m_IsSandboxed(true),
    m_Debug(false),
    m_StartMode(AUTO),
    m_NumProcs(SIZE_MAX),
    m_MqueueSize(SIZE_MAX),
    m_RtSignalQueueSize(SIZE_MAX),
    m_MemLimit(SIZE_MAX),
    m_CpuShare(SIZE_MAX),
    m_FileSystemSize(SIZE_MAX)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 *  Move constructor.
 */
//--------------------------------------------------------------------------------------------------
App::App
(
    App&& application  /// The object to move.
)
//--------------------------------------------------------------------------------------------------
:   m_Name(std::move(application.m_Name)),
    m_DefFilePath(std::move(application.m_DefFilePath)),
    m_IsSandboxed(std::move(application.m_IsSandboxed)),
    m_Debug(std::move(application.m_Debug)),
    m_StartMode(std::move(application.m_StartMode)),
    m_Components(std::move(application.m_Components)),
    m_Executables(std::move(application.m_Executables)),
    m_ProcEnvironments(std::move(application.m_ProcEnvironments)),
    m_IncludedFiles(std::move(application.m_IncludedFiles)),
    m_ImportedFiles(std::move(application.m_ImportedFiles)),
    m_Groups(std::move(application.m_Groups)),
    m_NumProcs(std::move(application.m_NumProcs)),
    m_MqueueSize(std::move(application.m_MqueueSize)),
    m_RtSignalQueueSize(std::move(application.m_RtSignalQueueSize)),
    m_MemLimit(std::move(application.m_MemLimit)),
    m_CpuShare(std::move(application.m_CpuShare)),
    m_FileSystemSize(std::move(application.m_FileSystemSize))
{
}




//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
App& App::operator =
(
    App&& application  ///
)
{
    if (&application != this)
    {
        m_Name = std::move(application.m_Name);
        m_DefFilePath = std::move(application.m_DefFilePath);
        m_IsSandboxed = std::move(application.m_IsSandboxed);
        m_Debug = std::move(application.m_Debug);
        m_StartMode = std::move(application.m_StartMode);
        m_Components = std::move(application.m_Components);
        m_Executables = std::move(application.m_Executables);
        m_ProcEnvironments = std::move(application.m_ProcEnvironments);
        m_IncludedFiles = std::move(application.m_IncludedFiles);
        m_ImportedFiles = std::move(application.m_ImportedFiles);
        m_Groups = std::move(application.m_Groups);
        m_NumProcs = std::move(application.m_NumProcs);
        m_MqueueSize = std::move(application.m_MqueueSize);
        m_RtSignalQueueSize = std::move(application.m_RtSignalQueueSize);
        m_MemLimit = std::move(application.m_MemLimit);
        m_CpuShare = std::move(application.m_CpuShare);
        m_FileSystemSize = std::move(application.m_FileSystemSize);
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the application definition file.
 *
 * @note    The application name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void App::DefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = path;

    m_Name = AppNameFromDefFilePath(m_DefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the application definition file.
 *
 * @note    The application name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void App::DefFilePath
(
    std::string&& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = std::move(path);

    m_Name = AppNameFromDefFilePath(m_DefFilePath);
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a component to the application.
 */
//--------------------------------------------------------------------------------------------------
Component& App::CreateComponent
(
    const std::string& name ///< Name of the component.
)
{
    auto foundItem = m_Components.find(name);

    if (foundItem != m_Components.end())
    {
        throw Exception("Attempting to add multiple components with the same name: '" + name + "'");
    }

    Component& component = m_Components[name];

    component.Name(name);

    component.AddImportedFile( {    PERMISSION_READABLE,
                                    std::string("lib/lib") + name + ".so",
                                    "/lib/" } );

    return component;
}



//--------------------------------------------------------------------------------------------------
/**
 * Creates a new executable in the application.
 *
 * @return A reference to the Executable object.
 */
//--------------------------------------------------------------------------------------------------
Executable& App::CreateExecutable
(
    const std::string& path ///< Name of the executable (path relative to exe output dir).
)
//--------------------------------------------------------------------------------------------------
{
    if (path[0] == '\0')
    {
        throw Exception("Executable has no name.");
    }

    auto foundItem = m_Executables.find(path);

    if (foundItem != m_Executables.end())
    {
        throw Exception("Attempting to add multiple executables with the same name: '"
                                  + path + "'");
    }

    // Create a new Executable object in the App's list of executables.
    Executable& exe = m_Executables[path];
    exe.OutputPath(path);   // NOTE: This has the side effect of setting the exe's CName.

    // Add the executable to the list of files to be imported into the sandbox.
    AddImportedFile( {  PERMISSION_READABLE | PERMISSION_EXECUTABLE,
                        std::string("bin/") + path,
                        "/bin/" } );

    return exe;
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a file from the build host's file system to an application (bundles it into the app),
 * making it appear at a specific location in the application sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void App::AddIncludedFile
(
    FileMapping&& mapping
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

    m_IncludedFiles.push_back(mapping);
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a file from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void App::AddImportedFile
(
    FileMapping&& mapping
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
 * Add the app's user to a group (copy the string in).
 **/
//--------------------------------------------------------------------------------------------------
void App::AddGroup
(
    const std::string& groupName
)
//--------------------------------------------------------------------------------------------------
{
    m_Groups.insert(groupName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the app's user to a group (move the string in).
 **/
//--------------------------------------------------------------------------------------------------
void App::AddGroup
(
    std::string&& groupName
)
//--------------------------------------------------------------------------------------------------
{
    m_Groups.insert(groupName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Bind two interfaces together.
 */
//--------------------------------------------------------------------------------------------------
void App::Bind
(
    const std::string& importProcess,
    const std::string& importInterface,
    const std::string& exportProcess,
    const std::string& exportInterface
)
//--------------------------------------------------------------------------------------------------
{
    throw Exception("Binding not supported.");
}


}
