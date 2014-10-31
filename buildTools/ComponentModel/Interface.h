//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Interface class and its sub-classes, which represent inter-component
 * interfaces.
 *
 * (NOT including the Bind objects that connect interface instances.  See IpcBinding.h for that.)
 *
 * Currently, only "Singleton interfaces" are supported, of which are shared by all instances
 * of the same component in the same executable.  Eventually, for client-side interfaces
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef INTERFACE_H_INCLUDE_GUARD
#define INTERFACE_H_INCLUDE_GUARD

namespace legato
{


class App;
class Executable;
class ComponentInstance;


/// Common base class for imported and exported interfaces.
class Interface
{
    protected:

        Interface() {};
        Interface(const std::string& name, Api_t* apiPtr);
        Interface(std::string&& name, Api_t* apiPtr);
        Interface(const Interface& original);
        Interface(Interface&& rvalue);

    public:

        virtual ~Interface() {};

    protected:

        std::string m_InternalName; ///< Name used inside the component to refer to the interface.
        std::string m_ExternalName; ///< Name used when talking to the Service Directory.
        const Api_t* m_ApiPtr;      ///< Pointer to the object representing the IPC API protocol.
        Library m_Library;          ///< The generated code library (.so) for the interface.
        bool m_IsExternalToApp;     ///< true if this is one of the app's external interfaces.
        bool m_ManualStart;  ///< true = IPC init function shouldn't be called by generated main().
        const ComponentInstance* m_ComponentInstancePtr; ///< Comp. instance interface belongs to.

    public:

        void InternalName(const std::string& name) { m_InternalName = name; }
        void InternalName(std::string&& name) { m_InternalName = std::move(name); }
        const std::string& InternalName() const { return m_InternalName; }

        std::string ExternalName() const;

        void ComponentInstancePtr(const ComponentInstance* componentInstancePtr);
        const ComponentInstance* ComponentInstancePtr() const { return m_ComponentInstancePtr; }

        std::string AppUniqueName() const;

        const Api_t& Api() const { return *m_ApiPtr; }

        void Lib(const Library& lib) { m_Library = lib; }
        void Lib(Library&& lib) { m_Library = std::move(lib); }
        Library& Lib() { return m_Library; }
        const Library& Lib() const { return m_Library; }

        void MakeExternalToApp(const std::string& name) {   m_IsExternalToApp = true;
                                                            m_ExternalName = name; }
        void MakeExternalToApp(std::string&& name) {    m_IsExternalToApp = true;
                                                        m_ExternalName = std::move(name); }
        bool IsExternalToApp() const { return m_IsExternalToApp; }

        bool ManualStart() const { return m_ManualStart; }
        void MarkManualStart() { m_ManualStart = true; }

    public:

        static void SplitAppUniqueName(const std::string& interfaceSpec,
                                       std::string& exeName,
                                       std::string& componentName,
                                       std::string& interfaceName);
};


/// Represents a client-side (required) IPC API Interface.
class ClientInterface : public Interface
{
    public:

        ClientInterface(): m_IsBound(false) {};
        ClientInterface(const std::string& name, Api_t* apiPtr);
        ClientInterface(const ClientInterface& original);
        ClientInterface(ClientInterface&& rvalue);

        virtual ~ClientInterface() { }

    private:

        bool m_IsBound;
        bool m_TypesOnly;

    public:

        ClientInterface& operator=(const ClientInterface& original);
        ClientInterface& operator=(ClientInterface&& rvalue);

    public:

        bool TypesOnly() const { return m_TypesOnly; }
        void MarkTypesOnly() { m_TypesOnly = true; }

        bool IsBound() const { return m_IsBound; }
        void MarkBound() { m_IsBound = true; }

        bool IsSatisfied() const;

};


/// Represents a server-side (provided) IPC API Interface.
class ServerInterface : public Interface
{
    public:
        ServerInterface() {};
        ServerInterface(const std::string& name, Api_t* apiPtr);
        ServerInterface(const ServerInterface& original);
        ServerInterface(ServerInterface&& rvalue);

        virtual ~ServerInterface() { }

    private:

        bool m_IsAsync; ///< true if the server needs to handle requests asynchronously.

    public:
        ServerInterface& operator=(const ServerInterface& original);
        ServerInterface& operator=(ServerInterface&& rvalue);

    public:

        bool IsAsync() const { return m_IsAsync; }
        void MarkAsync() { m_IsAsync = true; }
};


/// A map of interface names to client-side IPC API interface objects.
typedef std::map<std::string, ClientInterface> ClientInterfaceMap;

/// A map of interface names to server-side IPC API interface objects.
typedef std::map<std::string, ServerInterface> ServerInterfaceMap;


}

#endif // INTERFACE_H_INCLUDE_GUARD
