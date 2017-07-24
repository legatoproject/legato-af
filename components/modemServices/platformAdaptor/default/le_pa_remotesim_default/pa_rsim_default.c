/**
 * @file pa_rsim_default.c
 *
 * Default implementation of @ref c_pa_rsim.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_rsim.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to add an APDU indication notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_rsim_AddApduNotificationHandler
(
    pa_rsim_ApduIndHdlrFunc_t indicationHandler ///< [IN] The handler function to handle an APDU
                                                ///  notification reception.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to unregister an APDU indication notification handler.
 */
//--------------------------------------------------------------------------------------------------
void pa_rsim_RemoveApduNotificationHandler
(
    le_event_HandlerRef_t apduIndHandler
)
{
    // Stubbed function
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to add a SIM action request notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_rsim_AddSimActionRequestHandler
(
    pa_rsim_SimActionHdlrFunc_t actionHandler   ///< [IN] The handler function to handle a SIM
                                                ///  action request notification reception.
)
{
   return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to unregister a SIM action request notification handler.
 */
//--------------------------------------------------------------------------------------------------
void pa_rsim_RemoveSimActionRequestHandler
(
    le_event_HandlerRef_t actionRequestHandler
)
{
    // Stubbed function
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to notify the modem of the remote SIM disconnection.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_Disconnect
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to notify the modem of a remote SIM status change.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_BAD_PARAMETER  Unknown SIM status.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_NotifyStatus
(
    pa_rsim_SimStatus_t simStatus   ///< [IN] SIM status change
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to transfer an APDU response to the modem.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_BAD_PARAMETER  APDU too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_TransferApduResp
(
    const uint8_t* apduPtr,     ///< [IN] APDU buffer
    uint16_t       apduLen      ///< [IN] APDU length in bytes
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to indicate an APDU response error to the modem.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_TransferApduRespError
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to transfer an Answer to Reset (ATR) response to the modem.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 *  - LE_BAD_PARAMETER  ATR too long.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_TransferAtrResp
(
    pa_rsim_SimStatus_t simStatus,  ///< [IN] SIM status change
    const uint8_t* atrPtr,          ///< [IN] ATR buffer
    uint16_t       atrLen           ///< [IN] ATR length in bytes
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function indicates if the Remote SIM service is supported by the PA.
 *
 * @return
 *  - true      Remote SIM service is supported.
 *  - false     Remote SIM service is not supported.
 */
//--------------------------------------------------------------------------------------------------
bool pa_rsim_IsRsimSupported
(
    void
)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks if the remote SIM card is selected.
 *
 * @return true         If the remote SIM is selected.
 * @return false        It the remote SIM is not selected.
 */
//--------------------------------------------------------------------------------------------------
bool pa_rsim_IsRemoteSimSelected
(
    void
)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Remote SIM service module.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_rsim_Init
(
    void
)
{
    return LE_OK;
}
