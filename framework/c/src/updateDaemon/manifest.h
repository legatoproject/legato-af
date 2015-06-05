// -------------------------------------------------------------------------------------------------
/**
 * @file manifest.h
 *
 * Update(Install/uninstall) file comes with a manifest prepended to it.  This header file provides
 * APIs to extract manifest from update file.
 *
 * Manifest string is composed of a json string prepended with its length value. Installation
 * file contents are appended after manifest string. Its the responsibility of make tools to create
 * manifest, append installation files and therefore creating the final update package.
 *
 * Manifest Format ( tentative, subject to change later )
 *
 @verbatim
  <size of manifest, fixed size of 8 byte. Note: this is not included in json data>
   -----------JSON DATA STARTS-----------------------------------
  versionID(M)<Str>     : Version of currently running legato framework.
  deviceID(M)<Str>      : Target Device ID, i.e. ar7, wp7 etc
  command(M)<Str>       : Command to execute, i.e update app APP_NAME
  payload(M)<Str>       : Total size of update items attached with manifest.
  items(M)              : Contains array of todo items(i.e. update tasks)

  ------------CONTENT_OF_ITEMS_STARTS----------------------------
  type(M)<Str>           : Target where update task should be applied.
  command(M)<Str>        : Command to execute in this item.
  appName/version(M)<Str>: Name/version of item's app/firmware.
  size(M/O)<int>         : Size of the corresponding item data. If command is specified as remove,
                           then it is optional (manifest parser will ignore it).

  Example: suppose we want to install a app helloWorld whose size 5534 is bytes, then corresponding
  json item value will be {"type":"app", "command":"install", "appName":"helloWorld", "size":5534}
 -------------CONTENT_OF_ITEMS_STARTS-----------------------------
 -------------JSON_DATA_ENDS--------------------------------------

  (M)--> Mandatory field
  (O)--> Optional field
  <Str> --> String data type
  <int> -->  Integer data type
  @endverbatim
 *
 * Please note, current implementation of manifest allow to add multiple update task in single
 * update package. In that case, update package and manifest need to be generated accordingly.
 * Please look at update-pack/le_update api documentation for more details.
 *
 * A sample manifest string with size header:
 @verbatim
  00000429
  {
   "versionID":"15.01.0.Beta-2-gd1cae43",
   "deviceID":"ar7",
   "payload":42048992,
   "items":[
     {
      "type":"firmware",
      "version":"06.04.40.00",
      "command":"install",
      "size":42043458
     },
     {
      "type":"app",
      "appName":"helloWorld",
      "command":"install",
      "size":5534
     },
     {
      "type":"app",
      "appName":"oldApp",
      "command":"remove",
     }
    ]
  }
 @endverbatim
 *
 * @note Currently manifest doesn't support encryption/hashkey, however, these are included in
 * future task list.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef PARSE_MANIFEST_INCLUDE_GUARD
#define PARSE_MANIFEST_INCLUDE_GUARD


#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * Opaque reference to a manifest object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct manifest_Obj* manifest_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Opaque reference to a item object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct manifest_Item* manifest_ItemRef_t;


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a manifest object.
 *
 * @return
 *      - Reference to a manifest object if successful.
 *      - NULL if there is an error in manifest/file descriptor.
 */
//--------------------------------------------------------------------------------------------------
manifest_Ref_t manifest_Create
(
    int fileDesc                          ///<[IN] File descriptor containing the update package.
);


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
    manifest_Ref_t manRef                      ///<[IN] Reference to a manifest object.
)
__attribute__((nonnull));


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
    manifest_Ref_t manRef                      ///<[IN] Reference to a manifest object.
)
__attribute__((nonnull));


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
__attribute__((nonnull));


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
__attribute__((nonnull));


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
__attribute__((nonnull));


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
__attribute__((nonnull));


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
__attribute__((nonnull));


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
);


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
    manifest_ItemRef_t itemRef             ///<[IN] Reference to item object. Use null to get first item.
)
__attribute__((nonnull));;


#endif                             // Endif for PARSE_MANIFEST_INCLUDE_GUARD
