// -------------------------------------------------------------------------------------------------
/**
 *  SMS Inbox Server
 *
 * When the service is activated, or when a SMS is received, the SMS is copied from the SIM to a
 * specific folder (SMSINBOX_PATH/MSG_PATH).
 *
 * Each SMS is copied into a dedicated file (named with a unique message identifier). The data
 * are stored using the Jannsson representation: Each data (imsi, SMS format, message length,
 * text/pdu, sender telephone number, timestamp, read/unread) are recorded with a key to retrieve
 * each value.
 *
 * Each application using the SMS Inbox Server possesses a configuration file in
 * SMSINBOX_PATH/CONF_PATH directory (also encoding using Jansson format): this file is used to
 * store the message identifier contained in the application mailbox. It is updated each time is new
 * SMS is received.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "mdmCfgEntries.h"
#include "le_smsInbox.h"

#include "le_print.h"
#include "le_hex.h"

#include <dirent.h>
#include "jansson.h"

//--------------------------------------------------------------------------------------------------
// Symbols and enums.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * SMSInbox directory path.
 */
//--------------------------------------------------------------------------------------------------
#ifdef LEGATO_EMBEDDED
#define SMSINBOX_PATH "/data/smsInbox/"
#else
#define SMSINBOX_PATH "/tmp/smsInbox/"
#endif
#define MSG_PATH "msg/"
#define CONF_PATH "cfg/"

//--------------------------------------------------------------------------------------------------
/**
 * File extension definition.
 */
//--------------------------------------------------------------------------------------------------
#define FILE_EXTENSION ".json"

//--------------------------------------------------------------------------------------------------
/**
 * Json keys.
 */
//--------------------------------------------------------------------------------------------------
#define JSON_FORMAT "format"
#define JSON_SENDERTEL "senderTel"
#define JSON_TEXT "text"
#define JSON_PDU "pdu"
#define JSON_BIN "binary"
#define JSON_IMSI "imsi"
#define JSON_MSGLEN "msgLen"
#define JSON_TIMESTAMP "timestamp"
#define JSON_ISUNREAD "isUnread"
#define JSON_ISDELETED "isDeleted"
#define JSON_MSGINBOX "msgInBox"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of user applications.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_APPS 16

//--------------------------------------------------------------------------------------------------
/**
 * Default size of message box.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_MBOX_SIZE 10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of messages for message box.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MBOX_SIZE     100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of message box configuration path.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MBOX_CONFIG_PATH_LEN 100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of Message List objects we expect to have at one time.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_NUM_OF_LIST    MAX_APPS

//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------

#define SMSINBOX_CONFIG_TREE_ROOT_DIR   "smsInboxService:"

#define CFG_NODE_SMSINBOX               "smsInbox"
#define CFG_NODE_APPS                   "apps"

#define CFG_SMSINBOX_PATH               SMSINBOX_CONFIG_TREE_ROOT_DIR"/"CFG_NODE_SMSINBOX

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Message identifier definition.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef uint32_t MessageId_t;

//--------------------------------------------------------------------------------------------------
/**
 * Browsing structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    json_t* jsonObjPtr;
    json_t* jsonArrayPtr;
    uint32_t currentMessageIndex;
    uint32_t maxIndex;
}
BrowseCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Entry description structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    enum
    {
        DESC_INT,
        DESC_STRING,
        DESC_BOOL
    }                               type;       ///< Union type (string or integer
    union
    {
        struct
        {
            char*   strPtr;                     ///< String
            size_t  lenStr;                     ///< Length of the string
        }                  str;                 ///< Value for string
        uint32_t           val;                 ///< Value for integer
        bool               boolVal;             ///< Boolean value
    }                               uVal;       ///< Union type
}
EntryDesc_t;


//--------------------------------------------------------------------------------------------------
/**
 * message box object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char *    namePtr;                  ///< App name
    uint32_t inboxSize;                 ///< Max messages in the inbox
    uint32_t msgCount;                  ///< Number message

}
MboxCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * message box session structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    MboxCtx_t *    mboxCtxPtr; ///< message box object
    BrowseCtx_t    browseCtx;     ///< browsing context (for GetFirst/GetNext)
}
MboxSession_t;

//--------------------------------------------------------------------------------------------------
/**
 * Rx Event handler structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    SmsInbox_SessionRef_t            sessionRef; ///< Session reference.
    SmsInbox_RxMessageHandlerFunc_t  handlerPtr; ///< handler function.
    le_event_HandlerRef_t               handlerRef; ///< handler reference.
}
RxMsgReport_t;

//--------------------------------------------------------------------------------------------------
/**
 * smsInbox client request objet structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    SmsInbox_SessionRef_t       smsInboxSessionRef;///< smsInbox sessionRef store for each client
    MboxSession_t*              mboxSessionPtr;    ///< smsInbox messagebox session pointer
    le_msg_SessionRef_t         sessionRef;        ///< Client session identifier
    le_dls_Link_t               link;              ///< Object node link
}
ClientRequest_t;

//--------------------------------------------------------------------------------------------------
//                                       Extern declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * message box names.
 *
 */
//--------------------------------------------------------------------------------------------------
extern const char* le_smsInbox_mboxName[];

//--------------------------------------------------------------------------------------------------
/**
 * message boxes Number.
 *
 */
//--------------------------------------------------------------------------------------------------
extern const uint8_t le_smsInbox_NbMbx;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for the message box session context.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   MboxSessionPool;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for pool for the SMS RX handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  RxMsgReportPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for RX message report.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RxMsgReportMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New SMS message notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t RxMsgEventId;

//--------------------------------------------------------------------------------------------------
/**
 * SMS Inbox settings.
 *
 */
//--------------------------------------------------------------------------------------------------
static MboxCtx_t Apps[MAX_APPS];

//--------------------------------------------------------------------------------------------------
/**
 * Max messages in SMSInBox
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t MaxInboxSize;

//--------------------------------------------------------------------------------------------------
/**
 * SIM IMSI.
 *
 */
//--------------------------------------------------------------------------------------------------
static char SimImsi[LE_SIM_IMSI_BYTES];

//--------------------------------------------------------------------------------------------------
/**
 * Next message Identifier
 *
 */
//--------------------------------------------------------------------------------------------------
static MessageId_t NextMessageId = 1;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for SmsInbox Client Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t SmsInboxHandlerPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for service activation requests.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ActivationRequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Add a boolean value of a key in a Jason object
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddBooleanKeyInJsonObject
(
    json_t *jsonObjPtr, ///<[IN] json object
    const char* key,    ///<[IN] Key to be added
    bool value          ///<[IN] Value to be added
)
{
    json_t *jsonBoolPtr;
    int ret = -1;

    if ( value == true )
    {
        jsonBoolPtr = json_true();
    }
    else
    {
        jsonBoolPtr = json_false();
    }

    if (jsonBoolPtr)
    {
        ret = json_object_set_new(jsonObjPtr, key, jsonBoolPtr);
    }
    else
    {
        LE_ERROR("Error during set of the key %s", key);
    }

    if (ret == 0)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add an integer value of a key in a Jason object
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddIntegerKeyInJsonObject
(
    json_t *jsonObjPtr, ///<[IN] json object
    const char* key,    ///<[IN] Key to be added
    int  value          ///<[IN] Value to be added
)
{
    json_t *jsonIntPtr = json_integer(value);
    int ret = -1;

    if (jsonIntPtr)
    {
        if (json_is_array(jsonObjPtr))
        {
            ret = json_array_append_new(jsonObjPtr, jsonIntPtr);
        }
        else
        {
            ret = json_object_set_new(jsonObjPtr, key, jsonIntPtr);
        }
    }
    else
    {
        LE_ERROR("Error during set of the key %s", key);
    }

    if (ret == 0)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a string value of a key in a Jason object
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddStringKeyInJsonObject
(
    json_t *jsonObjPtr, ///<[IN] json object
    const char* key,    ///<[IN] Key to be added
    char* string        ///<[IN] String to be added
)
{
    json_t * jsonStrPtr = json_string(string);
    int ret = -1;

    if (jsonStrPtr)
    {
        ret = json_object_set_new(jsonObjPtr, key, jsonStrPtr);
    }
    else
    {
        LE_ERROR("Error during set of the key %s", key);
    }

    if (ret == 0)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SMSInbox directory path length
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GetSMSInboxMessagePathLen
(
    void
)
{
    return 2*sizeof(MessageId_t)+strlen(SMSINBOX_PATH)+strlen(MSG_PATH)+strlen(FILE_EXTENSION)+1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SMSInbox file path
 *
 */
//--------------------------------------------------------------------------------------------------
static void GetSMSInboxMessagePath
(
    MessageId_t messageId,  ///[IN] message identifier
    char* pathPtr,          ///<[OUT] path of the messageId file
    uint32_t pathLen        ///<[IN] Length of pathPtr
)
{
    snprintf(pathPtr, pathLen, "%s%s%08x%s", SMSINBOX_PATH, MSG_PATH,
                                            (int) messageId,
                                            FILE_EXTENSION);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SMSInbox configuration path length
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GetSMSInboxConfigPathLen
(
    char* appNamePtr    ///<[IN] Application name
)
{
    return strlen(appNamePtr)+strlen(SMSINBOX_PATH)+strlen(CONF_PATH)+strlen(FILE_EXTENSION)+1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the application's box file descriptor path
 *
 */
//--------------------------------------------------------------------------------------------------
static void GetSMSInboxConfigPath
(
    char* appNamePtr,   ///<[IN] Application name
    char* pathPtr,      ///<[OUT] configuration file path
    uint32_t pathLen    ///<[IN] path length
)
{
    snprintf(pathPtr, pathLen, "%s%s%s%s", SMSINBOX_PATH, CONF_PATH, appNamePtr, FILE_EXTENSION);
}

//--------------------------------------------------------------------------------------------------
/**
 * Modify Json object
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ModifyJsonObj
(
    json_t* jsonObjPtr,         ///<[IN] Json object (root object)
    json_t* jsonValPtr,         ///<[IN] Json value to be modified
    char*   key,                ///<[IN] Key to modify
    EntryDesc_t * modifPtr      ///<[IN] Requested modification
)
{
    int res = 0;

    if (jsonValPtr == NULL)
    {
        LE_ERROR("NULL jsonValPtr");
        return LE_FAULT;
    }

    switch (modifPtr->type)
    {
        case DESC_STRING:
            // check the type
            if (json_is_string(jsonValPtr) == false)
            {
                LE_ERROR("Bad key format");
                res = -1;
            }

            if ( res == 0 )
            {
                res = json_string_set(jsonValPtr, modifPtr->uVal.str.strPtr);
            }
        break;
        case DESC_INT:
            // check the type
            if (json_is_integer(jsonValPtr) == false )
            {
                LE_ERROR("Bad key format");
                res = -1;
            }

            if ( res == 0 )
            {
                res = json_integer_set(jsonValPtr, modifPtr->uVal.val);
            }
        break;
        case DESC_BOOL:
            // check the type
            if ( (json_is_true(jsonValPtr) != true) && (json_is_false(jsonValPtr) != true) )
            {
                LE_ERROR("Bad key format");
                res = -1;
            }

            if (( res == 0 ) &&
                (AddBooleanKeyInJsonObject(jsonObjPtr, key, modifPtr->uVal.boolVal) != LE_OK))
            {
                res = -1;
            }
        break;
        default:
            res = -1;
        break;
    }

    if (res < 0)
    {
        LE_ERROR(("Update error %d"), modifPtr->type);
        return LE_FAULT;
    }
    else
    {
        LE_DEBUG("ModifyJsonObj OK");
        return LE_OK;
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Modify message file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ModifyMsgEntry
(
    MessageId_t messageId,      ///<[IN] Message identifier to be modified
    char* keyPtr[],              ///<[IN] Key to modify
    uint8_t nbKey,              ///<[IN] Number of elements in keyPtr
    EntryDesc_t * modifPtr      ///<[IN] Requested modification
)
{
    uint16_t pathLen = GetSMSInboxMessagePathLen();
    char path[pathLen];
    memset(path, 0, pathLen);
    json_error_t error;
    le_result_t res;
    int indexKey = -1;

    GetSMSInboxMessagePath(messageId, path, pathLen);

    LE_DEBUG("ModifyMsgEntry: messageId %d, path %s",messageId, path);

    json_t* jsonRootPtr = json_load_file(path, 0, &error);

    if ( jsonRootPtr == NULL )
    {
        LE_ERROR("Json decoder error %s", error.text);
        return LE_FAULT;
    }

    json_t* jsonValPtr = jsonRootPtr;
    json_t* jsonSubObjPtr = jsonRootPtr;

    while ( ( json_is_object(jsonValPtr) == true ) && ( nbKey > indexKey ) )
    {
        indexKey++;
        jsonSubObjPtr = jsonValPtr;
        jsonValPtr = json_object_get(jsonSubObjPtr, keyPtr[indexKey]);
    }

    if ( !jsonValPtr )
    {
        if ((indexKey > -1) && (indexKey < nbKey))
        {
            LE_ERROR("Unable to get the object %s", keyPtr[indexKey]);
        }
        else
        {
            LE_ERROR("IndexKey: %d is out of range", indexKey);
        }

        res = LE_FAULT;
    }
    else
    {
        if ((indexKey > -1) && (indexKey < nbKey))
        {
            res = ModifyJsonObj(jsonSubObjPtr, jsonValPtr, keyPtr[indexKey], modifPtr);
        }
        else
        {
            LE_ERROR("IndexKey: %d is out of range", indexKey);
            res = LE_FAULT;
        }
    }

    if ( res == LE_OK )
    {
        if (json_dump_file(jsonRootPtr, path, JSON_INDENT(1) | JSON_PRESERVE_ORDER) < 0)
        {
            LE_ERROR("Json error");
            res = LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("Something was wrong in ModifyJsonObj");
    }

    json_decref(jsonRootPtr);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read Json object
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadJsonObj
(
    json_t* jsonValPtr,         ///<[IN] Json value to be read
    char* keyPtr[],             ///<[IN] Key to read
    uint8_t nbKey,              ///<[IN] Number of elements in keyPtr
    EntryDesc_t * decodePtr     ///<[OUT] Decoding result
)
{
    le_result_t res = LE_OK;

    if ( jsonValPtr == NULL )
    {
        LE_ERROR("No information");
        return LE_FAULT;
    }

    switch json_typeof(jsonValPtr)
    {
        case JSON_STRING:
        {
            if ( decodePtr->type != DESC_STRING )
            {
                LE_ERROR("Bad format %d", decodePtr->type);
                res = LE_FAULT;
            }

            if (res == LE_OK)
            {
                const char* strPtr = json_string_value((const json_t *)jsonValPtr);

                if ( strlen(strPtr) >= decodePtr->uVal.str.lenStr )
                {
                    LE_ERROR("String too long");
                    res = LE_OVERFLOW;
                }
                else
                {
                    strncpy(decodePtr->uVal.str.strPtr, strPtr, decodePtr->uVal.str.lenStr);
                    decodePtr->uVal.str.lenStr = strlen(strPtr);
                }
            }
        }
        break;
        case JSON_INTEGER:
        {
            if ( decodePtr->type != DESC_INT )
            {
                LE_ERROR("Bad format %d", decodePtr->type);
                res = LE_FAULT;
            }
            else
            {
                decodePtr->uVal.val = json_integer_value((const json_t *)jsonValPtr);
            }
        }
        break;
        case JSON_TRUE:
        case JSON_FALSE:
        {
            if ( decodePtr->type != DESC_BOOL )
            {
                LE_ERROR("Bad format %d", decodePtr->type);
                res = LE_FAULT;
            }
            else
            {
                decodePtr->uVal.boolVal = json_is_true((const json_t *)jsonValPtr);
            }
        }
        break;
        case JSON_OBJECT:
        {
            // if key-value is an object, read the next key
            if (nbKey >= 1)
            {
                json_t* jsonSubValPtr = json_object_get(jsonValPtr, keyPtr[0]);

                if ( jsonSubValPtr )
                {
                    res = ReadJsonObj(jsonSubValPtr, &keyPtr[1], nbKey-1,  decodePtr );
                }
                else
                {
                    LE_ERROR("Bad format");
                    res = LE_FAULT;
                }
            }
            else
            {
                LE_ERROR("Key error");
                res = LE_FAULT;
            }
        }
        break;
        case JSON_ARRAY:
        case JSON_REAL:
        case JSON_NULL:
        default:
            LE_ERROR("Bad json_typeof");
            res = LE_FAULT;
        break;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read Application's config file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetMsgListFromMbox
(
    char* pathPtr,              ///<[IN] Application's config file path
    json_t **jsonRootObjPtr,    ///<[OUT] json root object
    json_t **jsonArrayPtr       ///<[OUT] messages in box list
)
{
    json_error_t error;
    *jsonRootObjPtr = json_load_file(pathPtr, 0, &error);

    if ( !(*jsonRootObjPtr) )
    {
        // File doesn't exist, create objects
        *jsonRootObjPtr = json_object();

        if ( !(*jsonRootObjPtr) )
        {
            LE_ERROR("Json error");
            return LE_FAULT;
        }
    }

    *jsonArrayPtr = json_object_get(*jsonRootObjPtr, JSON_MSGINBOX);

    if ( !(*jsonArrayPtr) )
    {
        // Array doesn't exist, create it
        *jsonArrayPtr = json_array();

        if (!(*jsonArrayPtr))
        {
            json_decref(*jsonRootObjPtr);
            LE_ERROR("Json error");
            return LE_FAULT;
        }

        if ( json_object_set(*jsonRootObjPtr, JSON_MSGINBOX, *jsonArrayPtr) < 0 )
        {
            json_decref(*jsonRootObjPtr);
            LE_ERROR("Json error");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a message from the application's cfg file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteMsgInAppCfg
(
    char *appNamePtr,               ///<[IN] application name
    MessageId_t deleteMessageId     ///<[IN] message to delete
)
{
    uint32_t pathLen = GetSMSInboxConfigPathLen(appNamePtr);
    char path[pathLen];
    memset(path, 0, pathLen);
    GetSMSInboxConfigPath(appNamePtr, path, pathLen);
    json_t *jsonObjPtr;
    json_t *jsonArrayPtr;

    LE_DEBUG("DeleteMessageId %d, path %s",deleteMessageId, path);

    if (GetMsgListFromMbox(path, &jsonObjPtr, &jsonArrayPtr) != LE_OK)
    {
       LE_ERROR("No message");
       return LE_FAULT;
    }

    int i;
    MessageId_t messageId;

    for (i=0; i < json_array_size(jsonArrayPtr); i++)
    {
        json_t * jsonIntegerPtr = json_array_get(jsonArrayPtr, i);

        if (jsonIntegerPtr)
        {
            messageId = json_integer_value(jsonIntegerPtr);

            if (messageId == deleteMessageId)
            {
                LE_DEBUG("Remove %d", (int) messageId);
                json_array_remove(jsonArrayPtr, i);
            }
        }
        else
        {
            LE_ERROR("Json error");
        }
    }

    le_result_t res = LE_OK;
    if ( json_dump_file(jsonObjPtr, path, JSON_INDENT(1) | JSON_PRESERVE_ORDER) < 0 )
    {
        res = LE_FAULT;
        LE_ERROR("Json_dump_file error");
    }

    json_decref(jsonObjPtr);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a message belongs to a message box
 *
 */
//--------------------------------------------------------------------------------------------------

static le_result_t CheckMessageIdInMbox
(
    char* mboxName,
    MessageId_t messageId
)
{
    json_t *jsonRootObjPtr;
    json_t *jsonArrayPtr;
    uint32_t pathLen = GetSMSInboxConfigPathLen(mboxName);
    char path[pathLen];
    GetSMSInboxConfigPath(mboxName, path, pathLen);

    if ( GetMsgListFromMbox(path, &jsonRootObjPtr, &jsonArrayPtr) != LE_OK )
    {
        LE_ERROR("Error in GetMsgListFromMbox");
        return LE_FAULT;
    }

    int i = 0;

    while (i < json_array_size(jsonArrayPtr))
    {
        json_t * jsonIntegerPtr = json_array_get(jsonArrayPtr, i);
        MessageId_t msgId = 0;

        if (jsonIntegerPtr)
        {
            msgId = json_integer_value(jsonIntegerPtr);
        }
        else
        {
            LE_ERROR("Json error");
            json_decref(jsonRootObjPtr);
            return LE_FAULT;
        }

        LE_DEBUG("MessageId %d, msgId %d, i %d", messageId, msgId, i);
        if (msgId == messageId)
        {
            json_decref(jsonRootObjPtr);
            return LE_OK;
        }

        i++;
    }

    LE_ERROR("Bad msg id or mbox name");
    json_decref(jsonRootObjPtr);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode message file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DecodeMsgEntry
(
    char* mboxName,                      ///<[IN] mbox name
    MessageId_t messageId,               ///<[IN] Message identifier to decode
    char* keyPtr[],                      ///<[IN] Key to retrieve
    uint8_t nbKey,                       ///<[IN] Number of elements in keyPtr
    EntryDesc_t * decodePtr              ///<[IN/OUT] Decoding result
)
{
    uint16_t pathLen = GetSMSInboxMessagePathLen();
    char path[pathLen];
    memset(path, 0, pathLen);
    json_error_t error;
    le_result_t res = LE_OK;

    GetSMSInboxMessagePath(messageId, path, pathLen);

    json_t* jsonRootPtr = json_load_file(path, JSON_REJECT_DUPLICATES, &error);

    if ( jsonRootPtr == NULL )
    {
        LE_ERROR("Json decoder error %s mboxName %s", error.text, mboxName);
        DeleteMsgInAppCfg(mboxName, messageId);
        return LE_FAULT;
    }

    res = ReadJsonObj(jsonRootPtr, keyPtr, nbKey, decodePtr);

    json_decref(jsonRootPtr);

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if an application requested the deletion of a message
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsDeleted
(
    MessageId_t messageId,  ///<[IN] Message identifier
    char* appNamePtr        ///<[IN] Application name
)
{
    EntryDesc_t decode;
    decode.type = DESC_BOOL;
    char* key[2] = {JSON_ISDELETED, appNamePtr};

    if (DecodeMsgEntry(appNamePtr, messageId, key, 2, &decode) == LE_OK)
    {
        LE_DEBUG("Decode.uVal.boolVal %d", decode.uVal.boolVal);
        return decode.uVal.boolVal;
    }
    else
    {
        // if error, delete the message
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Perform the deleteion
 *
 */
//--------------------------------------------------------------------------------------------------
static void PerformDeletion
(
    MessageId_t messageId   ///<[IN] Message identifier
)
{
    int i;
    bool deleted = true;

    for (i=0; i < MAX_APPS; i++)
    {
        if ( Apps[i].namePtr && (strlen(Apps[i].namePtr) != 0) )
        {
            deleted &= IsDeleted(messageId, Apps[i].namePtr);
        }
    }

    // All applications mark this message to be delteted => erase physically the message
    if (deleted)
    {
        uint16_t pathLen = GetSMSInboxMessagePathLen();
        char path[pathLen];
        memset(path, 0, pathLen);

        GetSMSInboxMessagePath(messageId, path, pathLen);

        LE_DEBUG("Delete messageId %d, path %s",messageId, path);
        unlink(path);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a message in the application's cfg file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddMsgInAppCfg
(
    MboxCtx_t* appsPtr,         ///<[IN] application config
    MessageId_t messageId     ///<[IN] message to add
)
{
    uint32_t pathLen = GetSMSInboxConfigPathLen(appsPtr->namePtr);
    char path[pathLen];
    GetSMSInboxConfigPath(appsPtr->namePtr, path, pathLen);
    json_t* jsonMsgIdPtr = NULL;
    json_t* jsonPtr;
    json_t* jsonArrayPtr;

    if ( GetMsgListFromMbox (path, &jsonPtr, &jsonArrayPtr) != LE_OK )
    {
        LE_ERROR("Message %s not found", path);
        return LE_FAULT;
    }
    LE_DEBUG("Add messageId %d, path %s, array Size %zu", messageId,
                                                          path,
                                                          json_array_size(jsonArrayPtr) );

    if ( json_array_size(jsonArrayPtr) == appsPtr->inboxSize )
    {
        // delete older entry
        jsonMsgIdPtr = json_array_get(jsonArrayPtr, 0);
        MessageId_t messageId = 0;

        if (jsonMsgIdPtr)
        {
            messageId = json_integer_value(jsonMsgIdPtr);
        }

        if (messageId != 0)
        {
            json_array_remove(jsonArrayPtr, 0);

            EntryDesc_t modif;
            modif.type = DESC_BOOL;
            modif.uVal.boolVal = true;
            char* key[2]={JSON_ISDELETED, appsPtr->namePtr};

            if (ModifyMsgEntry(messageId, key, 2, &modif) != LE_OK)
            {
                LE_ERROR("Can't modify entry %08x, path %s", (int) messageId, path);
            }

            PerformDeletion(messageId);
        }
        else
        {
            json_decref(jsonPtr);
            LE_ERROR("Json error");
            return LE_FAULT;
        }
    }

    if (AddIntegerKeyInJsonObject(jsonArrayPtr, NULL, messageId) != LE_OK)
    {
        json_decref(jsonPtr);
        LE_ERROR("Json error");
        return LE_FAULT;
    }

    if ( json_dump_file(jsonPtr, path, JSON_INDENT(1) | JSON_PRESERVE_ORDER) < 0 )
    {
        json_decref(jsonPtr);
        LE_ERROR("Json error");
        return LE_FAULT;
    }

    json_decref(jsonPtr);

    // Use for the first initialization
    if (jsonArrayPtr->refcount > 0)
    {
        json_decref(jsonArrayPtr);
    }

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Encode Json file
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMsgEntry
(
    uint32_t fd,            ///<[IN] file descriptor
    le_sms_MsgRef_t msgRef  ///<[IN] SMS to be encoding
)
{
    json_t *jsonRootPtr = json_object();

    if (!jsonRootPtr)
    {
        LE_ERROR("Json error");
        return LE_FAULT;
    }

    // Add imsi in json file
    AddStringKeyInJsonObject(jsonRootPtr, JSON_IMSI, SimImsi);

    // Add sms format in json file
    le_sms_Format_t format = le_sms_GetFormat(msgRef);
    AddIntegerKeyInJsonObject(jsonRootPtr, JSON_FORMAT, (int) format);

    // Unread by default for all applications
    json_t *jsonUnreadPtr = json_object();
    int i;
    for (i=0; i < MAX_APPS; i++)
    {
        if ( Apps[i].namePtr && (strlen(Apps[i].namePtr) != 0) )
        {
            AddBooleanKeyInJsonObject(jsonUnreadPtr, Apps[i].namePtr, true);
        }
    }

    json_object_set_new(jsonRootPtr, JSON_ISUNREAD, jsonUnreadPtr);

    // Undeleted by default for all applications
    json_t *jsonDeletePtr = json_object();
    for (i=0; i < MAX_APPS; i++)
    {
        if ( Apps[i].namePtr && (strlen(Apps[i].namePtr) != 0) )
        {
            AddBooleanKeyInJsonObject(jsonDeletePtr, Apps[i].namePtr, false);
        }
    }

    json_object_set_new(jsonRootPtr, JSON_ISDELETED, jsonDeletePtr);

    switch ( format )
    {
        case LE_SMS_FORMAT_TEXT:
        case LE_SMS_FORMAT_BINARY:
        {
            char tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
            memset(tel, 0, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

            // Add phone number
            le_result_t result = le_sms_GetSenderTel(msgRef, tel, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);

            if (result != LE_OK)
            {
                LE_ERROR("Unable to get the tel number %d", result);
            }
            else
            {
                LE_DEBUG("Tel num: %s", tel);
                AddStringKeyInJsonObject(jsonRootPtr, JSON_SENDERTEL, tel);
            }

            // Add timestamp
            char timeStamp[LE_SMS_TIMESTAMP_MAX_BYTES];
            result = le_sms_GetTimeStamp (msgRef, timeStamp, LE_SMS_TIMESTAMP_MAX_BYTES);

            if (result != LE_OK)
            {
                LE_ERROR("Unable to get the timestamp %d", result);
            }
            else
            {
                LE_DEBUG("Timestamp: %s", timeStamp);
                AddStringKeyInJsonObject(jsonRootPtr, JSON_TIMESTAMP, timeStamp);
            }

            size_t len = le_sms_GetUserdataLen(msgRef);
            AddIntegerKeyInJsonObject(jsonRootPtr, JSON_MSGLEN, len);

            // Add a character for last '\0'
            len++;

            uint8_t bin[len];
            memset(bin, 0, len);
            int32_t strLen = 2*len + 1;
            char string[strLen];
            memset(string, 0, strLen);
            const char* jsonKey;

            if (format == LE_SMS_FORMAT_TEXT)
            {
                // Get text
                result = le_sms_GetText(msgRef, (char*) bin, len);
                jsonKey = JSON_TEXT;
            }
            else
            {
                // Get binary
                result = le_sms_GetBinary(msgRef, bin, &len);
                jsonKey = JSON_BIN;
            }

            if (result != LE_OK)
            {
                LE_ERROR("Unable to get payload %d", result);
                AddIntegerKeyInJsonObject(jsonRootPtr, JSON_MSGLEN, 0);
            }
            else
            {
                // Convert payload in string: json supports only utf-8 characters.
                // Some extended-ASCII characters can be sent by SMS (such as character with accent)
                // and can't be convert directly in utf-8. A convertion in string solves this issue.
                strLen = le_hex_BinaryToString(bin, len, string, strLen);
                AddStringKeyInJsonObject(jsonRootPtr, jsonKey, string);

            }
        }
        break;

        case LE_SMS_FORMAT_PDU:
        {
            size_t len = le_sms_GetPDULen(msgRef);
            AddIntegerKeyInJsonObject(jsonRootPtr, JSON_MSGLEN, len);
            // Add a character for last '\0'
            len++;
            uint8_t pdu[len];
            memset(pdu, 0, len);
            int32_t strLen = 2*len + 1;
            char string[strLen];
            memset(string, 0, strLen);

            // Add pdu
            le_result_t result = le_sms_GetPDU(msgRef, pdu, &len);

            if (result != LE_OK)
            {
                LE_ERROR("Unable to get pdu %d", result);
                AddIntegerKeyInJsonObject(jsonRootPtr, JSON_MSGLEN, 0);
            }
            else
            {
                // Convert pdu in string
                strLen = le_hex_BinaryToString(pdu, len, string, strLen);
                AddStringKeyInJsonObject(jsonRootPtr, JSON_PDU, string);
                LE_DEBUG("PDU format OK");
            }
        }
        break;
        case LE_SMS_FORMAT_UNKNOWN:
        default:
            LE_ERROR("Bad format %d", format);
    }

    // Write json file in the file system
    char* jsondumpStr = json_dumps( (const json_t *) jsonRootPtr,
                                    JSON_INDENT(1) | JSON_PRESERVE_ORDER);
    if (!jsondumpStr)
    {
        LE_ERROR("JsondumpStr is NULL");
        return LE_FAULT;
    }

    char* restJsondumpStr = jsondumpStr;
    ssize_t writeSize = strlen(jsondumpStr);
    ssize_t writtenSize;

    do
    {
        writtenSize = write( fd, restJsondumpStr, writeSize );
        if (writtenSize > 0)
        {
            writeSize -= writtenSize;
            restJsondumpStr += writtenSize;
        }
    }
    while ((writeSize > 0) && ((writtenSize >= 0) || (EINTR == errno)));

    free(jsondumpStr);
    json_decref(jsonRootPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new message entry
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateMsgEntry
(
    MessageId_t *msgPtr,    ///<[OUT] create messageId
    int32_t *fdPtr         ///<[OUT] Created file descriptor
)
{
    uint16_t pathLen = GetSMSInboxMessagePathLen();
    char path[pathLen];
    memset(path, 0, pathLen);

    GetSMSInboxMessagePath(NextMessageId, path, pathLen);
    LE_DEBUG("Create entry: NextMessageId %d, path %s",NextMessageId, path);

    *fdPtr = open( path, O_CREAT | O_RDWR | O_SYNC, S_IRUSR | S_IWUSR );

    if (*fdPtr < 0)
    {
        LE_ERROR("No such file: %s: %m", path);
        return LE_FAULT;
    }

    LE_DEBUG("New entry: %s", path);

    int i;

    // For all the applications
    for (i = 0; i < MAX_APPS; i++)
    {
        if ( Apps[i].namePtr && strlen(Apps[i].namePtr) )
        {
            AddMsgInAppCfg(&Apps[i], NextMessageId);
        }
    }

    *msgPtr = NextMessageId;

    NextMessageId++;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the file name string in hexa
 *
 * @return
 *      - Positive integer corresponding to the hexadecimal input string
 *      - -1 in case of error
 */
//--------------------------------------------------------------------------------------------------
static MessageId_t GetMessageId
(
    char* fileNamePtr  ///<[IN] file name to be converted
)
{
    char *savePtr;

    if(NULL != fileNamePtr)
    {
        char *str = strtok_r(fileNamePtr,".", &savePtr);

        if (NULL == str)
        {
            LE_ERROR("Unable to find . in the file name");
            return -1;
        }
        return le_hex_HexaToInteger(str);
    }
    else
    {
        LE_ERROR("Provided file name pointer is NULL");
        return -1;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a directory
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MkdirCreate
(
    const char* path    ///<[IN] path for directory creation
)
{
    int status = mkdir(path, S_IRWXU|S_IRWXG);
    if (0 != status)
    {
        if (EEXIST != errno)
        {
            LE_ERROR("Unable to create directory %s: %m", path);
            return LE_FAULT;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init the SMSInBox directory
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitSmsInBoxDirectory
(
    void
)
{
    struct dirent **namelist;
    uint32_t nbSmsEntries;

    LE_DEBUG("InitSmsInBoxDirectory");

    // create directories
    if (LE_OK != MkdirCreate(SMSINBOX_PATH))
    {
        return;
    }

    uint16_t pathLen = GetSMSInboxMessagePathLen();
    char path[pathLen];
    memset(path, 0, pathLen);
    snprintf(path, pathLen, "%s%s", SMSINBOX_PATH, CONF_PATH);

    if (LE_OK != MkdirCreate(path))
    {
        return;
    }

    memset(path, 0, pathLen);
    snprintf(path, pathLen, "%s%s", SMSINBOX_PATH, MSG_PATH);

    if (LE_OK != MkdirCreate(path))
    {
        return;
    }

    nbSmsEntries = scandir(path, &namelist, NULL, alphasort);
    if ( nbSmsEntries > 2 )
    {
        uint8_t len = strlen(namelist[nbSmsEntries-1]->d_name);
        char name[len+1];
        MessageId_t id;

        memset(name, 0, len+1);
        strncpy(name, namelist[nbSmsEntries-1]->d_name, len);

        id = GetMessageId(name);
        if (-1 == id)
        {
            LE_ERROR("Unable to get the id of %s", name);
            return;
        }

        NextMessageId = ++id;

        LE_DEBUG("NextMessageId %d", (int) NextMessageId);
    }

    while (nbSmsEntries--)
    {
        free(namelist[nbSmsEntries]);
    }

    free(namelist);
}

//--------------------------------------------------------------------------------------------------
/**
 * Move the SMS from SIM to folder
 *
 */
//--------------------------------------------------------------------------------------------------
static void MoveSmsFromSimToFolder
(
    void
)
{
    int32_t fd;
    le_result_t result = LE_OK;

    le_sms_MsgListRef_t msgListRef = le_sms_CreateRxMsgList();

    if (!msgListRef)
    {
        LE_DEBUG("No message in SIM");
        return;
    }

    le_sms_MsgRef_t smsRef = le_sms_GetFirst(msgListRef);

    while(smsRef)
    {
        MessageId_t msgId;

        result = CreateMsgEntry(&msgId, &fd);

        if (result == LE_OK)
        {
            if ( EncodeMsgEntry(fd, smsRef) != LE_OK )
            {
                LE_ERROR("Encoding issue");
            }

            close(fd);
        }
        else
        {
            LE_ERROR("Error during new entry creation");
        }

        // Delete the SMS, whatever the result (if something is going wrong, delete it, otherwise
        // it will be copied in the folder at each startup of the SmsInbox service)
        // TODO: Check why the smsinbox is died after this call
        result = le_sms_DeleteFromStorage(smsRef);
        LE_DEBUG("Delete from storage %d", result);

        smsRef = le_sms_GetNext(msgListRef);
    }

    le_sms_DeleteList(msgListRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * New SMS handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewSmsMsgHandler
(
    le_sms_MsgRef_t msgRef,     ///<[IN] New SMS reference
    void*           contextPtr
)
{
    int32_t fd;
    le_result_t result;
    MessageId_t msgId;

    LE_DEBUG("Receive new message");

    result = CreateMsgEntry(&msgId, &fd);

    if (result == LE_OK)
    {
        result = EncodeMsgEntry(fd, msgRef);
        close(fd);
    }
    else
    {
        LE_ERROR("CreateMsgEntry error");
    }

    if (result == LE_OK)
    {
        result = le_sms_DeleteFromStorage(msgRef);
        LE_DEBUG("Delete from storage %d", result);

        le_sms_Delete(msgRef);
        le_event_Report(RxMsgEventId, &msgId, sizeof(MessageId_t));
    }
    else
    {
        LE_ERROR("EncodeMsgEntry error");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM state handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimStateHandler
(
    le_sim_Id_t simId,
    le_sim_States_t simState,
    void* contextPtr
)
{
    LE_DEBUG("SimId %d simState %d",simId, simState);

    switch (simState)
    {
        case LE_SIM_READY:
        {
            if (le_sim_GetIMSI(simId, SimImsi, LE_SIM_IMSI_BYTES) != LE_OK)
            {
                LE_ERROR("Error in get IMSI");
                return;
            }

            MoveSmsFromSimToFolder();
        }
        break;
        default:
        break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the SMS Inbox settings
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT there are missing settings
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadInboxSettings
(
    void
)
{
    int i=0;
    char mboxConfigPath[100];

    while (i < le_smsInbox_NbMbx)
    {
        memset(mboxConfigPath, 0, 100);
        snprintf(mboxConfigPath, sizeof(mboxConfigPath), "%s/%s/%s",
                                                         CFG_SMSINBOX_PATH,
                                                         CFG_NODE_APPS,
                                                         le_smsInbox_mboxName[i]);

        le_cfg_IteratorRef_t appIter = le_cfg_CreateWriteTxn(mboxConfigPath);

        // if node does not exist, use the default profile
        if (!le_cfg_NodeExists(appIter, ""))
        {
            LE_INFO("use default size le_smsInbox_mboxName[i]");
            Apps[i].inboxSize = DEFAULT_MBOX_SIZE;
        }
        else
        {
            Apps[i].inboxSize = le_cfg_GetInt(appIter, "", DEFAULT_MBOX_SIZE);
        }

        Apps[i].namePtr = (char*) le_smsInbox_mboxName[i];

        le_cfg_CancelTxn(appIter);

        i++;
    }

    for (i = 0; i < MAX_APPS; i++)
    {
        if (Apps[i].inboxSize > MaxInboxSize)
        {
            MaxInboxSize = Apps[i].inboxSize;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New message Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerRxMsgHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    RxMsgReport_t* rxMsgReportPoolPtr = (RxMsgReport_t*) secondLayerHandlerFunc;
    MessageId_t* messageIdPtr = (MessageId_t*) reportPtr;

    LE_DEBUG("New SMS: %08x", (int) *messageIdPtr);

    rxMsgReportPoolPtr->handlerPtr(*messageIdPtr,
                                   le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to release smsInbox service
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_DEBUG("SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search for all smsInbox request references used by the current client session
    // that has been closed.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ActivationRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (result == LE_OK)
    {
        ClientRequest_t * clientRequestPtr = (ClientRequest_t*) le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (clientRequestPtr->sessionRef == sessionRef)
        {
            SmsInbox_SessionRef_t safeRef =
                            (SmsInbox_SessionRef_t) le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Call SmsInbox_Close 0x%p, Session 0x%p",safeRef,sessionRef);

            // Close smsInbox for current client.
            SmsInbox_Close(safeRef);
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Server Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("smsInbox Component Init started");

    memset(Apps, 0, sizeof(Apps) );

    // Create the RX message report reference pool
    RxMsgReportMap = le_ref_CreateMap("rxMsgReportMap", MAX_APPS);

    // Create a pool for message box session objects
    MboxSessionPool = le_mem_CreatePool("MboxSessionPool", sizeof(MboxSession_t));
    le_mem_ExpandPool(MboxSessionPool, MAX_APPS);

    // Create a pool for the SMS RX handler
    RxMsgReportPool = le_mem_CreatePool("RxMsgReportPool", sizeof(RxMsgReport_t));
    le_mem_ExpandPool(RxMsgReportPool, MAX_APPS);

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    ActivationRequestRefMap = le_ref_CreateMap("SmsInbox_Client", MAX_APPS);

    // Create a pool for smsInbox client objects.
    SmsInboxHandlerPoolRef = le_mem_CreatePool("SmsInboxHandlerPoolRef", sizeof(ClientRequest_t));
    le_mem_ExpandPool(SmsInboxHandlerPoolRef, MAX_APPS);

    // Retrieve the smsInbox settings from the configuration tree
    LoadInboxSettings();

    // Initialization of the smsInbox directory
    InitSmsInBoxDirectory();

    // Create an event Id for new messages
    RxMsgEventId = le_event_CreateId("RxMsgEventId", sizeof(MessageId_t));

    le_sim_AddNewStateHandler (SimStateHandler, NULL);

    le_sms_AddRxMessageHandler(NewSmsMsgHandler, NULL);

    // If SIM is ready, call the SIM handler
    if ( le_sim_GetState( le_sim_GetSelectedCard() ) == LE_SIM_READY )
    {
        SimStateHandler(le_sim_GetSelectedCard(), LE_SIM_READY, NULL);
    }

    LE_INFO("smsInbox Component Init done");
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a message box.
 *
 * @return
 * Reference on the opened message box
 */
//--------------------------------------------------------------------------------------------------
SmsInbox_SessionRef_t SmsInbox_Open
(
    const char* mboxName,
        ///< [IN]
        ///< Message box name

    le_msg_ServiceRef_t msgServiceRef,
        ///< [IN]
        ///< Message service reference

    le_msg_SessionRef_t msgSession
        ///< [IN]
        ///< Client session reference
)
{
    if (mboxName == NULL)
    {
        LE_ERROR("No mbox name");
        return NULL;
    }

    int i;

    for (i=0; i < MAX_APPS; i++)
    {
        if (Apps[i].namePtr && (strcmp(Apps[i].namePtr, mboxName) == 0))
        {
            ClientRequest_t* clientRequestPtr = le_mem_ForceAlloc(SmsInboxHandlerPoolRef);
            clientRequestPtr->mboxSessionPtr = (MboxSession_t*) le_mem_ForceAlloc(MboxSessionPool);

            clientRequestPtr->mboxSessionPtr->mboxCtxPtr = &Apps[i];
            memset(&clientRequestPtr->mboxSessionPtr->browseCtx, 0, sizeof(BrowseCtx_t));

            SmsInbox_SessionRef_t reqRef = le_ref_CreateRef(ActivationRequestRefMap,
                                                            clientRequestPtr);

            // Save the client session "msgSession" associated with the request reference "reqRef"
            clientRequestPtr->sessionRef = msgSession;
            clientRequestPtr->smsInboxSessionRef = reqRef;

            return reqRef;
        }
    }

    LE_ERROR("SmsInbox_Open error");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a previously open message box.
 *
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_Close
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< Session reference
)
{
    if ( sessionRef == NULL )
    {
        LE_ERROR("Bad mbox reference");
        return;
    }

    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return;
    }

    le_mem_Release(clientRequestPtr->mboxSessionPtr);
    le_mem_Release(clientRequestPtr);

    le_ref_DeleteRef(ActivationRequestRefMap, sessionRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'SmsInbox_RxMessage'
 *
 * This event provides information on new received messages.
 */
//--------------------------------------------------------------------------------------------------
SmsInbox_RxMessageHandlerRef_t SmsInbox_AddRxMessageHandler
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    SmsInbox_RxMessageHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return NULL;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return NULL;
    }

    if(handlerPtr == NULL)
    {
        LE_KILL_CLIENT("handlerPtr is NULL !");
        return NULL;
    }

    RxMsgReport_t* rxMsgReportPtr = (RxMsgReport_t*) le_mem_ForceAlloc(RxMsgReportPool);

    rxMsgReportPtr->sessionRef = sessionRef;
    rxMsgReportPtr->handlerPtr = handlerPtr;
    rxMsgReportPtr->handlerRef = le_event_AddLayeredHandler("RxMsgHandler",
                                             RxMsgEventId,
                                             FirstLayerRxMsgHandler,
                                             (le_event_HandlerFunc_t)rxMsgReportPtr);

    le_event_SetContextPtr(rxMsgReportPtr->handlerRef, contextPtr);

    return le_ref_CreateRef(RxMsgReportMap, rxMsgReportPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'SmsInbox_RxMessage'
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_RemoveRxMessageHandler
(
    SmsInbox_RxMessageHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    RxMsgReport_t* rxMsgReportPtr =
                                (RxMsgReport_t*) le_ref_Lookup(RxMsgReportMap, addHandlerRef);

    if (rxMsgReportPtr == NULL)
    {
        LE_ERROR("Bad reference");
        return;
    }

    le_ref_DeleteRef(RxMsgReportMap, addHandlerRef);

    le_event_RemoveHandler((le_event_HandlerRef_t)rxMsgReportPtr->handlerRef);

    le_mem_Release(rxMsgReportPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a Message.
 *
 * @note It is valid to delete a non-existent message.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_DeleteMsg
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t modif;
    modif.type = DESC_BOOL;
    modif.uVal.boolVal = true;
    char* key[2]={JSON_ISDELETED, clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr};

    if (ModifyMsgEntry(messageId, key, 2, &modif) != LE_OK)
    {
        LE_ERROR("ModifyMsgEntry error");
    }

    if (DeleteMsgInAppCfg(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId)
                          != LE_OK)
    {
        LE_ERROR("DeleteMsgInAppCfg error");
    }

    PerformDeletion(messageId);
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the IMSI of the message receiver SIM if it applies.
 *
 * @return
 *  - LE_NOT_FOUND     The message item is not tied to a SIM card, the imsi string is empty.
 *  - LE_OVERFLOW      The imsiPtr buffer was too small for the IMSI.
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FAULT         The function failed.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetImsi
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* imsiPtr,
        ///< [OUT]
        ///< IMSI.

    size_t imsiNumElements
        ///< [IN]
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    le_result_t res;

    memset(imsiPtr, 0, imsiNumElements);

    if (imsiNumElements < LE_SIM_IMSI_BYTES)
    {
        return LE_OVERFLOW;
    }

    EntryDesc_t decode;
    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = imsiPtr;
    decode.uVal.str.lenStr = imsiNumElements;
    char* key[1] = {JSON_IMSI};

    if ((res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr,
                              messageId, key, 1, &decode)) == LE_OK)
    {
        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the message format (text, binary or PDU).
 *
 * @return
 *  - Message format.
 *  - FORMAT_UNKNOWN when the message format cannot be identified or the message reference is
 *    invalid.
 */
//--------------------------------------------------------------------------------------------------
le_sms_Format_t SmsInbox_GetFormat
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    // Get the message box session context
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return 0;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad mbox reference");
        return 0;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return 0;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t decode;
    decode.type = DESC_INT;
    char* key[1] = {JSON_FORMAT};

    if (DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId, key, 1,
                      &decode) == LE_OK)
    {
        SmsInbox_MarkRead(sessionRef, msgId);
        return decode.uVal.val;
    }
    else
    {
        return LE_SMSINBOX_FORMAT_UNKNOWN;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Sender Identifier.
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_OVERFLOW      Identifier length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetSenderTel
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* telPtr,
        ///< [OUT]
        ///< Identifier string.

    size_t telNumElements
        ///< [IN]
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    le_result_t res;
    EntryDesc_t decode;
    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = telPtr;
    decode.uVal.str.lenStr = telNumElements;
    memset(telPtr, 0, telNumElements);
    char* key[1] = {JSON_SENDERTEL};

    if ((res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId, key,
                              1, &decode)) == LE_OK)
    {
        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Message Time Stamp string (it does not apply for PDU message).
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_NOT_FOUND     The message is a PDU message.
 *  - LE_OVERFLOW      Timestamp number length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetTimeStamp
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* timestampPtr,
        ///< [OUT]
        ///< Message time stamp (for text or binary
        ///<  messages). String format:
        ///< "yy/MM/dd,hh:mm:ss+/-zz"
        ///< (Year/Month/Day,Hour:Min:Seconds+/-TimeZone)

    size_t timestampNumElements
        ///< [IN]
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t decode;
    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = timestampPtr;
    decode.uVal.str.lenStr = timestampNumElements;
    memset(timestampPtr, 0, timestampNumElements);
    char* key[1] = {JSON_TIMESTAMP};
    le_result_t res;

    if ( (res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId,
                               key, 1, &decode)) == LE_OK )
    {
        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the message Length value.
 *
 * @return Number of characters for text messages, or the length of the data in bytes for raw
 *         binary and PDU messages.
 *  - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
size_t SmsInbox_GetMsgLen
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;

    EntryDesc_t decode;
    decode.type = DESC_INT;
    char* key[1] = {JSON_MSGLEN};
    le_result_t res;

    if ( (res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr,
                                messageId,
                                key,
                                1,
                                &decode)) == LE_OK )
    {
        SmsInbox_MarkRead(sessionRef, msgId);

        return decode.uVal.val;
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the text Message.
 *
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Message is not in text format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetText
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    char* textPtr,
        ///< [OUT]
        ///< Message text.

    size_t textNumElements
        ///< [IN]
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t decode;
    le_result_t res;
    size_t len = 2*textNumElements+1;
    char tmpTextPtr[len];
    memset(tmpTextPtr, 0, len);
    memset(textPtr, 0, textNumElements);

    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = tmpTextPtr;
    decode.uVal.str.lenStr = len;
    char* key[1] = {JSON_TEXT};

    res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId, key, 1,
                         &decode);

    if ( res == LE_OK )
    {
        if ( le_hex_StringToBinary(tmpTextPtr,
                                   decode.uVal.str.lenStr,
                                   (uint8_t*) textPtr,
                                   textNumElements ) < 0 )
        {
            return LE_FAULT;
        }

        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the binary Message.
 *
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Message is not in binary format.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetBinary
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    uint8_t* binPtr,
        ///< [OUT]
        ///< Binary message.

    size_t* binNumElementsPtr
        ///< [INOUT]
)
{
    if (binPtr == NULL)
    {
        LE_KILL_CLIENT("binPtr is NULL.");
        return LE_FAULT;
    }

    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    le_result_t res;
    EntryDesc_t decode;
    size_t len = 2*(*binNumElementsPtr)+1;
    char tmpBinPtr[len];
    memset(tmpBinPtr, 0, len);
    memset(binPtr, 0, *binNumElementsPtr);

    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = tmpBinPtr;
    decode.uVal.str.lenStr = len;
    char* key[1] = {JSON_BIN};

    res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId,
                         key, 1, &decode);

    if ( res == LE_OK )
    {
        int32_t binNumElements = le_hex_StringToBinary(tmpBinPtr,
                                                    decode.uVal.str.lenStr,
                                                    binPtr,
                                                    *binNumElementsPtr);

        if ( binNumElements < 0 )
        {
            return LE_FAULT;
        }

        *binNumElementsPtr = binNumElements;

        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the PDU message.
 *
 * Output parameters are updated with the PDU message content and the length of the PDU message
 * in bytes.
 *
 * @return
 *  - LE_BAD_PARAMETER The message reference is invalid.
 *  - LE_FORMAT_ERROR  Unable to encode the message in PDU.
 *  - LE_OVERFLOW      Message length exceed the maximum length.
 *  - LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetPdu
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId,
        ///< [IN]
        ///< Message identifier.

    uint8_t* pduPtr,
        ///< [OUT]
        ///< PDU message.

    size_t* pduNumElementsPtr
        ///< [INOUT]
)
{
    if (pduPtr == NULL)
    {
        LE_KILL_CLIENT("pduPtr is NULL.");
        return LE_FAULT;
    }

    // Get the mbox session context
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad mbox reference");
        return 0;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return 0;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    le_result_t res;
    EntryDesc_t decode;
    size_t len = 2*(*pduNumElementsPtr)+1;
    char tmpPduPtr[len];
    memset(tmpPduPtr, 0, len);
    memset(pduPtr, 0, *pduNumElementsPtr);

    decode.type = DESC_STRING;
    decode.uVal.str.strPtr = tmpPduPtr;
    decode.uVal.str.lenStr = len;
    char* key[1] = {JSON_PDU};

    res = DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId, key, 1,
                         &decode);

    if ( res == LE_OK )
    {
        int32_t pduNumElements = le_hex_StringToBinary(tmpPduPtr,
                                                    decode.uVal.str.lenStr,
                                                    pduPtr,
                                                    *pduNumElementsPtr);

        if ( pduNumElements < 0 )
        {
            return LE_FAULT;
        }

        *pduNumElementsPtr = pduNumElements;

        SmsInbox_MarkRead(sessionRef, msgId);
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the first Message object reference in the inbox message.
 *
 * @return
 *  - 0 No message found (message box parsing is over).
 *  - Message identifier.
 *  - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
uint32_t SmsInbox_GetFirst
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< Session reference to browse
)
{
    // Get the message box session context
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad mbox reference");
        return 0;
    }

    uint32_t pathLen = GetSMSInboxConfigPathLen(clientRequestPtr->
                                                mboxSessionPtr->mboxCtxPtr->namePtr);
    char path[pathLen];
    memset(path, 0, pathLen);
    GetSMSInboxConfigPath(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, path, pathLen);
    le_result_t res = LE_OK;
    MessageId_t messageId=0;

    if ( GetMsgListFromMbox(path,
                        &clientRequestPtr->mboxSessionPtr->browseCtx.jsonObjPtr,
                        &clientRequestPtr->mboxSessionPtr->browseCtx.jsonArrayPtr) != LE_OK )
    {
        LE_ERROR("Error in GetMsgListFromMbox");
        return 0;
    }

    clientRequestPtr->mboxSessionPtr->browseCtx.maxIndex =
                      json_array_size(clientRequestPtr->mboxSessionPtr->browseCtx.jsonArrayPtr);

    LE_DEBUG("MaxIndex %d", clientRequestPtr->mboxSessionPtr->browseCtx.maxIndex);

    if ( clientRequestPtr->mboxSessionPtr->browseCtx.maxIndex > 0 )
    {
        json_t * jsonIntegerPtr =
                 json_array_get(clientRequestPtr->mboxSessionPtr->browseCtx.jsonArrayPtr, 0);

        if (jsonIntegerPtr)
        {
            messageId = json_integer_value(jsonIntegerPtr);
        }
        else
        {
            LE_ERROR("Json error");
            res = LE_FAULT;
        }

        if (messageId)
        {
            clientRequestPtr->mboxSessionPtr->browseCtx.currentMessageIndex = 1;
        }
        else
        {
            LE_ERROR("Json error");
            res = LE_FAULT;
        }
    }
    else
    {
        res = LE_FAULT;
        LE_DEBUG("Empty mbox");
    }

    if (res == LE_FAULT)
    {
        if (clientRequestPtr->mboxSessionPtr->browseCtx.jsonObjPtr)
        {
            json_decref(clientRequestPtr->mboxSessionPtr->browseCtx.jsonObjPtr);
        }

        memset(&clientRequestPtr->mboxSessionPtr->browseCtx, 0, sizeof(BrowseCtx_t));

        return 0;
    }
    else
    {
        return messageId;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next Message object reference in the inbox message.
 *
 * @return
 *  - 0 No message found (message box parsing is over).
 *  - Message identifier.
 *  - LE_BAD_PARAMETER The message reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
uint32_t SmsInbox_GetNext
(
    SmsInbox_SessionRef_t sessionRef
        ///< [IN]
        ///< session reference to browse
)
{
    // Get the message box session context
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    while(clientRequestPtr->mboxSessionPtr->browseCtx.maxIndex !=
          clientRequestPtr->mboxSessionPtr->browseCtx.currentMessageIndex)
    {
        LE_DEBUG("CurrentIndex %d, maxIndex %d", clientRequestPtr->
                                                 mboxSessionPtr->browseCtx.currentMessageIndex,
                                                 clientRequestPtr->
                                                 mboxSessionPtr->browseCtx.maxIndex);

        json_t * jsonIntegerPtr = json_array_get(clientRequestPtr->
                                                 mboxSessionPtr->browseCtx.jsonArrayPtr,
                                                 clientRequestPtr->
                                                 mboxSessionPtr->browseCtx.currentMessageIndex);

        if (jsonIntegerPtr)
        {
            MessageId_t messageId = json_integer_value(jsonIntegerPtr);

            if (messageId > 0)
            {
                clientRequestPtr->mboxSessionPtr->browseCtx.currentMessageIndex++;

                // Check if the message exist (it may be deleted since the GetFirst call)
                uint16_t pathLen = GetSMSInboxMessagePathLen();
                char path[pathLen];
                memset(path, 0, pathLen);
                GetSMSInboxMessagePath(messageId, path, pathLen);
                int32_t fd = open(path, O_RDWR);

                if (fd >= 0)
                {
                    // message is still existing
                    close(fd);
                    return messageId;
                }

                // else continue the parsing
            }
            else
            {
                LE_ERROR("Json error");
            }
        }
        else
        {
            LE_ERROR("Json error");
        }
    }

    // Parsing end => free the memory
    LE_DEBUG("No more messages");
    json_decref(clientRequestPtr->mboxSessionPtr->browseCtx.jsonObjPtr);
    memset(&clientRequestPtr->mboxSessionPtr->browseCtx, 0, sizeof(BrowseCtx_t));

    return 0;
}
//--------------------------------------------------------------------------------------------------
/**
 * allow to know whether the message has been read or not. The message status is tied to the client
 * app.
 *
 * @return
 *  - True if the message is unread, false otherwise.
 *  - LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool SmsInbox_IsUnread
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return LE_BAD_PARAMETER;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return LE_BAD_PARAMETER;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return LE_BAD_PARAMETER;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t decode;
    decode.type = DESC_BOOL;
    char* key[] = {JSON_ISUNREAD, clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr};

    if (DecodeMsgEntry(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, messageId, key, 2,
                       &decode) == LE_OK)
    {
        return decode.uVal.boolVal;
    }
    else
    {
        LE_ERROR("Error in DecodeMsgEntry");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'read'.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_MarkRead
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t modif;
    modif.type = DESC_BOOL;
    modif.uVal.boolVal = false;
    char* key[2]={JSON_ISUNREAD, clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr};

    if (ModifyMsgEntry(messageId, key, 2, &modif) != LE_OK)
    {
        LE_ERROR("Error in ModifyMsgEntry");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark a message as 'unread'.
 *
 * @note If the caller is passing a bad message reference into this function, it is a fatal error,
 *        the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void SmsInbox_MarkUnread
(
    SmsInbox_SessionRef_t sessionRef,
        ///< [IN]
        ///< Session reference.

    uint32_t msgId
        ///< [IN]
        ///< Message identifier.
)
{
    ClientRequest_t* clientRequestPtr = le_ref_Lookup(ActivationRequestRefMap, sessionRef);
    if (NULL == clientRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", sessionRef);
        return;
    }

    if (clientRequestPtr->mboxSessionPtr == NULL)
    {
        LE_ERROR("Bad parameter");
        return;
    }

    if (CheckMessageIdInMbox(clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr, msgId) != LE_OK)
    {
        LE_ERROR("Message not included into the mbox");
        return;
    }

    MessageId_t messageId = (MessageId_t) msgId;
    EntryDesc_t modif;
    modif.type = DESC_BOOL;
    modif.uVal.boolVal = true;
    char* key[2]={JSON_ISUNREAD, clientRequestPtr->mboxSessionPtr->mboxCtxPtr->namePtr};

    if (ModifyMsgEntry(messageId, key, 2, &modif) != LE_OK)
    {
        LE_ERROR("Error in ModifyMsgEntry");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of messages for message box.
 *
 * @return
 *  - LE_BAD_PARAMETER The message box name is invalid.
 *  - LE_OVERFLOW      Message count exceed the maximum limit.
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_SetMaxMessages
(
    const char* mboxNamePtr,
        ///< [IN]
        ///< Message box name

    uint32_t maxMessageCount
        ///< [IN]
        ///< Maximum number of messages
)
{
    if (NULL == mboxNamePtr)
    {
        LE_ERROR("No mbox name");
        return LE_BAD_PARAMETER;
    }

    if (maxMessageCount > MAX_MBOX_SIZE)
    {
        LE_ERROR("Maximum number of messages is greater than max limit: %d", MAX_MBOX_SIZE);
        return LE_OVERFLOW;
    }

    int i;

    for (i = 0; i < MAX_APPS; i++)
    {
        if ((Apps[i].namePtr) && (0 == strcmp(Apps[i].namePtr, mboxNamePtr)))
        {
            char mboxConfigPath[MAX_MBOX_CONFIG_PATH_LEN];

            memset(mboxConfigPath, 0, MAX_MBOX_CONFIG_PATH_LEN);
            snprintf(mboxConfigPath, sizeof(mboxConfigPath), "%s/%s/%s",
                                                             CFG_SMSINBOX_PATH,
                                                             CFG_NODE_APPS,
                                                             mboxNamePtr);

            le_cfg_IteratorRef_t appIter = le_cfg_CreateWriteTxn(mboxConfigPath);
            Apps[i].inboxSize = maxMessageCount;
            le_cfg_SetInt(appIter, "", Apps[i].inboxSize);
            le_cfg_CommitTxn(appIter);

            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the maximum number of messages for message box.
 *
 * @return
 *  - LE_BAD_PARAMETER Invalid parameters.
 *  - LE_OK            Function succeeded.
 *  - LE_FAULT         Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t SmsInbox_GetMaxMessages
(
    const char* mboxNamePtr,
        ///< [IN]
        ///< Message box name

    uint32_t* maxMessageCountPtr
        ///< [OUT]
        ///< Maximum number of messages
)
{
    if (NULL == mboxNamePtr)
    {
        LE_ERROR("No mbox name");
        return LE_BAD_PARAMETER;
    }

    if (NULL == maxMessageCountPtr)
    {
        LE_ERROR("maxMessageCountPtr is NULL");
        return LE_BAD_PARAMETER;
    }

    int i;

    for (i = 0; i < MAX_APPS; i++)
    {
        if ((Apps[i].namePtr) && (0 == strcmp(Apps[i].namePtr, mboxNamePtr)))
        {
            char mboxConfigPath[MAX_MBOX_CONFIG_PATH_LEN];

            memset(mboxConfigPath, 0, MAX_MBOX_CONFIG_PATH_LEN);
            snprintf(mboxConfigPath, sizeof(mboxConfigPath), "%s/%s/%s",
                                                             CFG_SMSINBOX_PATH,
                                                             CFG_NODE_APPS,
                                                             mboxNamePtr);

            le_cfg_IteratorRef_t appIter = le_cfg_CreateReadTxn(mboxConfigPath);
            *maxMessageCountPtr = le_cfg_GetInt(appIter, "", 0);
            le_cfg_CancelTxn(appIter);

            return LE_OK;
        }
    }

    return LE_FAULT;

}
