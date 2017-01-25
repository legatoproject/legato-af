
//--------------------------------------------------------------------------------------------------
/**
 * .
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_AVC_UPDATE_DEFS_INCLUDE_GUARD
#define LEGATO_AVC_UPDATE_DEFS_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for for a Legato framework version string.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_VERSION_STR 100
#define MAX_VERSION_STR_BYTES (MAX_VERSION_STR + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Maximum allowed size for URI strings.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_URI_STR        255
#define MAX_URI_STR_BYTES  (MAX_URI_STR + 1)




//--------------------------------------------------------------------------------------------------
/**
 *  Called to register lwm2m object and field event handlers.
 */
//--------------------------------------------------------------------------------------------------
void aus_RegisterFieldEventHandlers
(
    const char* appNamePtr,                              ///< Namespace for the object.
    int objectId,                                        ///< The Id of the object.
    assetData_AssetActionHandlerFunc_t assetHandlerPtr,  ///< Handler for object level events.
    const int* monitorFields,                            ///< List of fields to monitor.
    size_t monFieldCount,                                ///< Size of the field list.
    assetData_FieldActionHandlerFunc_t fieldHandlerPtr   ///< Handler to be called for field
                                                         ///<   activity.
);




#endif
