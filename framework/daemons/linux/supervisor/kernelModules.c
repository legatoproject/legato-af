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
    int             argc;                       // insmod/rmmod argc
    char            *argv[KMODULE_MAX_ARGC];    // insmod/rmmod argv
    le_dls_Link_t   listLink;
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
    le_dls_List_t       moduleList;    // List of modules stored in order they were inserted
} KModuleHandler = {NULL, NULL, LE_DLS_LIST_INIT};


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

    /* Now build a parameter list that will be sent to execv */
    m->argv[0] = NULL;      /* First: reserved for execv command */
    m->argv[1] = m->path;   /* Second: file/module path */
    m->argc = 2;
    ModuleGetParams(m);       /* Read remaining parameters from configTree */

    /* Run insmod commands. */
    ExecuteCommand(INSMOD_COMMAND, m->path, m->argv);

    /* Free parameters - they did their job */
    ModuleFreeParams(m);

    /* Trim off the extension from path/name and store module record in list */
    ext = le_path_FindTrailing(m->path, KERNEL_MODULE_FILE_EXTENSION);
    LE_ASSERT(ext != NULL);
    *ext = '\0';
    m->listLink = LE_DLS_LINK_INIT;
    le_dls_Queue(&KModuleHandler.moduleList, &m->listLink);

    LE_INFO("New kernel module '%s'", m->name);
}


//--------------------------------------------------------------------------------------------------
/**
 * Predicate function to check if the given directory entry is named like a kernel module. Note that
 * no check is done to ensure that the given directory entry is a regular file nor is there a check
 * to ensure that the file is actually a Linux kernel module.
 *
 * @return
 *     non-zero if the directory entry is a kernel module or 0 otherwise.
 */
//--------------------------------------------------------------------------------------------------
static int IsKernelModule
(
    const struct dirent* entry  ///< Directory entry to check
)
{
    return (le_path_FindTrailing(entry->d_name, KERNEL_MODULE_FILE_EXTENSION) != NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Traverse module directory and insmod all modules in alphabetical order.
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Insert(void)
{
    struct dirent **entryList;
    int i;

    LE_DEBUG("Inserting kernel module files (*" KERNEL_MODULE_FILE_EXTENSION
        ") from " SYSTEM_MODULE_PATH "...");

    int scanRes = scandir(SYSTEM_MODULE_PATH, &entryList, IsKernelModule, alphasort);
    LE_FATAL_IF(scanRes < 0, "Error reading modules directory: %m");
    for (i = 0; i < scanRes; i++)
    {
        LE_DEBUG("Inserting kernel module '%s'.", entryList[i]->d_name);
        ModuleInsert(entryList[i]);
        free(entryList[i]);
    }
    free(entryList);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove previously inserted module in reverse order.
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Remove(void)
{
    /* Iterate through module list */
    le_dls_Link_t *listLink;
    for (listLink = le_dls_PopTail(&KModuleHandler.moduleList);
         listLink != NULL;
         listLink = le_dls_PopTail(&KModuleHandler.moduleList))
    {
        KModuleObj_t *m = CONTAINER_OF(listLink, KModuleObj_t, listLink);
        LE_ASSERT(KMODULE_OBJECT_COOKIE == m->cookie);

        /* Populate argv for rmmod */
        m->argv[0] = NULL;     /* Reserved for rmmod command */
        m->argv[1] = m->name;  /* Second argument is module name */
        m->argc = 2;

        /* rmmod does not take paramters from configTree; just run command. */
        ExecuteCommand(RMMOD_COMMAND, m->name, m->argv);

        /* Reset exec arguments */
        ModuleFreeParams(m);

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

    // Create list of kernel module objects
    KModuleHandler.moduleList = LE_DLS_LIST_INIT;
}
