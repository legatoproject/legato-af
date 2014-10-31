//--------------------------------------------------------------------------------------------------
/**
 * Definition of the IPC Binding class.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef IPC_BINDING_H_INCLUDE_GUARD
#define IPC_BINDING_H_INCLUDE_GUARD


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single bind of a client-side IPC interface run by
 * one user (e.g., an app) to a server-side interface that may be run by another user (e.g.,
 * another app) on the target system.
 *
 * @note On each side (client or server) either an application name or a user name should be used,
 *       but never both.
 **/
//--------------------------------------------------------------------------------------------------
class UserToUserApiBind
{
    private:
        std::string m_ClientAppName;        ///< Name of the application using the service.
        std::string m_ClientUserName;       ///< Name of the user using the service.
        std::string m_ClientInterfaceName;  ///< Name of the client-side interface.
        std::string m_ServerAppName;        ///< Name of the application running the server.
        std::string m_ServerUserName;       ///< Name of the user running the server.
        std::string m_ServerInterfaceName;  ///< Name of the server-side interface.

    public:
        void ClientUserName(const std::string& name) { m_ClientUserName = name; }
        void ClientUserName(std::string&& name) { m_ClientUserName = std::move(name); }
        const std::string& ClientUserName() const { return m_ClientUserName; }

        void ClientAppName(const std::string& name) { m_ClientAppName = name; }
        void ClientAppName(std::string&& name) { m_ClientAppName = std::move(name); }
        const std::string& ClientAppName() const { return m_ClientAppName; }

        void ClientInterfaceName(const std::string& name) { m_ClientInterfaceName = name; }
        void ClientInterfaceName(std::string&& name) { m_ClientInterfaceName = std::move(name); }
        const std::string& ClientInterfaceName() const { return m_ClientInterfaceName; }

        void ServerUserName(const std::string& name) { m_ServerUserName = name; }
        void ServerUserName(std::string&& name) { m_ServerUserName = std::move(name); }
        const std::string& ServerUserName() const { return m_ServerUserName; }

        void ServerAppName(const std::string& name) { m_ServerAppName = name; }
        void ServerAppName(std::string&& name) { m_ServerAppName = std::move(name); }
        const std::string& ServerAppName() const { return m_ServerAppName; }

        void ServerInterfaceName(const std::string& name) { m_ServerInterfaceName = name; }
        void ServerInterfaceName(std::string&& name) { m_ServerInterfaceName = std::move(name); }
        const std::string& ServerInterfaceName() const { return m_ServerInterfaceName; }

        bool IsClientAnApp() const { return (! (m_ClientAppName.empty())); }
        bool IsServerAnApp() const { return (! (m_ServerAppName.empty())); }
};


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single bind of a client-side IPC interface on a given
 * executable to a server-side interface that may be served by another user (e.g., another app)
 * on the target system.
 *
 * This type of binding only makes sense within the context of an application.
 *
 * @note The server should be either an application or a user, but never both.
 **/
//--------------------------------------------------------------------------------------------------
class ExeToUserApiBind
{
    private:
        std::string m_ClientInterfaceSpec;  ///< Client-side interface ("exe.comp.interface").
        std::string m_ServerAppName;        ///< Name of the application running the server.
        std::string m_ServerUserName;       ///< Name of the user running the server.
        std::string m_ServerInterfaceName;  ///< Name of the interface on the server-side.

    public:
        void ClientInterface(const std::string& name) { m_ClientInterfaceSpec = name; }
        void ClientInterface(std::string&& name) { m_ClientInterfaceSpec = std::move(name); }
        const std::string& ClientInterface() const { return m_ClientInterfaceSpec; }

        void ServerUserName(const std::string& name) { m_ServerUserName = name; }
        void ServerUserName(std::string&& name) { m_ServerUserName = std::move(name); }
        const std::string& ServerUserName() const { return m_ServerUserName; }

        void ServerAppName(const std::string& name) { m_ServerAppName = name; }
        void ServerAppName(std::string&& name) { m_ServerAppName = std::move(name); }
        const std::string& ServerAppName() const { return m_ServerAppName; }

        void ServerInterfaceName(const std::string& name) { m_ServerInterfaceName = name; }
        void ServerInterfaceName(std::string&& name) { m_ServerInterfaceName = std::move(name); }
        const std::string& ServerInterfaceName() const { return m_ServerInterfaceName; }
};


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single bind of an executable's client-side IPC interface
 * to an executable's server-side IPC interface that may be served by a process run by the same user
 * (e.g., within the same app).
 *
 * This type of bind only makes sense within the context of an application.
 **/
//--------------------------------------------------------------------------------------------------
class ExeToExeApiBind
{
    private:
        std::string m_ClientInterface;  ///< Client-side interface ("exe.comp.interface").
        std::string m_ServerInterface;  ///< Server-side interface ("exe.comp.interface").

    public:
        void ClientInterface(const std::string& name) { m_ClientInterface = name; }
        void ClientInterface(std::string&& name) { m_ClientInterface = std::move(name); }
        const std::string& ClientInterface() const { return m_ClientInterface; }

        void ServerInterface(const std::string& name) { m_ServerInterface = name; }
        void ServerInterface(std::string&& name) { m_ServerInterface = std::move(name); }
        const std::string& ServerInterface() const { return m_ServerInterface; }
};


}


#endif  // IPC_BINDING_H_INCLUDE_GUARD
