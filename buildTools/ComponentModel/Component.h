//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Component class.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_COMPONENT_H_INCLUDE_GUARD
#define LEGATO_COMPONENT_H_INCLUDE_GUARD


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Each object of this type represents a single static software component.
 *
 * @warning This is not to be confused with an @ref ComponentInstance, which represents a runtime
 *          instantiation of a component.
 **/
//--------------------------------------------------------------------------------------------------
class Component
{
    private:

        std::string m_Name;                 ///< Name of the component.
        std::string m_CName;                ///< Name to be used for the component in C identifiers.
        std::string m_Path;                 ///< Path to the component's directory.

        Library m_Library;                  ///< Details of the library file for this component.

        std::list<std::string> m_CSourcesList; ///< List of paths of C source code files to include.
        std::list<std::string> m_LibraryList;  ///< List of paths of external lib files to link to.
        std::list<std::string> m_IncludePath;  ///< List of paths to include file search dirs.

        bool m_HasCSources;
        bool m_HasCppSources;

        /// Map of import interface names to api files.
        ImportedInterfaceMap m_ImportedInterfaces;

        /// Map of export interface names to api files.
        ExportedInterfaceMap m_ExportedInterfaces;

        /// List of files to be included in any application that includes this component.
        std::list<FileMapping> m_BundledFiles;

        /// List of directories to be included in any application that includes this component.
        std::list<FileMapping> m_BundledDirs;

        /// List of files to be mapped into the app sandbox from elsewhere on the target.
        std::list<FileMapping> m_RequiredFiles;

        /// List of directories to be mapped into the app sandbox from elsewhere on the target.
        std::list<FileMapping> m_RequiredDirs;

        /// Map of pool names to their sizes (number of blocks).
//        std::map<std::string, size_t> m_Pools;

        /// Map of configuration tree paths to values.
        /// Paths are relative to the root of the application's own configuration tree.
//        std::list<ConfigItem> m_ConfigItems;

        /// Map of component paths to pointers to components that this component depends on.
        /// The content of an entry can be NULL if that component hasn't been parsed yet.
        std::map<std::string, Component*> m_SubComponents;

        /// true if the component is currently being processed (i.e., built, linked, etc.)
        /// This is used for dependency loop detection.
        bool m_BeingProcessed;

    public:

        Component() : m_HasCSources(false), m_HasCppSources(false), m_BeingProcessed(false) {}
        Component(Component&& component);
        Component(const Component& component) = delete;

    public:

        virtual ~Component() {};

    public:

        Component& operator =(Component&& component);
        Component& operator =(const Component& component) = delete;

    public:

        void Name(const std::string& name);
        void Name(std::string&& name);
        const std::string& Name() const;

        void CName(const std::string& name);
        void CName(std::string&& name);
        const std::string& CName() const;

        void Path(const std::string& path);
        void Path(std::string&& path);
        const std::string& Path() const;

        const Library& Lib() const { return m_Library; }
        Library& Lib() { return m_Library; }

        void AddSourceFile(const std::string& path);
        const std::list<std::string>& CSourcesList() const { return m_CSourcesList; }

        bool HasCSources() const { return m_HasCSources; }
        bool HasCppSources() const { return m_HasCppSources; }

        void AddBundledFile(FileMapping&& mapping);  ///< Include a file from the host file system.
        const std::list<FileMapping>& BundledFiles() const { return m_BundledFiles; }

        void AddBundledDir(FileMapping&& mapping);  ///< Include a directory from host file system.
        const std::list<FileMapping>& BundledDirs() const { return m_BundledDirs; }

        void AddRequiredFile(FileMapping&& mapping);  ///< Import a file from outside the sandbox.
        const std::list<FileMapping>& RequiredFiles() const { return m_RequiredFiles; }

        void AddRequiredDir(FileMapping&& mapping);  ///< Import a directory from outside sandbox.
        const std::list<FileMapping>& RequiredDirs() const { return m_RequiredDirs; }

        void AddLibrary(const std::string& path);
        const std::list<std::string>& LibraryList() const { return m_LibraryList; }

        void AddSubComponent(const std::string& path, Component* componentPtr);
        const std::map<std::string, Component*>& SubComponents() const { return m_SubComponents; }
        std::map<std::string, Component*>& SubComponents() { return m_SubComponents; }

        void AddIncludeDir(const std::string& path);
        const std::list<std::string>& IncludePath() const { return m_IncludePath; }

        ImportedInterface& AddRequiredApi(const std::string& name, Api_t* apiPtr);
        const ImportedInterfaceMap& RequiredApis(void) const { return m_ImportedInterfaces; }
        ImportedInterfaceMap& RequiredApis(void) { return m_ImportedInterfaces; }

        ExportedInterface& AddProvidedApi(const std::string& name, Api_t* apiPtr);
        const ExportedInterfaceMap& ProvidedApis(void) const { return m_ExportedInterfaces; }
        ExportedInterfaceMap& ProvidedApis(void) { return m_ExportedInterfaces; }

        bool BeingProcessed() const { return m_BeingProcessed; }
        void BeingProcessed(bool beingProcessed) { m_BeingProcessed = beingProcessed; }

    public:

        // Factory function.
        static Component* CreateComponent(const std::string& path);

        // Lookup pre-existing component.
        static Component* FindComponent(const std::string& path);
};


}

#endif  // LEGATO_COMPONENT_H_INCLUDE_GUARD
