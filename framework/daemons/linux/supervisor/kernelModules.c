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
#define INSMOD_COMMAND_FORMAT INSMOD_COMMAND" %s %s"


//--------------------------------------------------------------------------------------------------
/**
 * Module remove command and format, argument is module name
 */
//--------------------------------------------------------------------------------------------------
#define RMMOD_COMMAND "/sbin/rmmod"
#define RMMOD_COMMAND_FORMAT RMMOD_COMMAND" %s"


//--------------------------------------------------------------------------------------------------
/**
 * Enum for load status of modules: init, try, installed or removed
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    STATUS_INIT = 0,    ///< Module is in initialization state
    STATUS_TRY,         ///< Try state before installation or removal
    STATUS_INSTALLED,   ///< If insmod is executed on the module
    STATUS_REMOVED      ///< If rmmod is executed on the module
} ModuleLoadStatus_t;


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
    ModuleLoadStatus_t moduleLoadStatus;                     // Load status of the module
    bool               isLoadManual;                         // is module load set to auto or manual
    le_sls_Link_t      link;                                 // link object
    le_dls_Link_t      dlsLink;                              // link object for doubly linked list
    uint32_t           useCount;                             // Counter of usage, safe to remove
                                                             // module when counter is 0
    char               installScript[LIMIT_MAX_PATH_BYTES];  // Path to module install script file
    char               removeScript[LIMIT_MAX_PATH_BYTES];   // Path to module remove script file
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
    le_hashmap_Ref_t    moduleTable;       // table for kernel module objects
} KModuleHandler = {NULL};


//--------------------------------------------------------------------------------------------------
/**
 * Doubly linked list that stores the modules in alphabetical order of module name.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ModuleAlphaOrderList = LE_DLS_LIST_INIT;


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
 * Build and execute the insmod/rmmod command
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExecuteCommand(char *argv[])
{
    pid_t pid, p;
    int result;

    LE_FATAL_IF(argv[0] == NULL,
                "Internal error: command name must be supplied to execute command.");
    LE_FATAL_IF(argv[1] == NULL,
                "Internal error: execute command expects at least one parameter.");

    /* TODO: Check whether the content of last index of argv is a NULL.
     * execv() is valid only if the array of pointers is terminated by a NULL pointer.
     */

    /* First argument argv[0] is always the command */
    LE_INFO("Execute '%s %s'", argv[0], argv[1]);

    pid = fork();
    LE_FATAL_IF(-1 == pid, "fork() failed. (%m)");

    if (0 == pid)
    {
        /* Child, exec command. */
        execv(argv[0], argv);
        /* Should never be here. */
        LE_FATAL("Failed to run '%s %s'. Reason: %m, (%d)", argv[0], argv[1], errno);
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
        LE_CRIT("%s was killed by a signal %d.", argv[0], WTERMSIG(result));
        return LE_FAULT;
    }
    else if (WIFEXITED(result) && WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        LE_CRIT("%s exited with error code %d.", argv[0], WEXITSTATUS(result));
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
static ModuleLoadStatus_t CheckProcModules(char *modName)
{
    FILE* fPtr;
    char line[500];
    int size, usedbyNo;
    char usedbyName[200];
    char modStatus[10];
    char *stripExtModName;
    char scanModName[LE_CFG_STR_LEN_BYTES];
    ModuleLoadStatus_t loadStatus = STATUS_INIT;
    bool foundModule = false;

    fPtr = fopen("/proc/modules", "r");
    if (fPtr == NULL)
    {
        LE_CRIT("Error in opening file /proc/modules");
        return loadStatus;
    }

    stripExtModName = StripExtensionName(modName);

    /* Scan each line of /proc/modules for matching module name and its load status
     * There are 3 possible module load status: Live, Loading, Unloading.
     */
    while (fgets(line, sizeof(line), fPtr))
    {
        sscanf(line,  "%s %d %d %s %s", scanModName, &size, &usedbyNo, usedbyName, modStatus);
        if (strcmp(scanModName,stripExtModName) == 0)
        {
            foundModule = true;
            if (strcmp(modStatus, "Live") == 0)
            {
                loadStatus = STATUS_INSTALLED;
            }
            else
            {
                loadStatus = STATUS_TRY;
            }
            break;
        }
    }

    if (foundModule == false)
    {
        loadStatus = STATUS_REMOVED;
    }

    le_mem_Release(stripExtModName);
    fclose(fPtr);

    return loadStatus;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the load section to determine if the module is auto or manual load
 */
//--------------------------------------------------------------------------------------------------
static void ModuleGetLoad(KModuleObj_t *module)
{
    char cfgTreePath[LE_CFG_STR_LEN_BYTES];
    le_cfg_IteratorRef_t iter;

    cfgTreePath[0] = '\0';
    le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                   KMODULE_CONFIG_TREE_ROOT, module->name, (char*)NULL);
    iter = le_cfg_CreateReadTxn(cfgTreePath);


    module->isLoadManual = le_cfg_GetBool(iter, "loadManual", false);

    le_cfg_CancelTxn(iter);
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
    le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                   KMODULE_CONFIG_TREE_ROOT, module->name, "params", NULL);
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
        p[strlen(p)] = '=';
        p += strlen(p);

        /* now get the parameter value, should be string */
        LE_ASSERT_OK(le_cfg_GetString(iter, "", tmp, LE_CFG_STR_LEN_BYTES, ""));

        /* enclose the parameter in quotes if it contains white space */
        if (strpbrk(tmp, " \t\n"))
        {
            /* add space for quotes; likely ok to use sprintf, but anyway... */
            snprintf(p, LE_CFG_STR_LEN_BYTES + 2, "\"%s\"", tmp);
            p += strlen(tmp) + 2;
        }
        else
        {
            strcpy(p, tmp);
            p += strlen(tmp);
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
    le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                   KMODULE_CONFIG_TREE_ROOT, module->name, "requires/kernelModules", NULL);

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);
    if (le_cfg_GoToFirstChild(iter) != LE_OK)
    {
        goto done_deps;
    }

    do
    {
        if (le_cfg_GetNodeType(iter, ".") != LE_CFG_TYPE_STRING)
        {
            LE_WARN("Found non-string type kernel module dependency");
            continue;
        }

        modNameNodePtr = le_mem_ForceAlloc(KModuleHandler.reqModStringPool);
        modNameNodePtr->link = LE_SLS_LINK_INIT;

        le_cfg_GetString(iter, "", modNameNodePtr->modName, sizeof(modNameNodePtr->modName), "");
        if (strncmp(modNameNodePtr->modName, "", sizeof(modNameNodePtr->modName)) == 0)
        {
            LE_WARN("Found empty kernel module dependency");
            le_mem_Release(modNameNodePtr);
            continue;
        }

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
    le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                   KMODULE_CONFIG_TREE_ROOT, module->name, "scripts/install", NULL);

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);

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
    le_path_Concat("/", cfgTreePath, LE_CFG_STR_LEN_BYTES,
                   KMODULE_CONFIG_TREE_ROOT, module->name, "scripts/remove", NULL);

    le_cfg_IteratorRef_t iter = le_cfg_CreateReadTxn(cfgTreePath);

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
    m->useCount = 0;
    m->isLoadManual = false;

    ModuleGetLoad(m);
    ModuleGetParams(m);          /* Read module parameters from configTree */
    ModuleGetRequiredModules(m); /* Read required kernel modules from configTree */
    ModuleGetInstallScript(m);
    ModuleGetRemoveScript(m);

    /* Insert modules in alphabetical order of module name in a doubly linked list */
    le_dls_Queue(&ModuleAlphaOrderList, &(m->dlsLink));

    /* Insert in a hashMap */
    le_hashmap_Put(KModuleHandler.moduleTable, m->name, m);
}


//--------------------------------------------------------------------------------------------------
/**
 * For insertion, traverse through the module table and add modules with dependencies to Stack list
 */
//--------------------------------------------------------------------------------------------------
static void TraverseDependencyInsert(le_sls_List_t* ModuleInsertList, KModuleObj_t *m)
{
    LE_ASSERT(m != NULL);

    KModuleObj_t* KModulePtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;

    le_sls_Stack(ModuleInsertList, &(m->link));

    if (m->moduleLoadStatus != STATUS_INSTALLED)
    {
       m->moduleLoadStatus = STATUS_TRY;
    }

    modNameLinkPtr = le_sls_Peek(&(m->reqModuleName));

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);
        KModulePtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);

        TraverseDependencyInsert(ModuleInsertList, KModulePtr);

        modNameLinkPtr = le_sls_PeekNext(&(m->reqModuleName), modNameLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * insmod the kernel module
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InstallEachKernelModule(KModuleObj_t *m)
{
    le_result_t result;
    le_sls_Link_t *listLink;
    /* The ordered list of required kernel modules to install */
    le_sls_List_t ModuleInsertList = LE_SLS_LIST_INIT;
    ModuleLoadStatus_t loadStatusProcMod;
    char *scriptargv[3];

    TraverseDependencyInsert(&ModuleInsertList, m);

    for (listLink = le_sls_Pop(&ModuleInsertList);
        listLink != NULL;
        listLink = le_sls_Pop(&ModuleInsertList))
    {
        KModuleObj_t *mod = CONTAINER_OF(listLink, KModuleObj_t, link);

        mod->useCount++;

        if (mod->moduleLoadStatus != STATUS_INSTALLED)
        {
            /* If install script is provided, execute the script otherwise execute insmod */
            if (strcmp(mod->installScript, "") != 0)
            {
                scriptargv[0] =  mod->installScript;
                scriptargv[1] =  mod->path;
                scriptargv[2] =  NULL;

                result = ExecuteCommand(scriptargv);
                if (result != LE_OK)
                {
                    LE_CRIT("Install script %s execution failed", mod->installScript);
                    return result;
                }

                /* Read module load status from /proc/modules */
                loadStatusProcMod =  CheckProcModules(mod->name);
                if (loadStatusProcMod != STATUS_INSTALLED)
                {
                    LE_INFO("Module %s not in 'Live' state, wait for 10 seconds.", mod->name);
                    sleep(10);

                    /* If the module is not in live state, wait for 10 seconds to see if the
                     * module recovers to live state, otherwise restart the system.
                     */
                    if (loadStatusProcMod != STATUS_INSTALLED)
                    {
                        LE_CRIT("Module not in 'Live' state. Restart system ... ");
                        framework_Reboot();
                    }
                }
            }
            else
            {
                mod->argv[0] = INSMOD_COMMAND;
                result = ExecuteCommand(mod->argv);
                if (result != LE_OK)
                {
                    return result;
                }
            }

            mod->moduleLoadStatus = STATUS_INSTALLED;
            LE_INFO("New kernel module %s", mod->name);
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

        if (m->isLoadManual ||
            ((!m->isLoadManual) && (m->moduleLoadStatus != STATUS_INSTALLED)))
        {
            result = InstallEachKernelModule(m);
            if (result != LE_OK)
            {
                LE_ERROR("Error in installing module %s", m->name);
                return LE_FAULT;
            }
        }

        modNameLinkPtr = le_sls_PeekNext(&reqModuleName, modNameLinkPtr);
    }
    return LE_OK;
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
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, dlsLink);
        LE_ASSERT(modPtr != NULL);

        if (modPtr->isLoadManual)
        {
            linkPtr = le_dls_PeekNext(&ModuleAlphaOrderList, linkPtr);
            continue;
        }

        result = InstallEachKernelModule(modPtr);
        if (result != LE_OK)
        {
            LE_ERROR("Error in installing module %s", modPtr->name);
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
    le_cfg_GoToFirstChild(iter);

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

    installModules();
}


//--------------------------------------------------------------------------------------------------
/**
 * Release memory taken by kernel modules
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseModulesMemory(void)
{
    KModuleObj_t *m;
    le_hashmap_It_Ref_t iter;

    LE_INFO("Releasing kernel modules memory");

    iter = le_hashmap_GetIterator(KModuleHandler.moduleTable);

    /* Iterate through the kernel module table */
    while(le_hashmap_NextNode(iter) == LE_OK)
    {
        m = (KModuleObj_t*) le_hashmap_GetValue(iter);
        LE_ASSERT(m && KMODULE_OBJECT_COOKIE == m->cookie);

        /* Reset exec arguments */
        ModuleFreeParams(m);
        LE_ASSERT(m == le_hashmap_Remove(KModuleHandler.moduleTable, m->name));
        le_mem_Release(m);
        LE_INFO("Released memory of module '%s'", m->name);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * For removal, traverse through the module table and add modules with dependencies to Queue list
 */
//--------------------------------------------------------------------------------------------------
static void TraverseDependencyRemove(le_sls_List_t* ModuleRemoveList, KModuleObj_t *m)
{
    LE_ASSERT(m != NULL);

    KModuleObj_t* KModulePtr;
    le_sls_Link_t* modNameLinkPtr;
    ModNameNode_t* modNameNodePtr;

    le_sls_Queue(ModuleRemoveList, &(m->link));

    if (m->moduleLoadStatus != STATUS_REMOVED)
    {
        if ((m->useCount != 0) && (m->moduleLoadStatus == STATUS_INSTALLED))
        {
            LE_DEBUG("Module %s is installed and not yet ready to be removed.", m->name);
        }
        else
        {
            m->moduleLoadStatus = STATUS_TRY;
        }
    }

    modNameLinkPtr = le_sls_Peek(&(m->reqModuleName));

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);
        KModulePtr = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);

        TraverseDependencyRemove(ModuleRemoveList, KModulePtr);

        modNameLinkPtr = le_sls_PeekNext(&(m->reqModuleName), modNameLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * rmmod the kernel module
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveEachKernelModule(KModuleObj_t *m)
{
    le_sls_Link_t *listLink;
    le_result_t result;
    /* The ordered list of required kernel modules to remove */
    le_sls_List_t ModuleRemoveList = LE_SLS_LIST_INIT;
    ModuleLoadStatus_t loadStatusProcMod;
    char *scriptargv[3];
    char *rmmodargv[3];

    TraverseDependencyRemove(&ModuleRemoveList, m);

    for (listLink = le_sls_Pop(&ModuleRemoveList);
        listLink != NULL;
        listLink = le_sls_Pop(&ModuleRemoveList))
    {
        KModuleObj_t *mod = CONTAINER_OF(listLink, KModuleObj_t, link);
        if (mod->useCount != 0)
        {
            /* Keep decrementing useCount. When useCount = 0, safe to remove module */
            mod->useCount--;
        }

        if ((mod->useCount == 0)
             && (mod->moduleLoadStatus != STATUS_REMOVED))
        {
            /* If remove script is provided then execute the script otherwise execute rmmod */
            if (strcmp(mod->removeScript, "") != 0 )
            {
                scriptargv[0] =  mod->removeScript;
                scriptargv[1] =  mod->path;
                scriptargv[2] =  NULL;

                result = ExecuteCommand(scriptargv);
                if (result != LE_OK)
                {
                    LE_CRIT("Remove script %s execution failed.", mod->removeScript);
                    return result;
                }

                /* Check if the module is found in /proc/modules. If a module was successfully
                 * removed then it won't show up in /proc/modules.
                 */
                loadStatusProcMod = CheckProcModules(mod->name);
                if (loadStatusProcMod == STATUS_REMOVED)
                {
                    LE_DEBUG("Module %s not found in /proc/modules as expected", mod->name);
                }
                else
                {
                    LE_CRIT("Module %s found in /proc/modules. Module not removed", mod->name);
                    return LE_FAULT;
                }
            }
            else
            {
                /* Populate argv for rmmod. rmmod does not take any parameters. */
                rmmodargv[0] = RMMOD_COMMAND;
                rmmodargv[1] = mod->name;
                rmmodargv[2] = NULL;

                result = ExecuteCommand(rmmodargv);
                if (result != LE_OK)
                {
                    return result;
                }
            }
            mod->moduleLoadStatus = STATUS_REMOVED;
            LE_INFO("Removed kernel module %s", mod->name);
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
    le_result_t result;
    ModNameNode_t* modNameNodePtr;
    le_sls_Link_t* modNameLinkPtr = le_sls_Peek(&reqModuleName);

    while (modNameLinkPtr != NULL)
    {
        modNameNodePtr = CONTAINER_OF(modNameLinkPtr, ModNameNode_t,link);
        m = le_hashmap_Get(KModuleHandler.moduleTable, modNameNodePtr->modName);
        LE_ASSERT(m && KMODULE_OBJECT_COOKIE == m->cookie);

        if (m->isLoadManual)
        {
            result = RemoveEachKernelModule(m);
            if (result != LE_OK)
            {
                LE_ERROR("Error in removing module %s", m->name);
                return LE_FAULT;
            }
        }

        modNameLinkPtr = le_sls_PeekNext(&reqModuleName, modNameLinkPtr);
    }
    return LE_OK;
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
        modPtr = CONTAINER_OF(linkPtr, KModuleObj_t, dlsLink);
        LE_ASSERT(modPtr != NULL);

        if (modPtr->isLoadManual)
        {
            continue;
        }

        result  = RemoveEachKernelModule(modPtr);
        if (result != LE_OK)
        {
            LE_ERROR("Error in removing module %s", modPtr->name);
            break;
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

    // Note that modules.dep file cannot be used for the time being as it requires kernel changes.
    // This option will be investigated in the future. Also, to support backward compatibility of
    // existing targets, module dependency support without kernel changes is a must.

    // Create table of kernel module objects
    KModuleHandler.moduleTable = le_hashmap_Create("KModule Objects",
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_kernelModule_Load
(
    const char* moduleName  ///< [IN] Name of the module to load.
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Requested to load module '%s'.", moduleName);

    KModuleObj_t* moduleInfoPtr = le_hashmap_Get(KModuleHandler.moduleTable, moduleName);

    if (moduleInfoPtr == NULL)
    {
        LE_ERROR("Lookup for module '%s' failed.", moduleName);
        return LE_NOT_FOUND;
    }

    le_result_t result = InstallEachKernelModule(moduleInfoPtr);

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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_kernelModule_Unload
(
    const char* moduleName  ///< [IN] Name of the module to unload.
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Requested to unload module '%s'.", moduleName);

    KModuleObj_t* moduleInfoPtr = le_hashmap_Get(KModuleHandler.moduleTable, moduleName);

    if (moduleInfoPtr == NULL)
    {
        LE_ERROR("Lookup for module '%s' failed.", moduleName);
        return LE_NOT_FOUND;
    }

    le_result_t result = RemoveEachKernelModule(moduleInfoPtr);

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
