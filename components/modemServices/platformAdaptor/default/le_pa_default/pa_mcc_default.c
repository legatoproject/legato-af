/**
 * @file pa_mcc_default.c
 *
 * Default implementation of @ref c_pa_mcc.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_mcc.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Call event notifications.
 *
 * @return LE_FAULT         The function failed to register the handler.
 * @return LE_DUPLICATE     There is already a handler registered.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetCallEventHandler
(
    pa_mcc_CallEventHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
)
{
    LE_ERROR("Unsupported function called");
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for incoming calls handling.
 */
//--------------------------------------------------------------------------------------------------
void pa_mcc_ClearCallEventHandler
(
    void
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set a voice call.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BUSY          A call is already ongoing.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_VoiceDial
(
    const char*    phoneNumberPtr,          ///< [IN] The phone number.
    pa_mcc_clir_t  clir,                    ///< [IN] The CLIR supplementary service subscription.
    pa_mcc_cug_t   cug,                     ///< [IN] The CUG supplementary service information.
    uint8_t*       callIdPtr,               ///< [OUT] The outgoing call ID.
    le_mcc_TerminationReason_t*  errorPtr   ///< [OUT] Call termination error.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to answer a call.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_Answer
(
    uint8_t  callId     ///< [IN] The call ID to answer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disconnect the remote user.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUp
(
    uint8_t  callId     ///< [IN] The call ID to hangup
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to end all the ongoing calls.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUpAll
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates or deactivates the call waiting service.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetCallWaitingService
(
    bool active
        ///< [IN] The call waiting activation.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the call waiting service status.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_GetCallWaitingService
(
    bool* activePtr
        ///< [OUT] The call waiting activation.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function activates the specified call. Other calls are placed on hold.
 *
 * @return
 *     - LE_OK        The function succeed.
 *     - LE_FAULT     The function failed.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_ActivateCall
(
    uint8_t  callId     ///< [IN] The active call ID
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables/disables the audio AMR Wideband capability.
 *
 * @return
 *     - LE_OK            The function succeeded.
 *     - LE_UNSUPPORTED   The service is not available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetAmrWbCapability
(
    bool  enable    ///< [IN] True enables the AMR Wideband capability, false disables it.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the audio AMR Wideband capability
 *
 * @return
 *     - LE_OK             The function succeeded.
 *     - LE_UNSUPPORTED    The service is not available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_GetAmrWbCapability
(
    bool*  enabled     ///< [OUT] True if AMR Wideband capability is enabled, false otherwise.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}
