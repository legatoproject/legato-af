/**
 * @file pa_sms_default.c
 *
 * Default implementation of @ref c_pa_sms.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_sms.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to set the preferred SMS storage area.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetPreferredStorage
(
    le_sms_Storage_t prefStorage
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to get the preferred SMS storage area.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetPreferredStorage
(
    le_sms_Storage_t* prefStoragePtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for a new message reception handling.
 *
 * @return LE_FAULT         The function failed to register a new handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetNewMsgHandler
(
    pa_sms_NewMsgHdlrFunc_t msgHandler   ///< [IN] The handler function to handle a new message
                                         ///       reception.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add a status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_sms_AddStorageStatusHandler
(
    pa_sms_StorageMsgHdlrFunc_t statusHandler   ///< [IN] The handler function to handle a new status
                                                ///  notification
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for a new message reception handling.
 *
 * @return LE_FAULT         The function failed to unregister the handler.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearNewMsgHandler
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called to unregister from a storage message notification handler.
 */
//--------------------------------------------------------------------------------------------------
void pa_sms_RemoveStorageStatusHandler
(
    le_event_HandlerRef_t storageSMSHandler
)
{
    LE_ERROR("Unsupported function called");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function sends a message in PDU mode.
 *
 * @return LE_OK              The function succeeded.
 * @return LE_FAULT           The function failed to send a message in PDU mode.
 * @return LE_TIMEOUT         No response was received from the Modem.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SendPduMsg
(
    pa_sms_Protocol_t        protocol,   ///< [IN] protocol to use
    uint32_t                 length,     ///< [IN] The length of the TP data unit in bytes.
    const uint8_t*           dataPtr,    ///< [IN] The message.
    uint8_t*                 msgRef,     ///< [OUT] Message reference (TP-MR)
    uint32_t                 timeout,    ///< [IN] Timeout in seconds.
    pa_sms_SendingErrCode_t* errorCode   ///< [OUT] The error code.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the message from the preferred message storage.
 *
 * @return LE_FAULT        The function failed to get the message from the preferred message
 *                         storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RdPDUMsgFromMem
(
    uint32_t            index,      ///< [IN]  The place of storage in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol used for this message
    pa_sms_Storage_t    storage,    ///< [IN] SMS Storage used
    pa_sms_Pdu_t*       msgPtr      ///< [OUT] The message.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the indexes of messages stored in the preferred memory for a specific
 * status.
 *
 * @return LE_FAULT          The function failed to get the indexes of messages stored in the
 *                           preferred memory.
 * @return LE_BAD_PARAMETER  The parameters are invalid.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ListMsgFromMem
(
    le_sms_Status_t     status,     ///< [IN] The status of message in memory.
    pa_sms_Protocol_t   protocol,   ///< [IN] The protocol to read
    uint32_t           *numPtr,     ///< [OUT] The number of indexes retrieved.
    uint32_t           *idxPtr,     ///< [OUT] The pointer to an array of indexes.
                                    ///        The array is filled with 'num' index values.
    pa_sms_Storage_t    storage     ///< [IN] SMS Storage used
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes one specific Message from preferred message storage.
 *
 * @return LE_FAULT          The function failed to delete one specific Message from preferred
 *                           message storage.
 * @return LE_TIMEOUT        No response was received from the Modem.
 * @return LE_OK             The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelMsgFromMem
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes all Messages from preferred message storage.
 *
 * @return LE_FAULT        The function failed to delete all Messages from preferred message storage.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DelAllMsg
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function changes the message status.
 *
 * @return LE_FAULT        The function failed to change the message status.
 * @return LE_TIMEOUT      No response was received from the Modem.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ChangeMessageStatus
(
    uint32_t            index,    ///< [IN] Index of the message to be deleted.
    pa_sms_Protocol_t   protocol, ///< [IN] protocol
    le_sms_Status_t     status,   ///< [IN] The status of message in memory.
    pa_sms_Storage_t    storage   ///< [IN] SMS Storage used
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the SMS center.
 *
 * @return
 *   - LE_FAULT        The function failed.
 *   - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_GetSmsc
(
    char*        smscPtr,  ///< [OUT] The Short message service center string.
    size_t       len       ///< [IN] The length of SMSC string.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the SMS center.
 *
 * @return
 *   - LE_FAULT        The function failed.
 *   - LE_TIMEOUT      No response was received from the Modem.
 *   - LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_SetSmsc
(
    const char*    smscPtr  ///< [IN] The Short message service center.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ActivateCellBroadcast
(
    pa_sms_Protocol_t protocol
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deactivate Cell Broadcast message notification
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_DeactivateCellBroadcast
(
    pa_sms_Protocol_t protocol
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_AddCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RemoveCellBroadcastIds
(
    uint16_t fromId,
        ///< [IN]
        ///< Starting point of the range of cell broadcast message identifier.

    uint16_t toId
        ///< [IN]
        ///< Ending point of the range of cell broadcast message identifier.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear Cell Broadcast message Identifiers range.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearCellBroadcastIds
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_AddCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_RemoveCdmaCellBroadcastServices
(
    le_sms_CdmaServiceCat_t serviceCat,
        ///< [IN]
        ///< Service category assignment. Reference to 3GPP2 C.R1001-D
        ///< v1.0 Section 9.3.1 Standard Service Category Assignments.

    le_sms_Languages_t language
        ///< [IN]
        ///< Language Indicator. Reference to
        ///< 3GPP2 C.R1001-D v1.0 Section 9.2.1 Language Indicator
        ///< Value Assignments
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear CDMA Cell Broadcast category services.
 *
 * @return
 *  - LE_FAULT         Function failed.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_sms_ClearCdmaCellBroadcastServices
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
