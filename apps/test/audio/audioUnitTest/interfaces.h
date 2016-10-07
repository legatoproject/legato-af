#include "le_audio_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_WARN

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_audio_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_audio_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)

 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t myAddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Option LE_PM_REF_COUNT to manage a reference counted wakeup source
 */
//--------------------------------------------------------------------------------------------------
#define LE_PM_REF_COUNT 1


//--------------------------------------------------------------------------------------------------
/**
 * Reference to wakeup source used by StayAwake and Relax function
 */
//--------------------------------------------------------------------------------------------------
typedef void* le_pm_WakeupSourceRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Create a wakeup source  (STUBBED FUNCTION)
 *
 * @return
 *      - Reference to wakeup source (to be used later for acquiring/releasing)
 *
 * @note The process exits if an invalid or existing tag is passed
 */
//--------------------------------------------------------------------------------------------------
le_pm_WakeupSourceRef_t le_pm_NewWakeupSource
(
    uint32_t createOpts,
        ///< [IN] Wakeup source options

    const char* wsTag
        ///< [IN] Context-specific wakeup source tag
);

//--------------------------------------------------------------------------------------------------
/**
 * Acquire a wakeup source  (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
void le_pm_StayAwake
(
    le_pm_WakeupSourceRef_t wsRef
        ///< [IN] Reference to a created wakeup source
);

//--------------------------------------------------------------------------------------------------
/**
 * Release a previously acquired wakeup source  (STUBBED FUNCTION)
 *
 */
//--------------------------------------------------------------------------------------------------
void le_pm_Relax
(
    le_pm_WakeupSourceRef_t wsRef
        ///< [IN] Reference to a created wakeup source
);

