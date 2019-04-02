//--------------------------------------------------------------------------------------------------
/**
 * @file component.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Represents a single component.
 */
//--------------------------------------------------------------------------------------------------
struct Component_t : public HasTargetInfo_t
{
    const parseTree::CdefFile_t* defFilePtr;  ///< Pointer to root of parse tree for .cdef file.

    std::string dir;        ///< Absolute path to the directory containing the .cdef file.

    std::string name;       ///< Component name.

    std::string workingDir; ///< Working dir path for this component, relative to working dir root.

    std::string initFuncName;   ///< Real name of the COMPONENT_INIT function ("" if no lib).

    std::list<ObjectFile_t*> cObjectFiles;  ///< List of .o files to build from C source files.
    std::list<ObjectFile_t*> cxxObjectFiles;///< List of .o files to build from C++ source files.
    std::list<std::string>  headerDirs; ///< List of directory to search for header files.

    std::list<JavaPackage_t*> javaPackages; ///< List of packages of Java code.
    std::list<PythonPackage_t*> pythonPackages; ///< List of packages of Python code.
    std::list<std::string> externalBuildCommands; ///< List of external build commands.

    std::set<std::string> staticLibs;   ///< Static library files required by this component.
    std::set<std::string> providedLibs; ///< Library files provided by this component.

    std::list<std::string> ldFlags;     ///< List of linker options.
    std::list<std::string> cFlags;      ///< List of options to pass to the C compiler.
    std::list<std::string> cxxFlags;    ///< List of options to pass to the C++ compiler.

    typedef struct {
        Component_t* componentPtr;
        bool isProvideHeader;
    } ComponentProvideHeader_t; ///< Structure of component pointer and bool for [provide-header]

    std::list<ComponentProvideHeader_t> subComponents; ///< List of subcomponents

    FileObjectPtrSet_t bundledFiles; ///< List of files to be bundled in the app.
    FileObjectPtrSet_t bundledDirs;  ///< List of directories to be bundled in the app.

    FileObjectPtrSet_t requiredFiles; ///< List of files to be imported into the app.
    FileObjectPtrSet_t requiredDirs;  ///< List of dirs to be imported into the app.
    FileObjectPtrSet_t requiredDevices;///< List of devices to be imported into the app.

    ///< Map of required modules.
    ///< Key is module name and value is the struct of token pointer and its bool 'optional' value.
    std::map<std::string, model::Module_t::ModuleInfoOptional_t> requiredModules;

    std::list<ApiTypesOnlyInterface_t*> typesOnlyApis;///< List of API files to import types from.
    std::list<ApiServerInterface_t*> serverApis;  ///< List of server-side interfaces implemented.
    std::list<ApiClientInterface_t*> clientApis;  ///< List of client-side interfaces needed.

    std::set<const ApiFile_t*> clientUsetypesApis; ///< .api files imported by client-side APIs.
    std::set<const ApiFile_t*> serverUsetypesApis; ///< .api files imported by server-side APIs.

    std::set<std::string> implicitDependencies; ///< Changes to these files triggers a re-link.

    // Get a pre-existing Component object for the component found at a given directory path.
    // @return Pointer to the object or NULL if not found.
    static Component_t* GetComponent(const std::string& path);

    // Create a new Component object for the component found at a given directory path.
    // @return Pointer to the object.
    // @throw model::Exception_t if already exists.
    static Component_t* CreateComponent(const parseTree::CdefFile_t* filePtr);

    void GetBundledFilesOfType(BundleAccess_t access, const std::string& fileType,
                               std::list<std::string>& fileList);

    bool isStandAloneComp; ///< true = generate stand-alone component

    // Does the component have C code?
    bool HasCCode() const
    {
        return cObjectFiles.empty() != true;
    }

    // Does the component have C++ code?
    bool HasCppCode() const
    {
        return cxxObjectFiles.empty() != true;
    }

    // Does the component have C or C++ code?
    bool HasCOrCppCode() const
    {
        return HasCCode() || HasCppCode();
    }

    // Does the component have Java code?
    bool HasJavaCode() const
    {
        return javaPackages.empty() != true;
    }

    // Does the component have Python code?
    bool HasPythonCode() const
    {
        return pythonPackages.empty() != true;
    }

    // Is the component built using an external build process
    bool HasExternalBuild() const
    {
        return externalBuildCommands.empty() != true;
    }

    // Does the component have code in multiple languages that are incompatible?
    bool HasIncompatibleLanguageCode() const
    {
        int buildMethods = 0;
        if (HasCOrCppCode())    { ++buildMethods; }
        if (HasJavaCode())      { ++buildMethods; }
        if (HasPythonCode())    { ++buildMethods; }
        if (HasExternalBuild()) { ++buildMethods; }

        return buildMethods > 1;
    }

    void ThrowIncompatibleLanguageException(const parseTree::CompoundItem_t* conflictSectionPtr)
        const __attribute__((noreturn));

protected:

    Component_t(const parseTree::CdefFile_t* filePtr);

    /// Map of file paths to pointers to Component objects.
    /// This is used to keep a single, unique component object for each unique component directory
    /// path. The key is the cannonical path to the directory.  The value is a pointer to an object.
    static std::map<std::string, Component_t*> ComponentMap;

public:

    static const std::map<std::string, Component_t*>& GetComponentMap() { return ComponentMap; }
};


struct Exe_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents an instatiation of a component within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ComponentInstance_t : public HasTargetInfo_t
{
    Exe_t* exePtr;

    Component_t* componentPtr;

    std::list<ApiServerInterfaceInstance_t*> serverApis; ///< Server-side interface instances.

    std::list<ApiClientInterfaceInstance_t*> clientApis; ///< Client-side interface instances.

    /// Component instances required by this one (e.g. locally bound components).
    std::set<ComponentInstance_t*> requiredComponentInstances;

    ComponentInstance_t(Exe_t* ePtr, Component_t* cPtr);

    ApiServerInterfaceInstance_t* FindServerInterface(const std::string& name);
    ApiClientInterfaceInstance_t* FindClientInterface(const std::string& name);
};


#endif // LEGATO_DEFTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD
