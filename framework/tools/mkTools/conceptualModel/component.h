//--------------------------------------------------------------------------------------------------
/**
 * @file component.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single component.
 */
//--------------------------------------------------------------------------------------------------
struct Component_t
{
    const parseTree::CdefFile_t* defFilePtr;  ///< Pointer to root of parse tree for .cdef file.

    std::string dir;        ///< Absolute path to the directory containing the .cdef file.

    std::string name;       ///< Component name.

    std::string workingDir; ///< Working dir path for this component, relative to working dir root.

    std::string lib;        ///< Absolute path to component library file ("" if no lib).

    std::string initFuncName;   ///< Real name of the COMPONENT_INIT function ("" if no lib).

    std::list<std::string> cSources;    ///< List of C source code files.
    std::list<std::string> cxxSources;  ///< List of C++ source code files.

    std::list<std::string> ldFlags;     ///< List of linker options.
    std::list<std::string> cFlags;      ///< List of options to pass to the C compiler.
    std::list<std::string> cxxFlags;    ///< List of options to pass to the C++ compiler.

    std::list<Component_t*> subComponents;  ///< List of components this component requires.

    std::list<FileSystemObject_t*> bundledFiles; ///< List of files to be bundled in the app.
    std::list<FileSystemObject_t*> bundledDirs;  ///< List of directories to be bundled in the app.

    std::list<FileSystemObject_t*> requiredFiles; ///< List of files to be imported into the app.
    std::list<FileSystemObject_t*> requiredDirs;  ///< List of dirs to be imported into the app.

    std::list<ApiTypesOnlyInterface_t*> typesOnlyApis;///< List of API files to import types from.
    std::list<ApiServerInterface_t*> serverApis;  ///< List of server-side interfaces implemented.
    std::list<ApiClientInterface_t*> clientApis;  ///< List of client-side interfaces needed.

    std::set<const ApiFile_t*> clientUsetypesApis; ///< .api files imported by client-side APIs.
    std::set<const ApiFile_t*> serverUsetypesApis; ///< .api files imported by server-side APIs.

    std::list<std::string> requiredLibs;    ///< List of libraries to be linked with.

    // Get a pre-existing Component object for the component found at a given directory path.
    // @return Pointer to the object or NULL if not found.
    static Component_t* GetComponent(const std::string& path);

    // Create a new Component object for the component found at a given directory path.
    // @return Pointer to the object.
    // @throw model::Exception_t if already exists.
    static Component_t* CreateComponent(const parseTree::CdefFile_t* filePtr);

protected:

    Component_t(const parseTree::CdefFile_t* filePtr);

    /// Map of file paths to pointers to Component objects.
    /// This is used to keep a single, unique component object for each unique component directory
    /// path. The key is the cannonical path to the directory.  The value is a pointer to an object.
    static std::map<std::string, Component_t*> ComponentMap;
};


struct Exe_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents an instatiation of a component within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ComponentInstance_t
{
    Exe_t* exePtr;

    Component_t* componentPtr;

    std::list<ApiServerInterfaceInstance_t*> serverApis; ///< Server-side interface instances.

    std::list<ApiClientInterfaceInstance_t*> clientApis; ///< Client-side interface instances.

    ComponentInstance_t(Exe_t* ePtr, Component_t* cPtr);
};


#endif // LEGATO_MKTOOLS_MODEL_COMPONENT_H_INCLUDE_GUARD
