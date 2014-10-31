//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the System class.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "limit.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Extract the name of a system from the path of its .sdef file.
 *
 * @return the name.
 */
//--------------------------------------------------------------------------------------------------
static std::string SystemNameFromDefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto endPos = path.rfind(".");
    auto startPos = path.rfind("/");

    if ((endPos == std::string::npos) || (path.substr(endPos) != ".sdef"))
    {
        throw Exception("'" + path + "' does not appear to be a system definition file path.");
    }

    if (startPos == std::string::npos)
    {
        startPos = 0;
    }
    else
    {
        startPos++;
    }

    std::string systemName = path.substr(startPos, endPos - startPos);

    if (systemName.length() > LIMIT_MAX_SYSTEM_NAME_LEN)
    {
        throw Exception("System name " + systemName +
                        " is too long.  System names must be a maximum of " +
                        std::to_string(LIMIT_MAX_SYSTEM_NAME_LEN) + " characters.");
    }

    return systemName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move constructor.
 **/
//--------------------------------------------------------------------------------------------------
System::System
(
    System&& system
)
//--------------------------------------------------------------------------------------------------
:   m_Name(std::move(system.m_Name)),
    m_Version(std::move(system.m_Version)),
    m_DefFilePath(std::move(system.m_DefFilePath)),
    m_Apps(std::move(system.m_Apps)),
    m_ApiBinds(std::move(system.m_ApiBinds))
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 **/
//--------------------------------------------------------------------------------------------------
System& System::operator =
(
    System&& system
)
//--------------------------------------------------------------------------------------------------
{
    if (&system != this)
    {
        m_Name = std::move(system.m_Name);
        m_Version = std::move(system.m_Version);
        m_DefFilePath = std::move(system.m_DefFilePath);
        m_Apps = std::move(system.m_Apps);
        m_ApiBinds = std::move(system.m_ApiBinds);
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the system definition file.
 *
 * @note    The system name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void System::DefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = path;

    m_Name = SystemNameFromDefFilePath(m_DefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the system definition file.
 *
 * @note    The system name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void System::DefFilePath
(
    std::string&& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = std::move(path);

    m_Name = SystemNameFromDefFilePath(m_DefFilePath);
}



//--------------------------------------------------------------------------------------------------
/**
 * Creates an application object for a given .adef file path.
 *
 * @return A reference to the App object.
 **/
//--------------------------------------------------------------------------------------------------
App& System::CreateApp
(
    const std::string& adefPath ///< [in] Path to the application's .adef file.
)
//--------------------------------------------------------------------------------------------------
{
    std::string appName = App::AppNameFromDefFilePath(adefPath);

    auto foundItem = m_Apps.find(appName);

    if (foundItem != m_Apps.end())
    {
        throw Exception("Attempting to add the same application multiple times: '" + adefPath
                        + "'.");
    }

    // Create a new App object in the System's list of applications.
    App& app = m_Apps[appName];
    app.DefFilePath(adefPath);   // NOTE: This has the side effect of setting the App's Name.

    return app;
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a binding from a client-side IPC API interface to a server-side IPC API interface.
 **/
//--------------------------------------------------------------------------------------------------
void System::AddApiBind
(
    UserToUserApiBind&& bind
)
//--------------------------------------------------------------------------------------------------
{
    // Construct a specifier string for the client-side interface that can be used to detect
    // duplicates even if a binding is done both as a user-to-X binding and as an app-to-X binding.
    std::string clientSpec;

    if (bind.IsClientAnApp())
    {
        clientSpec = "<app" + bind.ClientAppName() + ">.";
    }
    else
    {
        clientSpec = "<" + bind.ClientUserName() + ">.";
    }
    clientSpec += bind.ClientInterfaceName();

    // Check for duplicate.
    if (m_ApiBinds.find(clientSpec) != m_ApiBinds.end())
    {
        if (bind.IsClientAnApp())
        {
            throw Exception("Multiple bindings of the same API client interface: '"
                            + bind.ClientAppName() + "." + bind.ClientInterfaceName() + "'.");
        }
        else
        {
            throw Exception("Multiple bindings of the same API client interface: '"
                            + clientSpec + "'.");
        }
    }

    // Add binding to map.
    bind = m_ApiBinds[clientSpec] = std::move(bind);

    // If the client is an app, make sure there is such an app and that it actually has an
    // external, client-side interface with this name.  Then mark it bound and add the binding
    // to the application (applying override, if interface was already bound in the .adef).
    if (bind.IsClientAnApp())
    {
        auto i = m_Apps.find(bind.ClientAppName());

        if (i == m_Apps.end())
        {
            throw legato::Exception("So such app '" + bind.ClientAppName() + "' in system.");
        }

        App& app = i->second;

        auto& interface = app.FindExternalClientInterface(bind.ClientInterfaceName());
        interface.MarkBound();
        app.OverrideExternalApiBind(interface, bind);
    }

    // If the server is an app, make sure there is such an app and that it actually has an
    // external, server-side interface with this name.
    if (bind.IsServerAnApp())
    {
        auto i = m_Apps.find(bind.ServerAppName());

        if (i == m_Apps.end())
        {
            throw legato::Exception("So such app '" + bind.ServerAppName() + "' in system.");
        }

        i->second.FindExternalServerInterface(bind.ServerInterfaceName());
    }
}



} // namespace legato
