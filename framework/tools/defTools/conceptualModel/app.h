//--------------------------------------------------------------------------------------------------
/**
 * @file app.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_APP_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_APP_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single application.
 */
//--------------------------------------------------------------------------------------------------
struct App_t : public HasTargetInfo_t
{
    /**
     * All possible update options for the pre-loaded application.
     */
    enum PreloadedMode_t
    {
        NONE,           ///< App is not preloaded.
        BUILD_VERSION,  ///< App is preloaded; MD5 hash must match the MD5 of this app
                        ///< in the build environment.
        SPECIFIC_MD5,   ///< App is preloaded; MD5 hash must match the MD5 that is explicitly
                        ///< specified in .sdef file.
        ANY_VERSION     ///< App is preloaded; no version check, MD5 hash can be any.
    };

    App_t(parseTree::AdefFile_t* filePtr);

    const parseTree::AdefFile_t* defFilePtr;  ///< Pointer to root of parse tree for the .adef file.
                                              ///< NULL if the app was created by mkexe.

    const parseTree::App_t* parseTreePtr; ///< Ptr to the app section in the .sdef file parse tree.
                                          ///< NULL if the app was created by mkapp.

    std::string dir;    ///< Absolute path to the directory containing the .adef file.

    std::string name;   ///< Name of the app (C identifier safe name). "" if created by mkexe.

    std::string workingDir; ///< Path to working dir for app, relative to build's root working dir.

    std::string version;    ///< Human-readable version label.

    bool isSandboxed;       ///< true if the application should be sandboxed.

    enum {AUTO, MANUAL} startTrigger;    ///< Start automatically or only when asked?

    PreloadedMode_t preloadedMode;  ///< Whether this app is preloaded, and in which mode.

    bool isPreBuilt;    ///< true = app is a pre-built app.  Affects how some error messages are
                        ///< displayed

    std::string preloadedMd5; ///< MD5 hash of preloaded app (empty if not specified).

    std::set<Component_t*> components;  ///< Set of components used in this app.

    std::map<std::string, Exe_t*> executables;  ///< Collection of executables defined in this app.

    FileObjectPtrSet_t bundledFiles; ///< List of files to be bundled in the app.
    FileObjectPtrSet_t bundledDirs;  ///< List of directories to be bundled in the app.
    FileObjectPtrSet_t bundledBinaries; ///< List of binaries to be bundled in a pre-built app.

    FileObjectPtrSet_t requiredFiles; ///< List of files to be imported into the app.
    FileObjectPtrSet_t requiredDirs;  ///< List of dirs to be imported into the app.
    FileObjectPtrSet_t requiredDevices;///< List of devices to be imported into the app.

    ///< Map of required modules.
    ///< Key is module name and value is structure of token pointer and its bool 'optional' value.
    std::map<std::string, model::Module_t::ModuleInfoOptional_t> requiredModules;

    std::list<ProcessEnv_t*> processEnvs;   ///< Process environments defined in the app.

    /// Set of the names of groups that this application's user should be a member of.
    std::set<std::string> groups;

    // Per-user limits:
    PositiveIntLimit_t      cpuShare;           ///< Relative share value
    NonNegativeIntLimit_t   maxFileSystemBytes; ///< Total bytes in sandbox tmpfs file system.
    PositiveIntLimit_t      maxMemoryBytes;     ///< Total bytes of RAM.
    NonNegativeIntLimit_t   maxMQueueBytes;     ///< Total bytes in all POSIX MQueues.
    NonNegativeIntLimit_t   maxQueuedSignals;   ///< Total number of queued signals.
    PositiveIntLimit_t      maxThreads;         ///< Number of threads.
    NonNegativeIntLimit_t   maxSecureStorageBytes;   ///< Total bytes in Secure Storage.

    // Watchdog
    WatchdogAction_t  watchdogAction;
    WatchdogTimeout_t watchdogTimeout;
    WatchdogTimeout_t maxWatchdogTimeout;

    /// Map of configuration tree names to access permissions (see permissions.h).
    std::map<std::string, Permissions_t> configTrees;

    /// Set of server-side IPC API interfaces provided by pre-built binaries in this app.
    /// Keyed by server interface name.
    std::map<std::string, ApiServerInterfaceInstance_t*> preBuiltServerInterfaces;

    /// Set of client-side IPC API interfaces required by pre-built binaries in this app.
    /// Keyed by client interface name.
    std::map<std::string, ApiClientInterfaceInstance_t*> preBuiltClientInterfaces;

    /// Map of server interfaces that external entities can bind to (key is external name).
    std::map<std::string, ApiServerInterfaceInstance_t*> externServerInterfaces;

    /// Map of client interfaces marked for later binding to external services (key is external name)
    std::map<std::string, ApiClientInterfaceInstance_t*> externClientInterfaces;

    // Function for finding component instance.  Throw exception if not found.
    ComponentInstance_t* FindComponentInstance(const parseTree::Token_t* exeTokenPtr,
                                               const parseTree::Token_t* componentTokenPtr);

    // Functions for looking up interface instances.  Throw exception if not found.
    ApiServerInterfaceInstance_t* FindServerInterface(const parseTree::Token_t* exeTokenPtr,
                                                      const parseTree::Token_t* componentTokenPtr,
                                                      const parseTree::Token_t* interfaceTokenPtr);
    ApiClientInterfaceInstance_t* FindClientInterface(const parseTree::Token_t* exeTokenPtr,
                                                      const parseTree::Token_t* componentTokenPtr,
                                                      const parseTree::Token_t* interfaceTokenPtr);
    ApiClientInterfaceInstance_t* FindClientInterface(const parseTree::Token_t* interfaceTokenPtr);
    ApiServerInterfaceInstance_t* FindServerInterface(const parseTree::Token_t* interfaceTokenPtr);
    ApiInterfaceInstance_t* FindInterface(const parseTree::Token_t* exeTokenPtr,
                                          const parseTree::Token_t* componentTokenPtr,
                                          const parseTree::Token_t* interfaceTokenPtr);

    // Get the path to the app's root.cfg file relative to the build's working directory.
    std::string ConfigFilePath() const;
};


#endif // LEGATO_DEFTOOLS_MODEL_APP_H_INCLUDE_GUARD
