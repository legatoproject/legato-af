//--------------------------------------------------------------------------------------------------
/**
 *  Secure Storage Server's header file for the shared routines.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SECSTORESERVER_H_INCLUDE_GUARD
#define LEGATO_SECSTORESERVER_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * The following are LE_SHARED functions
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t secStoreServer_GetClientName(le_msg_SessionRef_t sessionRef, char* bufPtr,
                                                   size_t bufSize, bool* isApp);

#endif // LEGATO_SECSTORESERVER_H_INCLUDE_GUARD
