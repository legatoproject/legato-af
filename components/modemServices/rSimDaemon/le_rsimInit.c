/**
 * @file le_rsimInit.c
 *
 * This file contains the source code of Remote SIM Service Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "le_rsim_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Remote SIM Service.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
   le_rsim_Init();
}
