/**
 * @page c_pa_rsim Remote SIM service Platform Adapter API
 *
 * @ref pa_rsim.h "API Reference"
 *
 * <HR>
 *
 * @section pa_rsim_toc Table of Contents
 *
 *  - @ref pa_rsim_intro
 *  - @ref pa_rsim_rationale
 *
 *
 * @section pa_rsim_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developed upon these APIs.
 *
 *
 * @section pa_rsim_rational Rationale
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occurred due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/**
 * @file pa_rsim.h
 *
 * Legato @ref c_pa_rsim include file.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PARSIM_INCLUDE_GUARD
#define LEGATO_PARSIM_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maximal APDU size.
 */
//--------------------------------------------------------------------------------------------------
#define PA_RSIM_APDU_MAX_SIZE   (256)

//--------------------------------------------------------------------------------------------------
/**
 * SIM status.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_RSIM_STATUS_UNKNOWN_ERROR     = 0,   ///< Unknown error.
    PA_RSIM_STATUS_RESET             = 1,   ///< SIM card reset.
    PA_RSIM_STATUS_NOT_ACCESSIBLE    = 2,   ///< SIM card not accessible.
    PA_RSIM_STATUS_REMOVED           = 3,   ///< SIM card removed.
    PA_RSIM_STATUS_INSERTED          = 4,   ///< SIM card inserted.
    PA_RSIM_STATUS_RECOVERED         = 5,   ///< Non-accessible SIM card is made accessible again.
    PA_RSIM_STATUS_AVAILABLE         = 6,   ///< SIM card available.
    PA_RSIM_STATUS_NO_LINK           = 7,   ///< No link established with remote SIM card.
    PA_RSIM_STATUS_COUNT             = 8    ///< This must always be the last element in this enum.
}
pa_rsim_SimStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * SIM action request.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_RSIM_CONNECTION       = 0,   ///< SIM connection request.
    PA_RSIM_DISCONNECTION    = 1,   ///< SIM disconnection request.
    PA_RSIM_RESET            = 2,   ///< SIM warm reset request.
    PA_RSIM_POWER_UP         = 3,   ///< SIM power up request.
    PA_RSIM_POWER_DOWN       = 4    ///< SIM power down request.
}
pa_rsim_Action_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report a SIM action request.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_rsim_SimActionHdlrFunc_t)
(
    pa_rsim_Action_t action     ///< SIM action requested by the modem
);

//--------------------------------------------------------------------------------------------------
/**
 * APDU indication structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t apduLength;                        ///< APDU length
    uint8_t  apduData[PA_RSIM_APDU_MAX_SIZE];   ///< APDU data
}
pa_rsim_ApduInd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report an APDU indication.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*pa_rsim_ApduIndHdlrFunc_t)
(
    pa_rsim_ApduInd_t* apduInd      ///< APDU information sent by the modem
);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to add an APDU indication notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_rsim_AddApduNotificationHandler
(
    pa_rsim_ApduIndHdlrFunc_t indicationHandler ///< [IN] The handler function to handle an APDU
                                                ///  notification reception.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to unregister an APDU indication notification handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_rsim_RemoveApduNotificationHandler
(
    le_event_HandlerRef_t apduIndHandler
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to add a SIM action request notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t pa_rsim_AddSimActionRequestHandler
(
    pa_rsim_SimActionHdlrFunc_t actionHandler   ///< [IN] The handler function to handle a SIM
                                                ///  action request notification reception.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to unregister a SIM action request notification handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_rsim_RemoveSimActionRequestHandler
(
    le_event_HandlerRef_t actionRequestHandler
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to notify the modem of the remote SIM disconnection.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_rsim_Disconnect
(
    void
);

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
LE_SHARED le_result_t pa_rsim_NotifyStatus
(
    pa_rsim_SimStatus_t simStatus   ///< [IN] SIM status change
);

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
LE_SHARED le_result_t pa_rsim_TransferApduResp
(
    const uint8_t* apduPtr,     ///< [IN] APDU buffer
    uint16_t       apduLen      ///< [IN] APDU length in bytes
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to indicate an APDU response error to the modem.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_rsim_TransferApduRespError
(
    void
);

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
LE_SHARED le_result_t pa_rsim_TransferAtrResp
(
    pa_rsim_SimStatus_t simStatus,  ///< [IN] SIM status change
    const uint8_t* atrPtr,          ///< [IN] ATR buffer
    uint16_t       atrLen           ///< [IN] ATR length in bytes
);

//--------------------------------------------------------------------------------------------------
/**
 * This function indicates if the Remote SIM service is supported by the PA.
 *
 * @return
 *  - true      Remote SIM service is supported.
 *  - false     Remote SIM service is not supported.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool pa_rsim_IsRsimSupported
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function checks if the remote SIM card is selected.
 *
 * @return true         If the remote SIM is selected.
 * @return false        It the remote SIM is not selected.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED bool pa_rsim_IsRemoteSimSelected
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA Remote SIM service module.
 *
 * @return
 *  - LE_OK             The function succeeded.
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_rsim_Init
(
    void
);

#endif // LEGATO_PARSIM_INCLUDE_GUARD
