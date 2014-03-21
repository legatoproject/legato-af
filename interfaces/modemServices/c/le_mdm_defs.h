/** @page c_mdm_apis Modem Services
 *
 * This file contains common definitions for the basic high-level APIs and Platform
 * Adapter APIs.
 *
 * @section c_mdm_apiList List of High-Level Modem APIs
 *
 *  - @subpage c_sms
 *  - @subpage c_sim
 *  - @subpage c_mdc
 *  - @subpage c_mrc
 *  - @subpage c_mcc
 *  - @subpage c_info
 *  - @subpage c_remoteMgmt
 *
 */

/** @file le_mdm_defs.h
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MDMDEFS_INCLUDE_GUARD
#define LEGATO_MDMDEFS_INCLUDE_GUARD

#include "legato.h"



//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                          SIM
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * SIM states.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SIM_INSERTED      = 0, ///< SIM card is inserted and locked.
    LE_SIM_ABSENT        = 1, ///< SIM card is absent.
    LE_SIM_READY         = 2, ///< SIM card is inserted and unlocked.
    LE_SIM_BLOCKED       = 3, ///< SIM card is blocked.
    LE_SIM_BUSY          = 4, ///< SIM card is busy.
    LE_SIM_STATE_UNKNOWN = 5, ///< Unknown SIM state.
}
le_sim_States_t;


//--------------------------------------------------------------------------------------------------
//                                          MRC
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Network Registration states.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MRC_REG_NONE      = 0, ///< Not registered and not currently searching for new operator.
    LE_MRC_REG_HOME      = 1, ///< Registered, home network.
    LE_MRC_REG_SEARCHING = 2, ///< Not registered but currently searching for a new operator.
    LE_MRC_REG_DENIED    = 3, ///< Registration was denied, usually because of invalid access credentials.
    LE_MRC_REG_ROAMING   = 5, ///< Registered to a roaming network.
    LE_MRC_REG_UNKNOWN   = 4, ///< Unknown state.
}
le_mrc_NetRegState_t;

//--------------------------------------------------------------------------------------------------
//                                          MCC
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Cf. ITU-T recommendations E.164/E.163. E.164 numbers can have a maximum of 15 digits except the
 * international prefix (‘+’ or '00'). One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_TEL_NMBR_MAX_LEN      (15+2+1)


//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible events that may be reported to a call event handler.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_EVENT_INCOMING,    ///< Incoming call attempt (new call).
    LE_MCC_EVENT_ALERTING,    ///< Far end is now alerting its user (outgoing call).
    LE_MCC_EVENT_EARLY_MEDIA, ///< Callee has not accepted the call, but a media stream
                              ///< is available.
    LE_MCC_EVENT_CONNECTED,   ///< Call has been established, and is media is active.
    LE_MCC_EVENT_TERMINATED,  ///< Call has terminated.
    LE_MCC_EVENT_ON_HOLD,     ///< Remote party has put the call on hold.
    LE_MCC_EVENT_TRANSFERED,  ///< Remote party transferred or forwarded the call.
}
le_mcc_call_Event_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Enumeration of the possible reasons for call termination.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_MCC_TERM_NETWORK_FAIL,  ///< Network could not complete the call.
    LE_MCC_TERM_BAD_ADDRESS,   ///< Remote address could not be resolved.
    LE_MCC_TERM_BUSY,          ///< Callee is currently busy and cannot take the call.
    LE_MCC_TERM_LOCAL_ENDED,   ///< Local party ended the call.
    LE_MCC_TERM_REMOTE_ENDED,  ///< Remote party ended the call.
    LE_MCC_TERM_NOT_DEFINED,   ///< Undefined reason.
}
le_mcc_call_TerminationReason_t;


//--------------------------------------------------------------------------------------------------
//                                          SMS
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Cf. ITU-T recommendations E.164/E.163. E.164 numbers can have a maximum of 15 digits and are
 * usually written with a ‘+’ prefix. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_TEL_NMBR_MAX_LEN      LE_TEL_NMBR_MAX_LEN

//--------------------------------------------------------------------------------------------------
/**
 * Time stamp string length.
 * The string format is "yy/MM/dd,hh:mm:ss+/-zz" (Year/Month/Day,Hour:Min:Seconds+/-TimeZone).
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_TIMESTAMP_MAX_LEN     (20+1)

//--------------------------------------------------------------------------------------------------
/**
 * The text message can be up to 160 characters long.
 * One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_TEXT_MAX_LEN           (160+1)

//--------------------------------------------------------------------------------------------------
/**
 * The raw binary message can be up to 140 bytes long.
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_BINARY_MAX_LEN           (140)

//--------------------------------------------------------------------------------------------------
/**
 * The PDU message can be up to 36 (header) + 140 (payload) bytes long.
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SMS_PDU_MAX_LEN           (36+140)

//--------------------------------------------------------------------------------------------------
/**
 * Message Format.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_FORMAT_PDU     = 0,  ///< PDU message format.
    LE_SMS_FORMAT_TEXT    = 1,  ///< Text message format.
    LE_SMS_FORMAT_BINARY  = 2,  ///< Binary message format.
    LE_SMS_FORMAT_UNKNOWN = 3,  ///< Unknown message format.
}
le_sms_msg_Format_t;

//--------------------------------------------------------------------------------------------------
/**
 * Message Status.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SMS_RX_READ        = 0,  ///< Message present in the message storage has been read.
    LE_SMS_RX_UNREAD      = 1,  ///< Message present in the message storage has not been read.
    LE_SMS_STORED_SENT    = 2,  ///< Message saved in the message storage has been sent.
    LE_SMS_STORED_UNSENT  = 3,  ///< Message saved in the message storage has not been sent.
    LE_SMS_SENT           = 4,  ///< Message has been sent.
    LE_SMS_UNSENT         = 5,  ///< Message has not been sent.
    LE_SMS_STATUS_UNKNOWN = 6,  ///< Unknown message status.
}
le_sms_msg_Status_t;


#endif // LEGATO_MDMDEFS_INCLUDE_GUARD
