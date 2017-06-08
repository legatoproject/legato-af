//--------------------------------------------------------------------------------------------------
/**
 * @file api.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD


struct Component_t;


//--------------------------------------------------------------------------------------------------
/**
 * Represents a component's reference to an API file.
 */
//--------------------------------------------------------------------------------------------------
struct ApiFile_t
{
    std::string path;   ///< Absolute path to the .api file.

    std::string defaultPrefix; ///< Default prefix for generated code identifiers and files.

    std::list<ApiFile_t*> includes; ///< List of other .api files that this one uses types from.

    bool isIncluded;    ///< true if this .api file is included by other .api files (via USETYPES).

    std::string codeGenDir; ///< Path to code generation dir relative to working directory.

    // Functions to fetch relative paths to files (relative to root of working dir tree).
    std::string GetClientInterfaceFile (const std::string& internalName) const;
    std::string GetServerInterfaceFile(const std::string& internalName) const;
    std::string GetAsyncServerInterfaceFile(const std::string& internalName) const;
    std::string GetJavaInterfaceFile(const std::string& internalName) const;

    /// Get a pre-existing API file object for the .api file at a given path.
    /// @return Pointer to the object or NULL if not found.
    static ApiFile_t* GetApiFile(const std::string& path);

    /// Create a new API file object for the .api file at a given path.
    /// @return Pointer to the object.
    /// @throw model::Exception_t if already exists.
    static ApiFile_t* CreateApiFile(const std::string& path);

    // Get a reference to the master map containing all the API files that have been referenced.
    static const std::map<std::string, ApiFile_t*>& GetApiFileMap() { return ApiFileMap; }

    // Get paths for all client-side interface .h files generated for all
    // .api files included by this one.  Results are added to the set provided.
    void GetClientUsetypesApiHeaders(std::set<std::string>& results);

    // Get paths for all server-side interface .h files generated for all
    // .api files included by this one.  Results are added to the set provided.
    void GetServerUsetypesApiHeaders(std::set<std::string>& results);

protected:

    ApiFile_t(const std::string& p);

    /// Map of file paths to pointers to API File objects.
    /// This is used to keep a single, unique API File object for each unique .api file.
    /// The key is the cannonical path to the .api file.  The value is a pointer to an object.
    static std::map<std::string, ApiFile_t*> ApiFileMap;

};


//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold paths to the C code for a generated interface.
 */
//--------------------------------------------------------------------------------------------------
struct InterfaceCFiles_t
{
    std::string interfaceFile;  ///< .h file that gets included by interfaces.h.
    std::string internalHFile;  ///< local.h file that gets included by generated .c code.
    std::string sourceFile;     ///< Generated .c file.
    std::string objectFile;     ///< Path to the .o file for this interface.
};


//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold paths to the Python code for a generated interface.
 */
//--------------------------------------------------------------------------------------------------
struct InterfacePythonFiles_t
{
    std::string cExtensionBinaryFile;
    std::string cExtensionObjectFile;
    std::string cdefSourceFile;
    std::string cExtensionSourceFile;
    std::string wrapperSourceFile;
};


//--------------------------------------------------------------------------------------------------
/**
 * Structure to hold paths to the Java code for a generated interface.
 */
//--------------------------------------------------------------------------------------------------
struct InterfaceJavaFiles_t
{
    std::string interfaceSourceFile;
    std::string implementationSourceFile;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a component's reference to an API file.
 */
//--------------------------------------------------------------------------------------------------
struct ApiRef_t
{
    ApiFile_t* apiFilePtr;    ///< Pointer to the API file object.

    Component_t* componentPtr;  ///< Pointer to the component (NULL if unknown).

    const std::string internalName;   ///< Name used inside the component to refer to the interface.

protected:

    ApiRef_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);

public:

    virtual void GetInterfaceFiles(InterfaceCFiles_t& cFiles) const = 0;
    virtual void GetInterfaceFiles(InterfaceJavaFiles_t& javaFiles) const = 0;
    virtual void GetInterfaceFiles(InterfacePythonFiles_t& pythonFiles) const = 0;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents inclusion of types from an IPC API interface definition (.api file).
 */
//--------------------------------------------------------------------------------------------------
struct ApiTypesOnlyInterface_t : public ApiRef_t
{
    ApiTypesOnlyInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);

    virtual void GetInterfaceFiles(InterfaceCFiles_t& cFiles) const;
    virtual void GetInterfaceFiles(InterfaceJavaFiles_t& javaFiles) const;
    virtual void GetInterfaceFiles(InterfacePythonFiles_t& pythonFiles) const;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a client-side IPC API interface.
 */
//--------------------------------------------------------------------------------------------------
struct ApiClientInterface_t : public ApiRef_t
{
    bool manualStart;   ///< true = generated main() should not call the ConnectService() function.
    bool optional;      ///< true = okay to not be bound.

    ApiClientInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName);

    virtual void GetInterfaceFiles(InterfaceCFiles_t& cFiles) const;
    virtual void GetInterfaceFiles(InterfaceJavaFiles_t& javaFiles) const;
    virtual void GetInterfaceFiles(InterfacePythonFiles_t& pythonFiles) const;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a server-side IPC API interface.
 */
//--------------------------------------------------------------------------------------------------
struct ApiServerInterface_t : public ApiRef_t
{
    const bool async;         ///< true = component wants to use asynchronous mode of operation.
    bool manualStart;   ///< true = generated main() should not call AdvertiseService() function.

    ApiServerInterface_t(ApiFile_t* aPtr, Component_t* cPtr, const std::string& iName, bool async);

    virtual void GetInterfaceFiles(InterfaceCFiles_t& cFiles) const;
    virtual void GetInterfaceFiles(InterfaceJavaFiles_t& javaFiles) const;
    virtual void GetInterfaceFiles(InterfacePythonFiles_t& pythonFiles) const;
};


struct ComponentInstance_t;
struct Binding_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of an IPC API interface within an executable.
 *
 * This is a base class that cannot be instantiated on its own.
 */
//--------------------------------------------------------------------------------------------------
struct ApiInterfaceInstance_t
{
    /// Component instance this interface instance belongs to (NULL = pre-built interface).
    ComponentInstance_t* componentInstancePtr;

    std::string name;   ///< Name used to identify this interface to the service directory.

    const parseTree::Token_t* externMarkPtr; ///< Ptr to the name token in the parse tree where
                                             /// this was marked "extern".  NULL if not extern.

protected:

    ApiInterfaceInstance_t(ComponentInstance_t* cInstPtr, const std::string& internalName);

    virtual ~ApiInterfaceInstance_t() {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of a client-side IPC API interface within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ApiClientInterfaceInstance_t : public ApiInterfaceInstance_t
{
    ApiClientInterface_t* ifPtr;    ///< Interface this is an instance of.

    Binding_t* bindingPtr;  ///< Ptr to the binding, or NULL if not bound.

    ApiClientInterfaceInstance_t(ComponentInstance_t* cInstPtr, ApiClientInterface_t* p);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents an instantiation of a server-side IPC API interface within an executable.
 **/
//--------------------------------------------------------------------------------------------------
struct ApiServerInterfaceInstance_t : public ApiInterfaceInstance_t
{
    ApiServerInterface_t* ifPtr;    ///< Interface this is an instance of.

    ApiServerInterfaceInstance_t(ComponentInstance_t* cInstPtr, ApiServerInterface_t* p);
};



#endif // LEGATO_MKTOOLS_MODEL_API_H_INCLUDE_GUARD
