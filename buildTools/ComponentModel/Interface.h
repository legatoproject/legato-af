//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Interface class, which represents an inter-component interface.
 * (NOT including the Bind objects that connect interface instances.  See Bind.h for that.)
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef INTERFACE_H_INCLUDE_GUARD
#define INTERFACE_H_INCLUDE_GUARD

namespace legato
{


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
        std::string m_ServiceInstanceName;///< Name used when talking to the Service Directory.
        const Api_t* m_ApiPtr;      ///< Pointer to the object representing the IPC API protocol.
        std::string m_AppUniqueName;///< Fully qualified name within app (exe.component.interface).
        Library m_Library;          ///< The generated code library (.so) for the interface.
        bool m_IsInterApp;   ///< true if this is one of the app's external interfaces.
        bool m_ManualStart;  ///< true = IPC init function shouldn't be called by generated main().

    public:

        void InternalName(const std::string& name) { m_InternalName = name; }
        void InternalName(std::string&& name) { m_InternalName = std::move(name); }
        const std::string& InternalName() const { return m_InternalName; }

        void ServiceInstanceName(const std::string& name) { m_ServiceInstanceName = name; }
        void ServiceInstanceName(std::string&& name) { m_ServiceInstanceName = std::move(name); }
        const std::string& ServiceInstanceName() const { return m_ServiceInstanceName; }

        void AppUniqueName(const std::string& name) { m_AppUniqueName = name; }
        void AppUniqueName(std::string&& name) { m_AppUniqueName = std::move(name); }
        const std::string& AppUniqueName() const { return m_AppUniqueName; }

        const Api_t& Api() const { return *m_ApiPtr; }

        void Lib(const Library& lib) { m_Library = lib; }
        void Lib(Library&& lib) { m_Library = std::move(lib); }
        Library& Lib() { return m_Library; }
        const Library& Lib() const { return m_Library; }

        void MakeExternal(const std::string& name) {    m_IsInterApp = true;
                                                        m_ServiceInstanceName = name; }
        void MakeExternal(std::string&& name) {     m_IsInterApp = true;
                                                    m_ServiceInstanceName = std::move(name); }
        bool IsExternal() const { return m_IsInterApp; }

        bool ManualStart() const { return m_ManualStart; }
        void MarkManualStart() { m_ManualStart = true; }

    public:

        static void SplitAppUniqueName(const std::string& interfaceSpec,
                                       std::string& exeName,
                                       std::string& componentName,
                                       std::string& interfaceName);
};


/// Represents a client-side (required) IPC API Interface.
class ImportedInterface : public Interface
{
    public:

        ImportedInterface(): m_IsBound(false) {};
        ImportedInterface(const std::string& name, Api_t* apiPtr);
        ImportedInterface(const ImportedInterface& original);
        ImportedInterface(ImportedInterface&& rvalue);

        virtual ~ImportedInterface() { }

    private:

        bool m_IsBound;
        bool m_TypesOnly;

    public:

        ImportedInterface& operator=(const ImportedInterface& original);
        ImportedInterface& operator=(ImportedInterface&& rvalue);

    public:

        bool TypesOnly() const { return m_TypesOnly; }
        void MarkTypesOnly() { m_TypesOnly = true; }

        bool IsBound() const { return m_IsBound; }
        void MarkBound() { m_IsBound = true; }

        bool IsSatisfied() const;

};


/// Represents a server-side (provided) IPC API Interface.
class ExportedInterface : public Interface
{
    public:
        ExportedInterface() {};
        ExportedInterface(const std::string& name, Api_t* apiPtr);
        ExportedInterface(const ExportedInterface& original);
        ExportedInterface(ExportedInterface&& rvalue);

        virtual ~ExportedInterface() { }

    private:

        bool m_IsAsync; ///< true if the server needs to handle requests asynchronously.

    public:
        ExportedInterface& operator=(const ExportedInterface& original);
        ExportedInterface& operator=(ExportedInterface&& rvalue);

    public:

        bool IsAsync() const { return m_IsAsync; }
        void MarkAsync() { m_IsAsync = true; }
};


/// A map of interface names to client-side IPC API interface objects.
typedef std::map<std::string, ImportedInterface> ImportedInterfaceMap;

/// A map of interface names to server-side IPC API interface objects.
typedef std::map<std::string, ExportedInterface> ExportedInterfaceMap;


}

#endif // INTERFACE_H_INCLUDE_GUARD
