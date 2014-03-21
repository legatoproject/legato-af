//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Executable object.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
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
        std::list<ComponentInstance> m_ComponentInstanceList;

        /// The "default" component that every executable has.
        /// Auto-generated code (such as main()) and all source files added directly to
        /// the executable are all in the "default" component.  There will always be exactly one
        /// instance of the default component, and it is treated differently than other components
        /// in that it does not get linked to a shared library file.
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

        void AddComponentInstance(ComponentInstance&& instance);
        const std::list<ComponentInstance>& ComponentInstanceList() const;

        /// Add a library directly to the executable's "default" component instance.
        void AddLibrary(std::string path);

        /// Add a C source code file directly to the executable's "default" component instance.
        void AddCSourceFile(std::string path);

        /// Fetch a reference to the executable's "default component".
        Component& DefaultComponent() { return m_DefaultComponent; }
        const Component& DefaultComponent() const { return m_DefaultComponent; }
};


}

#endif // EXECUTABLE_H_INCLUDE_GUARD
