//--------------------------------------------------------------------------------------------------
/**
 * @file le_power.c
 *
 * This file contains the source code of the Platform Power Source Information API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * List of power source information.  Initialized in le_power_Init().
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PowerInfoList;

//--------------------------------------------------------------------------------------------------
/**
 * Platform power source information node structure used for the PowerInfoList.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_power_PowerInfo_t powerInfo;   ///< Power source information
    le_dls_Link_t        link;        ///< Link for PowerInfoList.
}
powerInfoNode_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for platform power source information
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PowerInfoNodePool,
                          LE_POWER_SOURCE_MAX_NB,
                          sizeof(powerInfoNode_t));

//--------------------------------------------------------------------------------------------------
/**
 * Pool for platform power source information.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PowerInfoNodePool;

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the platform power source information.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_power_GetPowerInfo
(
    le_power_PowerInfo_t* powerInfoPtr,   ///< [OUT] The power source information array
    size_t* powerNbPtr                    ///< [OUT] The available power source number
)
{
    le_dls_Link_t* linkPtr;
    powerInfoNode_t* powerInfoNodePtr;
    uint8_t powerNb = 0;

    LE_ASSERT(powerInfoPtr != NULL);
    LE_ASSERT(powerNbPtr != NULL);

    linkPtr = le_dls_Peek(&PowerInfoList);

    while ( linkPtr != NULL )
    {
        powerInfoNodePtr = CONTAINER_OF(linkPtr, powerInfoNode_t, link);
        memcpy(powerInfoPtr, &powerInfoNodePtr->powerInfo, sizeof(le_power_PowerInfo_t));
        powerInfoPtr ++;
        powerNb ++;
        linkPtr = le_dls_PeekNext(&PowerInfoList, linkPtr);
    }

    *powerNbPtr = powerNb;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the platform power source information.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to set the value.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_power_SetPowerInfo
(
    const le_power_PowerInfo_t* powerInfoPtr   ///< [IN] The power source information
)
{
    le_dls_Link_t* linkPtr;
    powerInfoNode_t* powerInfoNodePtr;
    // Flag to indicate power source is in list or not.
    bool powerSourceExist = false;
    // Flag to indicate content of power source is empty or not.
    bool powerSourceEmpty = false;

    LE_ASSERT(powerInfoPtr != NULL);

    if (!powerInfoPtr->voltage && !powerInfoPtr->current
        && !powerInfoPtr->level && !powerInfoPtr->status)
    {
         powerSourceEmpty = true;
    }

    LE_DEBUG("Setting power source %d", powerInfoPtr->source);

    linkPtr = le_dls_Peek(&PowerInfoList);

    while ( linkPtr != NULL )
    {
        powerInfoNodePtr = CONTAINER_OF(linkPtr, powerInfoNode_t, link);

        if (powerInfoNodePtr->powerInfo.source == powerInfoPtr->source)
        {
            powerSourceExist = true;

            if (powerSourceEmpty)
            {
                // Power source with empty contents is designed to delete from the list.
                le_mem_Release(powerInfoNodePtr);
                le_dls_Remove(&PowerInfoList, linkPtr);
                linkPtr = NULL;
            }
            else
            {
                // Update the existing power source information.
                memcpy(&powerInfoNodePtr->powerInfo, powerInfoPtr, sizeof(le_power_PowerInfo_t));
            }

            break;
        }
        else
        {
            linkPtr = le_dls_PeekNext(&PowerInfoList, linkPtr);
        }
    }

    if (!powerSourceExist && !powerSourceEmpty)
    {
        // Add new power source information to list.
        powerInfoNodePtr = (powerInfoNode_t*)le_mem_TryAlloc(PowerInfoNodePool);

        if(powerInfoNodePtr)
        {
            memset(powerInfoNodePtr, 0, sizeof(powerInfoNode_t));
            memcpy(&powerInfoNodePtr->powerInfo, powerInfoPtr, sizeof(le_power_PowerInfo_t));
            powerInfoNodePtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(&PowerInfoList, &(powerInfoNodePtr->link));
        }
        else
        {
            LE_DEBUG("Failed to allocate memory");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Initialization of the power source information component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    // Create the pool for power source information list.
    PowerInfoNodePool = le_mem_InitStaticPool(PowerInfoNodePool,
                                              LE_POWER_SOURCE_MAX_NB,
                                              sizeof(powerInfoNode_t));
}

COMPONENT_INIT
{
    // Initialize the power source information list
    PowerInfoList = LE_DLS_LIST_INIT;
}
