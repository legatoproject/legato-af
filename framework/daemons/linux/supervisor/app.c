//--------------------------------------------------------------------------------------------------
/** @file supervisor/app.c
 *
 * This is the Supervisor application class.
 *
 * To instantiate an application object of this class use the app_Create() API.
 *
 * When an app object is created it sets up the working area for the application.  The working area
 * for an application is under the directory CURRENT_SYSTEM_PATH/appsWriteable/<appName>.
 *
 * Links to files that are needed by the app are created in the working area.  Bind mounts are used
 * to create links for sandboxed apps.  Symlinks are used to create links for unsandboxed apps.
 *
 * For sandboxed apps links are created to default libs/files, required dirs/files and readonly
 * bundled files.  A tmpfs is also mounted under CURRENT_SYSTEM_PATH/appsWriteable/<appName>/tmp.
 * This working area is the sandbox for the app.
 *
 * For unsandboxed apps links are created to required dirs/files and readonly bundled files.
 *
 * The reason that only readonly bundled files are linked into the working area is because the
 * writable bundled files are already copied into the working area by the app installer.
 *
 * Generally, only links to files are created because links to directories can lead to unexpected
 * behaviours.  For instance, if a link to a directory is created and then a link to a file under
 * that directory is created.  The linked file will not show up in the linked directory both inside
 * the app's working area and the in the directory's original location.
 * So, instead when a directory is required or bundled, all files in the directory are individually
 * linked.
 *
 * The working area is not cleaned up by the Supervisor, rather it is left to the installer to
 * clean up.
 *
 * @todo Implement support for dynamic files.
 *
 * The application objects instantiated by this class contains a list of process object containers
 * that belong to the application.  This list of processes is used to manage all processes that
 * need to be started for the application.  However, an application may contain other processes not
 * in this list, processes that were forked by processes within the app.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "watchdogAction.h"
#include "app.h"
#include "limit.h"
#include "proc.h"
#include "user.h"
#include "le_cfg_interface.h"
#include "resourceLimits.h"
#include "smack.h"
#include "supervisor.h"
#include "cgroups.h"
#include "killProc.h"
#include "interfaces.h"
#include "sysPaths.h"
#include "devSmack.h"
#include "dir.h"
#include "fileDescriptor.h"
#include "fileSystem.h"
#include "file.h"
#include "ima.h"
#include "kernelModules.h"

//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that specifies whether the app should be in a sandbox.
 *
 * If this entry in the config tree is missing or empty the application will be sandboxed.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SANDBOXED                              "sandboxed"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains a process's supplementary groups list.
 *
 * Supplementary groups list is only available for non-sandboxed apps.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_GROUPS                                 "groups"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of processes for the application.
 *
 * If this entry in the config tree is missing or empty the application will not be launched.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PROC_LIST                              "procs"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of bindings for the application.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_BINDINGS                               "bindings"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of import directives for
 * devices that an application needs.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_DEVICES                                "devices"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of bundled files and directories.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_BUNDLES                                "bundles"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of required files and directories.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_REQUIRES                               "requires"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of import directives for files
 * that an application needs.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_FILES                                  "files"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of import directives for
 * directories that an application needs.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_DIRS                                   "dirs"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of kernel modules that
 * an application needs.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_KERNELMODULES                           "kernelModules"


//--------------------------------------------------------------------------------------------------
/**
 * The name of the node in the config tree that contains the list of resources and access permission
 * for application needs.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_RESOURCES                               "resources:/"



//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes in a permission string for devices.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_DEVICE_PERM_STR_BYTES                       3


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of processes created with CreateProc from one executable.
 *
 * @note CreateProc code assumes this is a two-digit number.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CREATE_PROC                                   32


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes (including NUL terminator) in a single SMACK permission field.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SMACK_PERM_BYTES                            7


//--------------------------------------------------------------------------------------------------
/**
 * File link object.  Used to hold links that should be created for applications.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char src[LIMIT_MAX_PATH_BYTES];     ///< Absolute path to the source file.
    char dest[LIMIT_MAX_PATH_BYTES];    ///< Dest path relative to the application's runtime area.
                                        ///  If this ends in a separator then it is a directory else
                                        ///  it is a file.
    char perm[MAX_SMACK_PERM_BYTES];    ///< Default SMACK permissions for the file.
}
FileLinkObj_t;


#if ! defined LE_SVCDIR_SERVER_SOCKET_NAME
    #error LE_SVCDIR_SERVER_SOCKET_NAME not defined.
#endif
#if ! defined LE_SVCDIR_CLIENT_SOCKET_NAME
    #error LE_SVCDIR_CLIENT_SOCKET_NAME not defined.
#endif
//--------------------------------------------------------------------------------------------------
/**
 * Files to link into all sandboxed applications by default.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultLinks[] =
{
    {.src = "/dev/log",                             .dest = "/dev/", .perm = "rwa"},
    {.src = "/dev/null",                            .dest = "/dev/", .perm = "rwa"},
    {.src = "/dev/zero",                            .dest = "/dev/", .perm = "r"},
    {.src = "/dev/urandom",                         .dest = "/dev/", .perm = "r"},
    {.src = CURRENT_SYSTEM_PATH"/lib/liblegato.so", .dest = "/lib/", .perm = "r"}
};


//--------------------------------------------------------------------------------------------------
/**
 * Files to link into all sandboxed applications' /tmp directory by default.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultTmpLinks[] =
{
    {.src = LE_SVCDIR_SERVER_SOCKET_NAME, .dest = "/tmp/legato/"},
    {.src = LE_SVCDIR_CLIENT_SOCKET_NAME, .dest = "/tmp/legato/"}
};


#if defined(TARGET_IMPORTS_X86_64)

//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to link into all applications by default for the default system.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultSystemLinks[] =
{
    {.src = "/lib/ld-linux-x86-64.so.2", .dest = "/lib/"},
    {.src = "/lib/libc.so.6", .dest = "/lib/"},
    {.src = "/lib/libpthread.so.0", .dest = "/lib/"},
    {.src = "/lib/librt.so.1", .dest = "/lib/"},
    {.src = "/lib/libdl.so.2", .dest = "/lib/"},
    {.src = "/lib/libgcc_s.so.1", .dest = "/lib/"},
    {.src = "/lib/libm.so.6", .dest = "/lib/"},
    {.src = "/usr/lib/libstdc++.so.6", .dest = "/lib/"}
};

#elif defined(TARGET_IMPORTS_X86)

//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import into all applications by default for the default system.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultSystemLinks[] =
{
    {.src = "/lib/ld-linux.so.2", .dest = "/lib/"},
    {.src = "/lib/libc.so.6", .dest = "/lib/"},
    {.src = "/lib/libpthread.so.0", .dest = "/lib/"},
    {.src = "/lib/librt.so.1", .dest = "/lib/"},
    {.src = "/lib/libdl.so.2", .dest = "/lib/"},
    {.src = "/lib/libgcc_s.so.1", .dest = "/lib/"},
    {.src = "/lib/libm.so.6", .dest = "/lib/"},
    {.src = "/usr/lib/libstdc++.so.6", .dest = "/lib/"}
};

#elif defined(TARGET_IMPORTS_ARMV6) || defined(TARGET_IMPORTS_ARMV7)

//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import into all applications by default for the default system.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultSystemLinks[] =
{
    {.src = "/lib/ld-linux.so.3", .dest = "/lib/"},
    {.src = "/lib/libc.so.6", .dest = "/lib/"},
    {.src = "/lib/libpthread.so.0", .dest = "/lib/"},
    {.src = "/lib/librt.so.1", .dest = "/lib/"},
    {.src = "/lib/libdl.so.2", .dest = "/lib/"},
    {.src = "/lib/libgcc_s.so.1", .dest = "/lib/"},
    {.src = "/lib/libm.so.6", .dest = "/lib/"},
    {.src = "/usr/lib/libstdc++.so.6", .dest = "/lib/"}
};

#elif defined(TARGET_IMPORTS_RASPI)

//--------------------------------------------------------------------------------------------------
/**
 * Files and directories to import into all applications by default for the default system.
 */
//--------------------------------------------------------------------------------------------------
static const FileLinkObj_t DefaultSystemLinks[] =
{
    {.src = "/lib/ld-linux.so.3", .dest = "/lib/"},
    {.src = "/lib/ld-linux-armhf.so.3", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/libc.so.6", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/libpthread.so.0", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/librt.so.1", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/libdl.so.2", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/libgcc_s.so.1", .dest = "/lib/"},
    {.src = "/lib/arm-linux-gnueabihf/libm.so.6", .dest = "/lib/"},
    {.src = "/usr/lib/arm-linux-gnueabihf/libstdc++.so.6", .dest = "/lib/"}
};

#else
#error No "TARGET_IMPORTS_x" defined.
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Timeout value for killing processes in an app.
 */
//--------------------------------------------------------------------------------------------------
static const le_clk_Time_t KillTimeout =
{
    .sec = 1,
    .usec = 0
};


//--------------------------------------------------------------------------------------------------
/**
 * The application object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct app_Ref
{
    char*           name;                               // Name of the application.
    char            cfgPathRoot[LIMIT_MAX_PATH_BYTES];  // Our path in the config tree.
    bool            sandboxed;                          // true if this is a sandboxed app.
    char            installDirPath[LIMIT_MAX_PATH_BYTES]; // Abs path to install files dir.
    char            workingDir[LIMIT_MAX_PATH_BYTES];   // Abs path to the apps working directory.
    uid_t           uid;                // User ID for this application.
    gid_t           gid;                // Group ID for this application.
    gid_t           supplementGids[LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS];  // List of supplementary
                                                                         // group IDs.
    size_t          numSupplementGids;  // Number of supplementary groups for this app.
    app_State_t     state;              // Applications current state.
    le_dls_List_t   procs;              // List of processes in this application.
    le_dls_List_t   auxProcs;           // List of auxiliary processes in this application.
    le_timer_Ref_t  killTimer;          // Timeout timer for killing processes.
    le_sls_List_t   additionalLinks;    // List of additional links that are temporarily added to
                                        // the app.
    le_sls_List_t   reqModuleName;      // List of required kernel module names
}
App_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for application objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppPool;


//--------------------------------------------------------------------------------------------------
/**
 * A file link in the app's working directory.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char fileLink[LIMIT_MAX_PATH_BYTES];    ///< File link in the app's working directory.
    le_sls_Link_t link;                     ///< Link in the list of file links.
}
FileLinkNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for file link nodes.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FileLinkNodePool;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for process stopped handler.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*ProcStopHandler_t)
(
    proc_Ref_t procRef                  ///< [IN] The stopped process
);


//--------------------------------------------------------------------------------------------------
/**
 * The process container object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct app_ProcRef
{
    proc_Ref_t      procRef;        // The process reference.
    ProcStopHandler_t stopHandler;  // Handler function that gets called when this process stops.
    le_dls_Link_t   link;           // The link in the application's list of processes.
    app_Proc_StopHandlerFunc_t externStopHandler;   // External stop handler.
    void*           externContextPtr;   // Context pointer for the external stop handler.
}
ProcContainer_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for process container objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcContainerPool;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for required kernel modules strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ReqModStringPool;


//--------------------------------------------------------------------------------------------------
/**
 * Application kill type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    KILL_SOFT,          ///< Requests the application to clean up and shutdown.
    KILL_HARD           ///< Kills the application ASAP.
}
KillType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Create the supplementary groups for an application.
 *
 * @todo Move creation of the groups to the installer.  Make this function just read the groups
 *       list into the app object.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t CreateSupplementaryGroups
(
    app_Ref_t appRef        // The app to create groups for.
)
{
    // Get an iterator to the supplementary groups list in the config.
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    le_cfg_GoToNode(cfgIter, CFG_NODE_GROUPS);

    if (le_cfg_GoToFirstChild(cfgIter) != LE_OK)
    {
        LE_DEBUG("No supplementary groups for app '%s'.", appRef->name);
        le_cfg_CancelTxn(cfgIter);

        return LE_OK;
    }

    // Read the supplementary group names from the config.
    size_t i;
    for (i = 0; i < LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS; i++)
    {
        // Read the supplementary group name from the config.
        char groupName[LIMIT_MAX_USER_NAME_BYTES];
        if (le_cfg_GetNodeName(cfgIter, "", groupName, sizeof(groupName)) != LE_OK)
        {
            LE_ERROR("Could not read supplementary group for app '%s'.", appRef->name);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }

        // Create the group.
        gid_t gid;
        if (user_CreateGroup(groupName, &gid) == LE_FAULT)
        {
            LE_ERROR("Could not create supplementary group '%s' for app '%s'.",
                     groupName,
                     appRef->name);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }

        // Store the group id in the user's buffer.
        appRef->supplementGids[i] = gid;

        // Go to the next group.
        if (le_cfg_GoToNextSibling(cfgIter) != LE_OK)
        {
            break;
        }
        else if (i >= LIMIT_MAX_NUM_SUPPLEMENTARY_GROUPS - 1)
        {
            LE_ERROR("Too many supplementary groups for app '%s'.", appRef->name);
            le_cfg_CancelTxn(cfgIter);
            return LE_FAULT;
        }
    }

    appRef->numSupplementGids = i + 1;

    le_cfg_CancelTxn(cfgIter);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates the user and groups in the /etc/passwd and /etc/groups files for an application.  This
 * function sets the uid and primary gid for the appRef and also populates the appRef's
 * supplementary groups list for non-sandboxed apps.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateUserAndGroups
(
    app_Ref_t appRef        // The app to create user and groups for.
)
{
    // For sandboxed apps,
    if (appRef->sandboxed)
    {
        // Compute the unique user name for the application.
        char username[LIMIT_MAX_USER_NAME_BYTES];

        if (user_AppNameToUserName(appRef->name, username, sizeof(username)) != LE_OK)
        {
            LE_ERROR("The user name '%s' is too long for app '%s'.", username, appRef->name);
            return LE_FAULT;
        }

        // Get the user ID and primary group ID for this app.
        if (user_GetIDs(username, &(appRef->uid), &(appRef->gid)) != LE_OK)
        {
            LE_ERROR("Could not get uid and gid for user '%s' for app '%s'.",
                     username,
                     appRef->name);
            return LE_FAULT;
        }

        // Create the supplementary groups...
        return CreateSupplementaryGroups(appRef);
    }
    // For unsandboxed apps,
    else
    {
        // The user and group will be "root" (0).
        appRef->uid = 0;
        appRef->gid = 0;

        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the configured permissions for a device.  The permissions will be returned in the provided
 * buffer as a string.  The provided buffer must be greater than or equal to MAX_DEVICE_PERM_STR_BYTES
 * bytes long.
 **/
//--------------------------------------------------------------------------------------------------
static void GetCfgPermissions
(
    le_cfg_IteratorRef_t cfgIter,       ///< [IN] Config iterator pointing to the device file.
    char* bufPtr,                       ///< [OUT] Buffer to hold the permission string.
    size_t bufSize                      ///< [IN] Size of the buffer.
)
{
    LE_FATAL_IF(bufSize < MAX_DEVICE_PERM_STR_BYTES,
                "Buffer size for permission string too small.");

    int i = 0;

    if (le_cfg_GetBool(cfgIter, "isReadable", false))
    {
        bufPtr[i++] = 'r';
    }

    if (le_cfg_GetBool(cfgIter, "isWritable", false))
    {
        bufPtr[i++] = 'w';
    }

    if (le_cfg_GetBool(cfgIter, "isExecutable", false))
    {
        bufPtr[i++] = 'x';
    }

    bufPtr[i] = '\0';
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the source path for the device file at the current node in the config iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetDevSrcPath
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application object.
    le_cfg_IteratorRef_t cfgIter,       ///< [IN] Config iterator for the import.
    char* bufPtr,                       ///< [OUT] Buffer to store the source path.
    size_t bufSize                      ///< [IN] Size of the buffer.
)
{
    char srcPath[LIMIT_MAX_PATH_BYTES] = "";

    if (le_cfg_GetString(cfgIter, "src", srcPath, sizeof(srcPath), "") != LE_OK)
    {
        LE_ERROR("Source file path '%s...' for app '%s' is too long.", srcPath, app_GetName(appRef));
        return LE_FAULT;
    }

    if (strlen(srcPath) == 0)
    {
        LE_ERROR("Empty source file path supplied for app '%s'.", app_GetName(appRef));
        return LE_FAULT;
    }

    if (le_utf8_Copy(bufPtr, srcPath, bufSize, NULL) != LE_OK)
    {
        LE_ERROR("Source file path '%s...' for app '%s' is too long.", srcPath, app_GetName(appRef));
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the device ID of a device file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetDevID
(
    const char* fileNamePtr,        ///< [IN] Absolute path of the file.
    dev_t* idPtr                    ///< [OUT] Device ID.
)
{
    struct stat fileStat;

    if (stat(fileNamePtr, &fileStat) != 0)
    {
        LE_ERROR("Could not get file info for '%s'.  %m.", fileNamePtr);
        return LE_FAULT;
    }

    if (!S_ISCHR(fileStat.st_mode) && !S_ISBLK(fileStat.st_mode))
    {
        LE_ERROR("'%s' is not a device file.  %m.", fileNamePtr);
        return LE_FAULT;
    }

    *idPtr = fileStat.st_rdev;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets DAC and SMACK permissions for a device file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDevicePermissions
(
    const char* appSmackLabelPtr,   ///< [IN] SMACK label of the app.
    const char* devPathPtr,         ///< [IN] Source path.
    const char* permPtr             ///< [IN] Permissions.
)
{
    // Check that the source is a device file.
    dev_t devId;

    if (GetDevID(devPathPtr, &devId) != LE_OK)
    {
        return LE_FAULT;
    }

    // TODO: Disallow device files that are security risks, such as block flash devices.

    // Assign a SMACK label to the device file.
    char devLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    le_result_t result = devSmack_GetLabel(devId, devLabel, sizeof(devLabel));

    LE_FATAL_IF(result == LE_OVERFLOW, "Smack label '%s...' too long.", devLabel);

    if (result != LE_OK)
    {
        return LE_FAULT;
    }

    // Set the SMACK rule to allow the app to access the device.
    smack_SetRule(appSmackLabelPtr, permPtr, devLabel);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets DAC and SMACK permissions for device files in the app's configuration.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCfgDevicePermissions
(
    app_Ref_t appRef                ///< [IN] The application.
)
{
    // Create an iterator for the app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(app_GetConfigPath(appRef));

    // Get the list of device files.
    le_cfg_GoToNode(appCfg, CFG_NODE_REQUIRES);
    le_cfg_GoToNode(appCfg, CFG_NODE_DEVICES);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        // Get the app's SMACK label.
        char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppLabel(app_GetName(appRef), appLabel, sizeof(appLabel));

        do
        {
            // Get source path.
            char srcPath[LIMIT_MAX_PATH_BYTES];
            if (GetDevSrcPath(appRef, appCfg, srcPath, sizeof(srcPath)) != LE_OK)
            {
                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }

            // Get the required permissions for the device.
            char permStr[MAX_DEVICE_PERM_STR_BYTES];
            GetCfgPermissions(appCfg, permStr, sizeof(permStr));

            if (SetDevicePermissions(appLabel, srcPath, permStr) != LE_OK)
            {
                LE_ERROR("Failed to set permissions (%s) for app '%s' on device '%s'.",
                         permStr,
                         appRef->name,
                         srcPath);
                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

        le_cfg_GoToParent(appCfg);
    }

    le_cfg_CancelTxn(appCfg);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets DAC and SMACK permissions for device files that are provided to every app.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDefaultDevicePermissions
(
    app_Ref_t appRef                ///< [IN] The application.
)
{
    char        appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    size_t      i;
    struct stat srcStat;

    // Get the app's SMACK label.
    smack_GetAppLabel(app_GetName(appRef), appLabel, sizeof(appLabel));

    for (i = 0; i < NUM_ARRAY_MEMBERS(DefaultLinks); ++i)
    {
        if (stat(DefaultLinks[i].src, &srcStat) == -1 ||
            (!S_ISCHR(srcStat.st_mode) && !S_ISBLK(srcStat.st_mode)))
        {
            continue;
        }

        if (SetDevicePermissions(appLabel, DefaultLinks[i].src, DefaultLinks[i].perm) != LE_OK)
        {
            LE_ERROR("Failed to set permissions (%s) for app '%s' on device '%s'.",
                     DefaultLinks[i].perm,
                     appRef->name,
                     DefaultLinks[i].src);
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets SMACK rules for an application based on its bindings.
 */
//--------------------------------------------------------------------------------------------------
static void SetSmackRulesForBindings
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application.
    const char* appLabelPtr             ///< [IN] Smack label for the app.
)
{
    // Create a config read transaction to the bindings section for the application.
    le_cfg_IteratorRef_t bindCfg = le_cfg_CreateReadTxn(appRef->cfgPathRoot);
    le_cfg_GoToNode(bindCfg, CFG_NODE_BINDINGS);

    // Search the binding sections for server applications we need to set rules for.
    if (le_cfg_GoToFirstChild(bindCfg) != LE_OK)
    {
        // No bindings.
        le_cfg_CancelTxn(bindCfg);
    }

    do
    {
        char serverName[LIMIT_MAX_APP_NAME_BYTES];

        if ( (le_cfg_GetString(bindCfg, "app", serverName, sizeof(serverName), "") == LE_OK) &&
             (serverName[0] != '\0') )
        {
            // Get the server's SMACK label.
            char serverLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
            smack_GetAppLabel(serverName, serverLabel, sizeof(serverLabel));

            // Set the SMACK label to/from the server.
            // +x is needed as few servers (powerManager & watchdog) need to know the
            // name of their clients and go in to /proc/{pid} of the client.
            smack_SetRule(appLabelPtr, "rwx", serverLabel);
            smack_SetRule(serverLabel, "rwx", appLabelPtr);
        }
    } while (le_cfg_GoToNextSibling(bindCfg) == LE_OK);

    le_cfg_CancelTxn(bindCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets SMACK rules for an application and its folders.
 */
//--------------------------------------------------------------------------------------------------
static void SetDefaultSmackRules
(
    app_Ref_t appRef,                   ///< [IN] App reference.
    const char* appLabelPtr             ///< [IN] Smack label for the app.
)
{
#define NUM_PERMISSONS      7

    const char* permissionStr[NUM_PERMISSONS] = {"x", "w", "wx", "r", "rx", "rw", "rwx"};

    // Set the rules for the app to access its own folders.
    int i;

    for (i = 0; i < NUM_PERMISSONS; i++)
    {
        // Create the mode from the permissions.
        mode_t mode = 0;

        if (strstr(permissionStr[i], "r") != NULL)
        {
            mode |= S_IRUSR;
        }
        if (strstr(permissionStr[i], "w") != NULL)
        {
            mode |= S_IWUSR;
        }
        if (strstr(permissionStr[i], "x") != NULL)
        {
            mode |= S_IXUSR;
        }

        char dirLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppAccessLabel(appRef->name, mode, dirLabel, sizeof(dirLabel));

        smack_SetRule(appLabelPtr, permissionStr[i], dirLabel);

        // framework and admin need to have that priviledge as well
        smack_SetRule("framework", permissionStr[i], dirLabel);
        smack_SetRule("admin", permissionStr[i], dirLabel);
    }

    // Set default permissions between the app and the framework.
    // Give watchdog acces to read the procName from applications
    smack_SetRule("framework", "rwx", appLabelPtr);

    if (ima_IsEnabled())
    {
        smack_SetRule(appLabelPtr, "rx", LE_CONFIG_IMA_SMACK);
    }
    smack_SetRule(appLabelPtr, "rwx", "framework");

    // Set default permissions to allow the app to access the syslog.
    smack_SetRule(appLabelPtr, "w", "syslog");
    smack_SetRule("syslog", "w", appLabelPtr);

    // admin gets access to app labels.
    smack_SetRule("admin", "rwx", appLabelPtr);

    // Give unsandboxed apps access to "_"
    if (!appRef->sandboxed)
    {
        smack_SetRule(appLabelPtr, "rwx", "_");
    }

    static char* frameworkAppList[] =
       {
           "app.atAirVantage",
           "app.atQmiLinker",
           "app.atService",
           "app.audioService",
           "app.avcService",
           "app.cellNetService",
           "app.dataConnectionService",
           "app.devMode",
           "app.fwupdateService",
           "app.gpioService",
           "app.modemService",
           "app.portService",
           "app.positioningService",
           "app.powerMgr",
           "app.qmiAirVantage",
           "app.secStore",
           "app.smsInboxService",
           "app.spiService",
           "app.voiceCallService",
           "app.wifi",
           "app.wifiApTest",
           "app.wifiClientTest",
           "app.wifiService",
           "app.wifiWebAp"
       };

    // Providing legato platform service access to qmuxd
    for (i = 0; i < NUM_ARRAY_MEMBERS(frameworkAppList); i++)
    {
        if (0 == strcmp(frameworkAppList[i], appLabelPtr))
        {
            smack_SetRule(frameworkAppList[i], "rwx", "qmuxd");
            smack_SetRule("qmuxd", "rwx", frameworkAppList[i]);

            // Give app.fwupdateService r access to admin (pipe) in order to perform update
            if (0 == strcmp(frameworkAppList[i], "app.fwupdateService"))
            {
                smack_SetRule(frameworkAppList[i], "r", "admin");
            }

            // Give '_' x access to app.devMode since it performs some overlays on directories
            // required by the '_' label.
            else if (0 == strcmp(frameworkAppList[i], "app.devMode"))
            {
                smack_SetRule("_", "x", frameworkAppList[i]);
            }

            // Workaround for apps that needs access to services exposed by the supervisor.
            // Services such as 'le_framework', 'le_appInfo' or 'le_appCtrl' need the client
            // to have 'w' access to 'admin'.
            else if (0 == strcmp(frameworkAppList[i], "app.powerMgr"))
            {
                smack_SetRule(frameworkAppList[i], "w", "admin");
            }
            else if (0 == strcmp(frameworkAppList[i], "app.avcService"))
            {
                smack_SetRule(frameworkAppList[i], "w", "admin");
            }
            else if (0 == strcmp(frameworkAppList[i], "app.secStore"))
            {
                smack_SetRule(frameworkAppList[i], "w", "admin");
            }
            else if (0 == strcmp(frameworkAppList[i], "app.dataConnectionService"))
            {
                smack_SetRule(frameworkAppList[i], "w", "admin");
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleans up all SMACK permissions for a given app.
 **/
//--------------------------------------------------------------------------------------------------
static void CleanupAppSmackSettings
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Clean up SMACK rules.
    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appRef->name, appLabel, sizeof(appLabel));

    smack_RevokeSubject(appLabel);
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleans up the resource tree if an app is removed. Remove app from resources app list.
 * If there are no more apps under a resource, then remove the resource.
 **/
//--------------------------------------------------------------------------------------------------
static void CleanupResourceCfg
(
    app_Ref_t appRef
)
{
    // Get a config iterator for the resources.
    le_cfg_IteratorRef_t resourceCfg = le_cfg_CreateWriteTxn(CFG_NODE_RESOURCES);
    le_cfg_GoToNode(resourceCfg, CFG_NODE_DIRS);

    if (le_cfg_GoToFirstChild(resourceCfg) == LE_OK)
    {
        do
        {
            char resource[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetString(resourceCfg, "src", resource, sizeof(resource), "");

            le_cfg_GoToNode(resourceCfg, "app");
            int count = 0;

            if (le_cfg_GoToFirstChild(resourceCfg) == LE_OK)
            {
                do
                {
                    char appName[LIMIT_MAX_PATH_BYTES];
                    le_cfg_GetString(resourceCfg, "name", appName, sizeof(appName), "");

                    if (strcmp(appRef->name, appName) == 0)
                    {
                        LE_INFO("Deleting appName %s from resource %s", appName, resource);
                        le_cfg_DeleteNode(resourceCfg, "");
                        count--;
                    }

                    count++;
                }
                while (le_cfg_GoToNextSibling(resourceCfg) == LE_OK);
            }

            // move back up to the list of directory resources
            le_cfg_GoToNode(resourceCfg, "../..");

            // Delete the resource since we deleted the only app using this resource
            if (count == 0)
            {
                LE_INFO("Deleting resource: %s", resource);
                le_cfg_DeleteNode(resourceCfg, "");
            }
        }
        while (le_cfg_GoToNextSibling(resourceCfg) == LE_OK);

        le_cfg_GoToParent(resourceCfg);
    }

    // Manage files section
    le_cfg_GoToNode(resourceCfg, "../" CFG_NODE_FILES);

    if (le_cfg_GoToFirstChild(resourceCfg) == LE_OK)
    {
        do
        {
            char resource[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetString(resourceCfg, "src", resource, sizeof(resource), "");

            le_cfg_GoToNode(resourceCfg, "app");
            int count = 0;

            if (le_cfg_GoToFirstChild(resourceCfg) == LE_OK)
            {
                do
                {
                    char appName[LIMIT_MAX_PATH_BYTES];
                    le_cfg_GetString(resourceCfg, "name", appName, sizeof(appName), "");

                    if (strcmp(appRef->name, appName) == 0)
                    {
                        LE_INFO("Deleting appName %s from resource %s", appName, resource);
                        le_cfg_DeleteNode(resourceCfg, "");
                        count--;
                    }

                    count++;
                }
                while (le_cfg_GoToNextSibling(resourceCfg) == LE_OK);
            }

            // move back up to the list of file resources
            le_cfg_GoToNode(resourceCfg, "../..");

            // Delete the resource since we deleted the only app using this resource
            if (count == 0)
            {
                LE_INFO("Deleting resource: %s", resource);
                le_cfg_DeleteNode(resourceCfg, "");
            }
        }
        while (le_cfg_GoToNextSibling(resourceCfg) == LE_OK);

        le_cfg_GoToParent(resourceCfg);
    }
    le_cfg_CommitTxn(resourceCfg);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add application to the list of apps that require this specific resource. When the list of apps
 * for a resource is empty, we will remove the resource from the resource tree.
 */
//--------------------------------------------------------------------------------------------------
void AddAppToSharedResource
(
    le_cfg_IteratorRef_t iterRef,
    const char * appName
)
{
    char indexStr[LIMIT_MD5_STR_BYTES];

    le_cfg_GoToNode(iterRef, "app");

    if (le_cfg_GoToFirstChild(iterRef) != LE_OK)
    {
        snprintf(indexStr, sizeof(indexStr), "%d", -1);
    }
    else
    {
        do
        {
            le_cfg_GetNodeName(iterRef, "", indexStr, sizeof(indexStr));

            // if app already exist, do not proceed
            char currentAppName[LIMIT_MAX_APP_NAME_BYTES];
            le_cfg_GetString(iterRef, "name", currentAppName, sizeof(currentAppName), "");

            if (strcmp(currentAppName, appName) == 0)
            {
                LE_DEBUG("App already exists.");
                return;
            }
        }
        while (le_cfg_GoToNextSibling(iterRef) == LE_OK);

        le_cfg_GoToParent(iterRef);
    }

    LE_DEBUG("Adding app to resource: %s", appName);
    int index = atoi(indexStr);
    index++;
    snprintf(indexStr, sizeof(indexStr), "%d", index);
    le_cfg_GoToNode(iterRef, indexStr);
    le_cfg_SetString(iterRef, "name", appName);
}



//--------------------------------------------------------------------------------------------------
/**
 * Set SMACK rule that will allow this resource to access the resource with specificed access
 * permission from the required section.
 */
//--------------------------------------------------------------------------------------------------
static void SetSmackRuleForResource
(
    app_Ref_t appRef,
    const char * srcPath,
    const char * label,
    const char * permission
)
{
    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appRef->name, appLabel, sizeof(appLabel));
    smack_SetRule(appLabel, permission, label);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set DAC permission of the resource once with rwx on others (rely on MAC for access control)
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDACForResource
(
    const char * srcPath
)
{
    int fd = -1;
    struct stat srcStat;
    le_result_t result = LE_OK;

    LE_ASSERT(NULL != srcPath);

    if ((fd = open(srcPath, O_RDONLY)) < 0)
    {
        LE_ERROR("Unable to open %s", srcPath);
        result = LE_FAULT;
        goto cleanExit;
    }

    if (fstat(fd, &srcStat) != 0)
    {
        LE_ERROR("Unable to obtain status of %s", srcPath);
        result = LE_FAULT;
        goto cleanExit;
    }

    if (fchmod(fd, srcStat.st_mode | S_IROTH | S_IWOTH | S_IXOTH) != 0)
    {
        LE_ERROR("Unable to change permission bit on %s", srcPath);
        result = LE_FAULT;
    }

    cleanExit:
    if(fd >= 0)
    {
        if(close(fd) != 0)
        {
            LE_ERROR("Unable to close %s", srcPath);
            result = LE_FAULT;
        }
    }

    return result;

}


//--------------------------------------------------------------------------------------------------
/**
 * Search through our resource tree to find the SMACK label to use for sharing the resource.
 * If none is found, then we will:
 * - Generate a new entry that contains the new resource and the label used for it.
 * - Set DAC permission of the resource once with rwx on others (rely on MAC for access control)
 * - Set our resource with the new label.
 * - Set SMACK rule that will allow this resource to access the resource with specificed access
 *   permission from the requires section.
 * - Add application to the app list of the resource (to manage when to remove the resource from
     the resource tree).
 * If the resource exist, we will just set the SMACK rule and add add app to the app list of the
 * resource.
 */
//--------------------------------------------------------------------------------------------------
void SetPermissionForResource
(
    app_Ref_t appRef,
    const char * type,
    const char * srcPath,
    const char * permission
)
{
    char indexStr[LIMIT_MD5_STR_BYTES];
    char label[LIMIT_MAX_PATH_BYTES];

    // Go to the resources section.
    le_cfg_IteratorRef_t resourceCfg = le_cfg_CreateWriteTxn(CFG_NODE_RESOURCES);
    le_cfg_GoToNode(resourceCfg, type);

    if (le_cfg_GoToFirstChild(resourceCfg) != LE_OK)
    {
        snprintf(indexStr, sizeof(indexStr), "%d", -1);
    }
    else
    {
        do
        {
            le_cfg_GetNodeName(resourceCfg, "", indexStr, sizeof(indexStr));

            char resource[LIMIT_MAX_PATH_BYTES];
            le_cfg_GetString(resourceCfg, "src", resource, sizeof(resource), "");

            if (strcmp(resource, srcPath) == 0)
            {
                LE_DEBUG("Resource already exists, loading rules.");
                le_cfg_GetString(resourceCfg, "label", label, sizeof(label), "");
                SetSmackRuleForResource(appRef, srcPath, label, permission);
                AddAppToSharedResource(resourceCfg, appRef->name);
                le_cfg_CommitTxn(resourceCfg);
                return;
            }
        }
        while (le_cfg_GoToNextSibling(resourceCfg) == LE_OK);

        le_cfg_GoToParent(resourceCfg);
    }

    int index = atoi(indexStr);
    index++;
    snprintf(indexStr, sizeof(indexStr), "%d", index);
    le_cfg_GoToNode(resourceCfg, indexStr);
    LE_DEBUG("Adding new resource with index: %s", indexStr);

    // set the resource
    le_cfg_SetString(resourceCfg, "src", srcPath);

    // label will be the requires [type][index] e.g. file0
    snprintf(label, sizeof(label), "%s%s", type, indexStr);
    le_cfg_SetString(resourceCfg, "label", label);

    SetDACForResource(srcPath);
    smack_SetLabel(srcPath, label);
    SetSmackRuleForResource(appRef, srcPath, label, permission);
    AddAppToSharedResource(resourceCfg, appRef->name);

    le_cfg_CommitTxn(resourceCfg);
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets DAC and SMACK permissions for resources (files and dirs) defined in the access permission
 * section of requires.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetPermissionForRequired
(
    app_Ref_t appRef                      ///< [IN] Reference to the application.
)
{
    // Get a config iterator for this app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    // Go to the required directory section.
    le_cfg_GoToNode(appCfg, CFG_NODE_REQUIRES "/" CFG_NODE_DIRS);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        do
        {
            char permStr[MAX_DEVICE_PERM_STR_BYTES];
            GetCfgPermissions(appCfg, permStr, sizeof(permStr));

            // Only add dirs that require access permission
            if (strcmp(permStr, "") != 0)
            {
                char srcPath[LIMIT_MAX_PATH_BYTES];

                if (le_cfg_GetString(appCfg, "src", srcPath, sizeof(srcPath), "") != LE_OK)
                {
                    LE_ERROR("Source path '%s...' for app '%s' is too long.", srcPath, appRef->name);
                    return LE_FAULT;
                }

                SetPermissionForResource(appRef, CFG_NODE_DIRS, srcPath, permStr);
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

        le_cfg_GoToParent(appCfg);
    }

    // Go to the required files section.
    le_cfg_GoToParent(appCfg);
    le_cfg_GoToNode(appCfg, CFG_NODE_FILES);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        do
        {
            char permStr[MAX_DEVICE_PERM_STR_BYTES];
            GetCfgPermissions(appCfg, permStr, sizeof(permStr));

            // Only add files that require access permission
            if (strcmp(permStr, "") != 0)
            {
                char srcPath[LIMIT_MAX_PATH_BYTES];

                if (le_cfg_GetString(appCfg, "src", srcPath, sizeof(srcPath), "") != LE_OK)
                {
                    LE_ERROR("Source path '%s...' for app '%s' is too long.", srcPath, appRef->name);
                    return LE_FAULT;
                }

                SetPermissionForResource(appRef, CFG_NODE_FILES, srcPath, permStr);
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

        le_cfg_GoToParent(appCfg);
    }

    le_cfg_CancelTxn(appCfg);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets SMACK rules for an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetSmackRules
(
    app_Ref_t appRef                    ///< [IN] Reference to the application.
)
{
    // Clear resource
    CleanupResourceCfg(appRef);

    // Get the app label.
    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(appRef->name, appLabel, sizeof(appLabel));

    SetDefaultSmackRules(appRef, appLabel);

    SetSmackRulesForBindings(appRef, appLabel);

    le_result_t result = SetDefaultDevicePermissions(appRef);
    if (result != LE_OK)
    {
        return result;
    }

    result = SetPermissionForRequired(appRef);
    if (result != LE_OK)
    {
        return result;
    }

    return SetCfgDevicePermissions(appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tells all the child processes in the list that we are going to kill them.
 */
//--------------------------------------------------------------------------------------------------
static void StoppingProcsInList
(
    le_dls_List_t list              ///< [IN] List of process containers.
)
{
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&list);

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        if (proc_GetState(procContainerPtr->procRef) != PROC_STATE_STOPPED)
        {
            procContainerPtr->stopHandler = NULL;
            proc_Stopping(procContainerPtr->procRef);
        }

        procLinkPtr = le_dls_PeekNext(&list, procLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Kills all the processes in the specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no running processes in the app.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t KillAppProcs
(
    app_Ref_t appRef,       ///< [IN] Reference to the application whose processes should be killed.
    KillType_t  killType    ///< [IN] The type of kill to perform.
)
{
    // Freeze app procs.
    if (cgrp_frz_Freeze(appRef->name) == LE_OK)
    {
        // Wait till procs are frozen.
        while (1)
        {
            cgrp_FreezeState_t freezeState = cgrp_frz_GetState(appRef->name);

            if (freezeState == CGRP_FROZEN)
            {
                break;
            }
            else if ((le_result_t)freezeState == LE_FAULT)
            {
                LE_ERROR("Could not get freeze state of application '%s'.", appRef->name);
                break;
            }
        }

        LE_DEBUG("App '%s' frozen.", appRef->name);
    }
    else
    {
        LE_ERROR("Could not freeze processes for application '%s'.", appRef->name);
    }

    // Tell the child process objects we are going to kill them.
    StoppingProcsInList(appRef->procs);
    StoppingProcsInList(appRef->auxProcs);

    // Kill all procs in the app including child processes and forked processes.
    int killSig = (killType == KILL_SOFT) ? SIGTERM: SIGKILL;

    ssize_t numProcs = cgrp_SendSig(CGRP_SUBSYS_FREEZE, appRef->name, killSig);

    if (numProcs == LE_FAULT)
    {
        LE_ERROR("Could not kill processes for application '%s'.", appRef->name);
        return LE_NOT_FOUND;
    }

    // Thaw app procs to allow them to run and process the signal we sent them.
    if (cgrp_frz_Thaw(appRef->name) != LE_OK)
    {
        LE_ERROR("Could not thaw processes for application '%s'.", appRef->name);
    }

    if (numProcs == 0)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs a hard kill of all the processes in the specified application.  This function should be
 * called when the soft kill timeout expires.
 */
//--------------------------------------------------------------------------------------------------
static void HardKillApp
(
    le_timer_Ref_t timerRef
)
{
    app_Ref_t appRef = (app_Ref_t)le_timer_GetContextPtr(timerRef);

    LE_WARN("Hard killing app '%s'", appRef->name);

    KillAppProcs(appRef, KILL_HARD);
}


//--------------------------------------------------------------------------------------------------
/**
 * Finds a process container with this pid in the specified list.
 *
 * @return
 *      The pointer to a process container if successful.
 *      NULL if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static ProcContainer_t* FindProcContainerInList
(
    le_dls_List_t list,             ///< [IN] List of process containers.
    pid_t pid                       ///< [IN] The pid to search for.
)
{
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&list);

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        if (proc_GetPID(procContainerPtr->procRef) == pid)
        {
            return procContainerPtr;
        }

        procLinkPtr = le_dls_PeekNext(&list, procLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Finds a process container for the app by pid.
 *
 * @return
 *      The pointer to a process container if successful.
 *      NULL if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static ProcContainer_t* FindProcContainer
(
    app_Ref_t appRef,               ///< [IN] The application to search in.
    pid_t pid                       ///< [IN] The pid to search for.
)
{
    // Find the process in the app's list.
    ProcContainer_t* procContainerPtr = FindProcContainerInList(appRef->procs, pid);

    if (procContainerPtr != NULL)
    {
        return procContainerPtr;
    }

    return FindProcContainerInList(appRef->auxProcs, pid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the there is a running process in the specified list.
 *
 * @note This only applies to child processes.  Forked processes in the application are not
 *       monitored.
 *
 * @return
 *      true if there is at least one running process for in the list.
 *      false if there are no running processes in the list.
 */
//--------------------------------------------------------------------------------------------------
static bool HasRunningProcInList
(
    le_dls_List_t list              ///< [IN] List of process containers.
)
{
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&list);

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        if (proc_GetState(procContainerPtr->procRef) == PROC_STATE_RUNNING)
        {
            return true;
        }

        procLinkPtr = le_dls_PeekNext(&list, procLinkPtr);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the application has any processes running.
 *
 * @return
 *      true if there is at least one running process for the application.
 *      false if there are no running processes for the application.
 */
//--------------------------------------------------------------------------------------------------
static bool HasRunningProc
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Checks the appRef->procs list for processes that configured in the configuration DB.
    // Checks the appRef->auxProcs list for processes started by the le_appProc API.
    // Checks the cgroup for all running processes including non-direct descendant processes.
    return ( HasRunningProcInList(appRef->procs) ||
             HasRunningProcInList(appRef->auxProcs) ||
             !cgrp_IsEmpty(CGRP_SUBSYS_FREEZE, appRef->name) );
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops the specified process.
 */
//--------------------------------------------------------------------------------------------------
static void StopProc
(
    proc_Ref_t procRef
)
{
    proc_Stopping(procRef);

    pid_t pid = proc_GetPID(procRef);

    kill_Hard(pid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create the sandbox app's /tmp folder and mount a tmpfs at that location.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateTmpFs
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    // Create /tmp folder in the sandbox.
    char tmpPath[LIMIT_MAX_PATH_BYTES] = "";

    if (le_path_Concat("/", tmpPath, sizeof(tmpPath), appRef->workingDir, "tmp", NULL) != LE_OK)
    {
        LE_ERROR("Path '%s...' is too long. Can't create tmpfs file system for app '%s'.",
                 tmpPath,
                 appRef->name);
        return LE_FAULT;
    }

    if (dir_MakeSmack(tmpPath, S_IRWXO, appDirLabelPtr) == LE_FAULT)
    {
        LE_ERROR("Failed to create directory '%s' for app '%s'.", tmpPath, appRef->name);
        return LE_FAULT;
    }

    // Make the mount options.
    char opt[LIMIT_MAX_APP_NAME_BYTES*2 + 100];
    if (snprintf(opt, sizeof(opt), "size=%d,mode=%.4o,uid=%d,gid=%d"
#if LE_CONFIG_ENABLE_SMACK
                                   ",smackfsdef=%s,smackfsroot=%s"
#endif
                 , LE_CONFIG_SUPERV_APP_TMPFS_SIZE, S_IRWXO, 0, 0
#if LE_CONFIG_ENABLE_SMACK
                 , appDirLabelPtr, appDirLabelPtr
#endif
                 ) >= sizeof(opt))
    {
        LE_ERROR("Mount options string is too long (%s). Can't mount tmpfs for app '%s'.'",
                 opt,
                 appRef->name);

        return LE_FAULT;
    }

    // Unmount any previously mounted file system.
    fs_TryLazyUmount(tmpPath);

    // Mount the tmpfs for the sandbox.
    if (mount("tmpfs", tmpPath, "tmpfs", MS_NOSUID, opt) == -1)
    {
        LE_ERROR("Could not mount tmpfs for sandbox '%s'.  %m.", app_GetName(appRef));

        return LE_FAULT;
    }

    LE_INFO("Mounted tmpfs at %s.", tmpPath);

    return smack_SetLabel(tmpPath, appDirLabelPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the absolute destination path.  If the destination path ends with a '/' then the last node
 * of the source is appended to the destination.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW the provided buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAbsDestPath
(
    const char* destPtr,        ///< [IN] Relative destination path.
    const char* srcPtr,         ///< [IN] Source path.
    const char* appRunDir,      ///< [IN] Path to the app's runtime area.
    char* bufPtr,               ///< [OUT] Buffer to store the final destination path.
    size_t bufSize              ///< [IN] Size of the buffer.
)
{
    bufPtr[0] = '\0';

    if (destPtr[strlen(destPtr)-1] == '/')
    {
        return le_path_Concat("/", bufPtr, bufSize, appRunDir, destPtr,
                              le_path_GetBasenamePtr(srcPtr, "/"), NULL);
    }
    else
    {
        return le_path_Concat("/", bufPtr, bufSize, appRunDir, destPtr, NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates all intermediate directories along the path.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateIntermediateDirs
(
    const char* pathPtr,                ///< [IN] Path.
    const char* smackLabelPtr           ///< [IN] SMACK label to use for the created dirs.
)
{
    char dirPath[LIMIT_MAX_PATH_BYTES] = "";

    if (le_path_GetDir(pathPtr, "/", dirPath, sizeof(dirPath)) != LE_OK)
    {
        LE_ERROR("Path '%s' is too long.", dirPath);
        return LE_FAULT;
    }

    if (dir_MakePathSmack(dirPath,
                          S_IRUSR | S_IXUSR | S_IROTH | S_IXOTH,
                          smackLabelPtr) == LE_FAULT)
    {
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the link already exists.
 *
 * If there is a link to a different file then attempt to delete it.
 *
 * @return
 *      true if link already exists.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool DoesLinkExist
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    struct stat srcStat,                ///< [IN] Status of the source.
    const char* destPath                ///< [IN] Destination path.
)
{
    // See if the destination already exists.
    struct stat destStat;

    if (stat(destPath, &destStat) == -1)
    {
        if  (errno != ENOENT)
        {
            LE_WARN("Could not stat file at '%s'. %m", destPath);
        }
    }
    else
    {
        // Destination file already exists.  See if it has changed.
        if (S_ISCHR(srcStat.st_mode) || S_ISBLK(srcStat.st_mode))
        {
            // Special devices need to have same device number but different inode numbers
            if ((srcStat.st_rdev == destStat.st_rdev) &&
                (srcStat.st_ino != destStat.st_ino))
            {
                return true;
            }
        }
        else
        {
            if (srcStat.st_ino == destStat.st_ino)
            {
                // Link already exists.
                return true;
            }
        }

        // Attempt to delete the original link.
        if (!appRef->sandboxed)
        {
            if (unlink(destPath) == -1)
            {
                LE_WARN("Could not delete %s.  %m,", destPath);
            }
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a directory link from the source to the destination.  The source is always assumed to be
 * an absolute path while the destination is relative to the application runtime area.  If the
 * destination includes directories that do not exist then those directories are created.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateDirLink
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr,         ///< [IN] SMACK label to use for created directories.
    const char* srcPtr,                 ///< [IN] Source path.
    const char* destPtr                 ///< [IN] Destination path.
)
{
    // Check the source.
    struct stat srcStat;

    if (stat(srcPtr, &srcStat) == -1)
    {
        LE_ERROR("Could not stat file at '%s'. %m", srcPtr);
        goto failure;
    }

    if (!S_ISDIR(srcStat.st_mode))
    {
        LE_ERROR("'%s' is not a directory.", srcPtr);
        goto failure;
    }

    // Get the absolute destination path.
    char destPath[LIMIT_MAX_PATH_BYTES] = "";

    if (GetAbsDestPath(destPtr, srcPtr, appRef->workingDir, destPath, sizeof(destPath)) != LE_OK)
    {
        LE_ERROR("Link destination path '%s' is too long.", destPath);
        goto failure;
    }

    // Create the necessary intermediate directories along the destination path.
    if (CreateIntermediateDirs(destPath, appDirLabelPtr) != LE_OK)
    {
        goto failure;
    }

    // See if the destination already exists.
    if (DoesLinkExist(appRef, srcStat, destPath))
    {
        LE_INFO("Skipping directory link '%s' to '%s': Already exists", srcPtr, destPath);
        return LE_OK;
    }

    // Create the link.
    if (appRef->sandboxed)
    {
        // Make the destination directories.
        if (dir_MakeSmack(destPath,
                      S_IRUSR | S_IXUSR | S_IROTH | S_IXOTH,
                      appDirLabelPtr) == LE_FAULT)
        {
            goto failure;
        }

        // Bind mount into the sandbox.
        if (mount(srcPtr, destPath, NULL, MS_BIND, NULL) != 0)
        {
            LE_ERROR("Couldn't bind mount from '%s' to '%s'. %m", srcPtr, destPath);
            goto failure;
        }
    }
    else
    {
        // Create a symlink at the specified path.
        if (symlink(srcPtr, destPath) != 0)
        {
            LE_ERROR("Could not create symlink from '%s' to '%s'. %m", srcPtr, destPath);
            goto failure;
        }
    }

    LE_INFO("Created directory link '%s' to '%s'.", srcPtr, destPath);

    return LE_OK;

failure:
    LE_ERROR("Failed to create link at '%s' in app '%s'.", destPtr, appRef->name);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a file link from the source to the destination.  The source is always assumed to be
 * an absolute path while the destination is relative to the application runtime area.  If the
 * destination includes directories that do not exist then those directories are created.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateFileLink
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr,         ///< [IN] SMACK label to use for created directories.
    const char* srcPtr,                 ///< [IN] Source path.
    const char* destPtr                 ///< [IN] Destination path.
)
{
    // Check the source.
    struct stat srcStat;

    if (stat(srcPtr, &srcStat) == -1)
    {
        LE_ERROR("Could not stat file at '%s'. %m", srcPtr);
        goto failure;
    }

    if (S_ISDIR(srcStat.st_mode))
    {
        LE_ERROR("'%s' is a directory.", srcPtr);
        goto failure;
    }

    // Get the absolute destination path.
    char destPath[LIMIT_MAX_PATH_BYTES] = "";

    if (GetAbsDestPath(destPtr, srcPtr, appRef->workingDir, destPath, sizeof(destPath)) != LE_OK)
    {
        LE_ERROR("Link destination path '%s' is too long.", destPath);
        goto failure;
    }

    // Create the necessary intermediate directories along the destination path.
    if (CreateIntermediateDirs(destPath, appDirLabelPtr) != LE_OK)
    {
        goto failure;
    }

    // Treat files located in /dev/shm differently.  These are shared memory expected to be
    // shared between other apps but also other userland processes.  So set the SMACK label
    // to '*' to grant access to all.
    if (le_path_IsEquivalent("/dev/shm", srcPtr, "/") ||
        le_path_IsSubpath("/dev/shm", srcPtr, "/"))
    {
        if (smack_SetLabel(srcPtr, "*") != LE_OK)
        {
            LE_ERROR("Couldn't set SMACK label to '*' for %s", srcPtr);
            goto failure;
        }
        return LE_OK;
    }

    // See if the destination already exists.
    if (DoesLinkExist(appRef, srcStat, destPath))
    {
        LE_INFO("Skipping file link '%s' to '%s': Already exists", srcPtr, destPath);
        return LE_OK;
    }

    if (!appRef->sandboxed)
    {
        // Create a symlink at the specified path.
        if (symlink(srcPtr, destPath) != 0)
        {
            LE_ERROR("Could not create symlink from '%s' to '%s'. %m", srcPtr, destPath);
            goto failure;
        }
    }
    // For devices, create a new device node for the app
    else if (S_ISCHR(srcStat.st_mode) || S_ISBLK(srcStat.st_mode))
    {
        char devLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        le_result_t result = devSmack_GetLabel(srcStat.st_rdev, devLabel, sizeof(devLabel));

        LE_FATAL_IF(result == LE_OVERFLOW, "Smack label '%s...' too long.", devLabel);

        if (result != LE_OK)
        {
            LE_ERROR("Failed to get smack label for device '%s'", srcPtr);
            goto failure;
        }

        if (mknod(destPath,
                  (srcStat.st_mode & (S_IFCHR | S_IFBLK)) | S_IRUSR | S_IWUSR,
                  srcStat.st_rdev) == -1)
        {
            LE_ERROR("Could not create device '%s'.  %m", destPath);
            goto failure;
        }

        if (smack_SetLabel(destPath, devLabel) != LE_OK)
        {
            LE_ERROR("Failed to set smack label for device '%s'", destPath);
            goto failure;
        }

        // Gift the device to the app.
        if (chown(destPath, appRef->uid, appRef->gid) == -1)
        {
            LE_ERROR("Could not assign device '%s' to app.  %m", destPath);
            goto failure;
        }
    }
    else
    {
        // Create an empty file at the specified path.
        int fd;
        while ( ((fd = open(destPath, O_RDONLY | O_CREAT, S_IRUSR)) == -1) && (errno == EINTR) ) {}

        if (fd == -1)
        {
            LE_ERROR("Could not create file '%s'.  %m", destPath);
            goto failure;
        }

        fd_Close(fd);

        // Bind mount file into the sandbox.
        if (mount(srcPtr, destPath, NULL, MS_BIND, NULL) != 0)
        {
            LE_ERROR("Couldn't bind mount from '%s' to '%s'. %m", srcPtr, destPath);
            goto failure;
        }
    }

    LE_INFO("Created file link '%s' to '%s'.", srcPtr, destPath);

    return LE_OK;

failure:
    LE_ERROR("Failed to create link at '%s' in app '%s'.", destPtr, appRef->name);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively create links from all files under the source directory to corresponding files under
 * the destination directory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RecursivelyCreateLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr,         ///< [IN] SMACK label to use for created directories.
    const char* srcDirPtr,              ///< [IN] Source directory.
    const char* destDirPtr              ///< [IN] Destination directory.
)
{
    char baseDestPath[LIMIT_MAX_PATH_BYTES] = "";

    if (destDirPtr[strlen(destDirPtr)-1] == '/')
    {
        // Use the source directory name in the destination.
        if (le_path_Concat("/", baseDestPath, sizeof(baseDestPath), destDirPtr,
                            le_path_GetBasenamePtr(srcDirPtr, "/"), NULL) != LE_OK)
        {
            LE_ERROR("Destination path '%s...' for app %s is too long.", baseDestPath, appRef->name);
            return LE_FAULT;
        }
    }
    else
    {
        if (le_utf8_Copy(baseDestPath, destDirPtr, sizeof(baseDestPath), NULL) != LE_OK)
        {
            LE_ERROR("Destination path '%s...' for app %s is too long.", baseDestPath, appRef->name);
            return LE_FAULT;
        }
    }

    // Open the directory tree to search.
    char* pathArrayPtr[] = {(char*)srcDirPtr, NULL};

    FTS* ftsPtr = NULL;
    errno = 0;

    do
    {
        if (appRef->sandboxed)
        {
            ftsPtr = fts_open(pathArrayPtr, FTS_LOGICAL | FTS_NOSTAT, NULL);
        }
        else
        {
            ftsPtr = fts_open(pathArrayPtr, FTS_PHYSICAL | FTS_NOSTAT, NULL);
        }
    }
    while ( (ftsPtr == NULL) && (errno == EINTR) );

    if (ftsPtr == NULL)
    {
        LE_ERROR("Couldn't open directory '%s' (%m) while creating link in app '%s'",
                 srcDirPtr,
                 appRef->name);
        return LE_FAULT;
    }

    // Step through the directory tree.
    FTSENT* srcEntPtr;

    int srcDirLen = strlen(srcDirPtr);

    while ((srcEntPtr = fts_read(ftsPtr)) != NULL)
    {
        switch (srcEntPtr->fts_info)
        {
            case FTS_SL:
            case FTS_F:
            case FTS_NSOK:
            {
                // These are files and links.
                // Create the dest path.
                char destPath[LIMIT_MAX_PATH_BYTES] = "";

                if (le_path_Concat("/", destPath, sizeof(destPath), baseDestPath,
                                   srcEntPtr->fts_path + srcDirLen, NULL) != LE_OK)
                {
                    fts_close(ftsPtr);
                    LE_ERROR("Full destination path '%s...' for app %s is too long.",
                             destPath,
                             appRef->name);
                    return LE_FAULT;
                }

                // Create the link.
                if (CreateFileLink(appRef, appDirLabelPtr, srcEntPtr->fts_path, destPath) != LE_OK)
                {
                    fts_close(ftsPtr);
                    return LE_FAULT;
                }
            }
        }
    }

    int lastErrno = errno;

    // Close the directory tree.
    int r;
    if (NULL != ftsPtr)
    {
        do
        {
           r = fts_close(ftsPtr);

        }
        while ( (-1 == r) && (EINTR == errno) );
    }

    if (0 != lastErrno)
    {
        LE_ERROR("Could not read directory '%s' (%m) while creating link for app '%s'",
                 srcDirPtr,
                 appRef->name);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the default temporary files that all app's will likely need.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateDefaultTmpLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    int i = 0;

    for (i = 0; i < NUM_ARRAY_MEMBERS(DefaultTmpLinks); i++)
    {
        // Default links must work otherwise there is something very wrong.
        if (CreateFileLink(appRef, appDirLabelPtr,
                           DefaultTmpLinks[i].src, DefaultTmpLinks[i].dest) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the default libs and files that all app's will likely need.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateDefaultLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    int i = 0;

    for (i = 0; i < NUM_ARRAY_MEMBERS(DefaultLinks); i++)
    {
        // Default links must work otherwise there is something very wrong.
        if (CreateFileLink(appRef, appDirLabelPtr,
                           DefaultLinks[i].src, DefaultLinks[i].dest) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    for (i = 0; i < NUM_ARRAY_MEMBERS(DefaultSystemLinks); i++)
    {
        // Default links must work otherwise there is something very wrong.
        if (CreateFileLink(appRef, appDirLabelPtr, DefaultSystemLinks[i].src,
                           DefaultSystemLinks[i].dest) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the app's lib and bin files.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateLibBinLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    // Create links to the apps lib directory.
    char srcLib[LIMIT_MAX_PATH_BYTES] = "";

    if (le_path_Concat("/", srcLib, sizeof(srcLib), appRef->installDirPath, "read-only/lib", NULL)
          != LE_OK)
    {
        LE_ERROR("App's install dir path too long!");
        return LE_FAULT;
    }

    if (RecursivelyCreateLinks(appRef, appDirLabelPtr, srcLib, "/lib") != LE_OK)
    {
        return LE_FAULT;
    }

    // Create links to the apps bin directory.
    char srcBin[LIMIT_MAX_PATH_BYTES] = "";

    if (le_path_Concat("/", srcBin, sizeof(srcBin), appRef->installDirPath, "read-only/bin", NULL)
          != LE_OK)
    {
        LE_ERROR("App's install dir path too long!");
        return LE_FAULT;
    }

    if (RecursivelyCreateLinks(appRef, appDirLabelPtr, srcBin, "/bin") != LE_OK)
    {
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the source path for read only bundled files at the current node in the config iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetBundledReadOnlySrcPath
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application object.
    le_cfg_IteratorRef_t cfgIter,       ///< [IN] Config iterator.
    char* bufPtr,                       ///< [OUT] Buffer to store the source path.
    size_t bufSize                      ///< [IN] Size of the buffer.
)
{
    char srcPath[LIMIT_MAX_PATH_BYTES] = "";

    if (le_cfg_GetString(cfgIter, "src", srcPath, sizeof(srcPath), "") != LE_OK)
    {
        LE_ERROR("Source file path '%s...' for app '%s' is too long.", srcPath, app_GetName(appRef));
        return LE_FAULT;
    }

    if (strlen(srcPath) == 0)
    {
        LE_ERROR("Empty source file path supplied for app %s.", app_GetName(appRef));
        return LE_FAULT;
    }

    if (srcPath[0] == '/')
    {
        // The source path is an absolute path so just copy it to the user's buffer.
        if (le_utf8_Copy(bufPtr, srcPath, bufSize, NULL) != LE_OK)
        {
            LE_ERROR("Source file path '%s...' for app '%s' is too long.", srcPath, app_GetName(appRef));
            return LE_FAULT;
        }
    }
    else
    {
        // The source file path is relative to the app install directory.
        bufPtr[0] = '\0';
        if (LE_OK != le_path_Concat("/",
                                    bufPtr,
                                    bufSize,
                                    appRef->installDirPath,
                                    "read-only",
                                    srcPath,
                                    NULL) )
        {
            LE_ERROR("Import source path '%s' for app '%s' is too long.", bufPtr, app_GetName(appRef));
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the destination path for the app at the current node in the config iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetDestPath
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application object.
    le_cfg_IteratorRef_t cfgIter,       ///< [IN] Config iterator.
    char* bufPtr,                       ///< [OUT] Buffer to store the path.
    size_t bufSize                      ///< [IN] Size of the buffer.
)
{
    if (le_cfg_GetString(cfgIter, "dest", bufPtr, bufSize, "") != LE_OK)
    {
        LE_ERROR("Destination path '%s...' for app '%s' is too long.", bufPtr, appRef->name);
        return LE_FAULT;
    }

    if (bufPtr[0] == '\0')
    {
        LE_ERROR("Empty dest path supplied for app %s.", appRef->name);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the source path for the app at the current node in the config iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetSrcPath
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application object.
    le_cfg_IteratorRef_t cfgIter,       ///< [IN] Config iterator.
    char* bufPtr,                       ///< [OUT] Buffer to store the path.
    size_t bufSize                      ///< [IN] Size of the buffer.
)
{
    if (le_cfg_GetString(cfgIter, "src", bufPtr, bufSize, "") != LE_OK)
    {
        LE_ERROR("Source path '%s...' for app '%s' is too long.", bufPtr, appRef->name);
        return LE_FAULT;
    }

    if (bufPtr[0] == '\0')
    {
        LE_ERROR("Empty src path supplied for app %s.", appRef->name);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the app's read only bundled files.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateBundledLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    // Get a config iterator for this app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    // Go to the bundled directories section.
    le_cfg_GoToNode(appCfg, CFG_NODE_BUNDLES);
    le_cfg_GoToNode(appCfg, CFG_NODE_DIRS);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        do
        {
            // Only handle read only directories.
            if (!le_cfg_GetBool(appCfg, "isWritable", false))
            {
                // Get source path.
                char srcPath[LIMIT_MAX_PATH_BYTES];
                if (GetBundledReadOnlySrcPath(appRef, appCfg, srcPath, sizeof(srcPath)) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }

                // Get destination path.
                char destPath[LIMIT_MAX_PATH_BYTES];
                if (GetDestPath(appRef, appCfg, destPath, sizeof(destPath)) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }

                // Create links for all files in the source directory.
                if (RecursivelyCreateLinks(appRef, appDirLabelPtr, srcPath, destPath) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

        le_cfg_GoToParent(appCfg);
    }

    // Go to the requires files section.
    le_cfg_GoToParent(appCfg);
    le_cfg_GoToNode(appCfg, CFG_NODE_FILES);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        do
        {
            // Only handle read only files.
            if (!le_cfg_GetBool(appCfg, "isWritable", false))
            {
                // Get source path.
                char srcPath[LIMIT_MAX_PATH_BYTES];
                if (GetBundledReadOnlySrcPath(appRef, appCfg, srcPath, sizeof(srcPath)) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }

                // Get destination path.
                char destPath[LIMIT_MAX_PATH_BYTES];
                if (GetDestPath(appRef, appCfg, destPath, sizeof(destPath)) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }

                if (CreateFileLink(appRef, appDirLabelPtr, srcPath, destPath) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);
    }

    le_cfg_CancelTxn(appCfg);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the app's required files under the current node in the configuration iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateRequiredFileLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr,         ///< [IN] SMACK label to use for created directories.
    le_cfg_IteratorRef_t cfgIter        ///< [IN] Config iterator.
)
{
    if (le_cfg_GoToFirstChild(cfgIter) == LE_OK)
    {
        do
        {
            // Get source path.
            char srcPath[LIMIT_MAX_PATH_BYTES];

            if (GetSrcPath(appRef, cfgIter, srcPath, sizeof(srcPath)) != LE_OK)
            {
                return LE_FAULT;
            }

            // Get destination path.
            char destPath[LIMIT_MAX_PATH_BYTES];
            if (GetDestPath(appRef, cfgIter, destPath, sizeof(destPath)) != LE_OK)
            {
                return LE_FAULT;
            }

            if (CreateFileLink(appRef, appDirLabelPtr, srcPath, destPath) != LE_OK)
            {
                return LE_FAULT;
            }
        }
        while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);

        le_cfg_GoToParent(cfgIter);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create links to the app's required directories, files and devices.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateRequiredLinks
(
    app_Ref_t appRef,                   ///< [IN] Application reference.
    const char* appDirLabelPtr          ///< [IN] SMACK label to use for created directories.
)
{
    // Get a config iterator for this app.
    le_cfg_IteratorRef_t appCfg = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    // Go to the required directories section.
    le_cfg_GoToNode(appCfg, CFG_NODE_REQUIRES);
    le_cfg_GoToNode(appCfg, CFG_NODE_DIRS);

    if (le_cfg_GoToFirstChild(appCfg) == LE_OK)
    {
        do
        {
            // Get source path.
            char srcPath[LIMIT_MAX_PATH_BYTES];

            if (GetSrcPath(appRef, appCfg, srcPath, sizeof(srcPath)) != LE_OK)
            {
                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }

            // Get destination path.
            char destPath[LIMIT_MAX_PATH_BYTES];
            if (GetDestPath(appRef, appCfg, destPath, sizeof(destPath)) != LE_OK)
            {
                le_cfg_CancelTxn(appCfg);
                return LE_FAULT;
            }

            // Treat /dev/shm differently.  These are shared memory expected to be shared between
            // other apps but also other userland processes.  So export the entire directory.
            if (le_path_IsEquivalent("/dev/shm", srcPath, "/") ||
                     le_path_IsSubpath("/dev/shm", srcPath, "/"))
            {
                if ((CreateDirLink(appRef, appDirLabelPtr, srcPath, destPath) != LE_OK) ||
                    (smack_SetLabel(srcPath, "*") != LE_OK))
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }

            }
            else
            {
                if (CreateDirLink(appRef, appDirLabelPtr, srcPath, destPath) != LE_OK)
                {
                    le_cfg_CancelTxn(appCfg);
                    return LE_FAULT;
                }
            }
        }
        while (le_cfg_GoToNextSibling(appCfg) == LE_OK);

        le_cfg_GoToParent(appCfg);
    }

    // Go to the requires files section
    le_cfg_GoToParent(appCfg);
    le_cfg_GoToNode(appCfg, CFG_NODE_FILES);

    if (CreateRequiredFileLinks(appRef, appDirLabelPtr, appCfg) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    // Go to the devices section.
    le_cfg_GoToParent(appCfg);
    le_cfg_GoToNode(appCfg, CFG_NODE_DEVICES);

    if (CreateRequiredFileLinks(appRef, appDirLabelPtr, appCfg) != LE_OK)
    {
        le_cfg_CancelTxn(appCfg);
        return LE_FAULT;
    }

    le_cfg_CancelTxn(appCfg);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets up the application execution area in the file system.  For a sandboxed app this will be the
 * sandbox.  For an unsandboxed app this will be the app's current working directory..
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetupAppArea
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Get the SMACK label for the folders we create.
    char appDirLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppAccessLabel(app_GetName(appRef), S_IRWXU, appDirLabel, sizeof(appDirLabel));

    // Create the appsWritable/<appName> directory if it does not already exist.
    if (dir_MakeSmack(appRef->workingDir,
                      S_IRUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH,
                      appDirLabel) == LE_FAULT)
    {
        return LE_FAULT;
    }

    if (appRef->sandboxed)
    {
        if (!fs_IsMountPoint(appRef->workingDir))
        {
            // Bind mount the root of the sandbox unto itself so that we just lazy umount this when we
            // need to clean up.
            if (mount(appRef->workingDir, appRef->workingDir, NULL, MS_BIND, NULL) != 0)
            {
                LE_ERROR("Couldn't bind mount '%s' unto itself. %m", appRef->workingDir);
                return LE_FAULT;
            }
        }

        // Create default links.
        if (CreateDefaultLinks(appRef, appDirLabel) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    // Create links to the app's lib and bin directories.
    if (CreateLibBinLinks(appRef, appDirLabel) != LE_OK)
    {
        return LE_FAULT;
    }

    // Create links to bundled files.
    if (CreateBundledLinks(appRef, appDirLabel) != LE_OK)
    {
        return LE_FAULT;
    }

    // Create links to required files.
    if (CreateRequiredLinks(appRef, appDirLabel) != LE_OK)
    {
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether the destination path conflicts with anything under the specified working
 * directory.
 *
 * @return
 *      LE_OK if there are no conflicts.
 *      LE_DUPLICATE if there is a conflict.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckPathConflict
(
    const char* destPathPtr,                ///< [IN] Dest path relative to the working directory.
    const char* workingDirPtr               ///< [IN] Working directory to check.
)
{
    // Iterate through the nodes of the specified path checking if there are conflicts.
    le_pathIter_Ref_t pathIter = le_pathIter_CreateForUnix(destPathPtr);

    if (le_pathIter_GoToStart(pathIter) != LE_OK)
    {
        return LE_DUPLICATE;
    }

    char currPath[LIMIT_MAX_PATH_BYTES] = "";
    LE_FATAL_IF(le_utf8_Copy(currPath, workingDirPtr, sizeof(currPath), NULL) != LE_OK,
                "Path '%s...' is too long.", workingDirPtr);

    while (1)
    {
        // Get the current path.
        char currNode[LIMIT_MAX_PATH_BYTES] = "";
        le_result_t r = le_pathIter_GetCurrentNode(pathIter, currNode, sizeof(currNode));
        LE_FATAL_IF(r == LE_OVERFLOW, "Path '%s...' is too long.", currNode);

        if (r == LE_NOT_FOUND)
        {
            // This is the last node of the destination path so there must be a conflict.
            le_pathIter_Delete(pathIter);
            return LE_DUPLICATE;
        }

        LE_FATAL_IF(le_path_Concat("/", currPath, sizeof(currPath), currNode, NULL) != LE_OK,
                    "Path '%s...' is too long.", currPath);

        // Check the sandbox for items at the current path.
        struct stat statBuf;

        if (lstat(currPath, &statBuf) == -1)
        {
            if (errno == ENOENT)
            {
                // Current path does not exist so there are no conflicts.
                le_pathIter_Delete(pathIter);
                return LE_OK;
            }
            LE_FATAL("Could not stat path '%s'.  %m.", currPath);
        }

        if (!S_ISDIR(statBuf.st_mode))
        {
            // Conflict.
            le_pathIter_Delete(pathIter);
            return LE_DUPLICATE;
        }

        if (le_pathIter_GoToNext(pathIter) == LE_NOT_FOUND)
        {
            // This is the last node of the destination path so there must be a conflict.
            le_pathIter_Delete(pathIter);
            return LE_DUPLICATE;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get kernel modules dependency from configTree and trigger installation of unloaded modules
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetKernelModules(app_Ref_t appRef)
{
    appRef->reqModuleName = LE_SLS_LIST_INIT;

    ModNameNode_t* modNameNodePtr;
    // Get a config iterator for this app.
    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(appRef->cfgPathRoot);

    // Go to the required kernelModules section.
    le_cfg_GoToNode(iter, CFG_NODE_REQUIRES "/" CFG_NODE_KERNELMODULES);

    if (le_cfg_GoToFirstChild(iter) == LE_OK)
    {
        do
        {
            modNameNodePtr = le_mem_ForceAlloc(ReqModStringPool);
            modNameNodePtr->link = LE_SLS_LINK_INIT;

            le_cfg_GetNodeName(iter, "", modNameNodePtr->modName, sizeof(modNameNodePtr->modName));

            if (strncmp(modNameNodePtr->modName, "", sizeof(modNameNodePtr->modName)) == 0)
            {
                LE_WARN("Found empty kernel module dependency");
                le_mem_Release(modNameNodePtr);
                continue;
            }

            modNameNodePtr->isOptional = le_cfg_GetBool(iter, "isOptional", false);
            le_sls_Queue(&(appRef->reqModuleName), &(modNameNodePtr->link));
        }
        while (le_cfg_GoToNextSibling(iter) == LE_OK);
    }

    le_cfg_CancelTxn(iter);

    if (!le_sls_IsEmpty(&(appRef->reqModuleName)))
    {
        if (kernelModules_InsertListOfModules(appRef->reqModuleName) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the path refers to a directory.
 *
 * @return
 *      true if the path refers to a directory.  false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDir
(
    const char* pathNamePtr     ///< [IN] The path to the directory.
)
{
    struct stat stats;
    if (lstat(pathNamePtr, &stats) == -1)
    {
        if ( (errno == ENOENT) || (errno == ENOTDIR) )
        {
            return false;
        }

        LE_FATAL("Could not stat path '%s'.  %m", pathNamePtr);
    }

    return S_ISDIR(stats.st_mode);
}


//--------------------------------------------------------------------------------------------------
/**
 * Recursively removes all links under the specified path.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveLinks
(
    app_Ref_t appRef,                           ///< [IN] Application reference.
    const char* pathPtr                         ///< [IN] Path to link to remove.
)
{
    char fullPath[LIMIT_MAX_PATH_BYTES] = "";

    LE_FATAL_IF(le_path_Concat("/", fullPath, sizeof(fullPath),
                               appRef->workingDir, pathPtr, NULL) != LE_OK,
                "Path '%s...' is too long.", fullPath);

    LE_INFO("Removing link %s from %s.", pathPtr, appRef->name);

    if (appRef->sandboxed)
    {
        fs_TryLazyUmount(fullPath);
    }

    // For unsandboxed apps delete the symlink.  For sandboxed apps delete the mount point.
    if (IsDir(fullPath))
    {
        if (rmdir(fullPath) != 0)
        {
            LE_ERROR("Could not delete directory %s.  %m.", fullPath);
        }
    }
    else
    {
        file_Delete(fullPath);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a process container for the app by name from the given process list
 *
 * @return
 *      The pointer to a process container if successful.
 *      NULL if the process container could not be found.
 */
//--------------------------------------------------------------------------------------------------
static app_Proc_Ref_t FindProcContainerByName
(
    le_dls_List_t* procListPtr,     ///< [IN] The process list to search in.
    const char* procNamePtr         ///< [IN] The process name to get for.
)
{
    // Find the process in the app's list.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(procListPtr);

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        if (strcmp(procNamePtr, proc_GetName(procContainerPtr->procRef)) == 0)
        {
            return procContainerPtr;
        }

        procLinkPtr = le_dls_PeekNext(procListPtr, procLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the application system.
 */
//--------------------------------------------------------------------------------------------------
void app_Init
(
    void
)
{
    AppPool = le_mem_CreatePool("Apps", sizeof(App_t));
    FileLinkNodePool = le_mem_CreatePool("Links", sizeof(FileLinkNode_t));
    ProcContainerPool = le_mem_CreatePool("ProcContainers", sizeof(ProcContainer_t));
    ReqModStringPool = le_mem_CreatePool("Required Modules", sizeof(ModNameNode_t));

    proc_Init();

    // Create the appsWriteable area.
    if (le_dir_MakePath(APPS_WRITEABLE_DIR, S_IRUSR | S_IXUSR | S_IROTH | S_IXOTH) != LE_OK)
    {
        LE_ERROR("Could not make appsWriteable dir, applications may not start.");
    }

    // Required for a system update. Otherwise when new system starts up, the app process
    // will not have permission to change its working directory to the applications apps
    // writeable directory. (Defaults as admin)
    smack_SetLabel("/legato/systems/current/appsWriteable", "framework");
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a process container for the app by name.
 *
 * @return
 *      The pointer to a process container if successful.
 *      NULL if the process container could not be found.
 */
//--------------------------------------------------------------------------------------------------
app_Proc_Ref_t app_GetProcContainer
(
    app_Ref_t appRef,               ///< [IN] The application to search in.
    const char* procNamePtr         ///< [IN] The process name to get for.
)
{
    if (procNamePtr == NULL)
    {
        return NULL;
    }

    return FindProcContainerByName(&(appRef->procs), procNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new temporary process name, based on the executable name.
 *
 * Temporary process name is guaranteed to be unique among currently running processes of an app.
 * This is done by adding \@N to the end of the name, where N is a number from 0 - MAX_CREATE_PROC
 * which is not used by another process with the same name.  If this would create a name longer
 * than the maximum allowed process name, the last few characters at the end of the exeName are
 * overwritten.
 */
//--------------------------------------------------------------------------------------------------
static bool MakeTempProcName
(
    app_Ref_t appRef,
    char* tempProcNamePtr,
    size_t tempProcNameSize,
    const char* exeName
)
{
    size_t numBytesCopied;
    int i;

    le_utf8_Copy(tempProcNamePtr, exeName, tempProcNameSize, &numBytesCopied);
    for (i = 0; i < MAX_CREATE_PROC; ++i)
    {
        // Is there enough space for the process number?
        // Warning: assumes name is no more than 2 decimal digits.
        size_t numLen = ((i < 10)?1:2);
        while ((numBytesCopied + numLen + 1) >= tempProcNameSize)
        {
            // No.  Truncate characters off the end to make space
            while ((--numBytesCopied) &&
                   (0 == le_utf8_NumBytesInChar(tempProcNamePtr[numBytesCopied])))
            {
                // Finding prevous character start
            }
        }
        snprintf(tempProcNamePtr + numBytesCopied, tempProcNameSize - numBytesCopied,
                 "@%d", i);

        // Name is chosen to be invalid as a regular process name, so only search auxilliary
        // processes.
        if (!FindProcContainerByName(&(appRef->auxProcs), tempProcNamePtr))
        {
            // Found an available name
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new process container.
 *
 * @return
 *      Process container reference.
 */
//--------------------------------------------------------------------------------------------------
static ProcContainer_t* CreateProcContainer
(
    app_Ref_t appRef,                   ///< [IN] Application this process belongs to.
    proc_Ref_t procRef                  ///< [IN] Process reference.
)
{
    ProcContainer_t* procContainerPtr = le_mem_ForceAlloc(ProcContainerPool);

    procContainerPtr->procRef = procRef;
    procContainerPtr->stopHandler = NULL;
    procContainerPtr->link = LE_DLS_LINK_INIT;
    procContainerPtr->externStopHandler = NULL;
    procContainerPtr->externContextPtr = NULL;

    return procContainerPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an application object.
 *
 * @note
 *      The name of the application is the node name (last part) of the cfgPathRootPtr.
 *
 * @return
 *      A reference to the application object if success.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
app_Ref_t app_Create
(
    const char* cfgPathRootPtr      ///< [IN] The path in the config tree for this application.
)
{
    // Create a new app object.
    App_t* appPtr = le_mem_ForceAlloc(AppPool);

    // Save the config path.
    if (   le_utf8_Copy(appPtr->cfgPathRoot, cfgPathRootPtr, sizeof(appPtr->cfgPathRoot), NULL)
        != LE_OK)
    {
        LE_ERROR("Config path '%s' is too long.", cfgPathRootPtr);

        le_mem_Release(appPtr);
        return NULL;
    }

    // Store the app name.
    appPtr->name = le_path_GetBasenamePtr(appPtr->cfgPathRoot, "/");

    // Initialize the other parameters.
    appPtr->procs = LE_DLS_LIST_INIT;
    appPtr->auxProcs = LE_DLS_LIST_INIT;
    appPtr->additionalLinks = LE_SLS_LIST_INIT;
    appPtr->state = APP_STATE_STOPPED;
    appPtr->killTimer = NULL;

    LE_INFO("Creating app '%s'", appPtr->name);

    // Get a config iterator for this app.
    le_cfg_IteratorRef_t cfgIterator = le_cfg_CreateReadTxn(appPtr->cfgPathRoot);

    // See if this is a sandboxed app.
    appPtr->sandboxed = le_cfg_GetBool(cfgIterator, CFG_NODE_SANDBOXED, true);

    // @todo: Create the user and all the groups for this app.  This function has a side affect
    //        where it populates the app's supplementary groups list and sets the uid and the
    //        primary gid.  This behaviour will be changed when the create user functionality is
    //        moved to the app installer.
    if (CreateUserAndGroups(appPtr) != LE_OK)
    {
        goto failed;
    }

    // Get the app's install directory path.
    appPtr->installDirPath[0] = '\0';
    if (LE_OK != le_path_Concat("/",
                                appPtr->installDirPath,
                                sizeof(appPtr->installDirPath),
                                APPS_INSTALL_DIR,
                                appPtr->name,
                                NULL))
    {
        LE_ERROR("Install directory path '%s' is too long.  App '%s' cannot be started.",
                 appPtr->installDirPath,
                 appPtr->name);
        goto failed;
    }

    // Use the app's writeable files' directory path as the its working directory.
    appPtr->workingDir[0] = '\0';
    if (LE_OK != le_path_Concat("/",
                                appPtr->workingDir,
                                sizeof(appPtr->workingDir),
                                APPS_WRITEABLE_DIR,
                                appPtr->name,
                                NULL))
    {
        LE_ERROR("Writeable files directory path '%s' is too long.  App '%s' cannot be started.",
                 appPtr->workingDir,
                 appPtr->name);
        goto failed;
    }

    // Move the config iterator to the procs list for this app.
    le_cfg_GoToNode(cfgIterator, CFG_NODE_PROC_LIST);

    // Read the list of processes for this application from the config tree.
    if (le_cfg_GoToFirstChild(cfgIterator) == LE_OK)
    {
        do
        {
            // Get the process's config path.
            char procCfgPath[LIMIT_MAX_PATH_BYTES];

            if (le_cfg_GetPath(cfgIterator, "", procCfgPath, sizeof(procCfgPath)) == LE_OVERFLOW)
            {
                LE_ERROR("Internal path buffer too small.");
                goto failed;
            }

            // Strip off the trailing '/'.
            size_t lastIndex = le_utf8_NumBytes(procCfgPath) - 1;

            if (procCfgPath[lastIndex] == '/')
            {
                procCfgPath[lastIndex] = '\0';
            }

            // Get the process name.
            char* procNamePtr = le_path_GetBasenamePtr(procCfgPath, "/");

            // Create the process.
            proc_Ref_t procPtr;
            if ((procPtr = proc_Create(procNamePtr, appPtr, procCfgPath)) == NULL)
            {
                goto failed;
            }

            // Add the process to the app's process list.
            ProcContainer_t* procContainerPtr = CreateProcContainer(appPtr, procPtr);

            le_dls_Queue(&(appPtr->procs), &(procContainerPtr->link));
        }
        while (le_cfg_GoToNextSibling(cfgIterator) == LE_OK);
    }

    // Set the resource limit for this application.
    if (resLim_SetAppLimits(appPtr) != LE_OK)
    {
        LE_ERROR("Could not set application resource limits.  Application %s cannot be started.",
                 appPtr->name);

        goto failed;
    }

    // Enable "notify_on_release" for this app, so the Supervisor will be notified when this app
    // stops.
    // Need to account for the characters other than app name in the path of notify_on_release.
    char notifyPath[LIMIT_MAX_APP_NAME_BYTES + 41] = {0};
    LE_ASSERT(snprintf(notifyPath, sizeof(notifyPath),
                       "/sys/fs/cgroup/freezer/%s/notify_on_release", appPtr->name)
              < sizeof(notifyPath));

    file_WriteStr(notifyPath, "1", 0);

    le_cfg_CancelTxn(cfgIterator);
    return appPtr;

failed:

    app_Delete(appPtr);
    le_cfg_CancelTxn(cfgIterator);
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all process containers from the specified list
 */
//--------------------------------------------------------------------------------------------------
static void DeleteProcContainersList
(
    le_dls_List_t procsList             ///< [IN] List of proc containers to delete.
)
{
    // Pop all the processes off the list and free them.
    le_dls_Link_t* procLinkPtr = le_dls_Pop(&procsList);

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        proc_Delete(procContainerPtr->procRef);
        le_mem_Release(procContainerPtr);

        procLinkPtr = le_dls_Pop(&procsList);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all required kernel module nodes from the specified list
 */
//--------------------------------------------------------------------------------------------------
static void DeleteModuleNodeList
(
    le_sls_List_t moduleList
)
{
    le_sls_Link_t* moduleLinkPtr;

    while ((moduleLinkPtr = le_sls_Pop(&moduleList)) != NULL)
    {
        ModNameNode_t* moduleNodePtr = CONTAINER_OF(moduleLinkPtr, ModNameNode_t, link);
        le_mem_Release(moduleNodePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an application.  The application must be stopped before it is deleted.
 *
 * @note If this function fails it will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void app_Delete
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to delete.
)
{
    CleanupAppSmackSettings(appRef);

    CleanupResourceCfg(appRef);

    // Remove the resource limits.
    resLim_CleanupApp(appRef);

    // Delete all the process containers.
    DeleteProcContainersList(appRef->procs);
    DeleteProcContainersList(appRef->auxProcs);

    // Release the app timer.
    if (appRef->killTimer != NULL)
    {
        le_timer_Delete(appRef->killTimer);
    }

    // Release app.
    le_mem_Release(appRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_Start
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to start.
)
{
    LE_INFO("Starting app '%s'", appRef->name);

    bool moduleLoadFailed = false;

    if (appRef->state == APP_STATE_RUNNING)
    {
        LE_ERROR("Application '%s' is already running.", appRef->name);

        return LE_FAULT;
    }

    if (framework_IsStopping())
    {
        LE_ERROR("App '%s' cannot be started because framework is shutting down.",
                 appRef->name);
        return LE_FAULT;
    }

    // Install the required kernel modules
    if (GetKernelModules(appRef) != LE_OK)
    {
        LE_ERROR("Error in installing dependent kernel modules for app '%s'", appRef->name);
        moduleLoadFailed = true;
    }

    appRef->state = APP_STATE_RUNNING;

    // Set SMACK rules for this app.
    // Setup the runtime area in the file system.
    if ( (SetSmackRules(appRef) != LE_OK) ||
         (SetupAppArea(appRef) != LE_OK) )
    {
        LE_ERROR("Failed to set Smack rules or set up app area.");
        return LE_FAULT;
    }

    // Create /tmp for sandboxed apps and link in /tmp files.
    if (appRef->sandboxed)
    {
        // Get the SMACK label for the folders we create.
        char appDirLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
        smack_GetAppAccessLabel(app_GetName(appRef), S_IRWXU, appDirLabel, sizeof(appDirLabel));

        // Create the app's /tmp for sandboxed apps.
        if (CreateTmpFs(appRef, appDirLabel) != LE_OK)
        {
            return LE_FAULT;
        }

        // Create default links.
        if (CreateDefaultTmpLinks(appRef, appDirLabel) != LE_OK)
        {
            return LE_FAULT;
        }
    }

    // Start all the processes in the application.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        if (moduleLoadFailed)
        {
            // If a module failed to load then trigger fault action of the process.
            switch (proc_GetFaultAction(procContainerPtr->procRef))
            {
                case FAULT_ACTION_RESTART_APP:
                    LE_CRIT("Fault action is to restart app '%s'.", appRef->name);
                    return LE_TERMINATED;
                    break;

                case FAULT_ACTION_STOP_APP:
                    LE_CRIT("Fault action is to stop app '%s'.", appRef->name);
                    return LE_WOULD_BLOCK;
                    break;

                case FAULT_ACTION_REBOOT:
                    LE_EMERG("Fault action is to reboot the system.");
                    framework_Reboot();
                    return LE_FAULT;
                    break;

                case FAULT_ACTION_RESTART_PROC:
                case FAULT_ACTION_IGNORE:
                case FAULT_ACTION_NONE:
                default:
                    LE_INFO("Proceed with starting processes.");
                    break;
            }
        }

        le_result_t result = proc_Start(procContainerPtr->procRef);

        if (result != LE_OK)
        {
            LE_ERROR("Could not start all application processes.  Stopping the application '%s'.",
                     appRef->name);

            app_Stop(appRef);

            return LE_FAULT;
        }

        // Get the next process.
        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stops an application.  This is an asynchronous function call that returns immediately but
 * the application may not stop right away.  Check the application's state with app_GetState() to
 * see when the application actually stops.
 */
//--------------------------------------------------------------------------------------------------
void app_Stop
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
)
{
    LE_INFO("Stopping app '%s'", appRef->name);

    CleanupAppSmackSettings(appRef);

    if (appRef->state == APP_STATE_STOPPED)
    {
        LE_ERROR("Application '%s' is already stopped.", appRef->name);
        return;
    }

    if (!le_sls_IsEmpty(&(appRef->reqModuleName)))
    {
        if (kernelModules_RemoveListOfModules(appRef->reqModuleName) != LE_OK)
        {
            LE_ERROR("Error in removing the list of kernel modules");
        }
    }

    // Soft kill all the processes in the app.
    if (KillAppProcs(appRef, KILL_SOFT) == LE_OK)
    {
        // Start the kill timeout timer for this app.
        if (appRef->killTimer == NULL)
        {
            char timerName[LIMIT_MAX_PATH_BYTES];

            snprintf(timerName, sizeof(timerName), "%s_Killer", appRef->name);
            appRef->killTimer = le_timer_Create(timerName);

            LE_ASSERT(le_timer_SetInterval(appRef->killTimer, KillTimeout) == LE_OK);

            LE_ASSERT(le_timer_SetContextPtr(appRef->killTimer, (void*)appRef) == LE_OK);
            LE_ASSERT(le_timer_SetHandler(appRef->killTimer, HardKillApp) == LE_OK);
        }

        le_timer_Start(appRef->killTimer);
    }
    // This case is essential to stop a "running app" with no configured processes.
    else if (!HasRunningProc(appRef))
    {
        // There are no more running processes in the app.
        LE_DEBUG("app '%s' has stopped.", appRef->name);

        appRef->state = APP_STATE_STOPPED;
    }

    DeleteModuleNodeList(appRef->reqModuleName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's state.
 *
 * @return
 *      The application's state.
 */
//--------------------------------------------------------------------------------------------------
app_State_t app_GetState
(
    app_Ref_t appRef                    ///< [IN] Reference to the application to stop.
)
{
    return appRef->state;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the state of a process belonging to an application.
 *
 * @return
 *      The process's state.
 */
//--------------------------------------------------------------------------------------------------
app_ProcState_t app_GetProcState
(
    app_Ref_t appRef,                   ///< [IN] Reference to the application to stop.
    const char* procName                ///< [IN] Name of the process.
)
{
    if (appRef->state == APP_STATE_RUNNING)
    {
        // Find the process in the app's list.
        le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

        while (procLinkPtr != NULL)
        {
            ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

            if (strcmp(procName, proc_GetName(procContainerPtr->procRef)) == 0)
            {
                switch (proc_GetState(procContainerPtr->procRef))
                {
                    case PROC_STATE_STOPPED:
                        return APP_PROC_STATE_STOPPED;

                    case PROC_STATE_RUNNING:
                        return APP_PROC_STATE_RUNNING;

                    default:
                        LE_FATAL("Unrecognized process state.");
                }
            }

            procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
        }
    }

    return APP_PROC_STATE_STOPPED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if a given app is running a top-level process with given PID.
 *
 * An app's top-level processes are those that are started by the Supervisor directly.
 * If the Supervisor starts a process and that process starts another process, this function
 * will not find that second process.
 *
 * @return
 *      true if the process is one of this app's top-level processes, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool app_HasTopLevelProc
(
    app_Ref_t appRef,
    pid_t pid
)
{
    return (FindProcContainer(appRef, pid) != NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's name.
 *
 * @return
 *      The application's name.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetName
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->name;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's UID.
 *
 * @return
 *      The application's UID.
 */
//--------------------------------------------------------------------------------------------------
uid_t app_GetUid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return appRef->uid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's GID.
 *
 * @return
 *      The application's GID.
 */
//--------------------------------------------------------------------------------------------------
gid_t app_GetGid
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return appRef->gid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the application is sandboxed or not.
 *
 * @return
 *      True if the app is sandboxed, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool app_GetIsSandboxed
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return appRef->sandboxed;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the directory path for an app's installation directory in the current running system.
 *
 * @return
 *      The absolute directory path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetInstallDirPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
//--------------------------------------------------------------------------------------------------
{
    return (const char*)appRef->installDirPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's working directory.
 *
 * @return
 *      The application's sandbox path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetWorkingDir
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->workingDir;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's configuration path.
 *
 * @return
 *      The application's configuration path.
 */
//--------------------------------------------------------------------------------------------------
const char* app_GetConfigPath
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    return (const char*)appRef->cfgPathRoot;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets an application's supplementary groups list.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer was too small to hold all the gids.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_GetSupplementaryGroups
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    gid_t* groupsPtr,                   ///< [OUT] List of group IDs.
    size_t* numGroupsPtr                ///< [IN/OUT] Size of groupsPtr buffer on input.  Number of
                                        ///           groups in groupsPtr on output.
)
{
    int i = 0;

    if (*numGroupsPtr >= appRef->numSupplementGids)
    {
        for (i = 0; i < appRef->numSupplementGids; i++)
        {
            groupsPtr[i] = appRef->supplementGids[i];
        }

        *numGroupsPtr = appRef->numSupplementGids;

        return LE_OK;
    }
    else
    {
        for (i = 0; i < *numGroupsPtr; i++)
        {
            groupsPtr[i] = appRef->supplementGids[i];
        }

        *numGroupsPtr = appRef->numSupplementGids;

        return LE_OVERFLOW;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when the watchdog expires for a process that belongs to the
 * specified application.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the procPid was not found for the specified app.
 *
 *      The watchdog action passed in will be set to the action that should be taken for
 *      this process or one of the following:
 *          WATCHDOG_ACTION_NOT_FOUND - no action was configured for this process
 *          WATCHDOG_ACTION_ERROR     - the action could not be read or is unknown
 *          WATCHDOG_ACTION_HANDLED   - no further action is required, it is already handled.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_WatchdogTimeoutHandler
(
    app_Ref_t appRef,                                 ///< [IN] The application reference.
    pid_t procPid,                                    ///< [IN] The pid of the process that changed
                                                      ///< state.
    wdog_action_WatchdogAction_t* watchdogActionPtr   ///< [OUT] The fault action that should be
                                                      ///< taken.
)
{
    LE_FATAL_IF(appRef == NULL, "appRef is NULL");

    ProcContainer_t* procContainerPtr = FindProcContainer(appRef, procPid);

    if (procContainerPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    proc_Ref_t procRef = procContainerPtr->procRef;

    // Get the current process fault action.
    wdog_action_WatchdogAction_t watchdogAction = proc_GetWatchdogAction(procRef);

    // Set the action pointer to error. If it's still error when we leave here something
    // has gone wrong!!
    *watchdogActionPtr = WATCHDOG_ACTION_ERROR;

    // TODO: do watchdog timeouts count toward this total?
    switch (watchdogAction)
    {
        case WATCHDOG_ACTION_NOT_FOUND:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out but there is no \
policy. The process will be restarted by default.", proc_GetName(procRef), appRef->name);

            // Set the process to restart when it stops then stop it.
            procContainerPtr->stopHandler = proc_Start;
            StopProc(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_IGNORE:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be ignored \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_STOP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be terminated \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);
            StopProc(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_RESTART:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and will be restarted \
in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            // Set the process to restart when it stops then stop it.
            procContainerPtr->stopHandler = proc_Start;
            StopProc(procRef);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_RESTART_APP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and the app will be \
restarted in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_STOP_APP:
            LE_CRIT("The watchdog for process '%s' in app '%s' has timed out and the app will \
be stopped in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_REBOOT:
            LE_EMERG("The watchdog for process '%s' in app '%s' has timed out and the system will \
now be rebooted in accordance with its timeout policy.", proc_GetName(procRef), appRef->name);

            *watchdogActionPtr = watchdogAction;
            break;

        case WATCHDOG_ACTION_ERROR:
            // something went wrong reading the action.
            LE_CRIT("An error occurred trying to find the watchdog action for process '%s' in \
application '%s'. Restarting app by default.", proc_GetName(procRef), appRef->name);
            *watchdogActionPtr = WATCHDOG_ACTION_HANDLED;
            break;

        case WATCHDOG_ACTION_HANDLED:
            *watchdogActionPtr = watchdogAction;
            break;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This handler must be called when a SIGCHILD is received for a process that belongs to the
 * specified application.
 */
//--------------------------------------------------------------------------------------------------
void app_SigChildHandler
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    pid_t procPid,                      ///< [IN] The pid of the process that changed state.
    int procExitStatus,                 ///< [IN] The return status of the process given by wait().
    FaultAction_t* faultActionPtr       ///< [OUT] The fault action that should be taken.
)
{
    *faultActionPtr = FAULT_ACTION_IGNORE;

    ProcContainer_t* procContainerPtr = FindProcContainer(appRef, procPid);

    if (procContainerPtr != NULL)
    {
        // This proc has died call its external stop handler to inform interested parties of this
        // death.
        if (procContainerPtr->externStopHandler != NULL)
        {
            procContainerPtr->externStopHandler(procExitStatus, procContainerPtr->externContextPtr);
        }

        // Tell the "proc" module to handle the signal.  It will tell us what it wants us to do
        // about it, based on the process's faultAction.
        proc_Ref_t procRef = procContainerPtr->procRef;

        FaultAction_t procFaultAction = proc_SigChildHandler(procRef, procExitStatus);

        switch (procFaultAction)
        {
            case FAULT_ACTION_NONE:
                // This is something that happens if we have deliberately killed the proc or the
                // proc has terminated normally (EXIT_SUCCESS).
                // If the wdog stopped it then we may get here with an attached stop handler (to
                // call StartProc).
                if (procContainerPtr->stopHandler)
                {
                    if (procContainerPtr->stopHandler(procRef) != LE_OK)
                    {
                        LE_ERROR("Watchdog could not restart process '%s' in app '%s'.",
                                proc_GetName(procRef), appRef->name);

                        *faultActionPtr = FAULT_ACTION_STOP_APP;
                    }
                }
                break;

            case FAULT_ACTION_IGNORE:
                LE_WARN("Process '%s' in app '%s' faulted: Ignored.",
                        proc_GetName(procRef),
                        appRef->name);
                break;

            case FAULT_ACTION_RESTART_PROC:
                LE_CRIT("Process '%s' in app '%s' faulted: Restarting process.",
                        proc_GetName(procRef),
                        appRef->name);

                // Restart the process now.
                if (proc_Start(procRef) != LE_OK)
                {
                    LE_ERROR("Could not restart process '%s' in app '%s'.",
                             proc_GetName(procRef), appRef->name);

                    *faultActionPtr = FAULT_ACTION_STOP_APP;
                }
                break;

            case FAULT_ACTION_RESTART_APP:
                LE_CRIT("Process '%s' in app '%s' faulted: Restarting app.",
                        proc_GetName(procRef),
                        appRef->name);

                *faultActionPtr = FAULT_ACTION_RESTART_APP;
                break;

            case FAULT_ACTION_STOP_APP:
                LE_CRIT("Process '%s' in app '%s' faulted: Stopping app.",
                        proc_GetName(procRef),
                        appRef->name);

                *faultActionPtr = FAULT_ACTION_STOP_APP;
                break;

            case FAULT_ACTION_REBOOT:
                LE_EMERG("Process '%s' in app '%s' faulted: Rebooting system.",
                         proc_GetName(procRef),
                         appRef->name);

                *faultActionPtr = FAULT_ACTION_REBOOT;
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a reference to an application process.
 *
 * If the process name refers to an existing configured application process then a reference to that
 * process is simply returned.  In this case an executable path may be specified to override the
 * configured executable.
 *
 * If the process name does not match any configured application processes then a new process
 * is created.  In this case an executable path must be specified.
 *
 * Configured processes take their runtime parameters, such as environment variables, priority, etc.
 * from the configuration database while non-configured processes use default parameters.
 *
 * Parameters can be overridden by the other functions in this API such as app_AddProcArg(),
 * app_SetProcPriority(), etc.
 *
 * It is an error to call this function on a configured process that is already running.
 *
 * @return
 *      Reference to the application process if successful.
 *      NULL if there was an error.
 */
//--------------------------------------------------------------------------------------------------
app_Proc_Ref_t app_CreateProc
(
    app_Ref_t appRef,                   ///< [IN] The application reference.
    const char* procNamePtr,            ///< [IN] Name of the process.
    const char* execPathPtr             ///< [IN] Path to the executable file.
)
{
    // See if the process already exists.
    ProcContainer_t* procContainerPtr = app_GetProcContainer(appRef, procNamePtr);

    if (procContainerPtr == NULL)
    {
        char procNameToUse[LIMIT_MAX_PROCESS_NAME_LEN + 1];

        // This is not a configured process so make sure the executable path is provided.
        if (execPathPtr == NULL)
        {
            LE_ERROR("Executable path for process %s in app %s cannot be empty.",
                     procNamePtr, appRef->name);

            return NULL;
        }

        // If the process name is empty use the base name of the executable as the process name.
        const char* procNameToUsePtr = procNamePtr;

        if (procNameToUsePtr == NULL)
        {
            if (!MakeTempProcName(appRef,
                                  procNameToUse, sizeof(procNameToUse),
                                  le_path_GetBasenamePtr(execPathPtr, "/")))
            {
                return NULL;
            }
            procNameToUsePtr = procNameToUse;
        }

        // Create the process.
        proc_Ref_t procPtr;
        if ((procPtr = proc_Create(procNameToUsePtr, appRef, NULL)) == NULL)
        {
            return NULL;
        }

        // Store the executable path.
        if (proc_SetExecPath(procPtr, execPathPtr) != LE_OK)
        {
            LE_ERROR("Executable path '%s' is too long.", execPathPtr);

            proc_Delete(procPtr);
            return NULL;
        }

        // Create the process container.
        procContainerPtr = CreateProcContainer(appRef, procPtr);

        // Add the process to the app's auxiliary process list.
        le_dls_Queue(&(appRef->auxProcs), &(procContainerPtr->link));
    }
    else
    {
        // This is a configured process.
        if (proc_GetState(procContainerPtr->procRef) == PROC_STATE_RUNNING)
        {
            LE_ERROR("Process '%s' in app '%s' is already running.", procNamePtr, appRef->name);
            return NULL;
        }

        if (execPathPtr != NULL)
        {
             // Set the executable path.
            if (proc_SetExecPath(procContainerPtr->procRef, execPathPtr) != LE_OK)
            {
                LE_ERROR("Executable path '%s' is too long.", execPathPtr);
                return NULL;
            }
        }
    }

    return procContainerPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard in of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdIn
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdInFd                 ///< [IN] File descriptor to use as the app proc's standard in.
)
{
    proc_SetStdIn(appProcRef->procRef, stdInFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard out of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdOut
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdOutFd                ///< [IN] File descriptor to use as the app proc's standard out.
)
{
    proc_SetStdOut(appProcRef->procRef, stdOutFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the standard error of an application process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStdErr
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    int stdErrFd                ///< [IN] File descriptor to use as the app proc's standard error.
)
{
    proc_SetStdErr(appProcRef->procRef, stdErrFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets a stop handler to be called when the specified process stops.
 */
//--------------------------------------------------------------------------------------------------
void app_SetProcStopHandler
(
    app_Proc_Ref_t appProcRef,                  ///< [IN] Process reference.
    app_Proc_StopHandlerFunc_t stopHandler,     ///< [IN] Handler to call when the process exits.
    void* stopHandlerContextPtr                 ///< [IN] Context data for the stop handler.
)
{
    appProcRef->externStopHandler = stopHandler;
    appProcRef->externContextPtr = stopHandlerContextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the process's priority.
 *
 * This overrides the configured priority if available.
 *
 * The priority level string can be either "idle", "low", "medium", "high", "rt1" ... "rt32".
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the priority string is too long.
 *      LE_FAULT if the priority string is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetProcPriority
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    const char* priorityPtr     ///< [IN] Priority string.  NULL to clear the priority.
)
{
    return proc_SetPriority(appProcRef->procRef, priorityPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a cmd-line argument to a process.  Adding a NULL argPtr is valid and can be used to validate
 * the args list without actually adding an argument.  This is useful for overriding the configured
 * arguments with an empty list.
 *
 * This overrides the configured arguments if available.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the argument string is too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_AddArgs
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    const char* argPtr          ///< [IN] Argument string.
)
{
    return proc_AddArgs(appProcRef->procRef, argPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes and invalidates the cmd-line arguments to a process.  This means the process will only
 * use arguments from the config if available.
 */
//--------------------------------------------------------------------------------------------------
void app_ClearArgs
(
    app_Proc_Ref_t appProcRef   ///< [IN] Process reference.
)
{
    proc_ClearArgs(appProcRef->procRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets fault action for a process.
 *
 * This overrides the configured fault action if available.
 *
 * The fault action can be set to FAULT_ACTION_NONE to indicate that the configured fault action
 * should be used if available.
 */
//--------------------------------------------------------------------------------------------------
void app_SetFaultAction
(
    app_Proc_Ref_t appProcRef,          ///< [IN] Process reference.
    FaultAction_t faultAction           ///< [IN] Fault action.
)
{
    proc_SetFaultAction(appProcRef->procRef, faultAction);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag of a process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetRun
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    bool run                    ///< [IN] Run flag.
)
{
    proc_SetRun(appProcRef->procRef, run);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the run flag of all processes in an app.
 */
//--------------------------------------------------------------------------------------------------
void app_SetRunForAllProcs
(
    app_Ref_t appRef,  ///< [IN] App reference.
    bool run           ///< [IN] Run flag.
)
{
    // Go through all procs in app's proc list and set the run flag for each one.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        proc_SetRun(procContainerPtr->procRef, run);

        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the debug flag of a process.
 */
//--------------------------------------------------------------------------------------------------
void app_SetDebug
(
    app_Proc_Ref_t appProcRef,  ///< [IN] Process reference.
    bool debug                  ///< [IN] Run flag.
)
{
    proc_SetDebug(appProcRef->procRef, debug);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an application process.  This function assumes that the app has already started.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_StartProc
(
    app_Proc_Ref_t appProcRef                   ///< [IN] Process reference.
)
{
    if (proc_GetState(appProcRef->procRef) == PROC_STATE_STOPPED)
    {
        return proc_Start(appProcRef->procRef);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an application process from an app.
 *
 * If the process is running stop it first.
 *
 * If the process is a configured process the overriden parameters are cleared but the process is
 * not actually deleted.
 */
//--------------------------------------------------------------------------------------------------
void app_DeleteProc
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    app_Proc_Ref_t appProcRef                   ///< [IN] Process reference.
)
{
    if (proc_GetState(appProcRef->procRef) == PROC_STATE_RUNNING)
    {
        StopProc(appProcRef->procRef);
    }

    if (proc_GetConfigPath(appProcRef->procRef) != NULL)
    {
        // This is a configured process.  Simply reset all of the overrides.
        proc_SetStdIn(appProcRef->procRef, -1);
        proc_SetStdOut(appProcRef->procRef, -1);
        proc_SetStdErr(appProcRef->procRef, -1);

        proc_SetExecPath(appProcRef->procRef, NULL);
        proc_SetPriority(appProcRef->procRef, NULL);
        proc_ClearArgs(appProcRef->procRef);
        proc_SetFaultAction(appProcRef->procRef, FAULT_ACTION_NONE);
        proc_SetRun(appProcRef->procRef, true);
        proc_SetDebug(appProcRef->procRef, false);

        appProcRef->externStopHandler = NULL;
        appProcRef->externContextPtr = NULL;
    }
    else
    {
        // This is an auxiliary process.  Delete it.
        le_dls_Remove(&(appRef->auxProcs), &(appProcRef->link));

        proc_Delete(appProcRef->procRef);
        le_mem_Release(appProcRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a new link to a file for the app.  A link to the file will be created in the app's working
 * directory under the same path.  For example, if the path is /bin/ls then a link to the file will
 * be created at appsSandboxRoot/bin/ls.
 *
 * @return
 *      LE_OK if successful.
 *      LE_DUPLICATE if the link could not be created because the path conflicts with an existing
 *                   item already in the app.
 *      LE_NOT_FOUND if the source path does not point to a valid file.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_AddLink
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    const char* pathPtr                         ///< [IN] Absolute path to the file.
)
{
    // Check that the source path points to an existing file.
    struct stat statBuf;

    if (lstat(pathPtr, &statBuf) != 0)
    {
        if ( (errno == ENOENT) || (errno == ENOTDIR) )
        {
            return LE_NOT_FOUND;
        }

        LE_FATAL("Could not stat path %s.  %m.", pathPtr);
    }

    if (S_ISDIR(statBuf.st_mode))
    {
        return LE_NOT_FOUND;
    }

    // Construct the destination path.
    const char* destPathPtr = pathPtr;
    bool isDir = false;

    if ( le_path_IsEquivalent("/proc", pathPtr, "/") ||
         le_path_IsSubpath("/proc", pathPtr, "/") )
    {
        // Treat files in /proc differently because /proc contains dynamic files created by the
        // kernel that may be addressed differently (ie. /proc/self/...).
        // Just import the entire directory.
        destPathPtr = "/proc";
        isDir = true;
    }
    else if ( le_path_IsEquivalent("/sys", pathPtr, "/") ||
              le_path_IsSubpath("/sys", pathPtr, "/") )
    {
        // Treat files in /sys differently because /sys contains dynamic files created by the
        // kernel.  Just import the entire directory.
        destPathPtr = "/sys";
        isDir = true;
    }

    // Check that the dest path does not conflict with anything already in the app's
    // working directory.
    if (CheckPathConflict(destPathPtr, appRef->workingDir) != LE_OK)
    {
        return LE_DUPLICATE;
    }

    // Get the SMACK label for the folders we create.
    char appDirLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppAccessLabel(app_GetName(appRef), S_IRWXU, appDirLabel, sizeof(appDirLabel));

    // Create the link.
    le_result_t result = LE_FAULT;

    if (isDir)
    {
        result = CreateDirLink(appRef, appDirLabel, destPathPtr, destPathPtr);
    }
    else
    {
        result = CreateFileLink(appRef, appDirLabel, pathPtr, destPathPtr);
    }

    // Store a record of the new link.
    FileLinkNode_t* fileLinkNodePtr = le_mem_ForceAlloc(FileLinkNodePool);
    fileLinkNodePtr->link = LE_SLS_LINK_INIT;

    LE_FATAL_IF(le_utf8_Copy(fileLinkNodePtr->fileLink, destPathPtr,
                             sizeof(fileLinkNodePtr->fileLink), NULL) != LE_OK,
                "Dest path '%s...' is too long.", destPathPtr);

    le_sls_Queue(&(appRef->additionalLinks), &(fileLinkNodePtr->link));

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove all links added using app_AddLink().
 */
//--------------------------------------------------------------------------------------------------
void app_RemoveAllLinks
(
    app_Ref_t appRef                            ///< [IN] App reference.
)
{
    // Remove all additional file links for this app.
    le_sls_Link_t* nodeLinkPtr = le_sls_Pop(&(appRef->additionalLinks));

    while (nodeLinkPtr != NULL)
    {
        FileLinkNode_t* fileLinkNodePtr = CONTAINER_OF(nodeLinkPtr, FileLinkNode_t, link);

        RemoveLinks(appRef, fileLinkNodePtr->fileLink);

        le_mem_Release(fileLinkNodePtr);

        nodeLinkPtr = le_sls_Pop(&(appRef->additionalLinks));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the permissions for a device file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the source path does not point to a valid device.
 *      LE_FAULT if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_SetDevPerm
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    const char* pathPtr,                        ///< [IN] Absolute path to the device.
    const char* permissionPtr                   ///< [IN] Permission string, "r", "w", "rw".
)
{
    // Get the app's SMACK label.
    char appLabel[LIMIT_MAX_SMACK_LABEL_BYTES];
    smack_GetAppLabel(app_GetName(appRef), appLabel, sizeof(appLabel));

    le_result_t result = SetDevicePermissions(appLabel, pathPtr, permissionPtr);

    if (result != LE_OK)
    {
        LE_ERROR("Failed to set permissions (%s) for app '%s' on device '%s'.",
                 permissionPtr,
                 appRef->name,
                 pathPtr);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Blocks each app process on startup, after the process is forked and initialized but before it has
 * execed.  The specified callback function will be called when the process has blocked.  Clearing
 * the callback function means processes should not block on startup.
 */
//--------------------------------------------------------------------------------------------------
void app_SetBlockCallback
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    app_BlockFunc_t blockCallback,              ///< [IN] Callback function.  NULL to clear.
    void* contextPtr                            ///< [IN] Context pointer.
)
{
    // Set the callback for each process in the app.
    le_dls_Link_t* procLinkPtr = le_dls_Peek(&(appRef->procs));

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        proc_SetBlockCallback(procContainerPtr->procRef, blockCallback, contextPtr);

        procLinkPtr = le_dls_PeekNext(&(appRef->procs), procLinkPtr);
    }

    // Set the callback for each aux process in the app.
    procLinkPtr = le_dls_Peek(&(appRef->auxProcs));

    while (procLinkPtr != NULL)
    {
        ProcContainer_t* procContainerPtr = CONTAINER_OF(procLinkPtr, ProcContainer_t, link);

        proc_SetBlockCallback(procContainerPtr->procRef, blockCallback, contextPtr);

        procLinkPtr = le_dls_PeekNext(&(appRef->auxProcs), procLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Unblocks a process that was blocked on startup.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the process could not be found in the app.
 */
//--------------------------------------------------------------------------------------------------
le_result_t app_Unblock
(
    app_Ref_t appRef,                           ///< [IN] App reference.
    pid_t pid                                   ///< [IN] Process to unblock.
)
{
    ProcContainer_t* procContainerPtr = FindProcContainer(appRef, pid);

    if (procContainerPtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    proc_Unblock(procContainerPtr->procRef);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the application has any configured processes running.
 *
 * @return
 *      true if there is at least one configured running process for the application.
 *      false if there are no configured running processes for the application.
 */
//--------------------------------------------------------------------------------------------------
bool app_HasConfRunningProc
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Checks the appRef->procs list for processes that configured in the configuration DB.
    // Checks the appRef->auxProcs list for processes started by the le_appProc API.
    return ( HasRunningProcInList(appRef->procs) ||
             HasRunningProcInList(appRef->auxProcs) );
}


//--------------------------------------------------------------------------------------------------
/**
 * Performs tasks after an app has been stopped.
 */
//--------------------------------------------------------------------------------------------------
void app_StopComplete
(
    app_Ref_t appRef                    ///< [IN] The application reference.
)
{
    // Since the app has already stopped, we can stop the time-out timer now.
    if (appRef->killTimer != NULL)
    {
        le_timer_Stop(appRef->killTimer);
    }

    LE_INFO("app '%s' has stopped.", appRef->name);

    appRef->state = APP_STATE_STOPPED;
}
