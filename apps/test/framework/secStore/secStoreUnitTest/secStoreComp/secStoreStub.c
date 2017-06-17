
#include "legato.h"
#include "interfaces.h"
#include "appCfg.h"

//--------------------------------------------------------------------------------------------------
/**
 * Secure Storage default limit in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_LIMIT_SEC_STORE                     8192

#define DEFAULT_APPCFG_ITER                         ((appCfg_Iter_t)0xD34DB33F)

#define DEFAULT_UPDATE_SYSTEM_HASH                  "DEFAULTSYSTEMHASH"

//--------------------------------------------------------------------------------------------------
/**
 * Stub the client session reference for the current message for secStoreAdmin
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t secStoreAdmin_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t secStoreAdmin_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub the client session reference for the current message for le_secStore
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_secStore_GetClientSessionRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Fetches the user credentials of the client at the far end of a given IPC session.
 *
 * @warning This function can only be called for the server-side of a session.
 *
 * @return LE_OK if successful.
 *         LE_CLOSED if the session has closed.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t MsgGetClientUserCreds
(
    le_msg_SessionRef_t sessionRef,   ///< [in] Reference to the session.
    uid_t*              userIdPtr,    ///< [out] Ptr to where the uid is to be stored on success.
    pid_t*              processIdPtr  ///< [out] Ptr to where the pid is to be stored on success.
)
{
    *userIdPtr = 0;
    *processIdPtr = 0;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service open handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MsgAddServiceOpenHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add service close handler stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MsgAddServiceCloseHandler
(
    le_msg_ServiceRef_t serviceRef,
    le_msg_SessionEventHandler_t handlerFunc,
    void *contextPtr
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t AppInfoGetName
(
    int32_t pid,
        ///< [IN] PID of the process.

    char* appName,
        ///< [OUT] Application name.

    size_t appNameNumElements
        ///< [IN]
)
{
    return le_utf8_Copy(appName, LE_APPINFO_DEFAULT_APPNAME, appNameNumElements, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's Secure Storage limit in bytes.
 *
 * @return
 *      The size in bytes if available.  The default size if unavailable.
 */
//--------------------------------------------------------------------------------------------------
size_t appCfg_GetSecStoreLimit
(
    appCfg_Iter_t appIterRef    ///< [IN] Apps iterator
)
{
    return DEFAULT_LIMIT_SEC_STORE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates an iterator that can be used to iterate over the list of apps, but start the iterator at
 * the given app.
 *
 * @note
 *      Iterators have a time out and must be deleted before the timeout expires.
 *
 * @return
 *      Reference to the iterator, or NULL if the app was not found.
 */
//--------------------------------------------------------------------------------------------------
appCfg_Iter_t appCfg_FindApp
(
    const char* appName         ///< [IN] Name of the app to find.
)
{
    return DEFAULT_APPCFG_ITER;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes the iterator.
 */
//--------------------------------------------------------------------------------------------------
void appCfg_DeleteIter
(
    appCfg_Iter_t iter          ///< [IN] Iterator
)
{
    LE_ASSERT(DEFAULT_APPCFG_ITER == iter)
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the currently running system.
 *
 * @return The index of the running system.
 */
//--------------------------------------------------------------------------------------------------
int32_t UpdateGetCurrentSysIndex
(
    void
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the index for the previous system in the chain, using the current system as a starting point.
 *
 * @return The index to the system that's previous to the given system.  -1 is returned if the
 *         previous system was not found.
 */
//--------------------------------------------------------------------------------------------------
int32_t UpdateGetPreviousSystemIndex
(
    int32_t systemIndex
        ///< [IN] Get the system that's older than this system.
)
{
    return (-1);
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the hash ID for a given system.
 *
 * @return
 *      - LE_OK if no problems are encountered.
 *      - LE_NOT_FOUND if the given index does not correspond to an available system.
 *      - LE_OVERFLOW if the supplied buffer is too small.
 *      - LE_FORMAT_ERROR if there are problems reading the hash for the system.
 */
//--------------------------------------------------------------------------------------------------
le_result_t UpdateGetSystemHash
(
    int32_t systemIndex,
        ///< [IN] The system to read the hash for.

    char* hashStr,
        ///< [OUT] Buffer to hold the system hash string.

    size_t hashStrNumElements
        ///< [IN]
)
{
    return le_utf8_Copy(hashStr, DEFAULT_UPDATE_SYSTEM_HASH, hashStrNumElements, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
}
