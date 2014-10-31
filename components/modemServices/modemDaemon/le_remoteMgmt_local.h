/**
 * @file le_remoteMgmt_local.h
 *
 * This file contains the declaration of the Modem Remote Management Initialization.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  All rights reserved.
 */


#ifndef LE_REMOTE_MGMT_LOCAL_INCLUDE_GUARD
#define LE_REMOTE_MGMT_LOCAL_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Remote Management component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_remoteMgmt_Init
(
    void
);


#endif // MDM_REMOTE_MGMT_LOCAL_INCLUDE_GUARD
