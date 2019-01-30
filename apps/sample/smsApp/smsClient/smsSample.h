#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function sends a text message using Legato SMS APIs.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmo_SendMessage
(
    const char*   destinationPtr, ///< [IN] The destination number.
    const char*   textPtr,        ///< [IN] The SMS message text.
    const bool    async
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sends a text message using Legato AT commands APIs.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmo_SendMessageAT
(
    const char*   destinationPtr, ///< [IN] The destination number.
    const char*   textPtr         ///< [IN] The SMS message text.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function installs an handler for message reception.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_Receiver
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Checks the contents of a text message for specific pre-defined commands. If command is recognized
 * a reply will be sent to satisfy the request.
 *
 */
//--------------------- -----------------------------------------------------------------------------
le_result_t decodeMsgRequest
(
    char* tel,
    char* text
);

//--------------------------------------------------------------------------------------------------
/**
 * This function installs a handler for full storage indication.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_MonitorStorage
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes the handler for message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
void smsmt_HandlerRemover
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes the handler for full storage indication.
 *
 */
//--------------------------------------------------------------------------------------------------
void smsmt_StorageHandlerRemover
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function installs a handler for listing received messages and displays the first message.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_ListMessages
(
    void
);
