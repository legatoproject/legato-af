//--------------------------------------------------------------------------------------------------
/**
 * @file updateUnpack.h
 *
 * Interfaces provided by the Update Unpacker to other modules inside the Update Daemon.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_UPDATE_UNPACK_H_INCLUDE_GUARD
#define LEGATO_UPDATE_UNPACK_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Progress status codes.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    UPDATE_UNPACK_STATUS_UNPACKING,     ///< In progress
    UPDATE_UNPACK_STATUS_DONE,          ///< Finished successfully.
    UPDATE_UNPACK_STATUS_BAD_PACKAGE,   ///< Failed because something is wrong with the update pack.
    UPDATE_UNPACK_STATUS_INTERNAL_ERROR,///< Failed because of an internal error.
}
updateUnpack_ProgressCode_t;


//--------------------------------------------------------------------------------------------------
/**
 * The client callback function (handler) registered using le_update_AddProgressHandler() must
 * look like this.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*updateUnpack_ProgressHandler_t)
(
    updateUnpack_ProgressCode_t status,  ///< Current state of update.

    unsigned int  percentDone   ///< Percent done for current state. For example, in state
                                ///< UNPACKING, a percentDone of 80 means that 80% of the update
                                ///< data has been unpacked.
);


//--------------------------------------------------------------------------------------------------
/**
 * What type of update pack is it?
 **/
//--------------------------------------------------------------------------------------------------
typedef enum
{
    TYPE_UNKNOWN,           ///< Don't know yet.
    TYPE_SYSTEM_UPDATE,     ///< System update pack.
    TYPE_APP_UPDATE,        ///< Individual app update.
    TYPE_APP_REMOVE,        ///< Individual app remove.
    TYPE_FIRMWARE_UPDATE,   ///< Firmware update pack.
}
updateUnpack_Type_t;


//--------------------------------------------------------------------------------------------------
/**
 * Start unpacking an update pack.  When sections of the update pack are unpacked, the
 * update unpacker will call functions in the update executor to perform the update actions.
 */
//--------------------------------------------------------------------------------------------------
void updateUnpack_Start
(
    int fd,             ///< File descriptor to read the update pack from.
    updateUnpack_ProgressHandler_t progressHandler  ///< Progress reporting callback.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the type of the update pack (available when 100% done).
 *
 * @return The type of update (firmware, app, or system).
 **/
//--------------------------------------------------------------------------------------------------
updateUnpack_Type_t updateUnpack_GetType
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the app being changed (valid for app update or remove).
 *
 * @return The name of the app (valid until next unpack is started).
 **/
//--------------------------------------------------------------------------------------------------
const char* updateUnpack_GetAppName
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the MD5 sum of the app being updated (valid for app update only).
 *
 * @return The MD5 sum of the app, as a string (valid until next unpack is started).
 **/
//--------------------------------------------------------------------------------------------------
const char* updateUnpack_GetAppMd5
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Stop unpacking an update pack and reset the unpacker to its initial state.
 */
//--------------------------------------------------------------------------------------------------
void updateUnpack_Stop
(
    void
);


#endif // LEGATO_UPDATE_UNPACK_H_INCLUDE_GUARD
