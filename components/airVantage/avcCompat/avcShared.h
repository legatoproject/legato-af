/**
 * @file avcShared.h
 *
 * Header file containing shared definitions for AVC compatibility app
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_AVC_SHARED_DEFS_INCLUDE_GUARD
#define LEGATO_AVC_SHARED_DEFS_INCLUDE_GUARD

// -------------------------------------------------------------------------------------------------
/**
 *  Name of the AVC service running in legato.
 */
// ------------------------------------------------------------------------------------------------
#define AVC_APP_NAME "avcService"


//--------------------------------------------------------------------------------------------------
/**
 *  LWM2M object to manage apps.
 */
//--------------------------------------------------------------------------------------------------
#define LWM2M_SOFTWARE_UPDATE 9


//--------------------------------------------------------------------------------------------------
/**
 *  Path to the lwm2m configurations in the Config Tree.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_AVC_CONFIG_PATH "system:/apps/avcService/config"


//--------------------------------------------------------------------------------------------------
/**
 *  Max number of bytes of a retry timer name.
 */
//--------------------------------------------------------------------------------------------------
#define TIMER_NAME_BYTES 10


//--------------------------------------------------------------------------------------------------
/**
 * Import AVMS config from the modem to Legato.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void ImportConfig
(
    void
);


#endif // LEGATO_AVC_SHARED_DEFS_INCLUDE_GUARD
