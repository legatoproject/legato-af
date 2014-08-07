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
 * Each object of this type represents a single bind of a client-side IPC interface to a
 * service that may be served by another user (e.g., another app) on the target system.
 *
 * @note The server should be either an application or a user, but never both.
 **/
//--------------------------------------------------------------------------------------------------
class ExternalApiBind
{
    private:
        std::string m_ClientInterface;      ///< Client-side interface ("exe.comp.interface").
        std::string m_ServerAppName;        ///< Name of the application serving the service.
        std::string m_ServerUserName;       ///< Name of the user serving the service.
        std::string m_ServerServiceName;    ///< Name of the service on the server-side.

    public:
        void ClientInterface(const std::string& name) { m_ClientInterface = name; }
        void ClientInterface(std::string&& name) { m_ClientInterface = std::move(name); }
        const std::string& ClientInterface() const { return m_ClientInterface; }

        void ServerUserName(const std::string& name) { m_ServerUserName = name; }
        void ServerUserName(std::string&& name) { m_ServerUserName = std::move(name); }
        const std::string& ServerUserName() const { return m_ServerUserName; }

        void ServerAppName(const std::string& name) { m_ServerAppName = name; }
        void ServerAppName(std::string&& name) { m_ServerAppName = std::move(name); }
        const std::string& ServerAppName() const { return m_ServerAppName; }

        void ServerServiceName(const std::string& name) { m_ServerServiceName = name; }
        void ServerServiceName(std::string&& name) { m_ServerServiceName = std::move(name); }
        const std::string& ServerServiceName() const { return m_ServerServiceName; }
};


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single bind of a client-side IPC interface to a
 * service that may be served by a process run by the same user (e.g., within the same app).
 **/
//--------------------------------------------------------------------------------------------------
class InternalApiBind
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
