/** @file pa_sms_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PASMSLOCAL_INCLUDE_GUARD
#define LEGATO_PASMSLOCAL_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Processing of unsolicited result codes.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_NMI_MODE_0 = 0, ///< same processing as PA_SMS_NMI_MODE_2
    PA_SMS_NMI_MODE_1 = 1, ///< same processing as PA_SMS_NMI_MODE_2
    PA_SMS_NMI_MODE_2 = 2, ///< Buffer unsolicited result codes in the Terminal Adaptor (TA) when
                           ///  Terminal Adaptor-Terminal Equipment (TE) link is reserved and
                           ///  flush them to the TE after reservation. Otherwise forward them
                           ///  directly to the TE.
    PA_SMS_NMI_MODE_3 = 3, ///< Forward unsolicited result codes to the TE by using a specific
                           ///  inband technique:
                           ///  while TA-TE link is reserved (i.e. TE is in online data mode by
                           ///  CSD or GPRS call) unsolicited result codes are replaced by a
                           ///  break (100ms) and stored in a buffer.
                           ///  The unsolicited result codes buffer is flushed to the TE after
                           ///  reservation (after +++ was entered). Otherwise, (the TE is not in
                           ///  online data mode) forward them directly to the TE.
}
pa_sms_NmiMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Result code indication routing for SMS-DELIVER indications
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_MT_0 = 0,   ///< no SMS-DELIVER indications are routed.
    PA_SMS_MT_1 = 1,   ///< SMS-DELIVERs are routed using unsolicited code +CMTI.
    PA_SMS_MT_2 = 2,   ///< SMS-DELIVERs (except class 2 messages) are routed using unsolicited
                       ///  code +CMT.
    PA_SMS_MT_3 = 3,   ///< Class 3 SMS-DELIVERS are routed directly using code in PA_SMS_MT2.
                       ///  Other classes messages result in indication PA_SMS_MT1.
}
pa_sms_NmiMt_t;

//--------------------------------------------------------------------------------------------------
/**
 * Rules for storing the received CBMs (Cell Broadcast Message) types.
 * They also depend on the coding scheme (text or PDU) and the setting of Select CBM Types.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_BM_0 = 0,   ///< no CBM indications are routed to the Terminal Equipment (TE). The
                       ///  CBMs are stored.
    PA_SMS_BM_1 = 1,   ///< The CBM is stored and an indication of the memory location is routed
                       ///  to the customer application using unsolicited result code +CBMI.
    PA_SMS_BM_2 = 2,   ///< New CBMs are routed directly to the TE using unsolicited result code
                       ///  +CBM.
    PA_SMS_BM_3 = 3,   ///< Class 3 CBMs: as PA_SMS_BM2. Other classes CBMs: as PA_SMS_BM1.
}
pa_sms_NmiBm_t;

//--------------------------------------------------------------------------------------------------
/**
 * SMS-STATUS-REPORTs routing.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_DS_0 = 0,   ///< no SMS-STATUS-REPORTs are routed.
    PA_SMS_DS_1 = 1,   ///< SMS-STATUS-REPORTs are routed using unsolicited code +CDS.
    PA_SMS_DS_2 = 2,   ///< SMS-STATUS-REPORTs are stored and routed using the unsolicited result
                       ///  code +CDSI.
}
pa_sms_NmiDs_t;

//--------------------------------------------------------------------------------------------------
/**
 * TA buffer of unsolicited result codes mode.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_SMS_BFR_0 = 0,  ///< TA buffer defined within this command is flushed to the TE when
                       ///  PA_SMS_MODE1 to PA_SMS_MODE3 is entered (OK response shall be
                       ///  given before flushing the codes).
    PA_SMS_BFR_2 = 1,  ///< TA buffer of unsolicited result codes defined within this command is
                       ///  cleared when PA_SMS_MODE1 to PA_SMS_MODE3 is entered.
}
pa_sms_NmiBfr_t;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the sms module.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function selects the procedure for message reception from the network (New Message
 * Indication settings).
 *
 * @return LE_FAULT         The function failed to select the procedure for message reception.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgIndic
(
    pa_sms_NmiMode_t mode,   ///< [IN] Processing of unsolicited result codes.
    pa_sms_NmiMt_t   mt,     ///< [IN] Result code indication routing for SMS-DELIVER indications.
    pa_sms_NmiBm_t   bm,     ///< [IN] Rules for storing the received CBMs (Cell Broadcast Message)
                             ///       types.
    pa_sms_NmiDs_t   ds,     ///< [IN] SMS-STATUS-REPORTs routing.
    pa_sms_NmiBfr_t  bfr     ///< [IN] TA buffer of unsolicited result codes mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the New Message Indication settings.
 *
 * @return LE_FAULT         The function failed to get the New Message Indication settings.
 * @return LE_BAD_PARAMETER Bad parameter, one is NULL.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetNewMsgIndic
(
    pa_sms_NmiMode_t* modePtr,  ///< [OUT] Processing of unsolicited result codes.
    pa_sms_NmiMt_t*   mtPtr,    ///< [OUT] Result code indication routing for SMS-DELIVER
                                ///        indications.
    pa_sms_NmiBm_t*   bmPtr,    ///< [OUT] Rules for storing the received CBMs (Cell Broadcast
                                ///        Message) types.
    pa_sms_NmiDs_t*   dsPtr,    ///< [OUT] SMS-STATUS-REPORTs routing.
    pa_sms_NmiBfr_t*  bfrPtr    ///< [OUT] Terminal Adaptor buffer of unsolicited result codes mode.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the Preferred Message Format (PDU or Text mode).
 *
 * @return LE_FAULT         The function failed to sets the Preferred Message Format.
 * @return LE_TIMEOUT       No response was received from the Modem.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetMsgFormat
(
    le_sms_Format_t   format   ///< [IN] The preferred message format.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function saves the SMS Settings.
 *
 * @return LE_FAULT        The function failed to save the SMS Settings.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SaveSettings
(
    void
);

#endif // LEGATO_PASMSLOCAL_INCLUDE_GUARD
