/**
 * @file pa_mcc_simu.c
 *
 * Simulation implementation of @ref c_pa_mcc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_mcc.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Call event notifications.
 *
 * @return
 *      \return LE_FAULT         The function failed to register the handler.
 *      \return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_SetCallEventHandler
(
    pa_mcc_CallEventHandlerFunc_t   handlerFuncPtr ///< [IN] The event handler function.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for incoming calls handling.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mcc_ClearCallEventHandler
(
    void
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set a voice call.
 *
 * @return
 *      \return LE_FAULT        The function failed to set the call.
 *      \return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_VoiceDial
(
    const char*    pn,        ///< [IN] The phone number.
    pa_mcc_clir_t  clir,      ///< [IN] The CLIR supplementary service subscription.
    pa_mcc_cug_t   cug,       ///< [IN] The CUG supplementary service information.
    uint8_t*       callIdPtr  ///< [OUT] The outgoing call ID.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to answer a call.
 *
 * @return
 *      \return LE_FAULT        The function failed to answer a call.
 *      \return LE_COMM_ERROR   The communication device has returned an error.
 *      \return LE_TIMEOUT      No response was received from the Modem.
 *      \return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_Answer
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disconnect the remote user.
 *
 * @return
 *      \return LE_FAULT        The function failed to disconnect the remote user.
 *      \return LE_COMM_ERROR   The communication device has returned an error.
 *      \return LE_TIMEOUT      No response was received from the Modem.
 *      \return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUp
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to end all the ongoing calls.
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mcc_HangUpAll
(
    void
)
{
    return LE_OK;
}

