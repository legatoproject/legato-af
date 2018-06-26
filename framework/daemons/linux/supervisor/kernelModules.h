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
    le_sls_Link_t link;                    ///< Link in the node
}
ModNameNode_t;


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
