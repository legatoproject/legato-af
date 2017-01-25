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
 * Legato kernel module object.
 */
//--------------------------------------------------------------------------------------------------
#define KMODULE_OBJECT_COOKIE 0x71a89c35
typedef struct
{
    uint32_t        cookie;                     // KModuleObj_t identifier
    char            *name;                      // Module name
    char            path[LIMIT_MAX_PATH_BYTES]; // Path to module's .ko file
    struct dirent   *entry;                     // Module inode pointer
    int             argc;                       // insmod/rmmod argc
    char            *argv[KMODULE_MAX_ARGC];    // insmod/rmmod argv
}
KModuleObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * Legato kernel module handler object.
 */
//--------------------------------------------------------------------------------------------------
static struct {
    le_mem_PoolRef_t    modulePool;    // memory pool of KModuleObj_t objects
    le_mem_PoolRef_t    stringPool;    // memory pool of strings (for argv)
    le_hashmap_Ref_t    table;         // table of objects, indexed by name
} KModuleHandler = {NULL, NULL, NULL};


//--------------------------------------------------------------------------------------------------
/**
 * Populate list of module parameters
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
 * Free list of module parameters
 */
//--------------------------------------------------------------------------------------------------
static void ModuleFreeParams(KModuleObj_t *module)
{
    char **p;

    module->argv[0] = NULL; // Contained exec'ed command, not allocated
    module->argv[1] = NULL; // Contained module path/name, not allocated

    /* Iterate through remaining parameters and free buffers */
    p = module->argv + 2;
    while (NULL != *p)
    {
        le_mem_Release(*p);
        *p = NULL;
        p++;
    }

    /* Reset number of parameters */
    module->argc = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Build and execute the insmod/rmmod command
 */
//--------------------------------------------------------------------------------------------------
static void ExecuteCommand(char *command, char *modulePath, char *argv[])
{
    pid_t pid, p;
    int result;

    argv[0] = command;  /* First argument is always the command */
    LE_DEBUG("Execute '%s %s'", argv[0], argv[1]);

    pid = fork();
    LE_FATAL_IF(-1 == pid, "fork() failed. (%m)");

    if (0 == pid)
    {
        /* Child, exec command. */
        execv(argv[0], argv);
        /* Should never be here. */
        LE_FATAL("Failed to run '%s %s'. (%m)", argv[0], argv[1]);
    }

    /* Wait for command to complete; restart on EINTR. */
    while (-1 == (p = waitpid(pid, &result, 0)) && EINTR == errno);
    if (p != pid)
    {
        if (p == -1)
        {
            LE_FATAL("waitpid() failed: %m");
        }
        else
        {
            LE_FATAL("waitpid() returned unexpected result %d", p);
        }
    }

    /* Check exit status and errors */
    if (WIFSIGNALED(result))
    {
        LE_CRIT("%s was killed by a signal %d.", argv[0], WTERMSIG(result));
    }
    else if (WIFEXITED(result) && WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        LE_CRIT("%s exited with error code %d.", argv[0], WEXITSTATUS(result));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Insert a module with a given module file name
 */
//--------------------------------------------------------------------------------------------------
static void ModuleInsert(struct dirent *entry)
{
    KModuleObj_t *m;
    char *ext;

    /* Allocate module object */
    m = (KModuleObj_t*)le_mem_ForceAlloc(KModuleHandler.modulePool);
    memset(m, 0, sizeof(KModuleObj_t));
    LE_ASSERT_OK(le_path_Concat("/",
                                m->path,
                                LIMIT_MAX_PATH_BYTES,
                                SYSTEM_MODULE_PATH,
                                entry->d_name,
                                NULL));
    m->cookie = KMODULE_OBJECT_COOKIE;
    m->name = le_path_GetBasenamePtr(m->path, "/");
    m->entry = entry;

    /* Now build a parameter list that will be sent to execv */
    m->argv[0] = NULL;      /* First: reserved for execv command */
    m->argv[1] = m->path;   /* Second: file/module path */
    m->argc = 2;
    ModuleGetParams(m);       /* Read remaining parameters from configTree */

    /* Run insmod commands. */
    ExecuteCommand(INSMOD_COMMAND, m->path, m->argv);

    /* Free parameters - they did their job */
    ModuleFreeParams(m);

    /* Trim off the extension from path/name and store module record in table */
    ext = le_path_FindTrailing(m->path, KERNEL_MODULE_FILE_EXTENSION);
    LE_ASSERT(ext != NULL);
    *ext = '\0';
    LE_FATAL_IF(NULL != le_hashmap_Put(KModuleHandler.table, m->name, m),
                "Module '%s' already present.", m->name);

    LE_INFO("New kernel module '%s'", m->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse module directory and insmod all modules.
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Insert(void)
{
    DIR *moduleDir;
    struct dirent *entry;

    /* Open system module directory */
    moduleDir = opendir(SYSTEM_MODULE_PATH);
    if (!moduleDir)
    {
        LE_WARN("Cannot open " SYSTEM_MODULE_PATH " (%m). Module support disabled.");
        return;
    }

    LE_DEBUG("Inserting kernel module files (*" KERNEL_MODULE_FILE_EXTENSION
        ") from " SYSTEM_MODULE_PATH "...");

    /* Iterate through directory, assume single-threaded environment. */
    while (NULL != (entry = readdir(moduleDir)))
    {
        if (!le_path_FindTrailing(entry->d_name, KERNEL_MODULE_FILE_EXTENSION))
        {
            LE_DEBUG("Skip non-module file '%s'.", entry->d_name);
            continue;
        }

        /* Found a module file, insmod it. */
        LE_DEBUG("Inserting kernel module '%s'.", entry->d_name);
        ModuleInsert(entry);
    }
    LE_FATAL_IF(0 != closedir(moduleDir), "Error closing '%s'. (%m)",
                SYSTEM_MODULE_PATH);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove previously inserted module in reverse order.
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Remove(void)
{
    KModuleObj_t *m;
    le_hashmap_It_Ref_t iter;

    /* Iterate through module table */
    iter = le_hashmap_GetIterator(KModuleHandler.table);
    while (LE_OK == le_hashmap_NextNode(iter))
    {
        m = (KModuleObj_t*)le_hashmap_GetValue(iter);
        LE_ASSERT(m && KMODULE_OBJECT_COOKIE == m->cookie);

        /* Populate argv for rmmod */
        m->argv[0] = NULL;     /* Reserved for rmmod command */
        m->argv[1] = m->name;  /* Second argument is module name */
        m->argc = 2;

        /* rmmod does not take paramters from configTree; just run command. */
        ExecuteCommand(RMMOD_COMMAND, m->name, m->argv);

        /* Reset exec arguments */
        ModuleFreeParams(m);

        /* Make sure the exact record is removed */
        LE_ASSERT(m == le_hashmap_Remove(KModuleHandler.table, m->name));
        LE_INFO("Removed module '%s'", m->name);
        le_mem_Release(m);
    }
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

    // Create table of kernel module objects
    KModuleHandler.table = le_hashmap_Create("KModule Objects",
                                             31,
                                             le_hashmap_HashString,
                                             le_hashmap_EqualsString);
}
