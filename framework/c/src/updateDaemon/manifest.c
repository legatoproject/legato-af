/** @file manifest.c
 *
 * Update(install/uninstall) file comes with a manifest prepended to it.  This file provides
 * implementation of manifest APIs(declared in manifest.h file) to extract manifest from update file.
 * Please look at header file(i.e. manifest.h) documentation for details about manifest.
 *
 * @note Current implementation of manifest is part of updateDaemon. This implementation is based
 * on JSON library that uses malloc/free which may cause memory fragmentation.
 *
 * @todo Implement manifest parsing part in separate process which is short lived. This will help
 * to avoid the problem of memory fragmentation.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "jansson.h"
#include "manifest.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Available json fields in main manifest string.
 *
 * @note These fields are subject to change.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define JSON_FIELD_VERSION_ID           "versionID"
#define JSON_FIELD_DEVICE_ID            "deviceID"
#define JSON_FIELD_TOTAL_PAYLOAD        "payload"
#define JSON_FIELD_ITEMS                "items"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Available json fields in item object.
 *
 * @note These fields are subject to change.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define JSON_FIELD_TYPE                 "type"
#define JSON_FIELD_COMMAND              "command"
#define JSON_FIELD_SIZE                 "size"
#define JSON_FIELD_NAME                 "name"
#define JSON_FIELD_VERSION              "version"
// @}

//--------------------------------------------------------------------------------------------------
/**
 * Supported commands inside manifest.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define CMD_STR_INSTALL         "install"
#define CMD_STR_REMOVE          "remove"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Targets to be supported by update-daemon.
 *
 * @todo No support for framework and system yet
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define PLATFORM_FIRMWARE               "firmware"
#define PLATFORM_APPLICATION            "app"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Max device id size.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define MAX_DEVICE_ID_LEN            ((size_t)32)
#define MAX_DEVICE_ID_BYTES          (MAX_DEVICE_ID_LEN + 1)
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Max size of legato version id.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define MAX_VERSION_ID_LEN           ((size_t)64)
#define MAX_VERSION_ID_BYTES         (MAX_VERSION_ID_LEN + 1)
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Width of first entry of manifest(i.e. manifestSize), length of this field is fixed( 8 bytes).
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define MANIFEST_SIZE_FIELD_LEN      ((size_t)8)
#define MANIFEST_SIZE_FIELD_BYTES    (MANIFEST_SIZE_FIELD_LEN + 1)
// @}

//--------------------------------------------------------------------------------------------------
/**
 * Manifest item's ID length.
 */
//--------------------------------------------------------------------------------------------------
#define MANIFEST_NAME_STR_BYTES (LE_UPDATE_ID_STR_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Maximum allowed size for manifest string.
 *
 * @note Value of this field is subject to change.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MANIFEST_SIZE            ((size_t)2048)


/**************************************************************************************************/
/***************************** Declaration of private data types***********************************/
/**************************************************************************************************/


// -------------------------------------------------------------------------------------------------
/**
 *  The Manifest header object structure.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    char versionID[MAX_VERSION_ID_BYTES];  ///< Legato Version ID??
    char deviceID[MAX_DEVICE_ID_BYTES];    ///< Target Device ID (i.e. ar7, wp7)
    size_t totalPayLoad;                   ///< Attached payload size
    le_dls_List_t itemList;                ///< Linked list containing manifest item list.
}
Manifest_t;


// -------------------------------------------------------------------------------------------------
/**
 *  Structure for firmware item.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    char version[MANIFEST_NAME_STR_BYTES];    ///< Firmware version ID.
    size_t size;                              ///< Size of the firmware.
}
FirmwareItem_t;


// -------------------------------------------------------------------------------------------------
/**
 *  Structure for app item.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    le_update_Command_t command;              ///< Command (install/remove) for this app item.
    char appName[MANIFEST_NAME_STR_BYTES];    ///< App name specified in app item.
    size_t size;                              ///< Size of app installation file. Will be ignored
                                              ///< if remove command is specified.
}
AppItem_t;


// -------------------------------------------------------------------------------------------------
/**
 *  Structure for containing different update item.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t link;                       ///< Link in manifest item list.
    le_update_ItemType_t type;                ///< Item type of the ActionItem union.

    union
    {
        FirmwareItem_t firmware;              ///< Firmware item.
        AppItem_t app;                        ///< App item.
    }
    ActionItem;
}
Item_t;


/*************************************NOTE**********************************************************
 * It is a good practice to crash early in case of internal error. However, if input is coming from
 * outside(i.e client apps, outside of target device etc), it should never cause fatal error. In
 * this file, later approach is used as it is assumed that functions in this file will be used by
 * update daemon and it won't be a good idea to kill update daemon based on some invalid parameter
 * passed via corrupted update file.
 **************************************************************************************************/


/**************************************************************************************************/
/********************************** Private variables  ********************************************/
/**************************************************************************************************/


//--------------------------------------------------------------------------------------------------
/**
 * Manifest memory pool.  Must be initialized before creating any manifest object.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ManifestPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Manifest task item memory pool.  Must be initialized before creating any manifest object.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ItemPoolRef = NULL;


/**************************************************************************************************/
/***************************** Private Function definition ****************************************/
/**************************************************************************************************/


//--------------------------------------------------------------------------------------------------
/**
 * Extract item type from json object.
 *
 * @return
 *      - LE_OK if json object contains valid item type.
 *      - LE_FAULT if json object contains invalid item type.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetItemType
(
    json_t* itemPtr,                            ///<[IN] Manifest item as json object
    le_update_ItemType_t* itemType              ///<[OUT] Item type specified in json object
)
{
    const char *itemTypeStr = json_string_value(json_object_get(itemPtr, JSON_FIELD_TYPE));

    if (itemTypeStr == NULL)
    {
        LE_ERROR("Mandatory field: %s is missing in items", JSON_FIELD_TYPE);
        return LE_FAULT;
    }
    else if (strcmp(itemTypeStr, PLATFORM_APPLICATION) == 0)
    {
        *itemType = LE_UPDATE_APP;
    }
    else if (strcmp(itemTypeStr, PLATFORM_FIRMWARE) == 0)
    {
        *itemType = LE_UPDATE_FIRMWARE;
    }
    else
    {
        LE_ERROR("Unsupported item type: %s", itemTypeStr);
        return LE_FAULT;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract command type from json object.
 *
 * @return
 *      - LE_OK if json object contains valid command.
 *      - LE_FAULT if json object contains invalid command.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetCommand
(
    json_t* itemPtr,                            ///<[IN] Manifest item as json object
    le_update_Command_t* command                ///<[OUT] Command specified in json object.
)
{
    // Get the command.
     const char *commandStr = json_string_value(json_object_get(itemPtr, JSON_FIELD_COMMAND));

     if (commandStr == NULL)
     {
         LE_ERROR("Mandatory field: %s is missing in item", JSON_FIELD_COMMAND);
         return LE_FAULT;
     }

     if (strcmp(commandStr, CMD_STR_INSTALL) == 0)
     {
         *command = LE_UPDATE_CMD_INSTALL;
     }
     else if (strcmp(commandStr, CMD_STR_REMOVE) == 0)
     {
         *command = LE_UPDATE_CMD_REMOVE;
     }
     else
     {
         LE_ERROR("Unknown command: %s", commandStr);
         return LE_FAULT;
     }
     return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract mandatory json field (that is marked string) and store it into destination pointer.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetJsonStrField
(
    json_t *srcJsonPtr,                    ///<[IN]  Json source object.
    const char *keyName,                   ///<[IN]  Key in json object.
    char *const destStr,                   ///<[OUT] Destination where json value will be stored.
    size_t destLen                         ///<[IN]  Max allowed length of destination string.
)
{
    // Get key value.
    const char *srcJsonStr = json_string_value(json_object_get(srcJsonPtr, keyName));
    if (srcJsonStr == NULL)
    {
       LE_WARN("Field: %s is missing in item", keyName);
       // No need to deallocate via le_mem_Release(), Delete method will take care of it.
       return LE_FAULT;
    }

    if (le_utf8_Copy(destStr, srcJsonStr, destLen, NULL) != LE_OK)
    {
       // sizeof(destStr)-1 because last one reserved for null char.
       LE_ERROR("Item field(%s:%s) too long, Allowed: %zd B",
                keyName,
                srcJsonStr,
                destLen - 1);

       return LE_FAULT;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract mandatory json field (that is marked int) and store it into destination pointer.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetJsonSizeField
(
    json_t *srcJsonPtr,                    ///<[IN]  Json source object.
    const char *keyName,                   ///<[IN]  Key in json object.
    size_t *const destValue                ///<[OUT] Destination where json value will be stored.
)
{
    if (json_is_integer(json_object_get(srcJsonPtr, keyName)) == false)
     {
         LE_ERROR("Incorrect/Missing item field: %s", JSON_FIELD_SIZE);
         return LE_FAULT;
     }

     ssize_t size = json_integer_value(json_object_get(srcJsonPtr, keyName));
     if (size < 0)
     {
         LE_ERROR("Negative size value: %zd", size);
         return LE_FAULT;
     }
     *destValue = size;
     return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract app item from json object and store in item pool.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAppItem
(
    json_t *jsonItemPtr,                    ///<[IN] json object containing App item.
    Manifest_t *manPtr                      ///<[IN] Object containing manifest header.
)
{
    Item_t* item = le_mem_ForceAlloc(ItemPoolRef);

    item->type = LE_UPDATE_APP;
    // Now add item in the linked list
    le_dls_Queue(&(manPtr->itemList), &(item->link));

    // Get AppName
    if (GetJsonStrField(jsonItemPtr, JSON_FIELD_NAME,
                        (item->ActionItem).app.appName,
                        sizeof((item->ActionItem).app.appName)) != LE_OK)
    {
        return LE_FAULT;
    }

    le_update_Command_t command;
    le_result_t result = GetCommand(jsonItemPtr, &command);

    if (result == LE_OK)
    {
        switch(command)
        {
            case LE_UPDATE_CMD_INSTALL:
                result = GetJsonSizeField(jsonItemPtr,
                                          JSON_FIELD_SIZE,
                                          &(item->ActionItem.app.size));
                break;
            case LE_UPDATE_CMD_REMOVE:
                // Ignore size field and set it to zero.
                (item->ActionItem).app.size = 0;
                break;
        }
        (item->ActionItem).app.command = command;
    }
    LE_DEBUG("Got app item: %p, size: %zd",
              item,
              (item->ActionItem).app.size);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract firmware item from json object and store in item pool.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetFirmwareItem
(
    json_t *jsonItemPtr,                    ///<[IN] json object containing firmware item.
    Manifest_t *manPtr                      ///<[IN] Object containing manifest header.
)
{
    Item_t* item = le_mem_ForceAlloc(ItemPoolRef);

    item->type = LE_UPDATE_FIRMWARE;

    // Now add item in the linked list
    le_dls_Queue(&(manPtr->itemList), &(item->link));

    memset((item->ActionItem).firmware.version,
           0,
           sizeof((item->ActionItem).firmware.version));

    // Get firmware version. This is optional field
    le_result_t result = GetJsonStrField(jsonItemPtr,
                                         JSON_FIELD_VERSION,
                                         (item->ActionItem).firmware.version,
                                         sizeof((item->ActionItem).firmware.version));

    result = GetJsonSizeField(jsonItemPtr,
                              JSON_FIELD_SIZE,
                              &((item->ActionItem).firmware.size));
    LE_DEBUG("Got firmware item: %p, size: %zd",
              item,
              (item->ActionItem).firmware.size);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extracts manifest items from json object and store in item pool.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetManifestItems
(
    json_t *jsonItemPtr,                    ///<[IN] json object containing manifest items.
    Manifest_t *manPtr                      ///<[IN] Object containing manifest header.
)
{
    size_t arraySize;

    // Manifest items are collection of action items to do. Extract all action items.
    arraySize = json_array_size(jsonItemPtr);
    if (arraySize == 0)
    {
        LE_ERROR("Bad format. Json field %s must be an array.", JSON_FIELD_ITEMS);
        return LE_FAULT;
    }

    json_t* value;
    size_t index;

    json_array_foreach(jsonItemPtr, index, value)  // Iterates until all json items finish.
    {
        le_update_ItemType_t itemType;
        // Get the item type.
        le_result_t result = GetItemType(value, &itemType);

        if (result == LE_OK)
        {
            switch (itemType)
            {
                // Now fill up the item pool as per the item type.
                case LE_UPDATE_APP:
                    result = GetAppItem(value, manPtr);
                    break;
                case LE_UPDATE_FIRMWARE:
                    result = GetFirmwareItem(value, manPtr);
                    break;
            }
        }

        // Return immediately in case of any bad item.
        if(result != LE_OK)
        {
            return LE_FAULT;
        }
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract manifest fields from json object and store them to manifest/item structure.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetManifestFromJson
(
    json_t *srcJsonPtr,                    ///<[IN]  json object containing manifest data.
    Manifest_t *destManPtr                 ///<[OUT] Destination to store manifest data.
)
{
    // Get version ID
    if (GetJsonStrField(srcJsonPtr,
                        JSON_FIELD_VERSION_ID,
                        destManPtr->versionID,
                        sizeof(destManPtr->versionID)) != LE_OK)
    {
        return LE_FAULT;
    }

    // Get device ID
    if (GetJsonStrField(srcJsonPtr,
                        JSON_FIELD_DEVICE_ID,
                        destManPtr->deviceID,
                        sizeof(destManPtr->deviceID)) != LE_OK)
    {
        return LE_FAULT;
    }

    if (GetJsonSizeField(srcJsonPtr, JSON_FIELD_TOTAL_PAYLOAD, &(destManPtr->totalPayLoad)) != LE_OK)
    {
        return LE_FAULT;
    }

    // Now get the todo items.
    json_t* jsonItem;
    jsonItem = json_object_get(srcJsonPtr, JSON_FIELD_ITEMS);
    if (jsonItem == NULL)
    {
        LE_ERROR("Mandatory field: %s is missing in manifest", JSON_FIELD_ITEMS);
        return LE_FAULT;
    }

    // Parse and store all todo items.
    if (GetManifestItems(jsonItem, destManPtr) != LE_OK)
    {
        LE_ERROR("Bad Manifest field %s.", JSON_FIELD_ITEMS);
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify the supplied manifest.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VerifyManifest
(
    Manifest_t *manPtr                  ///<[IN] Supplied manifest to verify.
)
{
    // TODO: Partially implemented(only command and its params are verified). Include other feature.

    // Verify whether size is consistent.
    size_t allItemSize = 0;
    le_dls_Link_t* itemLinkPtr = le_dls_Peek(&(manPtr->itemList));
    bool otherFlag = false;
    bool fwFlag = false;

    // Calculate total payload size using size field in each item.
    // Also check the exclusiveness.
    while (itemLinkPtr != NULL)
    {
        Item_t* itemPtr = CONTAINER_OF(itemLinkPtr, Item_t, link);
        size_t itemSize = 0;

        switch (itemPtr->type)
        {
            case LE_UPDATE_APP:
                itemSize = (itemPtr->ActionItem).app.size;
                otherFlag = true;
                break;
            case LE_UPDATE_FIRMWARE:
                itemSize = (itemPtr->ActionItem).firmware.size;
                fwFlag = true;
                break;
        }

        if (otherFlag & fwFlag)
        {
            LE_ERROR("Bad update package. Firmware can't be put with other package");
            return LE_FAULT;
        }

        allItemSize += itemSize;
        itemLinkPtr = le_dls_PeekNext(&(manPtr->itemList), itemLinkPtr);
    }

    // Now compare calculated payload size and manifest header payload size.
    if (manPtr->totalPayLoad != allItemSize)
    {
        LE_ERROR("Bad payload. Specified: %zd, calculated: %zd",
                 manPtr->totalPayLoad,
                 allItemSize);
        return LE_FAULT;
    }
    // TODO: Get legato version and compare with manifest data??
    // TODO: Get target device ID and compare with manifest data
    return LE_OK;
}


// -------------------------------------------------------------------------------------------------
/**
 * Get the content of manifest from supplied fileDescriptor.
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if there was an error.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t ParseManifest
(
    int fileDescrptr,                    ///<[IN] File pointer to read manifest.
    Manifest_t *manifestPtr              ///<[OUT] Destination structure to store manifest data.
)
{
    json_t *jsonDataPtr = NULL;
    json_error_t error;
    char manifestSizeStr[MANIFEST_SIZE_FIELD_LEN + 1];
    size_t manifestSize;
    ssize_t result;
    char* end;

    manifestSizeStr[MANIFEST_SIZE_FIELD_LEN] = '\0';

    result = fd_ReadSize(fileDescrptr, manifestSizeStr, MANIFEST_SIZE_FIELD_LEN);
    if (result == LE_FAULT)
    {
        LE_ERROR("Error reading manifest size (%m)");
        return LE_FAULT;
    }
    if (result != MANIFEST_SIZE_FIELD_LEN)
    {
        LE_ERROR("Reached EOF before reading expected amount of data. Expected: %zd B, Read: %zd B"
                 "Manifest string: %s", MANIFEST_SIZE_FIELD_LEN, result, manifestSizeStr);
        return LE_FAULT;
    }

    manifestSize = strtoll(manifestSizeStr, &end, 10);

    if ((*end) || (manifestSize > MAX_MANIFEST_SIZE) || (manifestSize <= 0))
    {
        LE_ERROR("Manifest size(or parse) error, Read from file: %s, Allowed: %zd B, Parsed: %zd B",
                 manifestSizeStr, MAX_MANIFEST_SIZE, manifestSize);
        return LE_FAULT;
    }

    char manifestStr[manifestSize + 1];
    manifestStr[manifestSize] = '\0';

    // Read manifest data from file.
    result = fd_ReadSize(fileDescrptr, manifestStr, manifestSize);
    if (result == LE_FAULT)
    {
        LE_ERROR("Error reading manifest (%m).");
        return LE_FAULT;
    }
    if (result != manifestSize)
    {
        LE_ERROR("Reached EOF while reading manifest. Bad manifest size: %zd B, Actual: %zd B",
                 manifestSize, result);
        return LE_FAULT;
    }
    LE_DEBUG(" ManifestString : %s", manifestStr);

    // Manifest string is in json format, feed it into json library for parsing.
    if ((jsonDataPtr = json_loadb(manifestStr, manifestSize, 0, &error)) == NULL)
    {

        LE_ERROR("JSON import error. line: %d, column: %d, position: %d, source: '%s', error: %s",
                  error.line,
                  error.column,
                  error.position,
                  error.source,
                  error.text);
        return LE_FAULT;
    }

    //JSON loaded, so load manifest data from json and copy it to manifest structure
    le_result_t jsonResult = GetManifestFromJson(jsonDataPtr, manifestPtr);
    json_decref(jsonDataPtr);
    if (jsonResult != LE_OK)
    {
        return jsonResult;
    }

    if (VerifyManifest(manifestPtr) != LE_OK)
    {
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the supplied manifest and including its Items.
 */
//--------------------------------------------------------------------------------------------------
void DeleteManifest
(
    Manifest_t* manPtr              ///<[IN] Manifest object to delete.
)
{
    LE_DEBUG("Deleting manifest, %p", manPtr);
    // First release all items related to this manifest.
    le_dls_Link_t* itemLinkPtr = le_dls_Peek(&(manPtr->itemList));
    while (itemLinkPtr != NULL)
    {
        Item_t* itemPtr = CONTAINER_OF(itemLinkPtr, Item_t, link);
        // Get the next link, otherwise it will be destroyed as part of calling le_dls_Remove().
        itemLinkPtr = le_dls_PeekNext(&(manPtr->itemList), itemLinkPtr);

        LE_DEBUG("Deleting item with itemLinkPtr: %p, ItemPtr: %p, Link: %p",
                 itemLinkPtr,
                 itemPtr,
                 &(itemPtr->link));
        // Remove from linked list.
        le_dls_Remove(&(manPtr->itemList), &(itemPtr->link));
        // Deallocate the memory.
        le_mem_Release(itemPtr);
    }
    // Now release manifest from memory pool.
    le_mem_Release(manPtr);
    LE_DEBUG("Deleted manifest: %p", manPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a manifest object with its items.
 *
 * @return
 *      - Reference to a manifest object if successful.
 *      - NULL if file contains wrong manifest.
 */
//--------------------------------------------------------------------------------------------------
Manifest_t* CreateManifest
(
    int fileDesc                          ///<[IN] File descriptor containing the update package.
)
{

    Manifest_t *manifestPtr;

    // Force allocate memory pool.
    manifestPtr = le_mem_ForceAlloc(ManifestPoolRef);
    // Initialize item linked list with null. and then populate it.
    manifestPtr->itemList = LE_DLS_LIST_INIT;

    if (ParseManifest(fileDesc, manifestPtr) != LE_OK)
    {
        LE_ERROR("Manifest parsing error");
        DeleteManifest(manifestPtr);
        return NULL;
    }

    return manifestPtr;
}


/**************************************************************************************************/
/*****************************  Public Function definition ****************************************/
/**************************************************************************************************/


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the manifest module.
 *
 * @note
 *      This function must be called exactly once before creation of any manifest object.
 */
//--------------------------------------------------------------------------------------------------
void manifest_Init
(
    void
)
{

#define DEFAULT_MANIFEST_POOL_SIZE      1
#define DEFAULT_ITEM_POOL_SIZE          1
#define MANIFEST_POOL_NAME              "ManifestObjPool"
#define ITEM_POOL_NAME                  "ItemPool"

    ManifestPoolRef = le_mem_CreatePool(MANIFEST_POOL_NAME, sizeof(Manifest_t));
    ItemPoolRef = le_mem_CreatePool(ITEM_POOL_NAME, sizeof(Item_t));
    le_mem_ExpandPool(ManifestPoolRef, DEFAULT_MANIFEST_POOL_SIZE);
    le_mem_ExpandPool(ItemPoolRef, DEFAULT_ITEM_POOL_SIZE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a manifest object.
 *
 * @return
 *      - Reference to a manifest object if successful.
 *      - NULL if there is an error or reached maximum number of allowed manifest.
 */
//--------------------------------------------------------------------------------------------------
manifest_Ref_t manifest_Create
(
    int fileDesc                          ///<[IN] File descriptor containing the update package.
)
{
    LE_ASSERT(fileDesc >= 0);
    return (manifest_Ref_t)(CreateManifest(fileDesc));
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a manifest object including its items.
 *
 * @note
 *      Must supply a valid manifest reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
void manifest_Delete
(
    manifest_Ref_t manifestRef            ///<[IN] Reference to a manifest object.
)
{
    LE_ASSERT(manifestRef != NULL);
    // Delete manifest and its items.
    DeleteManifest((Manifest_t*)manifestRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get payload size from manifest.
 *
 * @return
 *      - Size of total PayLoad.
 *
 * @note
 *      Must supply a valid manifest reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
size_t manifest_GetTotalPayLoad
(
    manifest_Ref_t manifestRef            ///<[IN] Reference to a manifest object.
)
{
    LE_ASSERT(manifestRef != NULL);
    return ((Manifest_t *)manifestRef)->totalPayLoad;
}


///--------------------------------------------------------------------------------------------------
/**
 * Function to get Item type for supplied item.
 *
 * @return
 *      - Item type of supplied item.
 *
 * @note
 *      Must supply a valid item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
le_update_ItemType_t manifest_GetItemType
(
    manifest_ItemRef_t itemRef             ///<[IN] Reference to manifest item object.
)
{
    LE_ASSERT(itemRef != NULL);
    return ((Item_t *)itemRef)->type;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get item command for supplied item.
 *
 * @return
 *      - Command in supplied item.
 *
 * @note
 *      Must supply a valid item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
le_update_Command_t manifest_GetItemCmd
(
    manifest_ItemRef_t itemRef             ///<[IN] Reference to manifest item object.
)
{
    LE_ASSERT(itemRef != NULL);

    le_update_Command_t command;
    switch (((Item_t *)itemRef)->type)
    {
        case LE_UPDATE_APP:
            command = ((Item_t *)itemRef)->ActionItem.app.command;
            break;
        case LE_UPDATE_FIRMWARE:
            command = LE_UPDATE_CMD_INSTALL;
            break;
    }

    return command;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get item size for supplied item.
 *
 * @return
 *      - Item size for supplied item.
 *
 * @note
 *      Must supply a valid item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
size_t manifest_GetItemSize
(
    manifest_ItemRef_t itemRef             ///<[IN] Reference to manifest item object.
)
{
    LE_ASSERT(itemRef != NULL);

    size_t size=0;
    switch (((Item_t *)itemRef)->type)
    {
        case LE_UPDATE_APP:
            size = (((Item_t *)itemRef)->ActionItem).app.size;
            break;
        case LE_UPDATE_FIRMWARE:
            size = (((Item_t *)itemRef)->ActionItem).firmware.size;
            break;
    }
    LE_DEBUG("Item size: %zd", size);
    return size;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get app item name for supplied manifest item.
 *
 * @return
 *      - AppName in manifest item
 *
 * @note
 *      Must supply a valid app item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
const char* manifest_GetAppItemName
(
    manifest_ItemRef_t itemRef              ///<[IN] Reference to manifest item object.
)
{
    LE_ASSERT(itemRef != NULL);
    LE_ASSERT(((Item_t *)itemRef)->type == LE_UPDATE_APP);
    return ((Item_t *)itemRef)->ActionItem.app.appName;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get firmware item version for supplied manifest item.
 *
 * @return
 *      - Version in manifest item.
 *
 * @note
 *      Must supply a valid firmware item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
const char* manifest_GetFwItemVersion
(
    manifest_ItemRef_t itemRef              ///<[IN] Reference to manifest item object.
)
{
    LE_ASSERT(itemRef != NULL);
    LE_ASSERT(((Item_t *)itemRef)->type == LE_UPDATE_FIRMWARE);
    return ((Item_t *)itemRef)->ActionItem.firmware.version;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get next item reference.
 *
 * @return
 *      - Next item reference if exists.
 *      - Null if there is not further item.
 */
//--------------------------------------------------------------------------------------------------
manifest_ItemRef_t manifest_GetNextItem
(
    manifest_Ref_t manifestRef,            ///<[IN] Reference to a manifest object.
    manifest_ItemRef_t itemRef             ///<[IN] Reference to item object. Use null to get first item.
)
{
    LE_ASSERT(manifestRef != NULL);

    le_dls_Link_t* itemLinkPtr;
    manifest_ItemRef_t nextItemRef = NULL;

    if (itemRef == NULL)
    {
         itemLinkPtr = le_dls_Peek(&(((Manifest_t *)manifestRef)->itemList));
    }
    else
    {
        itemLinkPtr = le_dls_PeekNext(&(((Manifest_t *)manifestRef)->itemList),
                                      &(((Item_t*)itemRef)->link));
    }

    if(itemLinkPtr)
    {
        nextItemRef = (manifest_ItemRef_t)(CONTAINER_OF(itemLinkPtr, Item_t, link));
        LE_DEBUG("Got item: %p", nextItemRef);
    }

    return nextItemRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to check whether it is last item.
 *
 * @return
 *      - true if it is last item
 *      - false if it is not last item
 *
 * @note
 *      Must supply a valid manifest and item reference, otherwise process will exit.
 */
//--------------------------------------------------------------------------------------------------
bool manifest_IsLastItem
(
    manifest_Ref_t manifestRef,            ///<[IN] Reference to a manifest object.
    manifest_ItemRef_t itemRef             ///<[IN] Reference to item. Use null to get first item.
)
{
    LE_ASSERT(manifestRef != NULL);
    LE_ASSERT(itemRef != NULL);
    return manifest_GetNextItem(manifestRef, itemRef) == NULL;
}
