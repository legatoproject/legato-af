
#include "legato.h"
#include "interfaces.h"
#include "../avcDaemon/assetData.h"
#include "avcUpdateShared.h"




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
)
//--------------------------------------------------------------------------------------------------
{
    assetData_AssetDataRef_t assetRef = NULL;

    LE_DEBUG("Registering on %s/%d.", appNamePtr, objectId);

    LE_FATAL_IF(assetData_GetAssetRefById(appNamePtr, objectId, &assetRef) != LE_OK,
                "Could not reference object %s/%d data.",
                appNamePtr,
                objectId);

    if (assetHandlerPtr != NULL)
    {
        LE_DEBUG("Registering AssetActionHandler");

        LE_FATAL_IF(assetData_client_AddAssetActionHandler(assetRef,
                                                           assetHandlerPtr,
                                                           NULL) == NULL,
                    "Could not register for instance activity on %s/%d.",
                    appNamePtr,
                    objectId);
    }

    for (size_t i = 0; i < monFieldCount; i++)
    {
        LE_DEBUG("Registering %s/%d/%d field handler.", appNamePtr, objectId, monitorFields[i]);

        LE_FATAL_IF(assetData_client_AddFieldActionHandler(assetRef,
                                                           monitorFields[i],
                                                           fieldHandlerPtr,
                                                           NULL) == NULL,
                    "Could not register for object %s/%d field activity.",
                    appNamePtr,
                    objectId);
    }
}
