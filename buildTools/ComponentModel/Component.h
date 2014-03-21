//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Component class.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
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

        std::list<std::string> m_CSourcesList; ///< List of paths of C source code files to include.
        std::list<std::string> m_LibraryList;  ///< List of paths of external library files to link to.
        std::list<std::string> m_IncludePath;  ///< List of paths to include file search directories.

        /// Map of import interface names to api files.
        ImportedInterfaceMap m_ImportedInterfaces;

        /// Map of export interface names to api files.
        ExportedInterfaceMap m_ExportedInterfaces;

        /// List of files to be included in any application that includes this component.
        std::list<FileMapping> m_IncludedFiles;

        /// List of files that are to be mapped into the app sandbox from elsewhere on the target.
        std::list<FileMapping> m_ImportedFiles;

        /// Map of pool names to their sizes (number of blocks).
//        std::map<std::string, size_t> m_PoolMap;

        /// Map of configuration tree paths to values.
        /// Paths are relative to the root of the application's own configuration tree.
//        std::list<ConfigItem> m_ConfigItemMap;

    public:
        Component() {};
        Component(Component&& component);
        Component(const Component& component) = delete;
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
        const std::string& Path(void) const;

        void AddSourceFile(const std::string& path);
        const std::list<std::string>& CSourcesList() const { return m_CSourcesList; }

        void AddIncludedFile(FileMapping&& mapping);  ///< Include a file from the host file system.
        const std::list<FileMapping>& IncludedFiles() const { return m_IncludedFiles; }

        void AddImportedFile(FileMapping&& mapping);  ///< Import a file from outside the sandbox.
        const std::list<FileMapping>& ImportedFiles() const { return m_ImportedFiles; }

        void AddLibrary(const std::string& path);
        const std::list<std::string>& LibraryList() const { return m_LibraryList; }

        void AddIncludeDir(const std::string& path);
        const std::list<std::string>& IncludePath() const { return m_IncludePath; }

        void AddImportedInterface(const std::string& name, const std::string& apiFilePath);
        const ImportedInterfaceMap& ImportedInterfaces(void) const { return m_ImportedInterfaces; }

        void AddExportedInterface(const std::string& name, const std::string& apiFilePath);
        const ExportedInterfaceMap& ExportedInterfaces(void) const { return m_ExportedInterfaces; }
};


}

#endif  // LEGATO_COMPONENT_H_INCLUDE_GUARD
