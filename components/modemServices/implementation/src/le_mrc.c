/** @file le_mrc.c
 *
 * This file contains the the prototypes definitions of the high level MRC (Modem Radio Control)
 * APIs.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "le_mrc.h"
#include "pa_mrc.h"



//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for New Network Registration State notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewNetRegStateId;

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Network Registration State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNetRegStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_NetRegState_t*           statePtr = reportPtr;
    le_mrc_NetRegStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*statePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * New Network Registration State handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NewRegStateHandler
(
    le_mrc_NetRegState_t* regStatePtr
)
{
    LE_DEBUG("Handler Function called with regStat %d", *regStatePtr);

    // Notify all the registered client's handlers
    le_event_ReportWithRefCounting(NewNetRegStateId, regStatePtr);
}



//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the MRC component.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_Init
(
    void
)
{
    le_result_t                 result=LE_OK;
    pa_mrc_NetworkRegSetting_t  setting;

    // Create an event Id for new Network Registration State notification
    NewNetRegStateId = le_event_CreateIdWithRefCounting("NewNetRegState");

    // Register a handler function for new Registration State indication
    LE_DEBUG("Add pa_mrc_SetNetworkRegHandler");
    LE_FATAL_IF((pa_mrc_AddNetworkRegHandler(NewRegStateHandler) == NULL),
                "Add pa_mrc_AddNetworkRegHandler failed");

    // Get & Set the Network registration state notification
    LE_DEBUG("Get the Network registration state notification configuration");
    result=pa_mrc_GetNetworkRegConfig(&setting);
    if ((result != LE_OK) || (setting == PA_MRC_DISABLE_REG_NOTIFICATION))
    {
        LE_ERROR_IF((result != LE_OK),
                    "Fails to get the Network registration state notification configuration");

        LE_INFO("Enable the Network registration state notification");
        LE_FATAL_IF((pa_mrc_ConfigureNetworkReg(PA_MRC_ENABLE_REG_NOTIFICATION)  != LE_OK),
                    "Enable the Network registration state notification failure");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for Network registration state change.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegStateHandlerRef_t le_mrc_AddNetRegStateHandler
(
    le_mrc_NetRegStateHandlerFunc_t handlerFuncPtr, ///< [IN] The handler function.
    void*                           contextPtr      ///< [IN] The handler's context.
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NewNetRegStateHandler",
                                            NewNetRegStateId,
                                            FirstLayerNetRegStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_NetRegStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an handler for Network registration state changes.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegStateHandler
(
    le_mrc_NetRegStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    le_result_t res;

    res=pa_mrc_SetRadioPower(power);

    if (res != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Radio Module power state.
 * @return LE_OK            The function succeed.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioPower
(
    le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    le_result_t res;
    if (powerPtr == NULL)
    {
        LE_KILL_CLIENT("powerPtr is NULL !");
        return LE_FAULT;
    }

    res=pa_mrc_GetRadioPower(powerPtr);

    if (res != LE_OK)
    {
        return LE_NOT_POSSIBLE;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Network registration state.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Network registration state.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetNetRegState
(
    le_mrc_NetRegState_t*   statePtr  ///< [OUT] The Network Registration state.
)
{
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("statePtr is NULL !");
        return LE_FAULT;
    }

    if (pa_mrc_GetNetworkRegState(statePtr) == LE_OK)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Signal Quality information.
 *
 * @return LE_NOT_POSSIBLE  The function failed to get the Signal Quality information.
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetSignalQual
(
    uint32_t*   qualityPtr  ///< [OUT] The received signal strength quality (0 = no signal strength,
                            ///        5 = very good signal strength).
)
{
    le_result_t   res;
    int32_t       rssi;   // The received signal strength (in dBm).
    int32_t       thresholds[] = {-113, -100, -90, -80, -65}; // TODO: Verify thresholds !
    uint32_t      i=0;
    size_t        thresholdsCount = NUM_ARRAY_MEMBERS(thresholds);

    if (qualityPtr == NULL)
    {
        LE_KILL_CLIENT("qualityPtr is NULL !");
        return LE_FAULT;
    }

    if ((res=pa_mrc_GetSignalQuality(&rssi)) == LE_OK)
    {
        for (i=0; i<thresholdsCount; i++)
        {
            if (rssi <= thresholds[i])
            {
                *qualityPtr = i;
                break;
            }
        }
        if (i == thresholdsCount)
        {
            *qualityPtr = i;
        }

        LE_DEBUG("pa_mrc_GetSignalQuality has returned rssi=%ddBm", rssi);
        return LE_OK;
    }
    else if (res == LE_OUT_OF_RANGE)
    {
        LE_DEBUG("pa_mrc_GetSignalQuality has returned LE_OUT_OF_RANGE");
        *qualityPtr = 0;
        return LE_OK;
    }
    else
    {
        LE_ERROR("pa_mrc_GetSignalQuality has returned %d", res);
        *qualityPtr = 0;
        return LE_NOT_POSSIBLE;
    }
}

