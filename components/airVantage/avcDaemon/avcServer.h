/**
 * @file avcServer.h
 *
 * Interface for AVC Server (for internal use only)
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#ifndef LEGATO_AVC_SERVER_INCLUDE_GUARD
#define LEGATO_AVC_SERVER_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
// Definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler used with avcServer_QueryInstall() to return install response.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*avcServer_InstallHandlerFunc_t)
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler used with avcServer_QueryUninstall() to return uninstall response.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*avcServer_UninstallHandlerFunc_t)
(
    void
);



//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Query the AVC Server if it's okay to proceed with an application install
 *
 * If an install can't proceed right away, then the handlerRef function will be called when it is
 * okay to proceed with an install. Note that handlerRef will be called at most once.
 *
 * @return
 *      - LE_OK if install can proceed right away (handlerRef will not be called)
 *      - LE_BUSY if handlerRef will be called later to notify when install can proceed
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t avcServer_QueryInstall
(
    avcServer_InstallHandlerFunc_t handlerRef  ///< [IN] Handler to receive install response.
);

//--------------------------------------------------------------------------------------------------
/**
 * Report the status of install back to the control app
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void avcServer_ReportInstallProgress
(
    le_avc_Status_t updateStatus,
    uint installProgress           ///< [IN]  Percentage of install completed.
                                   ///        Applicable only when le_avc_Status_t is one of
                                   ///        LE_AVC_INSTALL_IN_PROGRESS, LE_AVC_INSTALL_COMPLETE
                                   ///        or LE_AVC_INSTALL_FAILED.
);

//--------------------------------------------------------------------------------------------------
/**
 * Sends a registration update to the server.
 *
 * ToDo: Remove this function after block transfer is implemented. This function is a temporary
 * solution to force a registration update and sync object 9 contents with firmware. If block
 * transfer is implemented, there is no need for firmware to have its own copy of obj9.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void avcServer_RegistrationUpdate
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Query the AVC Server if it's okay to proceed with an application uninstall
 *
 * If an uninstall can't proceed right away, then the handlerRef function will be called when it is
 * okay to proceed with an uninstall. Note that handlerRef will be called at most once.
 *
 * @return
 *      - LE_OK if uninstall can proceed right away (handlerRef will not be called)
 *      - LE_BUSY if handlerRef will be called later to notify when uninstall can proceed
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t avcServer_QueryUninstall
(
    avcServer_UninstallHandlerFunc_t handlerRef  ///< [IN] Handler to receive uninstall response.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the update type of the currently pending update, used only during restore
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void avcServer_SetUpdateType
(
    le_avc_UpdateType_t updateType  ///< [IN]
);

#endif // LEGATO_AVC_SERVER_INCLUDE_GUARD
