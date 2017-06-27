/**
 * @file le_flash.c
 *
 * Implementation of flash definition
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"


//--------------------------------------------------------------------------------------------------
/**
 * Event ID on bad image notification.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t BadImageEventId = NULL;


//--------------------------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function
 */
//--------------------------------------------------------------------------------------------------
static void Init
(
    void
)
{
    BadImageEventId = le_event_CreateId("BadImageEvent", LE_FLASH_IMAGE_NAME_MAX_BYTES);
}

//--------------------------------------------------------------------------------------------------
/**
 * First layer Bad image handler
 */
//--------------------------------------------------------------------------------------------------
static void BadImageHandler
(
    void *reportPtr,       ///< [IN] Pointer to the event report payload.
    void *secondLayerFunc  ///< [IN] Address of the second layer handler function.
)
{
    le_flash_BadImageDetectionHandlerFunc_t clientHandlerFunc = secondLayerFunc;

    char *imageName = (char*)reportPtr;

    LE_DEBUG("Call client handler bad image name '%s'", imageName);

    clientHandlerFunc(imageName, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler
 */
//--------------------------------------------------------------------------------------------------
le_flash_BadImageDetectionHandlerRef_t le_flash_AddBadImageDetectionHandler
(
    le_flash_BadImageDetectionHandlerFunc_t handlerPtr, ///< [IN] Handler pointer
    void* contextPtr                                    ///< [IN] Associated context pointer
)
{
    if (handlerPtr == NULL)
    {
        LE_ERROR("Bad parameters");
        return NULL;
    }

    if (BadImageEventId == NULL)
    {
        Init();
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
        "BadImageDetectionHandler",
        BadImageEventId,
        BadImageHandler,
        (void*)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    if (LE_OK != pa_fwupdate_StartBadImageIndication(BadImageEventId))
    {
        return NULL;
    }

    return (le_flash_BadImageDetectionHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler...
 */
//--------------------------------------------------------------------------------------------------
void le_flash_RemoveBadImageDetectionHandler
(
    le_flash_BadImageDetectionHandlerRef_t handlerRef   ///< [IN] Connection state handler reference
)
{
    if (BadImageEventId && handlerRef)
    {
        le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
        pa_fwupdate_StopBadImageIndication();
    }
}


