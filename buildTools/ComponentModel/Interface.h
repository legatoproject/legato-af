//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Interface class, which represents an inter-component interface.
 * (NOT including the Bind objects that connect interface instances.  See Bind.h for that.)
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. All rights reserved. Use of this work is subject to license.
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
        Interface(const std::string& name, const std::string& apiFilePath);
//        Interface(const Interface& original);
//        Interface(Interface&& original);
        virtual ~Interface() {};

    private:
        std::string m_InstanceName; ///< "Friendly name" of the interface instance.
        std::string m_ApiFilePath;  ///< Path to the .api file.
        std::string m_Hash;         ///< UTF-8 representation of the crypto hash of the .api file.

    public:
        void InstanceName(const std::string& name) { m_InstanceName = name; }
        void InstanceName(std::string&& name) { m_InstanceName = std::move(name); }
        const std::string& InstanceName() const { return m_InstanceName; }

        void ApiFilePath(const std::string& path) { m_ApiFilePath = path; }
        void ApiFilePath(std::string&& path) { m_ApiFilePath = std::move(path); }
        const std::string& ApiFilePath() const { return m_ApiFilePath; }
};


class ImportedInterface : public Interface
{
    public:
        ImportedInterface() {};
        ImportedInterface(const std::string& name, const std::string& apiFilePath);
//        ImportedInterface(const ImportedInterface& original);
//        ImportedInterface(ImportedInterface&& original);
        virtual ~ImportedInterface() { }

    private:
        std::string m_BoundTo;  ///< Service instance name to connect to.  Empty if unbound.

    public:
        void Bind(const std::string& serviceInstanceName);
        std::string BoundTo() { return m_BoundTo; }
};


class ExportedInterface : public Interface
{
    public:
        ExportedInterface() {};
        ExportedInterface(const std::string& name, const std::string& apiFilePath);
//        ExportedInterface(const ExportedInterface& original);
//        ExportedInterface(ExportedInterface&& original);
        virtual ~ExportedInterface() { }

    private:
        bool m_IsHidden;    ///< true if the service should not be advertized automatically.

    public:
        void Hide() { m_IsHidden = true; }
        bool IsHidden() { return m_IsHidden; }
};


typedef std::map<std::string, ImportedInterface> ImportedInterfaceMap;
typedef std::map<std::string, ExportedInterface> ExportedInterfaceMap;


}

#endif // INTERFACE_H_INCLUDE_GUARD
