/** @file manifest.c
 *
 * Install/uninstall file comes with a manifest prepended to it.  This manifest.c provides
 * APIs to extract manifest from install/uninstall file.
 *
 * Manifest string is composed of a json string prepended with its length value. Installation
 * file contents are appended after manifest string.
 *
 * Manifest Format ( tentative, subject to change later )
 *
 * <size of manifest, fixed size of 8 byte. Note: this is not included in json data>
 *  -----------JSON DATA STARTS-----------------------------------
 * versionID(M)<Str>     : Version of currently running legato framework.
 * deviceID(M)<Str>      : Target Device ID, i.e. ar7, wp7 etc
 * command(M)<Str>       : Command to execute, i.e app install APP_NAME
 * payloadSize(M)<Str>   : Size of update file attached with manifest.  For remove, update tool
 *                         will discard this field and take payloadSize 0.
 * hashKey(O)<int>       : Hash key for the payload.
 *-------------JSON_DATA_ENDS-----------------------------------
 *
 * (M)--> Mandatory field
 * (O)--> Optional field
 * <Str> --> String data type
 * <int> -->  Integer data type
 *
 * A sample manifest string with size header
 * 00000085{"versionID":"legato 1.2.3","deviceID":"ar7","command":"remove app *","payloadSize":0}
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "jansson.h"
#include "manifest.h"
#include "fileDescriptor.h"


/// Currently available json fields in manifest string.
// NOTE: These fields are subject to change.
// TODO: Might receive these fields from config file for better flexibility
#define JSON_FIELD_VERSION_ID           "versionID"
#define JSON_FIELD_DEVICE_ID            "deviceID"
#define JSON_FIELD_COMMAND              "command"
#define JSON_FIELD_PAYLOAD_SIZE         "payloadSize"
#define JSON_FIELD_HASH_KEY             "hashKey"

/// Max command string size
// TODO: Take this data from limit.h
#define MAX_CMD_BYTES                MAN_MAX_TOKENS_IN_CMD_STR*MAN_MAX_CMD_TOKEN_BYTES
#define MAX_CMD_LEN                  (MAX_CMD_BYTES - 1)

/// Max device id size
#define MAX_DEVICE_ID_LEN            (size_t)32
#define MAX_DEVICE_ID_BYTES          (MAX_DEVICE_ID_LEN + 1)

///Max size of legato version id
#define MAX_VERSION_ID_LEN           (size_t)64
#define MAX_VERSION_ID_BYTES         (MAX_VERSION_ID_LEN + 1)

///Max Hash key size
#define HASH_KEY_LEN                 (size_t)128
#define HASH_KEY_BYTES               (HASH_KEY_LEN + 1)

/// Width of first entry of manifest(i.e. manifestSize), length of this field is fixed( 8 bytes)
#define MANIFEST_SIZE_FIELD_LEN      (size_t)8
#define MANIFEST_SIZE_FIELD_BYTES   (MANIFEST_SIZE_FIELD_LEN + 1)

/// Maximum allowed size for manifest string.
// NOTE: Value of this field is subject to change.
// TODO: Might receive this field from config file for better flexibility
#define MAX_MANIFEST_SIZE               (size_t)2048

/// Delimiters for command string supplied in Manifest
#define CMD_DELIMIT_STR                 " \n\t"



//--------------------------------------------------------------------------------------------------
/**
 * Macro for printing an error message and return the supplied error code,
 */
#define RETURN_ERR_IF(condition, returnVal, formatString, ...)                          \
        if (condition) { LE_ERROR(formatString, ##__VA_ARGS__);                         \
                     return returnVal; }


/**************************************************************************************************/
/***************************** Declaration of private data types***********************************/
/**************************************************************************************************/

// -------------------------------------------------------------------------------------------------
/**
 *  The Manifest object structure.
 */
// -------------------------------------------------------------------------------------------------
typedef struct Manifest
{
    size_t manifestSize;                   ///< Size of encrypted manifest

    char versionID[MAX_VERSION_ID_BYTES];  ///< Legato Version ID
    char deviceID[MAX_DEVICE_ID_BYTES];    ///< Target Device ID (i.e. ar7, wp7)
    char command[MAX_CMD_BYTES];           ///< Command with its parameters

    size_t payLoadSize;                    ///< Attached payload size
    char hashKey[HASH_KEY_BYTES];          ///< Hashkey for the payload
}
Manifest_t;


/*************************************NOTE*********************************************************
 * Probably it is good practice to crash early in case of error. In this file, this approach is
 * not used as it is assumed that functions in this file will be used by update daemon. Probably
 * it won't be a good idea to kill update daemon based on some invalid parameter passed via
 * corrupted update file.
 **************************************************************************************************/


/**************************************************************************************************/
/********************************** Private variables  ********************************************/
/**************************************************************************************************/
/// Variables for managing manifest pool
// TODO: Extend it for handling multiple manifest.
static Manifest_t ManifestObj;
static bool       IsManifestObjCreated = false;

/**************************************************************************************************/
/***************************** Private Function definition ****************************************/
/**************************************************************************************************/

//--------------------------------------------------------------------------------------------------
/**
 * Private helper function. Extract manifest fields from json object and store them to manifest
 * structure
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetManifestFrmJson
(
    json_t *srcJsonManfstObj,             ///<[IN]  json object containing manifest data.
    man_Ref_t destManRef                  ///<[OUT] Destination to store manifest data.
)
{
    // Get version ID
    const char *temp = json_string_value(json_object_get(srcJsonManfstObj, JSON_FIELD_VERSION_ID));
    RETURN_ERR_IF((temp == NULL), LE_FAULT, "Mandatory field: %s is missing in manifest",
                  JSON_FIELD_VERSION_ID);
    RETURN_ERR_IF(
        (le_utf8_Copy(destManRef->versionID, temp, sizeof(destManRef->versionID), NULL) != LE_OK),
        LE_FAULT, "Manifest field(%s:%s) too long, Allowed: %zd B", JSON_FIELD_VERSION_ID,
        temp, MAX_VERSION_ID_LEN);

    LE_DEBUG("Manifest field: %s, Value: %s", JSON_FIELD_VERSION_ID, temp);

    temp = json_string_value(json_object_get(srcJsonManfstObj, JSON_FIELD_DEVICE_ID));
    RETURN_ERR_IF((temp == NULL), LE_FAULT, "Mandatory field: %s is missing in manifest",
                  JSON_FIELD_DEVICE_ID);
    RETURN_ERR_IF(
        (le_utf8_Copy(destManRef->deviceID, temp, sizeof(destManRef->deviceID), NULL) != LE_OK),
        LE_FAULT, "Manifest field(%s:%s) too long, Allowed: %zd B", JSON_FIELD_DEVICE_ID,
        temp, MAX_DEVICE_ID_LEN);


    LE_DEBUG("Manifest field: %s, Value: %s", JSON_FIELD_DEVICE_ID, temp);

    temp = json_string_value(json_object_get(srcJsonManfstObj, JSON_FIELD_COMMAND));
    RETURN_ERR_IF((temp == NULL), LE_FAULT, "Mandatory field: %s is missing in manifest",
                    JSON_FIELD_COMMAND);
    RETURN_ERR_IF(
        (le_utf8_Copy(destManRef->command, temp, sizeof(destManRef->command), NULL) != LE_OK),
        LE_FAULT, "Manifest field(%s:%s) too long, Allowed: %zd B", JSON_FIELD_COMMAND,
        temp, MAX_CMD_LEN);

    LE_DEBUG("Manifest field: %s, Value: %s", JSON_FIELD_COMMAND, temp);

    RETURN_ERR_IF(
        (json_is_integer(json_object_get(srcJsonManfstObj, JSON_FIELD_PAYLOAD_SIZE)) == false),
        LE_FAULT, "Incorrect/Missing manifest payload");
        destManRef->payLoadSize = json_integer_value(json_object_get(srcJsonManfstObj,
                                 JSON_FIELD_PAYLOAD_SIZE));

    LE_DEBUG("Manifest field: %s, Value: %zd", JSON_FIELD_PAYLOAD_SIZE, destManRef->payLoadSize);

    // For current Manifest, for time being hashkey is optional
    temp = json_string_value(json_object_get(srcJsonManfstObj, JSON_FIELD_HASH_KEY));
    if (temp != NULL)
    {
        RETURN_ERR_IF(
            (le_utf8_Copy(destManRef->hashKey, temp, sizeof(destManRef->hashKey), NULL) != LE_OK),
            LE_FAULT, "Manifest field(%s:%s) too long, Allowed: %zd B", JSON_FIELD_HASH_KEY,
            temp, HASH_KEY_LEN);

        LE_DEBUG("Manifest field: %s, Value: %s", JSON_FIELD_HASH_KEY, temp);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify version/target ID etc. Not implemented yet.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VerifyManifest
(
    man_Ref_t manifestData
)
{
    RETURN_ERR_IF((manifestData == NULL), LE_FAULT, "Supplied NULL manifest");
    // TODO: Get legato version and compare with manifest data??
    // TODO: Get target device ID and compare with manifest data
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * In manifest string command is provided as a single string with its parameters,
 * e.g. "app install helloWorld".  This function will take the command string and tokenize into a
 * string array. Used as a helper function of man_GetCmd() function.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCmd
(
    char *cmdStr,                             ///<[IN] String contains command and related params.
    char (*cmdlist)[MAN_MAX_CMD_TOKEN_BYTES], ///<[OUT] Output to store tokenized command & params.
    int  *cmdTokenNum                         ///<[OUT] Returns number to tokens in command string.
)
{
    char *cmdToken = cmdStr;
    int tokenNum = 0;

    *cmdTokenNum = tokenNum;

    cmdToken = strtok(cmdStr, CMD_DELIMIT_STR);

    while(cmdToken != NULL)
    {
        strcpy(cmdlist[tokenNum], cmdToken);
        RETURN_ERR_IF(
            (le_utf8_Copy(cmdlist[tokenNum], cmdToken, sizeof(cmdlist[tokenNum]), NULL) != LE_OK),
            LE_FAULT, "Too long command token: %s, Allowed: %zd B", cmdToken, MAN_MAX_CMD_TOKEN_LEN);
        tokenNum++;
        cmdToken = strtok(NULL, CMD_DELIMIT_STR);
    }

    cmdlist[tokenNum][0] = '\0';
    *cmdTokenNum = tokenNum;

    return LE_OK;
}

// -------------------------------------------------------------------------------------------------
/**
 * Get the content of manifest from supplied fileDescriptor. Manifest string is composed of a json
 * string prepended with its length value. Manifest string is feed into a json parser that returns
 * pointer to json obj structure. Later manifest data is copied from json object to  manifest
 * structure.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ParseManifest
(
    int fileDescrptr,                     ///<[IN] File pointer to read manifest.
    man_Ref_t manifestData                ///<[OUT] Destination structure to store manifest data.
)
{
    json_t *jsonData = NULL;
    json_error_t error;
    char manifestSizeStr[MANIFEST_SIZE_FIELD_LEN + 1];
    size_t manifestSize;
    ssize_t result;

    manifestSizeStr[MANIFEST_SIZE_FIELD_LEN] = '\0';

    result = fd_ReadSize(fileDescrptr, manifestSizeStr, MANIFEST_SIZE_FIELD_LEN);
    RETURN_ERR_IF((result == LE_FAULT), LE_FAULT, "Error reading manifest size, errno: %d (%m)",
                  errno);
    RETURN_ERR_IF((result != MANIFEST_SIZE_FIELD_LEN), LE_FAULT,
                  "Reached EOF before reading expected amount of data. Expected: %zd B, Read: %zd B",
                  MANIFEST_SIZE_FIELD_LEN, result);

    manifestSize = atol(manifestSizeStr);

    RETURN_ERR_IF((manifestSize > MAX_MANIFEST_SIZE) || (manifestSize <= 0), LE_FAULT,
                  "Manifest size(or parse) error, Read from file: %s, Allowed: %zd B, Parsed: %zd B",
                  manifestSizeStr, MAX_MANIFEST_SIZE, manifestSize);

    char manifestStr[manifestSize + 1];
    manifestStr[manifestSize] = '\0';

    // Read manifest data from file
    result = fd_ReadSize(fileDescrptr, manifestStr, manifestSize);
    RETURN_ERR_IF((result == LE_FAULT), LE_FAULT, "Error reading manifest. errno: %d (%m)",
                  errno);
    RETURN_ERR_IF((result != manifestSize), LE_FAULT,
                  "Reached EOF while reading manifest. Bad manifest size: %zd B, Actual: %zd B",
                  manifestSize, result);

    LE_DEBUG(" ManifestString : %s", manifestStr);

    ///TODO: Decrypt the manifest

    // Manifest string is in json format, feed it into json library for parsing.
    RETURN_ERR_IF((jsonData = json_loadb(manifestStr, manifestSize, 0, &error)) == NULL,
                  LE_FAULT,
                  "JSON import error. line: %d, column: %d, position: %d, source: '%s', error: %s",
                  error.line,
                  error.column,
                  error.position,
                  error.source,
                  error.text);

    //JSON loaded, so load manifest data from json and copy it to manifest structure
    le_result_t jsonResult = GetManifestFrmJson(jsonData, manifestData);
    json_decref(jsonData);

    return jsonResult;
}

/**************************************************************************************************/
/*****************************  Public Function definition ****************************************/
/**************************************************************************************************/

//--------------------------------------------------------------------------------------------------
/**
 * Creates a manifest object.
 *
 * @return
 *      Reference to a manifest object.
 */
//--------------------------------------------------------------------------------------------------
man_Ref_t man_Create
(
    int fileDesc                          ///<[IN] File descriptor containing the update package.
)
{
    LE_FATAL_IF((fileDesc < 0), "Supplied invalid file descriptor");
    RETURN_ERR_IF((IsManifestObjCreated == true), NULL, "Can not create multiple manifest");

    RETURN_ERR_IF((ParseManifest(fileDesc, &ManifestObj) != LE_OK), NULL, "Manifest parsing error");
    RETURN_ERR_IF((VerifyManifest(&ManifestObj) != LE_OK), NULL, "Manifest verification error");
    IsManifestObjCreated = true;

    return &ManifestObj;
}

//--------------------------------------------------------------------------------------------------
/**
 * Deletes provided manifest object.
 */
//--------------------------------------------------------------------------------------------------
void man_Delete
(
    man_Ref_t manObj                      ///<[IN] Reference to a manifest object.
)
{
    LE_FATAL_IF((IsManifestObjCreated == false), "No manifest object created");
    LE_FATAL_IF((manObj == NULL), "Supplied NULL Manifest reference");
    LE_FATAL_IF((manObj != &ManifestObj),
       "Invalid Manifest reference. Manifest reference should be created using man_Create() API");

    IsManifestObjCreated = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to get payload size from manifest.
 *
 * @return
 *      Size of PayLoad.
 */
//--------------------------------------------------------------------------------------------------
size_t man_GetPayLoadSize
(
    man_Ref_t manObj                      ///<[IN] Reference to a Manifest_t object.
)
{
    LE_FATAL_IF((IsManifestObjCreated == false), "No manifest object created yet");
    LE_FATAL_IF((manObj == NULL), "Supplied NULL Manifest reference");
    LE_FATAL_IF((manObj != &ManifestObj),
        "Invalid Manifest reference. Manifest reference should be created using man_Create() API.");


    return manObj->payLoadSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * Extracts command and its parameters from provided manifest and copy them to provided buffer.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t man_GetCmd
(
    man_Ref_t manifest,                       ///<[IN] Manifest to extract command string.
    char (*cmdList)[MAN_MAX_CMD_TOKEN_BYTES], ///<[OUT] Buffer to store command & its params.
    int *cmdParamsNum                         ///<[OUT] Destination to store no. of tokens.
)
{
    LE_FATAL_IF((IsManifestObjCreated == false), "No Manifest object created yet");
    LE_FATAL_IF((manifest == NULL), "Supplied NULL Manifest reference");
    LE_FATAL_IF((manifest != &ManifestObj),
        "Invalid Manifest reference. Manifest reference should be created using man_Create() API");
    LE_FATAL_IF((cmdList == NULL), "Invalid destination to store command and its parameters");
    LE_FATAL_IF((cmdParamsNum == NULL), "Invalid destination to store number of command tokens");

    return GetCmd(manifest->command, cmdList, cmdParamsNum);
}

