//-------------------------------------------------------------------------------------------------
/**
 * @file cm_sms.c
 *
 * Handle SMS related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_sms.h"
#include "cm_common.h"

//-------------------------------------------------------------------------------------------------
/**
 * Print the SMS help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_PrintSmsHelp
(
    void
)
{
    printf("SMS usage\n"
            "=========\n\n"
            "To monitor incoming SMS:\n"
            "\tcm sms monitor\n\n"
            "To send a text SMS:\n"
            "\tcm sms send <number> <content>\n\n"
            "To send a binary SMS:\n"
            "\tcm sms sendbin <number> <file> <optional max sms>\n\n"
            "To list all stored SMS:\n"
            "\tcm sms list\n\n"
            "To get specific stored SMS:\n"
            "\tcm sms get <idx>\n\n"
            "To clear stored SMS:\n"
            "\tcm sms clear\n\n"
            "To count stored SMS:\n"
            "\tcm sms count\n\n"
            "Options:\n"
            "\t<number>: Destination number\n"
            "\t<content>: Text is encoded in ASCII format (ISO8859-15) and"
            " characters have to exist in the GSM 23.038 7 bit alphabet\n"
            "\t<file>: File path OR - for standard input (stdin)\n"
            "\t<optional max sms>: (Optional) Limit for the number of SMS the file is split in\n"
            );
}

//-------------------------------------------------------------------------------------------------
/**
 * Structure to buffer received content.
 */
//-------------------------------------------------------------------------------------------------
typedef union {
    char     text[LE_SMS_TEXT_MAX_BYTES];
    uint8_t  binary[LE_SMS_BINARY_MAX_BYTES];
    uint8_t  pdu[LE_SMS_PDU_MAX_BYTES];
    uint16_t ucs2[LE_SMS_UCS2_MAX_CHARS];
}
SmsContent_t;

//-------------------------------------------------------------------------------------------------
/**
 * Structure to hold the context of PrintMessage function.
 */
//-------------------------------------------------------------------------------------------------
typedef struct {
    int  nbSms;                 ///< Message counter
    bool shouldDeleteMessages;  ///< Whether the handler should delete the message or not ?
    int  msgToPrint;            ///< Index of message to print (-1 for all)
}
PrintMessageContext_t;

//-------------------------------------------------------------------------------------------------
/**
 * Helper function to print an array of binary data (hexdump like).
 */
//-------------------------------------------------------------------------------------------------
static void PrintUCS2Data
(
    const uint16_t * dataPtr,
    size_t dataNb
)
{
    int i;
    const char * paddingPtr = "       ";

    printf("%s", paddingPtr);

    for(i = 0; i < dataNb; i++)
    {
        printf("%04X ", dataPtr[i]);

        switch(i % 8)
        {
            case 3:
                printf("  ");
                break;

            case 7:
                printf("\n%s", paddingPtr);
                break;
        }
    }

    printf("\n");
}


//-------------------------------------------------------------------------------------------------
/**
 * Helper function to print an array of binary data (hexdump like).
 */
//-------------------------------------------------------------------------------------------------
static void PrintBinaryData
(
    const uint8_t * dataPtr,
    size_t dataSz
)
{
    int i;
    const char * paddingPtr = "       ";

    printf("%s", paddingPtr);

    for(i = 0; i < dataSz; i++)
    {
        printf("%02X ", dataPtr[i]);

        switch(i % 16)
        {
            case 7:
                printf("  ");
                break;

            case 15:
                printf("\n%s", paddingPtr);
                break;
        }
    }

    printf("\n");
}

//-------------------------------------------------------------------------------------------------
/**
 * Message handler used to print a single message, and eventually delete it from storage.
 */
//-------------------------------------------------------------------------------------------------
static void PrintMessage
(
    le_sms_MsgRef_t msgRef, ///< [IN] Message ref
    void* contextPtr        ///< [IN] Context
)
{
    PrintMessageContext_t * msgContextPtr = (PrintMessageContext_t *)(contextPtr);
    le_result_t res;
    le_sms_Type_t smsType;
    le_sms_Format_t format;
    size_t length, contentSz;
    SmsContent_t content;
    char header[20];

    if ( (msgContextPtr->msgToPrint != -1) &&
         (msgContextPtr->msgToPrint != msgContextPtr->nbSms) )
    {
        /* Skipping message */
        msgContextPtr->nbSms++;
        return;
    }

    printf("--[%2u]---------------------------------------------------------------\n", msgContextPtr->nbSms);

    smsType = le_sms_GetType(msgRef);
    switch (smsType)
    {
        case LE_SMS_TYPE_RX:
            cm_cmn_FormatPrint(" Type", "LE_SMS_TYPE_RX");
            break;

        case LE_SMS_TYPE_BROADCAST_RX:
            cm_cmn_FormatPrint(" Type", "LE_SMS_TYPE_BROADCAST_RX");
            break;

        case LE_SMS_TYPE_STATUS_REPORT:
            cm_cmn_FormatPrint(" Type", "LE_SMS_TYPE_STATUS_REPORT");
            break;

        default:
            cm_cmn_FormatPrint(" Type", "Unexpected");
            break;
    }

    res = le_sms_GetSenderTel(msgRef, content.text, sizeof(content.text));
    if (res == LE_OK)
    {
        cm_cmn_FormatPrint(" Sender", content.text);
    }

    res = le_sms_GetTimeStamp(msgRef, content.text, sizeof(content.text));
    if (res == LE_OK)
    {
        cm_cmn_FormatPrint(" Timestamp", content.text);
    }

    format = le_sms_GetFormat(msgRef);
    switch (format)
    {
        case LE_SMS_FORMAT_TEXT:
        {
            cm_cmn_FormatPrint(" Format", "LE_SMS_FORMAT_TEXT");
            res = le_sms_GetText(msgRef, content.text, sizeof(content.text));
            LE_ASSERT(res == LE_OK);

            length = le_sms_GetUserdataLen(msgRef);

            snprintf(header, sizeof(header), " Text (%zd)", length);
            cm_cmn_FormatPrint(header, content.text);
            break;
        }

        case LE_SMS_FORMAT_BINARY:
        {
            cm_cmn_FormatPrint(" Format", "LE_SMS_FORMAT_BINARY");
            contentSz = sizeof(content.binary);
            res = le_sms_GetBinary(msgRef, content.binary, &contentSz);
            LE_ASSERT(res == LE_OK);

            length = le_sms_GetUserdataLen(msgRef);

            snprintf(header, sizeof(header), " Binary (%zd)", length);
            cm_cmn_FormatPrint(header, "");
            PrintBinaryData(content.binary, contentSz);
            break;
        }

        case LE_SMS_FORMAT_UNKNOWN:
        case LE_SMS_FORMAT_PDU:
        {
            if (LE_SMS_FORMAT_PDU == format)
            {
                cm_cmn_FormatPrint(" Format", "LE_SMS_FORMAT_PDU");
            }
            else
            {
                cm_cmn_FormatPrint(" Format", "LE_SMS_FORMAT_UNKNOWN");
            }
            contentSz = sizeof(content.pdu);

            res = le_sms_GetPDU(msgRef, content.pdu, &contentSz);
            LE_ASSERT(res == LE_OK);

            length = le_sms_GetPDULen(msgRef);

            snprintf(header, sizeof(header), " PDU (%zd)", length);
            cm_cmn_FormatPrint(header, "");
            PrintBinaryData(content.pdu, length);
            break;
        }

        case LE_SMS_FORMAT_UCS2:
        {
            cm_cmn_FormatPrint(" Format", "LE_SMS_FORMAT_UCS2");
            contentSz = sizeof(content.ucs2) / 2;

            res = le_sms_GetUCS2(msgRef, content.ucs2, &contentSz);
            LE_ASSERT(res == LE_OK);

            length = le_sms_GetUserdataLen(msgRef);

            snprintf(header, sizeof(header), " UserDataLen (%zd)", length);
            cm_cmn_FormatPrint(header, "UCS2");
            PrintUCS2Data(content.ucs2, length);
        }
        break;

        default:
        {
            fprintf(stderr, "Invalid format '%i'\n", format);
            exit(EXIT_FAILURE);
        }
    }

    if (msgContextPtr->shouldDeleteMessages)
    {
        res = le_sms_DeleteFromStorage(msgRef);
        LE_ASSERT((LE_OK == res) || (LE_NO_MEMORY == res));

        le_sms_Delete(msgRef);
    }

    msgContextPtr->nbSms++;
}

//-------------------------------------------------------------------------------------------------
/**
 * Monitor incoming messages.
 *
 * @warning Doesn't return.
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_Monitor
(
    void
)
{
    static PrintMessageContext_t context = {
        .nbSms = 0,
        .shouldDeleteMessages = true,
        .msgToPrint = -1,
    };

    le_sms_AddRxMessageHandler(PrintMessage, &context);
}

//-------------------------------------------------------------------------------------------------
/**
 * Send an SMS with the default alphabet (text).
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_SendText
(
    const char * numberPtr,     ///< [IN] Destination number
    const char * contentPtr     ///< [IN] Text content
)
{
    le_sms_MsgRef_t msgRef;
    le_result_t result;
    uint32_t numLen = strlen(numberPtr);
    uint32_t smsLen = strlen(contentPtr);

    if (numLen == 0)
    {
        fprintf(stderr, "ERROR: Phone number can't be empty\n");
        exit(EXIT_FAILURE);
    }
    else if (numLen > (LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1))
    {
        fprintf(stderr, "ERROR: Too large phone number. Max allowed: %d digits, Provided: %d digits\n",
                LE_MDMDEFS_PHONE_NUM_MAX_BYTES-1,
                numLen);
        exit(EXIT_FAILURE);
    }

    if (smsLen == 0)
    {
        fprintf(stderr, "ERROR: SMS can't be empty\n");
        exit(EXIT_FAILURE);
    }
    else if (smsLen > (LE_SMS_TEXT_MAX_BYTES-1))
    {
        fprintf(stderr, "ERROR: Too large sms. Max allowed: %d characters, Provided: %d characters\n",
                LE_SMS_TEXT_MAX_BYTES-1,
                smsLen);
        exit(EXIT_FAILURE);
    }

    msgRef = le_sms_Create();

    result = le_sms_SetDestination(msgRef, numberPtr);
    LE_ASSERT(result == LE_OK);

    result = le_sms_SetText(msgRef, contentPtr);
    LE_ASSERT(result == LE_OK);

    result = le_sms_Send(msgRef);
    if (result != LE_OK)
    {
        fprintf(stderr, "ERROR: Failed to send SMS. Please see log for details\n");
        exit(EXIT_FAILURE);
    }

    le_sms_Delete(msgRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Send an SMS with binary content.
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_SendBinary
(
    const char * numberPtr,     ///< [IN] Destination number
    const uint8_t * contentPtr, ///< [IN] Binary content
    size_t contentLen           ///< [IN] Content length
)
{
    le_sms_MsgRef_t msgRef;
    le_result_t result;

    msgRef = le_sms_Create();

    result = le_sms_SetDestination(msgRef, numberPtr);
    LE_ASSERT(result == LE_OK);

    result = le_sms_SetBinary(msgRef, contentPtr, contentLen);
    LE_ASSERT(result == LE_OK);

    result = le_sms_Send(msgRef);
    if (result != LE_OK)
    {
        fprintf(stderr, "Error while sending SMS\n");
        exit(EXIT_FAILURE);
    }

    le_sms_Delete(msgRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Execute function for all received messages.
 *
 * @return The number of messages processed.
 */
//-------------------------------------------------------------------------------------------------
static int ForEachMessage
(
    le_sms_RxMessageHandlerFunc_t handlerPtr,   //!< [IN] Callback function
    void * contextPtr                           //!< [IN] Callback context
)
{
    le_sms_MsgListRef_t listRef = NULL;
    le_sms_MsgRef_t msgRef = NULL;
    int nbSms = 0;

    /* Get the ptr of SMS list */
    listRef = le_sms_CreateRxMsgList();
    if (listRef == NULL)
    {
        return 0;
    }

    msgRef = le_sms_GetFirst(listRef);

    do {
        if (msgRef == NULL)
        {
            break;
        }

        nbSms++;

        if (handlerPtr != NULL)
        {
            handlerPtr(msgRef, contextPtr);
        }
    }
    while ( (msgRef = le_sms_GetNext(listRef)) != NULL);

    /* Delete the SMS list object */
    le_sms_DeleteList(listRef);

    return nbSms;
}


//-------------------------------------------------------------------------------------------------
/**
 * Read all messages
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_ListAllMessages
(
    void
)
{
    PrintMessageContext_t context = {
        .nbSms = 0,
        .shouldDeleteMessages = false,
        .msgToPrint = -1,
    };

    ForEachMessage(PrintMessage, &context);
}

//-------------------------------------------------------------------------------------------------
/**
 * Read one message
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_GetMessage
(
    int index       ///< [IN] Message index
)
{
    PrintMessageContext_t context = {
        .nbSms = 0,
        .shouldDeleteMessages = false,
        .msgToPrint = index,
    };

    ForEachMessage(PrintMessage, &context);

    if (context.nbSms <= index)
    {
        fprintf(stderr, "Unable to get message %d\n", index);
        exit(EXIT_FAILURE);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Callback function used by cm_sms_ClearAllMessages to clear one message.
 */
//-------------------------------------------------------------------------------------------------
static void ClearOneMessage
(
    le_sms_MsgRef_t msgRef,
    void * contextPtr
)
{
    le_result_t res = LE_FAULT;
    int * nbSmsPtr = (int *)contextPtr;

    res = le_sms_DeleteFromStorage(msgRef);
    if (res != LE_OK)
    {
        fprintf(stderr, "Unable to remove SMS '%d'\n", *nbSmsPtr);
        exit(EXIT_FAILURE);
    }

    ++(*nbSmsPtr);
}

//-------------------------------------------------------------------------------------------------
/**
 * Clear all messages
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_ClearAllMessages
(
    void
)
{
    int nbSms = 0;

    nbSms = ForEachMessage(ClearOneMessage, &nbSms);

    if (nbSms == 0)
    {
        printf("No stored SMS.\n");
    }
    else
    {
        printf("Removed %d SMS message%s.\n",
            nbSms, (nbSms == 1) ? "" : "s");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Count all messages
 */
//-------------------------------------------------------------------------------------------------
void cm_sms_CountAllMessages
(
    void
)
{
    int nbSms;

    nbSms = ForEachMessage(NULL, NULL);

    printf("%d\n", nbSms);
}

//-------------------------------------------------------------------------------------------------
/**
 * Handle the 'sendbin' command.
 */
//-------------------------------------------------------------------------------------------------
static void HandleSendBin
(
    size_t numArgs          ///< [IN] Number of arguments
)
{
    FILE * filePtr;
    uint8_t content[LE_SMS_BINARY_MAX_BYTES];
    ssize_t contentLen = 0;
    int index = 0;
    int maxCountSms = CMODEM_SMS_DEFAULT_MAX_BIN_SMS;

    const char* number = le_arg_GetArg(2);
    if (NULL == number)
    {
        LE_ERROR("number is NULL");
        exit(EXIT_FAILURE);
    }

    const char* filePath = le_arg_GetArg(3);
    if (NULL == filePath)
    {
        LE_ERROR("filePath is NULL");
        exit(EXIT_FAILURE);
    }

    if (numArgs > 4)
    {
        const char* arg = le_arg_GetArg(4);
        if (NULL == arg)
        {
            LE_ERROR("arg is NULL");
            exit(EXIT_FAILURE);
        }

        maxCountSms = atoi(arg);

        if (maxCountSms <= 0)
        {
            fprintf(stderr, "Invalid max sms limit '%s'\n", arg);
            exit(EXIT_FAILURE);
        }

        printf("Limiting to %d SMS\n", maxCountSms);
    }

    if (0 == strcmp(filePath, "-"))
    {
        printf("From stdin ...\n");
        filePtr = stdin;
    }
    else
    {
        printf("From '%s'\n", filePath);
        filePtr = fopen(filePath, "r");
        if (filePtr == NULL)
        {
            fprintf(stderr, "Unable to open file '%s': %s\n", filePath, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    do {
        contentLen = fread(content, sizeof(uint8_t), sizeof(content), filePtr);

        if ((-1 == contentLen) || (0 == contentLen))
        {
            fprintf(stderr, "Error reading input: %s\n", strerror(errno));
            fclose(filePtr);
            exit(EXIT_FAILURE);
        }
        else if ((contentLen < sizeof(content)) && (0x0A == content[contentLen-1]))
        {
            contentLen--;
        }

        if (contentLen <= 0)
        {
            fprintf(stderr, "Nothing to send\n");
            fclose(filePtr);
            exit(EXIT_SUCCESS);
        }

        printf("Sending '%d': length[%zd]\n", index, contentLen);
        PrintBinaryData(content, contentLen);

        cm_sms_SendBinary(number, content, contentLen);

        if (contentLen < sizeof(content))
        {
            printf("Done\n");
            fclose(filePtr);
            exit(EXIT_FAILURE);
        }

        index++;
    }
    while ((contentLen > 0) && (index < maxCountSms));

    if (0 != strcmp(filePath, "-"))
    {
        fclose(filePtr);
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for SMS service.
 */
//--------------------------------------------------------------------------------------------------
void cm_sms_ProcessSmsCommand
(
    const char * command,   ///< [IN] Command
    size_t numArgs          ///< [IN] Number of arguments
)
{

    if (strcmp(command, "help") == 0)
    {
        cm_sms_PrintSmsHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "monitor") == 0)
    {
        cm_sms_Monitor();
    }
    else if (strcmp(command, "send") == 0)
    {
        cm_cmn_CheckEnoughParams(2, numArgs, "Destination or content missing. e.g. cm sms send <number> <content>");

        const char* number = le_arg_GetArg(2);
        if (NULL == number)
        {
            LE_ERROR("number is NULL");
            exit(EXIT_FAILURE);
        }
        const char* content = le_arg_GetArg(3);
        if (NULL == content)
        {
            LE_ERROR("content is NULL");
            exit(EXIT_FAILURE);
        }

        cm_sms_SendText(number, content);

        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "sendbin") == 0)
    {
        cm_cmn_CheckEnoughParams(2, numArgs, "Destination or content missing. e.g. cm sms sendbin <number> <file> <optional max sms>");

        HandleSendBin(numArgs);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "list") == 0)
    {
        cm_sms_ListAllMessages();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "get") == 0)
    {
        cm_cmn_CheckEnoughParams(1, numArgs, "Index of message missing. e.g. cm sms get <idx>");

        const char* indexStr = le_arg_GetArg(2);
        if (NULL == indexStr)
        {
            LE_ERROR("indexStr is NULL");
            exit(EXIT_FAILURE);
        }
        int index = atoi(indexStr);

        cm_sms_GetMessage(index);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "clear") == 0)
    {
        cm_sms_ClearAllMessages();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "count") == 0)
    {
        cm_sms_CountAllMessages();
        exit(EXIT_SUCCESS);
    }
    else
    {
        printf("Invalid command for SMS service.\n");
        exit(EXIT_FAILURE);
    }
}
