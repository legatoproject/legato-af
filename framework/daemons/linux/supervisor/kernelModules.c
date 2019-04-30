//--------------------------------------------------------------------------------------------------
/** @file kernelModules.c
 *
 * API for managing Legato-bundled kernel modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "smack.h"
#include "sysPaths.h"
#include "kernelModules.h"
#include "le_cfg_interface.h"
#include "supervisor.h"

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool size for module objects and strings
 */
//--------------------------------------------------------------------------------------------------
#define KMODULE_DEFAULT_POOL_SIZE 8
#define STRINGS_DEFAULT_POOL_SIZE 8


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of parameters passed to a kernel module during insmod.
 */
//--------------------------------------------------------------------------------------------------
#define KMODULE_MAX_ARGC 256


//--------------------------------------------------------------------------------------------------
/**
 * Maximum parameter string buffer size in the form of "<name>=<value>\0".
 * Use maximum name and string value size from configTree.
 * Allow extra space (2 bytes) for enclosing value in quotes, if necessary.
 */
//--------------------------------------------------------------------------------------------------
#define STRINGS_MAX_BUFFER_SIZE (LE_CFG_NAME_LEN + LE_CFG_STR_LEN + 2 + 2)


//--------------------------------------------------------------------------------------------------
/**
 * Root of configTree containing module parameters
 */
//--------------------------------------------------------------------------------------------------
#define KMODULE_CONFIG_TREE_ROOT "/modules"


//--------------------------------------------------------------------------------------------------
/**
 * Module insert command and format, arguments are module path and module params
 */
//--------------------------------------------------------------------------------------------------
#define INSMOD_COMMAND "/sbin/insmod"


//--------------------------------------------------------------------------------------------------
/**
 * Module remove command and format, argument is module name
 */
//--------------------------------------------------------------------------------------------------
#define RMMOD_COMMAND "/sbin/rmmod"


//--------------------------------------------------------------------------------------------------
/**
 * Modprobe command and format.
 * Argument is module name for adding module and "-r" module name for removing module.
 */
//--------------------------------------------------------------------------------------------------
#define MODPROBE_COMMAND "/sbin/modprobe"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum buffer length to store the output of modinfo.
 */
//--------------------------------------------------------------------------------------------------
#define MODINFO_MAX_BUFFER_LEN 4096


//--------------------------------------------------------------------------------------------------
/**
 * Macro to get the length of an array.
 */
//--------------------------------------------------------------------------------------------------
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))


//--------------------------------------------------------------------------------------------------
/**
 * Legato kernel module object.
 */
//--------------------------------------------------------------------------------------------------
#define KMODULE_OBJECT_COOKIE 0x71a89c35
typedef struct
{
    uint32_t           cookie;                               // KModuleObj_t identifier
    char               *name;                                // Module name
    char               path[LIMIT_MAX_PATH_BYTES];           // Path to module's .ko file
    int                argc;                                 // insmod argc
    char               *argv[KMODULE_MAX_ARGC];              // insmod argv
    le_sls_List_t      reqModuleName;                        // List of required kernel modules
    le_sls_List_t      dependsModuleName;                    // List of system dependency modules
    ModuleLoadStatus_t moduleLoadStatus;                     // Load status of the module
    bool               isLoadManual;                         // is module load set to auto or manual
    bool               isOptional;                           // is the module required or optional
    le_dls_Link_t      dependencyLink;                       // link object for dependency list
    le_dls_Link_t      alphabeticalLink;                     // link object for alphabetical list
    le_sls_Link_t      cyclicDepLink;                        // link object for cyclic dep list
    uint32_t           useCount;                             // Counter of usage, safe to remove
                                                             // module when counter is 0
    char               installScript[LIMIT_MAX_PATH_BYTES];  // Path to module install script file
    char               removeScript[LIMIT_MAX_PATH_BYTES];   // Path to module remove script file
    bool               isRequiredModule;                     // is required module or not
    bool               isCyclicDependency;                   // is the module involved in circular
                                                             // dependency
    bool               visited;                              // Track visited kernel modules while
                                                             // traversing to detect cycle
    bool               recurStack;                           // Track recursion stack while
                                                             // traversing to detect cycle
}
KModuleObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Legato kernel module handler object.
 */
//--------------------------------------------------------------------------------------------------
static struct {
    le_mem_PoolRef_t    modulePool;        // memory pool of KModuleObj_t objects
    le_mem_PoolRef_t    stringPool;        // memory pool of strings (for argv)
    le_mem_PoolRef_t    reqModStringPool;  // memory pool of required kernel modules strings
    le_mem_PoolRef_t    depModStringPool;  // memory pool of depend system kernel modules strings
    le_hashmap_Ref_t    moduleTable;       // table for kernel module objects
    le_hashmap_Ref_t    dependModuleTable; // table for depends system kernel modules
} KModuleHandler = {NULL};


//--------------------------------------------------------------------------------------------------
/**
 * Doubly linked list that stores the modules in alphabetical order of module name.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ModuleAlphaOrderList = LE_DLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Singly linked list that stores the modules involved in a cyclic dependency.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t CyclicDependencyList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Free list of module parameters starting from argv[2]
 */
//--------------------------------------------------------------------------------------------------
static void FreeArgvParams(KModuleObj_t *module)
{
    char **p;

    /* Iterate through remaining parameters and free buffers */
    p = module->argv + 2;
    while (NULL != *p)
    {
        le_mem_Release(*p);
        *p = NULL;
        p++;
    }

    /* Reset number of parameters */
    module->argc = 2;
}


//--------------------------------------------------------------------------------------------------
/**
 * Free list of module parameters
 */
//--------------------------------------------------------------------------------------------------
static void ModuleFreeParams(KModuleObj_t *module)
{
    module->argv[0] = NULL; // Contained exec'ed command, not allocated
    module->argv[1] = NULL; // Contained module path/name, not allocated

    FreeArgvParams(module);

    /* Reset number of parameters */
    module->argc = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Build and execute the command passed.
 * filedes is not NULL if output of the command (e.g. modinfo) needs to be captured.
 * filedes is NULL if output of the command (e.g. insmod, rmmod, modprobe) need not be captured.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExecuteCommand(char *argv[], int argc, int *filedes)
{
    pid_t pid, p;
    int result;

    /* First argument argv[0] is always the command */
    LE_FATAL_IF(argv[0] == NULL,
                "Internal error: command name must be supplied to execute command.");
    LE_FATAL_IF(argv[1] == NULL,
                "Internal error: execute command expects at least one parameter.");

    /*  execv() is valid only if the array of pointers is terminated by a NULL pointer */
    if (argv[argc] != NULL)
    {
        LE_ERROR("Internal error: command '%s %s' must be terminated by NULL", argv[0], argv[1]);
        return LE_FAULT;
    }

    /* Concatenate all arguments into one string for logging purpose. */
    char logStr[LE_CFG_STR_LEN_BYTES] = "", *pLog = logStr;
    const char dots[] = "...";
    size_t logSpaceLeft = pLog - logStr - (sizeof(dots) - 1);
    int count;

    for (count = 0; count < argc; count++)
    {
        size_t printSize = snprintf(pLog, logSpaceLeft, "%s ", argv[count]);

        if (printSize >= logSpaceLeft) {
            snprintf(pLog, logSpaceLeft, "%s", dots);
            break;
        }

        pLog += printSize;
        logSpaceLeft -= printSize;
    }

    LE_INFO("Execute '%s'", logStr);

    pid = fork();
    LE_FATAL_IF(-1 == pid, "fork() failed. (%m)");

    if (0 == pid)
    {
        /* If file descriptor is provided, output of child process needs to be captured. */
        if (filedes != NULL)
        {
            int ret = dup2(filedes[1], STDOUT_FILENO);
            if (ret == -1)
            {
                LE_ERROR("Internal error: cannot duplicate a file descriptor (%m)");
                return LE_FAULT;
            }

            fd_Close(filedes[1]);
            fd_Close(filedes[0]);
        }

        /* Child, exec command. */
        execv(argv[0], argv);
        /* Should never be here. */
        LE_FATAL("Failed to run '%s %s'. Reason: (%d), %m", argv[0], argv[1], errno);
    }

    /* Wait for command to complete; restart on EINTR. */
    while (-1 == (p = waitpid(pid, &result, 0)) && EINTR == errno);
    if (p != pid)
    {
        if (p == -1)
        {
            LE_FATAL("waitpid() failed: %m");
            return LE_FAULT;
        }
        else
        {
            LE_FATAL("waitpid() returned unexpected result %d", p);
            return LE_FAULT;
        }
    }

    /* Check exit status and errors */
    if (WIFSIGNALED(result))
    {
        LE_CRIT("%s was killed by a signal %d.", logStr, WTERMSIG(result));
        return LE_FAULT;
    }
    else if (WIFEXITED(result) && WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        LE_CRIT("%s exited with error code %d.", logStr, WEXITSTATUS(result));
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Strip extension ".ko" from module name
 */
//--------------------------------------------------------------------------------------------------
static char *StripExtensionName(char *name)
{
    char *stripExtName = le_mem_ForceAlloc(KModuleHandler.reqModStringPool);
    memset(stripExtName, 0, LE_CFG_STR_LEN_BYTES);
    int ind = 0;

    while(name[ind] != '.')
    {
        stripExtName[ind] = name[ind];
        ind++;
    }
    stripExtName[ind] = '\0';
    return stripExtName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check proc/modules for a given module
 */
//--------------------------------------------------------------------------------------------------
static ProcModules_t CheckProcModules(char *modName)
{
    FILE* fPtr;
    char line[500];
    int size;
    char usedbyName[200];
    char modStatus[10];
    char *stripExtModName;
    char scanModName[LE_CFG_STR_LEN_BYTES];
    bool foundModule = false;
    int rc = 0;

    ProcModules_t  procModules;
    procModules.usedbyNumMod = 0;
    procModules.loadStatus = STATUS_INIT;

    fPtr = fopen("/proc/modules", "r");
    if (fPtr == NULL)
    {
        LE_CRIT("Error in opening file /proc/modules");
        return procModules;
    }

    stripExtModName = StripExtensionName(modName);

    /* Scan each line of /proc/modules for matching module name, its load status and module usage.
     * There are 3 possible module load status: Live, Loading, Unloading.
     */
    while (fgets(line, sizeof(line), fPtr))
    {
        if (line[0] == '\0')
        {
            break;
        }

        rc = sscanf(line, "%511s %d %d %199s %9s",
                    scanModName, &size, &procModules.usedbyNumMod, usedbyName, modStatus);
        if (rc == EOF)
        {
            LE_ERROR("Error in scanning /proc/modules '%s'", line);
            break;
        }

        if (strcmp(scanModName,stripExtModName) == 0)
        {
            foundModule = true;
            if (strcmp(modStatus, "Live") == 0)
            {
                procModules.loadStatus = STATUS_INSTALLED;
            }
            else
            {
                procModules.loadStatus = STATUS_TRY_INSTALL;
            }
            break;
        }
    }

    if (foundModule == false)
    {
        procModules.loadStatus = STATUS_REMOVED;
    }

    le_mem_Release(stripExtModName);
    fclose(fPtr);

    return procModules;
}


//--------------------------------------------------------------------------------------------------
/**
 * Helper utility function for detecting cycle in kernel module dependencies
 *
 * A cycle exists if a back edge (edge from a module to itself or one of its ancestor) is found in
 * the graph during the depth first search traversal. Keep track of the visited modules and the
 * recursion stack. If a module is reached that is already in the recursion stack then a cycle is
 * found.
 */
//--------------------------------------------------------------------------------------------------
static bool hasCyclicDependencyUtil (char* modName)
{
    KModuleObj_t *modPtr;
    KModuleObj_t *reqModPtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;

    modPtr = le_hashmap_Get(KModuleHandler.moduleTable, modName);
    if (modPtr == NULL)
    {
        LE_ERROR("Lookup for module '%s' failed.", modName);
        return LE_NOT_FOUND;
    }

    if (modPtr->visited == false)
    {
        modPtr->visited = true;
        modPtr->recurStack = true;

        /* Traverse through the module dependencies of modName */
        modNameLinkPtr = le_sls_Peek(&(modPtr->reqModuleName));
        while (modNameLinkPtr != NULL)
        {
            modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t, link);

            reqModPtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
            if (reqModPtr == NULL)
            {
                LE_ERROR("Lookup for module '%s' failed.", modNameNodePtr->modName);
                return LE_NOT_FOUND;
            }

            /*
             * If the module is not visited, keep traversing through the module dependencies
             * If the module is in recursion stack, cycle is found
             */
            if (((!reqModPtr->visited) && hasCyclicDependencyUtil(reqModPtr->name))
                 || (reqModPtr->recurStack))
            {
                /* Cyclic dependency found */
                if (!le_sls_IsInList(&CyclicDependencyList, &(reqModPtr->cyclicDepLink)))
                {
                    le_sls_Queue(&CyclicDependencyList, &(reqModPtr->cyclicDepLink));
                }

                return true;
            }
            modNameLinkPtr = le_sls_PeekNext(&(modPtr->reqModuleName), modNameLinkPtr);
        }
    }

    /* Remove from recursion stack */
    modPtr->recurStack = false;

    /* Cyclic dependency not found */
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to detect cyclic kernel module dependencies.
 *
 * Use depth first traversal to iterate through the kernel modules and its dependencies. Call the
 * helper utility function hasCyclicDependencyUtil(). If a cycle is found, print the error message
 * with the list of modules involved in the cycle.
 */
//--------------------------------------------------------------------------------------------------
static bool hasCyclicDependency()
{
    le_dls_Link_t *linkPtr;
    KModuleObj_t *modPtr;
    le_sls_Link_t *depLinkPtr;
    KModuleObj_t *depModPtr;
    char *depMod;

    /* Iterate modules list and perform depth first search traversal */
    linkPtr = le_dls_Peek(&ModuleAlphaOrderList);
    while (linkPtr != NULL)
    {
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, alphabeticalLink);
        LE_ASSERT(modPtr != NULL);

        if (hasCyclicDependencyUtil(modPtr->name))
        {
            LE_ERROR("Circular dependency found in kernel modules:");
            depLinkPtr = le_sls_Peek(&CyclicDependencyList);
            if (depLinkPtr == NULL)
            {
                LE_ERROR("CyclicDependencyList is empty");
                return true;
            }

            depModPtr = CONTAINER_OF(depLinkPtr, KModuleObj_t, cyclicDepLink);
            depMod = depModPtr->name;

            /* Print the list of modules involved in cycle */
            while (depLinkPtr != NULL)
            {
                depModPtr = CONTAINER_OF(depLinkPtr, KModuleObj_t, cyclicDepLink);
                depModPtr->isCyclicDependency = true;
                LE_ERROR("%s ->", depModPtr->name);
                depLinkPtr = le_sls_PeekNext(&CyclicDependencyList, depLinkPtr);
            }
            LE_ERROR("%s", depMod);
            return true;
        }

        linkPtr = le_dls_PeekNext(&ModuleAlphaOrderList, linkPtr);
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the load section to determine if the module is auto or manual load
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetLoad(KModuleObj_t *module)
{
    char cfgTreePath[LE_CFG_STR_LEN_BYTES] = {0};

    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "loadManual", (char*)NULL));

    module->isLoadManual = le_cfg_QuickGetBool(cfgTreePath, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the "[optional]" tag to determine if the module is required or optional
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetIsOptional(KModuleObj_t *module)
{
    char cfgTreePath[LE_CFG_STR_LEN_BYTES] = {0};

    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "isOptional", (char*)NULL));

    module->isOptional = le_cfg_QuickGetBool(cfgTreePath, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Populate list of module parameters for argv
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetParams(KModuleObj_t *module)
{
    char *p;
    char cfgTreePath[LE_CFG_STR_LEN_BYTES];
    char tmp[LE_CFG_STR_LEN_BYTES];
    le_cfg_IteratorRef_t iter;

    cfgTreePath[0] = '\0';
    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "params", NULL));
    iter = le_cfg_CreateReadTxn(cfgTreePath);

    if (LE_OK != le_cfg_GoToFirstChild(iter))
    {
        LE_INFO("Module %s uses no parameters.", module->name);
        le_cfg_CancelTxn(iter);
        return;
    }

    /* Populate parameters list from configTree; careful not to overrun array */
    do
    {
        /* allocate and clear string buffer */
        p = (char*)le_mem_ForceAlloc(KModuleHandler.stringPool);
        memset(p, 0, STRINGS_MAX_BUFFER_SIZE);
        module->argv[module->argc] = p;

        /* first get the parameter name, append a '=' and advance to end */
        LE_ASSERT_OK(le_cfg_GetNodeName(iter, "", p, LE_CFG_NAME_LEN_BYTES));

        p += strlen(p);
        *p = '=';
        p++;
        *p = '\0';

        /* now get the parameter value, should be string */
        LE_ASSERT_OK(le_cfg_GetString(iter, "", tmp, LE_CFG_STR_LEN_BYTES, ""));

        /* enclose the parameter in quotes if it contains white space */
        if (strpbrk(tmp, " \t\n"))
        {
            /* add space for quotes; likely ok to use sprintf, but anyway... */
            snprintf(p, LE_CFG_STR_LEN_BYTES + 2, "\"%s\"", tmp);
        }
        else
        {
            LE_ASSERT_OK(le_utf8_Copy(p, tmp, LE_CFG_STR_LEN_BYTES, NULL));
        }

        /* increment parameter counter */
        module->argc++;
    }
    while((KMODULE_MAX_ARGC > (module->argc + 1)) &&
          (LE_OK == le_cfg_GoToNextSibling(iter)));

    le_cfg_CancelTxn(iter);

    /* Last argument to execv must be null */
    module->argv[module->argc] = NULL;

    if (KMODULE_MAX_ARGC <= module->argc + 1)
        LE_WARN("Parameters list truncated for module '%s'", module->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Populate list of required kernel modules that a given module depends on
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetRequiredModules(KModuleObj_t *module)
{
    ModNameNode_t* modNameNodePtr;
    char cfgTreePath[LE_CFG_STR_LEN_BYTES];

    cfgTreePath[0] = '\0';
    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "requires/kernelModules", NULL));

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);
    if (le_cfg_GoToFirstChild(iter) != LE_OK)
    {
        goto done_deps;
    }

    do
    {
        modNameNodePtr = le_mem_ForceAlloc(KModuleHandler.reqModStringPool);
        modNameNodePtr->link = LE_SLS_LINK_INIT;

        le_cfg_GetNodeName(iter, "", modNameNodePtr->modName, sizeof(modNameNodePtr->modName));

        if (strncmp(modNameNodePtr->modName, "", sizeof(modNameNodePtr->modName)) == 0)
        {
            LE_WARN("Found empty kernel module dependency for '%s'", module->name);
            le_mem_Release(modNameNodePtr);
            continue;
        }

        modNameNodePtr->isOptional = le_cfg_GetBool(iter, "isOptional", false);

        le_sls_Queue(&(module->reqModuleName), &(modNameNodePtr->link));
    }
    while(le_cfg_GoToNextSibling(iter) == LE_OK);

done_deps:
    le_cfg_CancelTxn(iter);
    module->moduleLoadStatus = STATUS_INIT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Populate the module install script path from config tree
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetInstallScript(KModuleObj_t *module)
{
    char cfgTreePath[LE_CFG_STR_LEN_BYTES];
    char installScriptPath[LIMIT_MAX_PATH_BYTES];
    char *installScriptName;
    char *stripExtName;

    cfgTreePath[0] = '\0';
    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "scripts/install", NULL));

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);

    if (le_cfg_GetNodeType(iter, ".") != LE_CFG_TYPE_EMPTY)
    {
        LE_DEBUG("Found empty intall script node");
        le_cfg_CancelTxn(iter);
        return;
    }

    if (le_cfg_GetNodeType(iter, ".") != LE_CFG_TYPE_STRING)
    {
        LE_WARN("Found non-string type scripts");
        le_cfg_CancelTxn(iter);
        return;
    }

    le_cfg_GetString(iter, "", installScriptPath, sizeof(installScriptPath), "");
    if (strncmp(installScriptPath, "", sizeof(installScriptPath)) == 0)
    {
        LE_DEBUG("Found empty install script");
        le_cfg_CancelTxn(iter);
        return;
    }

    installScriptName = le_path_GetBasenamePtr(installScriptPath, "/");

    stripExtName = StripExtensionName(module->name);
    LE_ASSERT_OK(le_path_Concat("/",
                            module->installScript,
                            LIMIT_MAX_PATH_BYTES,
                            SYSTEM_MODULE_FILES_PATH,
                            stripExtName,
                            "scripts",
                            installScriptName,
                            NULL));
    le_mem_Release(stripExtName);

    le_cfg_CancelTxn(iter);
}


//--------------------------------------------------------------------------------------------------
/**
 * Populate the module remove script path from config tree
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetRemoveScript(KModuleObj_t *module)
{
    char cfgTreePath[LE_CFG_STR_LEN_BYTES];
    char removeScriptPath[LIMIT_MAX_PATH_BYTES];
    char *removeScriptName;
    char *stripExtName;

    cfgTreePath[0] = '\0';
    LE_ASSERT_OK(le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                 KMODULE_CONFIG_TREE_ROOT, module->name, "scripts/remove", NULL));

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);

    if (le_cfg_GetNodeType(iter, ".") != LE_CFG_TYPE_EMPTY)
    {
        LE_DEBUG("Found empty remove script node");
        le_cfg_CancelTxn(iter);
        return;
    }

    if (le_cfg_GetNodeType(iter, ".") != LE_CFG_TYPE_STRING)
    {
        LE_WARN("Found non-string type scripts");
        le_cfg_CancelTxn(iter);
        return;
    }

    le_cfg_GetString(iter, "", removeScriptPath, sizeof(removeScriptPath), "");
    if (strncmp(removeScriptPath, "", sizeof(removeScriptPath)) == 0)
    {
        LE_DEBUG("Found empty remove script");
        le_cfg_CancelTxn(iter);
        return;
    }

    removeScriptName = le_path_GetBasenamePtr(removeScriptPath, "/");
    stripExtName = StripExtensionName(module->name);

    LE_ASSERT_OK(le_path_Concat("/",
                            module->removeScript,
                            LIMIT_MAX_PATH_BYTES,
                            SYSTEM_MODULE_FILES_PATH,
                            stripExtName,
                            "scripts",
                            removeScriptName,
                            NULL));

    le_mem_Release(stripExtName);
    le_cfg_CancelTxn(iter);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get system dependency modules for a given kernel module. This is done by parsing the 'depends'
 * field from the output of modinfo.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ModuleGetDependsInfo(KModuleObj_t *mod)
{
    /* File descriptor for entrance to the pipe is written to filedes[1] and exit to filedes [0]. */
    int filedes[2];

    if (pipe(filedes) == -1)
    {
        LE_FATAL("Internal error: pipe() failed! (%m)");
        return LE_FAULT;
    }

    char *depargv[] = {"/sbin/modinfo",  "-F", "depends", mod->path, NULL};

    /* Execute command "/sbin/modinfo -F depends <mod->path>" to get system dependency modules. */
    ExecuteCommand(depargv, ARRAY_LENGTH(depargv)-1, filedes);

    /* Close the entrance to the pipe within the parent process */
    fd_Close(filedes[1]);

    /* Buffer to store the output of modinfo */
    char modinfoOutput[MODINFO_MAX_BUFFER_LEN] = "";
    while (1)
    {
        /* Read the output of the child process from file descriptor filedes[0] */
        ssize_t count = read(filedes[0], modinfoOutput, sizeof(modinfoOutput));
        if (count >= (MODINFO_MAX_BUFFER_LEN-1))
        {
            LE_FATAL("Internal error: Overflow! (%m)");
        }

        if (count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                LE_FATAL("Internal error: read() failed! (%m)");
                return LE_FAULT;
            }
        }
        else if (count == 0)
        {
            break;
        }
    }

    fd_Close(filedes[0]);

    /* System modules are separated by ',' or '\n' in the output of modinfo. */
    char* modinfoOutputPtr = modinfoOutput;
    char* token;
    int count = 0;
    DepModNameNode_t* depModNameNodePtr;

    while ((token = strtok_r(modinfoOutputPtr, ",\n", &modinfoOutputPtr)))
    {
        count++;
        depModNameNodePtr = le_mem_ForceAlloc(KModuleHandler.depModStringPool);
        depModNameNodePtr->link = LE_SLS_LINK_INIT;
        depModNameNodePtr->useCount = 0;

        const char koExt[] = ".ko";

        if (le_utf8_Copy(depModNameNodePtr->modName,
                        token,
                        LE_CFG_STR_LEN_BYTES - strlen(koExt),
                        NULL) != LE_OK)
        {
            LE_ERROR("Internal error: size of token is greater than module name string buffer.");
            return LE_OVERFLOW;
        }

        /* Concatenate ".ko" to the end of the module name as the legato modules stored in the hash
         * map are suffixed by ".ko". The module name is used to search through the hash map.
         */
        le_utf8_Append(depModNameNodePtr->modName,
                       koExt,
                       LE_CFG_STR_LEN_BYTES,
                       NULL);

        /* Depend modules might also include legato externel kernel modules. These modules cannot
         * cannot be installed using modprobe and is handled separtely using insmod/rmmod. Skip
         * these modules from adding to dependsModuleName list.
         */
        if (!le_sls_IsInList(&(mod->dependsModuleName), &(depModNameNodePtr->link)) &&
            !le_hashmap_ContainsKey(KModuleHandler.moduleTable, depModNameNodePtr->modName))
        {
            /* Add to the queue of dependency modules for this Legato kernel module. */
            le_sls_Queue(&(mod->dependsModuleName), &(depModNameNodePtr->link));

            /* Add to the hashmap to keep track of the usage of a particular system module. */
            le_hashmap_Put(KModuleHandler.dependModuleTable, depModNameNodePtr->modName,
                           depModNameNodePtr);
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Insert a module to the table with a given module name
 */
//--------------------------------------------------------------------------------------------------
static void ModuleInsert(char *modName)
{
    KModuleObj_t *m;

    /* Allocate module object */
    m = (KModuleObj_t*)le_mem_ForceAlloc(KModuleHandler.modulePool);
    memset(m, 0, sizeof(KModuleObj_t));

    LE_ASSERT_OK(le_path_Concat("/",
                            m->path,
                            LIMIT_MAX_PATH_BYTES,
                            SYSTEM_MODULE_PATH,
                            modName,
                            NULL));

    m->cookie = KMODULE_OBJECT_COOKIE;
    m->name = le_path_GetBasenamePtr(m->path, "/");

    /* Now build a parameter list that will be sent to execv */
    m->argv[0] = NULL;      /* First: reserved for execv command */
    m->argv[1] = m->path;   /* Second: file/module path */
    m->argc = 2;
    m->reqModuleName = LE_SLS_LIST_INIT;
    m->dependsModuleName = LE_SLS_LIST_INIT;
    m->useCount = 0;
    m->isLoadManual = false;
    m->isOptional = false;
    m->dependencyLink = LE_DLS_LINK_INIT;
    m->alphabeticalLink = LE_DLS_LINK_INIT;
    m->isRequiredModule = false;
    m->isCyclicDependency = false;
    m->visited = false;
    m->recurStack = false;

    ModuleGetLoad(m);            /* Read load from configTree */
    ModuleGetIsOptional(m);      /* Read if the module is optional from configTree */
    ModuleGetParams(m);          /* Read module parameters from configTree */
    ModuleGetRequiredModules(m); /* Read required kernel modules from configTree */
    ModuleGetInstallScript(m);   /* Read the install script path from configTree */
    ModuleGetRemoveScript(m);    /* Read the remove script path from configTree */
    ModuleGetDependsInfo(m);     /* Read the depends system kernel module from modinfo */

    /* Insert modules in alphabetical order of module name in a doubly linked list */
    le_dls_Queue(&ModuleAlphaOrderList, &(m->alphabeticalLink));

    /* Insert in a hashMap */
    le_hashmap_Put(KModuleHandler.moduleTable, m->name, m);
}


//--------------------------------------------------------------------------------------------------
/**
 * For insertion, traverse through the module table and add modules with dependencies to Stack list
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TraverseDependencyInsert
(
    le_dls_List_t* ModuleInsertList,
    KModuleObj_t *m,
    bool enableUseCount
)
{
    LE_ASSERT(m != NULL);

    KModuleObj_t* KModulePtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;
    le_result_t result;

    if (enableUseCount)
    {
        /* Increment the usage count of the module */
        m->useCount++;
    }

    /* Return if the module is involved in cylic dependency */
    if (m->isCyclicDependency)
    {
        LE_ERROR("Module '%s' involved in cyclic dependency", m->name);
        return LE_FAULT;
    }

    /*
     * We must not add duplicate objects to the linked list to avoid undesired loops.
     * If the object is already in ModuleInsertList, remove it and add it to the top of the stack.
     */
    if (le_dls_IsInList(ModuleInsertList, &(m->dependencyLink)))
    {
       le_dls_Remove(ModuleInsertList, &(m->dependencyLink));
    }

    le_dls_Stack(ModuleInsertList, &(m->dependencyLink));

    if (m->moduleLoadStatus != STATUS_INSTALLED)
    {
       m->moduleLoadStatus = STATUS_TRY_INSTALL;
    }

    modNameLinkPtr = le_sls_Peek(&(m->reqModuleName));

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);

        KModulePtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
        if (KModulePtr == NULL)
        {
            LE_ERROR("Lookup for module '%s' failed.", modNameNodePtr->modName);
            return LE_FAULT;
        }

        /* Get the isOptional value of the module from the reqModuleName list instead */
        KModulePtr->isOptional = modNameNodePtr->isOptional;

        result =  TraverseDependencyInsert(ModuleInsertList, KModulePtr, enableUseCount);
        if (result != LE_OK)
        {
            return result;
        }

        modNameLinkPtr = le_sls_PeekNext(&(m->reqModuleName), modNameLinkPtr);
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Install each kernel module.
 * modprobe the system dependency module and insmod the Legato kernel module.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InstallEachKernelModule(KModuleObj_t *m, bool enableUseCount)
{
    le_result_t result;
    le_dls_Link_t *listLink;
    /* The ordered list of required kernel modules to install */
    le_dls_List_t ModuleInsertList = LE_DLS_LIST_INIT;
    ProcModules_t procModules;

    result = TraverseDependencyInsert(&ModuleInsertList, m, enableUseCount);
    if (result != LE_OK)
    {
        /* If the module is marked optional, ignore fault, otherwise take fault action. */
        if (m->isOptional)
        {
            LE_WARN("Traversing module '%s' dependencies failed, ignore as module is optional",
                    m->name);
            return LE_OK;
        }
        else
        {
            LE_ERROR("Traversing module '%s' dependencies failed, fault action will be taken",
                     m->name);
            return result;
        }
    }

    while ((listLink = le_dls_Pop(&ModuleInsertList)) != NULL)
    {
        KModuleObj_t *mod = CONTAINER_OF(listLink, KModuleObj_t, dependencyLink);

        if (mod->moduleLoadStatus != STATUS_INSTALLED)
        {
            le_sls_Link_t *depModNameLinkPtr = le_sls_Peek(&(mod->dependsModuleName));

            while (depModNameLinkPtr != NULL)
            {
                /* Install dependency system modules if any before installing the Legato module */
                DepModNameNode_t* depModNameNodePtr = CONTAINER_OF(depModNameLinkPtr,
                                                                   DepModNameNode_t, link);
                char *depargv[] = {MODPROBE_COMMAND, depModNameNodePtr->modName, NULL};

                result = ExecuteCommand(depargv, ARRAY_LENGTH(depargv)-1, NULL);
                if (result != LE_OK)
                {
                    LE_CRIT("Command '%s' '%s' execution failed.", depargv[0], depargv[1]);
                    return result;
                }

                DepModNameNode_t *depModPtr = le_hashmap_Get(KModuleHandler.dependModuleTable,
                                                             depModNameNodePtr->modName);
                if (depModPtr == NULL)
                {
                    LE_ERROR("Lookup for module '%s' failed.", depModNameNodePtr->modName);
                    return LE_NOT_FOUND;
                }

                depModPtr->useCount++;
                depModNameLinkPtr = le_sls_PeekNext(&(mod->dependsModuleName), depModNameLinkPtr);
            }

            /* If install script is provided, execute the script otherwise execute insmod */
            if (strcmp(mod->installScript, "") != 0)
            {
                char *scriptargv[] = {mod->installScript, mod->path, NULL};

                result = ExecuteCommand(scriptargv, ARRAY_LENGTH(scriptargv)-1, NULL);
                if (result != LE_OK)
                {
                    LE_CRIT("Install script '%s' execution failed", mod->installScript);

                    if (mod->isOptional)
                    {
                        continue;
                    }
                    return result;
                }

                /* Read module load status from /proc/modules */
                procModules =  CheckProcModules(mod->name);

                if (procModules.loadStatus != STATUS_INSTALLED)
                {
                    LE_INFO("Module '%s' not in 'Live' state, wait for 10 seconds.", mod->name);
                    sleep(10);

                    /* If the module is not in live state, wait for 10 seconds to see if the
                     * module recovers to live state, otherwise restart the system.
                     */
                    if (procModules.loadStatus != STATUS_INSTALLED)
                    {
                        if (mod->isOptional)
                        {
                            LE_INFO(
                                "Module '%s' not in 'Live' state and is optional. "
                                "Skip restarting system.",
                                mod->name);
                            continue;
                        }

                        LE_CRIT("Module '%s' not in 'Live' state. Restart system ...", mod->name);
                        return LE_FAULT;
                    }
                }
            }
            else
            {
                mod->argv[0] = INSMOD_COMMAND;

                result = ExecuteCommand(mod->argv, mod->argc, NULL);
                if (result != LE_OK)
                {
                    if (mod->isOptional)
                    {
                        LE_INFO("Ignoring failure. "
                                 "Module '%s' failed to load and is an optional module.", mod->name);
                        continue;
                    }
                    return result;
                }
            }

            mod->moduleLoadStatus = STATUS_INSTALLED;
            LE_INFO("New kernel module '%s'", mod->name);
        }
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse through the given list of kernel module names and install each module
 */
//--------------------------------------------------------------------------------------------------
le_result_t kernelModules_InsertListOfModules(le_sls_List_t reqModuleName)
{
    KModuleObj_t* m;
    le_result_t result;
    ModNameNode_t* modNameNodePtr;

    le_sls_Link_t* modNameLinkPtr = le_sls_Peek(&reqModuleName);

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);

        m = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
        LE_ASSERT(m && KMODULE_OBJECT_COOKIE == m->cookie);

        /* Get the isOptional value of the module from reqModuleName list instead */
        m->isOptional = modNameNodePtr->isOptional;

        /* Install only if the module is set to manual load and not a dependency module */
        if (m->isLoadManual)
        {
            result = InstallEachKernelModule(m, true);
            if (result != LE_OK)
            {
                LE_ERROR("Error in installing module '%s'.", m->name);
                return LE_FAULT;
            }
        }

        modNameLinkPtr = le_sls_PeekNext(&reqModuleName, modNameLinkPtr);
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse through the given list of kernel module and mark the ones which are required.
 * If 'x.ko' depends on 'y.ko' then isRequiredModule is set true for 'y.ko'.
 */
//--------------------------------------------------------------------------------------------------
static void setIsRequiredModule()
{
    le_dls_Link_t* linkPtr;
    KModuleObj_t *modPtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;
    KModuleObj_t *KModulePtr;

    linkPtr = le_dls_Peek(&ModuleAlphaOrderList);
    while (linkPtr != NULL)
    {
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, alphabeticalLink);
        LE_ASSERT(modPtr != NULL);

        modNameLinkPtr = le_sls_Peek(&(modPtr->reqModuleName));

        while (modNameLinkPtr != NULL)
        {
            modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);
            LE_ASSERT(modNameNodePtr != NULL);
            KModulePtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
            if (KModulePtr == NULL)
            {
                LE_ERROR("Lookup for module '%s' failed.", modNameNodePtr->modName);
                return;
            }

            KModulePtr->isRequiredModule = true;

            modNameLinkPtr = le_sls_PeekNext(&(modPtr->reqModuleName), modNameLinkPtr);
        }

        linkPtr = le_dls_PeekNext(&ModuleAlphaOrderList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Iterate through the module table and install kernel module
 */
//--------------------------------------------------------------------------------------------------
static void installModules()
{
    KModuleObj_t *modPtr;
    le_result_t result;
    le_dls_Link_t* linkPtr;

    /* Traverse linked list in alphabetical order of module name and traverse dependencies. */
    linkPtr = le_dls_Peek(&ModuleAlphaOrderList);
    while (linkPtr != NULL)
    {
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, alphabeticalLink);
        LE_ASSERT(modPtr != NULL);

        /*
         * Skip if the modules are loaded manually via app or if it is a required module.
         * If the module is load manual, it will be loaded when app starts.
         * If the module is a required module, it will be loaded with its parent module.
         */
        if (modPtr->isLoadManual)
        {
            linkPtr = le_dls_PeekNext(&ModuleAlphaOrderList, linkPtr);
            continue;
        }

        result = InstallEachKernelModule(modPtr, true);
        if (result != LE_OK)
        {
            LE_ERROR("Error in installing module %s. Restarting system ...", modPtr->name);
            framework_Reboot();
            break;
        }

        linkPtr = le_dls_PeekNext(&ModuleAlphaOrderList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse modules configTree (system:/modules) and insmod all modules in the order of dependencies
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Insert(void)
{
    char modName[LE_CFG_STR_LEN_BYTES];

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn("system:");
    le_cfg_GoToNode(iter, "/modules");

    le_result_t result = le_cfg_GoToFirstChild(iter);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to read /modules config. Result = %d (%s).",
                 result, LE_RESULT_TXT(result));
        le_cfg_CancelTxn(iter);
        return;
    }

    do
    {
        le_cfg_GetNodeName(iter, "", modName, sizeof(modName));

        if (strncmp(modName, "", LE_CFG_STR_LEN_BYTES) == 0)
        {
            LE_WARN("Found empty kernel module node");
            continue;
        }

        /* Check for valid kernel module file extension ".ko" */
        if (le_path_FindTrailing(modName, KERNEL_MODULE_FILE_EXTENSION) != NULL)
        {
            ModuleInsert(modName);
        }
    }
    while (le_cfg_GoToNextSibling(iter) == LE_OK);

    le_cfg_CancelTxn(iter);

    /* Check for any cyclic dependency before installing modules */
    if (hasCyclicDependency())
    {
        LE_ERROR("Modules involved in circular dependency will not be installed.");
    }

    setIsRequiredModule();

    installModules();
}


//--------------------------------------------------------------------------------------------------
/**
 * Release memory taken by kernel modules
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseModulesMemory(void)
{
    KModuleObj_t *modPtr;
    DepModNameNode_t *depModPtr;
    le_hashmap_It_Ref_t modIter;
    le_hashmap_It_Ref_t depModIter;

    LE_INFO("Releasing kernel modules memory");

    modIter = le_hashmap_GetIterator(KModuleHandler.moduleTable);

    /* Iterate through the kernel module table */
    while (le_hashmap_NextNode(modIter) == LE_OK)
    {
        modPtr = (KModuleObj_t*) le_hashmap_GetValue(modIter);
        LE_ASSERT(modPtr && KMODULE_OBJECT_COOKIE == modPtr->cookie);

        /* Reset exec arguments */
        ModuleFreeParams(modPtr);
        LE_ASSERT(modPtr == le_hashmap_Remove(KModuleHandler.moduleTable, modPtr->name));
        le_mem_Release(modPtr);
        LE_INFO("Released memory of module '%s'", modPtr->name);
    }

    depModIter = le_hashmap_GetIterator(KModuleHandler.dependModuleTable);

    /* Iterate through the kernel module table */
    while (le_hashmap_NextNode(depModIter) == LE_OK)
    {
        depModPtr = (DepModNameNode_t*) le_hashmap_GetValue(depModIter);
        LE_ASSERT(depModPtr != NULL);

        LE_ASSERT(depModPtr == le_hashmap_Remove(KModuleHandler.dependModuleTable, depModPtr->modName));
        le_mem_Release(depModPtr);
        LE_INFO("Released memory of module '%s'", depModPtr->modName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * For removal, traverse through the module table and add modules with dependencies to Queue list
 */
//--------------------------------------------------------------------------------------------------
static void TraverseDependencyRemove
(
    le_dls_List_t* ModuleRemoveList,
    KModuleObj_t *m,
    bool enableUseCount
)
{
    LE_ASSERT(m != NULL);

    KModuleObj_t* KModulePtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;

    /* If the module is already removed or in initialization state then return */
    if ((m->moduleLoadStatus == STATUS_REMOVED) || (m->moduleLoadStatus == STATUS_INIT))
    {
        return;
    }

    if (enableUseCount)
    {
        LE_ASSERT(m->useCount != 0);

        /* Keep decrementing useCount. When useCount = 0, safe to remove module */
        m->useCount--;
    }

    /*
     * We must not add duplicate objects to the linked list to avoid undesired loops.
     * If the object is already in the ModuleRemoveList, remove it and add to the end of the queue.
     */
    if (le_dls_IsInList(ModuleRemoveList, &(m->dependencyLink)))
    {
        le_dls_Remove(ModuleRemoveList, &(m->dependencyLink));
    }

    le_dls_Queue(ModuleRemoveList, &(m->dependencyLink));


    if (m->moduleLoadStatus != STATUS_REMOVED)
    {
        if (m->useCount == 0)
        {
            switch (m->moduleLoadStatus)
            {
                case STATUS_TRY_INSTALL:
                case STATUS_INIT:
                    LE_DEBUG("Module '%s' not ready to be removed.", m->name);
                    break;
                default:
                    m->moduleLoadStatus = STATUS_TRY_REMOVE;
                    break;
            }

        }
    }

    modNameLinkPtr = le_sls_Peek(&(m->reqModuleName));

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);

        KModulePtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
        if (KModulePtr == NULL)
        {
            LE_ERROR("Lookup for module '%s' failed.", modNameNodePtr->modName);
            return;
        }

        TraverseDependencyRemove(ModuleRemoveList, KModulePtr, enableUseCount);

        modNameLinkPtr = le_sls_PeekNext(&(m->reqModuleName), modNameLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * rmmod the kernel module
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveEachKernelModule(KModuleObj_t *m, bool enableUseCount)
{
    le_dls_Link_t *listLink;
    le_result_t result;
    /* The ordered list of required kernel modules to remove */
    le_dls_List_t ModuleRemoveList = LE_DLS_LIST_INIT;
    ProcModules_t procModules;

    TraverseDependencyRemove(&ModuleRemoveList, m, enableUseCount);

    while ((listLink = le_dls_Pop(&ModuleRemoveList)) != NULL)
    {
        KModuleObj_t *mod = CONTAINER_OF(listLink, KModuleObj_t, dependencyLink);

        if ((mod->useCount == 0)
             && (mod->moduleLoadStatus == STATUS_TRY_REMOVE))
        {
            /* If remove script is provided then execute the script otherwise execute rmmod */
            if (strcmp(mod->removeScript, "") != 0 )
            {
                char *scriptargv[] = {mod->removeScript, mod->path, NULL};

                result = ExecuteCommand(scriptargv, ARRAY_LENGTH(scriptargv)-1, NULL);
                if (result != LE_OK)
                {
                    LE_CRIT("Remove script '%s' execution failed.", mod->removeScript);
                    return result;
                }

                /* Check if the module is found in /proc/modules. If a module was successfully
                 * removed then it won't show up in /proc/modules.
                 */
                procModules = CheckProcModules(mod->name);

                if (procModules.loadStatus == STATUS_REMOVED)
                {
                    LE_DEBUG("Module '%s' not found in /proc/modules as expected", mod->name);
                }
                else
                {
                    LE_CRIT("Module '%s' found in /proc/modules. Module not removed", mod->name);
                    return LE_FAULT;
                }
            }
            else
            {
                /* Populate argv for rmmod. rmmod does not take any parameters. */
                char *rmmodargv[] = {RMMOD_COMMAND, mod->name, NULL};

                result = ExecuteCommand(rmmodargv, ARRAY_LENGTH(rmmodargv)-1, NULL);
                if (result != LE_OK)
                {
                    return result;
                }
            }
            mod->moduleLoadStatus = STATUS_REMOVED;
            LE_INFO("Removed kernel module '%s'", mod->name);

            /* Traverse system dependency modules and remove them if any. */
            le_sls_Link_t *depModNameLinkPtr = le_sls_Peek(&(mod->dependsModuleName));
            while (depModNameLinkPtr != NULL)
            {
                DepModNameNode_t* depModNameNodePtr = CONTAINER_OF(depModNameLinkPtr,
                                                                   DepModNameNode_t, link);

                DepModNameNode_t *depModPtr = le_hashmap_Get(KModuleHandler.dependModuleTable,
                                                             depModNameNodePtr->modName);
                if (depModPtr == NULL)
                {
                    LE_ERROR("Lookup for module '%s' failed.", depModNameNodePtr->modName);
                    return LE_NOT_FOUND;
                }

                if (depModPtr->useCount > 0)
                {
                    depModPtr->useCount--;
                }

                /* After the use count of a system module is 0, need to check if other modules
                 * outside Legato is using the system module or not. This is checked by reading the
                 * output of /prod/modules and checking the module usage. It is safe to remove the
                 * system module only if no other modules are using the system module.
                 */
                if (depModPtr->useCount == 0)
                {
                    ProcModules_t procModules = CheckProcModules(depModNameNodePtr->modName);

                    if (procModules.usedbyNumMod == 0)
                    {
                        char* depargv[] = {MODPROBE_COMMAND, "-r", depModNameNodePtr->modName,
                                           NULL};

                        result = ExecuteCommand(depargv, ARRAY_LENGTH(depargv)-1, NULL);
                        if (result != LE_OK)
                        {
                            LE_CRIT("Command '%s' '%s' '%s' execution failed.",
                                    depargv[0], depargv[1], depargv[2]);
                            return result;
                        }
                    }
                }
                depModNameLinkPtr = le_sls_PeekNext(&(mod->dependsModuleName), depModNameLinkPtr);
            }
        }
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse through the given list of kernel module names and remove each module
 */
//--------------------------------------------------------------------------------------------------
le_result_t kernelModules_RemoveListOfModules(le_sls_List_t reqModuleName)
{
    KModuleObj_t* m;
    le_result_t result = LE_OK;
    ModNameNode_t* modNameNodePtr;
    le_sls_Link_t* modNameLinkPtr = le_sls_Peek(&reqModuleName);

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);
        m = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
        LE_ASSERT(m && KMODULE_OBJECT_COOKIE == m->cookie);

        /* Remove only if the module is set to manual load */
        if (m->isLoadManual)
        {
            result = RemoveEachKernelModule(m, true);
            if (result != LE_OK)
            {
                LE_ERROR("Error in removing module '%s'", m->name);
                /* If an error occurs removing a module, continue removing others in the list */
                result = LE_FAULT;
            }
        }

        modNameLinkPtr = le_sls_PeekNext(&reqModuleName, modNameLinkPtr);
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove previously inserted modules in the order of dependencies
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Remove(void)
{
    KModuleObj_t *modPtr;
    le_result_t result;
    le_dls_Link_t* linkPtr;

    /* Traverse linked list in reverse alphabetical order of module name and traverse each module
     * dependencies
     */
    while ((linkPtr = le_dls_PopTail(&ModuleAlphaOrderList)) != NULL)
    {
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, alphabeticalLink);
        LE_ASSERT(modPtr != NULL);

        /*
         * Skip if the modules are loaded manually via app
         * If the module is load manual, it will be unloaded when app stops.
         */
        if (modPtr->isLoadManual)
        {
            continue;
        }

        result  = RemoveEachKernelModule(modPtr, true);
        if (result != LE_OK)
        {
            LE_ERROR("Error in removing module '%s'", modPtr->name);

            /* If an error occurs removing a module, continue removing others in the list */
            continue;
        }
    }

    ReleaseModulesMemory();
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize kernel module handler
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Init(void)
{
    // Create memory pool of kernel modules
    KModuleHandler.modulePool = le_mem_CreatePool("Kernel Module Mem Pool",
                                                  sizeof(KModuleObj_t));
    le_mem_ExpandPool(KModuleHandler.modulePool, KMODULE_DEFAULT_POOL_SIZE);

    // Create memory pool of strings for module parameters
    KModuleHandler.stringPool = le_mem_CreatePool("Module Params Mem Pool",
                                                  STRINGS_MAX_BUFFER_SIZE);
    le_mem_ExpandPool(KModuleHandler.stringPool, STRINGS_DEFAULT_POOL_SIZE);

    // Create memory pool of strings for required kernel module names
    KModuleHandler.reqModStringPool = le_mem_CreatePool("Required Module Mem Pool",
                                                        sizeof(ModNameNode_t));
    le_mem_ExpandPool(KModuleHandler.reqModStringPool, STRINGS_DEFAULT_POOL_SIZE);

    // Create memory pool of strings for depends system module names
    KModuleHandler.depModStringPool = le_mem_CreatePool("Depends Module Mem Pool",
                                                        sizeof(DepModNameNode_t));
    le_mem_ExpandPool(KModuleHandler.depModStringPool, STRINGS_DEFAULT_POOL_SIZE);

    // Note that modules.dep file cannot be used for the time being as it requires kernel changes.
    // This option will be investigated in the future. Also, to support backward compatibility of
    // existing targets, module dependency support without kernel changes is a must.

    // Create table of kernel module objects
    KModuleHandler.moduleTable = le_hashmap_Create("KModule Objects",
                                             31,
                                             le_hashmap_HashString,
                                             le_hashmap_EqualsString);

    // Create table of depends kernel module (system kernel module dependency) objects
    KModuleHandler.dependModuleTable = le_hashmap_Create("DepKModule Objects",
                                             31,
                                             le_hashmap_HashString,
                                             le_hashmap_EqualsString);
}


//--------------------------------------------------------------------------------------------------
/**
 * Load the specified kernel module that was bundled with a Legato system.
 *
 * @return
 *   - LE_OK if the module has been successfully loaded into the kernel.
 *   - LE_NOT_FOUND if the named module was not found in the system.
 *   - LE_FAULT if errors were encountered when loading the module, or one of the module's
 *     dependencies.
 *   - LE_DUPLICATE if the module has been already loaded into the kernel.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_kernelModule_Load
(
    const char* moduleName  ///< [IN] Name of the module to load.
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Requested to load module '%s'.", moduleName);

    KModuleObj_t* moduleInfoPtr;
    le_result_t result;
    bool enableUseCount;

    moduleInfoPtr = le_hashmap_Get(KModuleHandler.moduleTable, moduleName);
    if (moduleInfoPtr == NULL)
    {
        LE_ERROR("Lookup for module '%s' failed.", moduleName);
        return LE_NOT_FOUND;
    }

    if (moduleInfoPtr->moduleLoadStatus == STATUS_INSTALLED)
    {
        LE_INFO("Module '%s' is already installed.", moduleInfoPtr->name);
        return LE_DUPLICATE;
    }

    if (moduleInfoPtr->isCyclicDependency)
    {
        LE_INFO(
            "Module '%s' is involved in circular dependency. Cannot install.",
            moduleInfoPtr->name);
        return LE_FAULT;
    }

    /* If a module is loaded manually via app, then no need to enable useCount for kmod API */
    enableUseCount = (moduleInfoPtr->isLoadManual) ? false : true;

    result = InstallEachKernelModule(moduleInfoPtr, enableUseCount);

    if (result == LE_OK)
    {
        LE_INFO("Load module, '%s', was successful.", moduleName);
    }
    else
    {
        LE_ERROR("Load module, '%s', failed.  (%s)", moduleName, LE_RESULT_TXT(result));
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Unload the specified module.  The module to be unloaded must be one that was bundled with the
 * system.
 *
 * @return
 *   - LE_OK if the module has been successfully unloaded from the kernel.
 *   - LE_NOT_FOUND if the named module was not found in the system.
 *   - LE_FAULT if errors were encountered during the module, or one of the module's dependencies
 *     unloading.
 *   - LE_DUPLICATE if the module has been already unloaded from the kernel.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_kernelModule_Unload
(
    const char* moduleName  ///< [IN] Name of the module to unload.
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Requested to unload module '%s'.", moduleName);

    KModuleObj_t* moduleInfoPtr;
    bool enableUseCount;
    le_result_t result;

    moduleInfoPtr = le_hashmap_Get(KModuleHandler.moduleTable, moduleName);
    if (moduleInfoPtr == NULL)
    {
        LE_ERROR("Lookup for module '%s' failed.", moduleName);
        return LE_NOT_FOUND;
    }

    if (moduleInfoPtr->moduleLoadStatus == STATUS_REMOVED)
    {
        LE_INFO("Module '%s' not found. Already removed.", moduleInfoPtr->name);
        return LE_NOT_FOUND;
    }

    if (moduleInfoPtr->isRequiredModule)
    {
        if ((moduleInfoPtr->isLoadManual)
            || (!moduleInfoPtr->isLoadManual && (moduleInfoPtr->useCount > 1)))
        {
            LE_INFO("Module '%s' is a dependency module for another module.", moduleInfoPtr->name);
            return LE_BUSY;
        }
    }

    if ((moduleInfoPtr->isLoadManual) && (moduleInfoPtr->useCount != 0))
    {
        LE_INFO("Module '%s' is a dependency module for an app.", moduleInfoPtr->name);
        return LE_BUSY;
    }

    /* If a module is loaded manually via app then no need to enable useCount for kmod API */
    enableUseCount = (moduleInfoPtr->isLoadManual) ? false : true;

    result = RemoveEachKernelModule(moduleInfoPtr, enableUseCount);

    if (result == LE_OK)
    {
        LE_INFO("Unloading module, '%s', was successful.", moduleName);
    }
    else
    {
        LE_ERROR("Unloading module, '%s', failed.  (%s)", moduleName, LE_RESULT_TXT(result));
    }

    return result;
}
