//--------------------------------------------------------------------------------------------------
/**
 * @file updateUnpack.h
 *
 * Interfaces provided by the Update Unpacker to other modules inside the Update Daemon.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
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
    UPDATE_UNPACK_STATUS_UNPACKING,
    UPDATE_UNPACK_STATUS_APPLYING,
    UPDATE_UNPACK_STATUS_APP_UPDATED,       ///< Changes to one or more individual apps completed.
    UPDATE_UNPACK_STATUS_SYSTEM_UPDATED,    ///< System update completed.
    UPDATE_UNPACK_STATUS_WAIT_FOR_REBOOT,   ///< Firmware update requires reboot to complete.
    UPDATE_UNPACK_STATUS_BAD_PACKAGE,
    UPDATE_UNPACK_STATUS_INTERNAL_ERROR,
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
 * Stop unpacking an update pack.
 */
//--------------------------------------------------------------------------------------------------
void updateUnpack_Stop
(
    void
);


#endif // LEGATO_UPDATE_UNPACK_H_INCLUDE_GUARD
