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

        bool m_IsDebuggingEnabled;  ///< True if the executable should be built with debugging
                                    ///  support turned on.

        /// List of instances of components that will be started by this executable.
        /// (Including one instance of the default component.)
        std::list<ComponentInstance> m_ComponentInstances;

        /// The "default" component that every executable has.
        /// Auto-generated code (such as main()) and all source files added directly to
        /// the executable are all in the "default" component.  There will always be exactly one
        /// instance of the default component, and it is treated differently than other components
        /// in that it gets statically linked into the executable, while other libs get
        /// dynamically linked
        /// TODO: Change to treat more like a regular component instance when --static-link option
        ///       is supported in mk tools.
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

        void EnableDebugging() { m_IsDebuggingEnabled = true; }
        bool IsDebuggingEnabled() const { return m_IsDebuggingEnabled; }

        ComponentInstance& AddComponentInstance(ComponentInstance&& instance);
        const std::list<ComponentInstance>& ComponentInstances() const;
        std::list<ComponentInstance>& ComponentInstances() { return m_ComponentInstances; }
        ComponentInstance& FindComponentInstance(const std::string& name);

        /// Add a library directly to the executable's "default" component instance.
        void AddLibrary(std::string path);

        /// Add a C/C++ source code file directly to the executable's "default" component instance.
        void AddCSourceFile(std::string path);

        /// Fetch a reference to the executable's "default component".
        Component& DefaultComponent() { return m_DefaultComponent; }
        const Component& DefaultComponent() const { return m_DefaultComponent; }

        bool HasCSources() const;
        bool HasCppSources() const;
};


}

#endif // EXECUTABLE_H_INCLUDE_GUARD
