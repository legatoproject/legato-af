//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Executable object.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef EXECUTABLE_H_INCLUDE_GUARD
#define EXECUTABLE_H_INCLUDE_GUARD

namespace legato
{


/// Represents an executable file in the target file system.
class Executable
{
    private:
        std::string m_OutputPath;   ///< File system path at which the exe will be put when built.
        std::string m_CName;        ///< Name to use in identifiers in generated C code.

        /// List of pointers to components used in the executable, keyed by component path.
        std::map<std::string, Component*> m_Components;

        /// List of instances of components that will be started by this executable.
        /// (Including one instance of the default component.)
        std::list<ComponentInstance> m_ComponentInstances;

        /// The "default" component that every executable has.
        /// Auto-generated code (such as main()) and all source files added directly to
        /// the executable are all in the "default" component.
        /// @todo Remove the Default Component member and have the Application Parser and mkexe
        ///       construct the default component (with all other components in the exe as
        ///       sub-components) and then add an instance of the default component just like
        ///       it would add any other component instance (by calling AddComponentInstance()).
        Component m_DefaultComponent;

    public:
        Executable();
        Executable(Executable&& executable);
        Executable(const Executable& executable) = delete;
        virtual ~Executable() {};

    public:
        Executable& operator =(Executable&& executable);
        Executable& operator =(const Executable& executable) = delete;

    public:
        void OutputPath(const std::string& path);
        const std::string& OutputPath() const;

        void CName(const std::string& name);
        const std::string& CName() const;

        const std::map<std::string, Component*>& ComponentMap() const { return m_Components; }
        std::map<std::string, Component*>& ComponentMap() { return m_Components; }

        ComponentInstance* AddComponentInstance(Component* componentPtr);
        const std::list<ComponentInstance>& ComponentInstances() const;
        std::list<ComponentInstance>& ComponentInstances() { return m_ComponentInstances; }
        ComponentInstance& FindComponentInstance(const std::string& name);

        /// Add a library directly to the executable's "default" component.
        void AddLibrary(std::string path);

        /// Add a source code file directly to the executable's "default" component.
        void AddSourceFile(std::string path);

        /// Fetch a reference to the executable's "default component".
        Component& DefaultComponent() { return m_DefaultComponent; }
        const Component& DefaultComponent() const { return m_DefaultComponent; }

        bool HasCSources() const;
        bool HasCxxSources() const;
};


}

#endif // EXECUTABLE_H_INCLUDE_GUARD
