//--------------------------------------------------------------------------------------------------
/** @file kernelModules.h
 *
 * API for managing Legato-bundled kernel modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_KERNEL_MODULES_INCLUDE_GUARD
#define LEGATO_SRC_KERNEL_MODULES_INCLUDE_GUARD

#include "le_cfg_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Kernel module file extenstion.
 */
//--------------------------------------------------------------------------------------------------
#define KERNEL_MODULE_FILE_EXTENSION ".ko"


//--------------------------------------------------------------------------------------------------
/**
 * Required kernel module name node
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char modName[LE_CFG_STR_LEN_BYTES];    ///< Kernel module name
    bool isOptional;                       ///< is the module optional or not
    le_sls_Link_t link;                    ///< Link in the node
}
ModNameNode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Dependency system kernel module node
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char modName[LE_CFG_STR_LEN_BYTES];    ///< System kernel module name
    uint32_t useCount;                     ///< Use count of a system kernel module
    le_sls_Link_t link;                    ///< Link in the node
}
DepModNameNode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enum for load status of modules: init, try, installed or removed
 */
//--------------------------------------------------------------------------------------------------
typedef enum {
    STATUS_INIT = 0,    ///< Module is in initialization state
    STATUS_TRY_INSTALL, ///< Try state before installing the module
    STATUS_INSTALLED,   ///< If insmod is executed on the module
    STATUS_TRY_REMOVE,  ///< Try state before removing the module
    STATUS_REMOVED      ///< If rmmod is executed on the module
} ModuleLoadStatus_t;


//--------------------------------------------------------------------------------------------------
/**
 * Node for /proc/modules that has information of the module load status and the number of other
 * modules using a given module.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
   int usedbyNumMod;                    ///< Number of other modules using this module
   ModuleLoadStatus_t loadStatus;       ///< Load status of the module: Live, Loading, Unloading
}
ProcModules_t;


//--------------------------------------------------------------------------------------------------
/**
 * Insert all kernel modules
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Insert
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove all kernel modules
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Remove
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize kernel module handler
 */
//--------------------------------------------------------------------------------------------------
void kernelModules_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Insert the given list of kernel modules
 */
//--------------------------------------------------------------------------------------------------
le_result_t kernelModules_InsertListOfModules
(
    le_sls_List_t reqModuleName
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove the given list of kernel modules
 */
//--------------------------------------------------------------------------------------------------
le_result_t kernelModules_RemoveListOfModules
(
    le_sls_List_t reqModuleName
);

#endif // LEGATO_SRC_KERNEL_MODULES_INCLUDE_GUARD
