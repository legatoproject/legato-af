//--------------------------------------------------------------------------------------------------
/**
 * Simple app that brings up the Air Vantage LWM2M connection and logs status messages from the
 * Air Vantage agent.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


//-------------------------------------------------------------------------------------------------
/**
 * Fetch a string describing the type of update underway over Air Vantage.
 *
 * @return Pointer to a null-terminated string constant.
 */
//-------------------------------------------------------------------------------------------------
static const char* GetUpdateType
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_avc_UpdateType_t type;
    le_result_t res = le_avc_GetUpdateType(&type);
    if (res != LE_OK)
    {
        LE_CRIT("Unable to get update type (%s)", LE_RESULT_TXT(res));
        return "UNKNOWN";
    }
    else
    {
        switch (type)
        {
            case LE_AVC_FIRMWARE_UPDATE:
                return "FIRMWARE";

            case LE_AVC_APPLICATION_UPDATE:
                return "APPLICATION";

            case LE_AVC_FRAMEWORK_UPDATE:
                return "FRAMEWORK";

            case LE_AVC_UNKNOWN_UPDATE:
                return "UNKNOWN";
        }

        LE_CRIT("Unexpected update type %d", type);
        return "UNKNOWN";
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates. Writes update status to configTree.
 */
//-------------------------------------------------------------------------------------------------
static void StatusHandler
(
    le_avc_Status_t updateStatus,
    int32_t totalNumBytes,
    int32_t downloadProgress,
    void* contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    const char* statusPtr = NULL;

    switch (updateStatus)
    {
        case LE_AVC_NO_UPDATE:
            statusPtr = "NO_UPDATE";
            break;
        case LE_AVC_DOWNLOAD_PENDING:
            statusPtr = "DOWNLOAD_PENDING";
            break;
        case LE_AVC_DOWNLOAD_IN_PROGRESS:
            statusPtr = "DOWNLOAD_IN_PROGRESS";
            break;
        case LE_AVC_DOWNLOAD_COMPLETE:
            statusPtr = "DOWNLOAD_COMPLETE";
            break;
        case LE_AVC_DOWNLOAD_FAILED:
            statusPtr = "DOWNLOAD_FAILED";
            break;
        case LE_AVC_INSTALL_PENDING:
            statusPtr = "INSTALL_PENDING";
            break;
        case LE_AVC_INSTALL_IN_PROGRESS:
            statusPtr = "INSTALL_IN_PROGRESS";
            break;
        case LE_AVC_INSTALL_COMPLETE:
            statusPtr = "INSTALL_COMPLETE";
            break;
        case LE_AVC_INSTALL_FAILED:
            statusPtr = "INSTALL_FAILED";
            break;
        case LE_AVC_UNINSTALL_PENDING:
            statusPtr = "UNINSTALL_PENDING";
            break;
        case LE_AVC_UNINSTALL_IN_PROGRESS:
            statusPtr = "UNINSTALL_IN_PROGRESS";
            break;
        case LE_AVC_UNINSTALL_COMPLETE:
            statusPtr = "UNINSTALL_COMPLETE";
            break;
        case LE_AVC_UNINSTALL_FAILED:
            statusPtr = "UNINSTALL_FAILED";
            break;
        case LE_AVC_SESSION_STARTED:
            statusPtr = "SESSION_STARTED";
            break;
        case LE_AVC_SESSION_STOPPED:
            statusPtr = "SESSION_STOPPED";
            break;
    }

    if (statusPtr == NULL)
    {
        LE_ERROR("Air Vantage agent reported unexpected update status: %d", updateStatus);
    }
    else
    {
        LE_INFO("Air Vantage agent reported update status: %s", statusPtr);

        le_result_t res;

        if (updateStatus == LE_AVC_DOWNLOAD_PENDING)
        {
            LE_INFO("Accepting %s update.", GetUpdateType());
            res = le_avc_AcceptDownload();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept download from Air Vantage (%s)", LE_RESULT_TXT(res));
            }
        }
        else if (updateStatus == LE_AVC_INSTALL_PENDING)
        {
            LE_INFO("Accepting %s installation.", GetUpdateType());
            res = le_avc_AcceptInstall();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept install from Air Vantage (%s)", LE_RESULT_TXT(res));
            }
        }
       else if (updateStatus == LE_AVC_UNINSTALL_PENDING)
        {
            LE_INFO("Accepting %s uninstall.", GetUpdateType());
            res = le_avc_AcceptUninstall();
            if (res != LE_OK)
            {
                LE_ERROR("Failed to accept uninstall from Air Vantage (%s)", LE_RESULT_TXT(res));
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.  Must return when done initializing.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Air Vantage Connection Controller started.");

    // Register Air Vantage status report handler.
    le_avc_AddStatusEventHandler(StatusHandler, NULL);

    // Start an AV session.
    le_result_t res = le_avc_StartSession();
    if (res != LE_OK)
    {
        LE_ERROR("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));

        LE_INFO("Attempting to stop previous session, in case one is still active...");
        res = le_avc_StopSession();
        if (res != LE_OK)
        {
            LE_ERROR("Failed to stop session: %s", LE_RESULT_TXT(res));
        }
        else
        {
            LE_INFO("Successfully stopped session.  Attempting to start a new one.");
            res = le_avc_StartSession();
            if (res != LE_OK)
            {
                LE_FATAL("Failed to connect to AirVantage: %s", LE_RESULT_TXT(res));
            }
        }
    }

    LE_INFO("Air Vantage session started successfully.");
}
