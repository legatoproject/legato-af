/**
 * @file le_rsim_local.h
 *
 * Local Remote SIM service Definitions
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_RSIM_LOCAL_INCLUDE_GUARD
#define LEGATO_RSIM_LOCAL_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * SAP message identifiers (cf. SIM Access Profile specification section 1.13)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_MSGID_CONNECT_REQ                       0x00
#define SAP_MSGID_CONNECT_RESP                      0x01
#define SAP_MSGID_DISCONNECT_REQ                    0x02
#define SAP_MSGID_DISCONNECT_RESP                   0x03
#define SAP_MSGID_DISCONNECT_IND                    0x04
#define SAP_MSGID_TRANSFER_APDU_REQ                 0x05
#define SAP_MSGID_TRANSFER_APDU_RESP                0x06
#define SAP_MSGID_TRANSFER_ATR_REQ                  0x07
#define SAP_MSGID_TRANSFER_ATR_RESP                 0x08
#define SAP_MSGID_POWER_SIM_OFF_REQ                 0x09
#define SAP_MSGID_POWER_SIM_OFF_RESP                0x0A
#define SAP_MSGID_POWER_SIM_ON_REQ                  0x0B
#define SAP_MSGID_POWER_SIM_ON_RESP                 0x0C
#define SAP_MSGID_RESET_SIM_REQ                     0x0D
#define SAP_MSGID_RESET_SIM_RESP                    0x0E
#define SAP_MSGID_TRANSFER_CARD_READER_STATUS_REQ   0x0F
#define SAP_MSGID_TRANSFER_CARD_READER_STATUS_RESP  0x10
#define SAP_MSGID_STATUS_IND                        0x11
#define SAP_MSGID_ERROR_RESP                        0x12
#define SAP_MSGID_SET_TRANSPORT_PROTOCOL_REQ        0x13
#define SAP_MSGID_SET_TRANSPORT_PROTOCOL_RESP       0x14

//--------------------------------------------------------------------------------------------------
/**
 * SAP parameters identifiers (cf. SIM Access Profile specification section 5.2)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_PARAMID_MAX_MSG_SIZE            0x00
#define SAP_PARAMID_CONNECTION_STATUS       0x01
#define SAP_PARAMID_RESULT_CODE             0x02
#define SAP_PARAMID_DISCONNECTION_TYPE      0x03
#define SAP_PARAMID_COMMAND_APDU            0x04
#define SAP_PARAMID_COMMAND_APDU_7816       0x10
#define SAP_PARAMID_RESPONSE_APDU           0x05
#define SAP_PARAMID_ATR                     0x06
#define SAP_PARAMID_CARD_READER_STATUS      0x07
#define SAP_PARAMID_STATUS_CHANGE           0x08
#define SAP_PARAMID_TRANSPORT_PROTOCOL      0x09

//--------------------------------------------------------------------------------------------------
/**
 * Length in bytes of different parameters of SAP messages (cf. SIM Access Profile specification).
 * Only parameters with fixed lengths are listed here
 */
//--------------------------------------------------------------------------------------------------
#define SAP_LENGTH_SAP_HEADER           4   ///< 4 bytes header for SAP messages
#define SAP_LENGTH_PARAM_HEADER         4   ///< 4 bytes header for each parameter
#define SAP_LENGTH_MAX_MSG_SIZE         2
#define SAP_LENGTH_CONNECTION_STATUS    1
#define SAP_LENGTH_RESULT_CODE          1
#define SAP_LENGTH_DISCONNECTION_TYPE   1
#define SAP_LENGTH_CARD_READER_STATUS   1
#define SAP_LENGTH_STATUS_CHANGE        1
#define SAP_LENGTH_TRANSPORT_PROTOCOL   1
#define SAP_LENGTH_PARAM_PAYLOAD        4   ///< Parameter payload is 4 bytes long with padding
#define SAP_LENGTH_PARAM                (SAP_LENGTH_PARAM_HEADER + SAP_LENGTH_PARAM_PAYLOAD)

//--------------------------------------------------------------------------------------------------
/**
 * SAP ConnectionStatus values (cf. SIM Access Profile specification section 5.2.2)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_CONNSTATUS_OK               0x00    ///< OK, Server can fulfill requirements
#define SAP_CONNSTATUS_SERVER_NOK       0x01    ///< Error, Server unable to establish connection
#define SAP_CONNSTATUS_MAXMSGSIZE_NOK   0x02    ///< Error, Server does not support
                                                ///< maximum message size
#define SAP_CONNSTATUS_SMALL_MAXMSGSIZE 0x03    ///< Error, maximum message size
                                                ///< by Client is too small
#define SAP_CONNSTATUS_OK_ONGOING_CALL  0x04    ///< OK, ongoing call

//--------------------------------------------------------------------------------------------------
/**
 * SAP DisconnectionType values (cf. SIM Access Profile specification section 5.2.3)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_DISCONNTYPE_GRACEFUL        0x00    ///< Graceful
#define SAP_DISCONNTYPE_IMMEDIATE       0x01    ///< Immediate

//--------------------------------------------------------------------------------------------------
/**
 * SAP ResultCode values (cf. SIM Access Profile specification section 5.2.4)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_RESULTCODE_OK                   0x00    ///< OK, request processed correctly
#define SAP_RESULTCODE_ERROR_NO_REASON      0x01    ///< Error, no reason defined
#define SAP_RESULTCODE_ERROR_CARD_NOK       0x02    ///< Error, card not accessible
#define SAP_RESULTCODE_ERROR_CARD_OFF       0x03    ///< Error, card (already) powered off
#define SAP_RESULTCODE_ERROR_CARD_REMOVED   0x04    ///< Error, card removed
#define SAP_RESULTCODE_ERROR_CARD_ON        0x05    ///< Error, card already powered on
#define SAP_RESULTCODE_ERROR_NO_DATA        0x06    ///< Error, data not available
#define SAP_RESULTCODE_ERROR_NOT_SUPPORTED  0x07    ///< Error, not supported

//--------------------------------------------------------------------------------------------------
/**
 * SAP StatusChange values (cf. SIM Access Profile specification section 5.2.8)
 */
//--------------------------------------------------------------------------------------------------
#define SAP_STATUSCHANGE_UNKNOWN_ERROR  0x00    ///< Unknown Error
#define SAP_STATUSCHANGE_CARD_RESET     0x01    ///< Card reset
#define SAP_STATUSCHANGE_CARD_NOK       0x02    ///< Card not accessible
#define SAP_STATUSCHANGE_CARD_REMOVED   0x03    ///< Card removed
#define SAP_STATUSCHANGE_CARD_INSERTED  0x04    ///< Card inserted
#define SAP_STATUSCHANGE_CARD_RECOVERED 0x05    ///< Card recovered

//--------------------------------------------------------------------------------------------------
/**
 * Bit shift to access MSB byte
 */
//--------------------------------------------------------------------------------------------------
#define MSB_SHIFT   8


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Remote SIM service
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_rsim_Init
(
    void
);


#endif // LEGATO_RSIM_LOCAL_INCLUDE_GUARD

