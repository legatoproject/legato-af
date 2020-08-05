//--------------------------------------------------------------------------------------------------
/**
 *  Clock Service's C code implementation of its le_clkSync APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include "legato.h"
#include "interfaces.h"
#if LE_CONFIG_ENABLE_CONFIG_TREE
#include "le_cfg_interface.h"
#endif
#include "le_print.h"
#include "clockSync.h"
#include "pa_clkSync.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
/**
 * Timer interval to kick the watchdog chain
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 30


//--------------------------------------------------------------------------------------------------
/**
 * Names of all clock sources in string format
 */
//--------------------------------------------------------------------------------------------------
static const char* ClockSourceTypeString[LE_CLKSYNC_CLOCK_SOURCE_MAX] = {"tp", "ntp", "gps"};

//--------------------------------------------------------------------------------------------------
/**
 * ClockSyncUpdateDb_t is the data structure Db that holds a clock source config for acquiring this
 * source's provided current time.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int16_t priority;                                        ///< priority of the source
    uint16_t source;                                         ///< source type: TP, NTP, GPS
    char sourceConfig1[LE_CLKSYNC_SOURCE_CONFIG_LENGTH_MAX]; ///< 1st source config
    char sourceConfig2[LE_CLKSYNC_SOURCE_CONFIG_LENGTH_MAX]; ///< 2nd source config
    le_clkSync_UpdateSystemStatus_t updateStatus;            ///< update status
    le_dls_Link_t dbLink;                                    ///< double link list's link element
} ClockSyncUpdateDb_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data structures used to maintain the clock sources in priority
 * ClockSyncSourcePrioritizedList: the double linked-list used to keep the clock sources in their
 * ascending order of priority
 * ClockSyncUpdateDbs: the array of all the configured clock sources read from the config tree.
 *
 * Note that ClockSyncUpdateDbs[] is not always in-sync with the configs on the config tree; in
 * another word, the config changes on the config tree do not show up in ClockSyncUpdateDbs[] right
 * away until the Clock Service starts acquiring the current clock time. And when the Clock Service
 * is running to acquire the current clock time, any sudden config changes wouldn't be picked up to
 * take effect.
 * This data structure is designed for operational use in executing clock time update than for
 * config management. Up-to-date clock source configs have to be always retrieved from the config
 * tree, not ClockSyncUpdateDbs[].
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ClockSyncSourcePrioritizedList;
static ClockSyncUpdateDb_t ClockSyncUpdateDbs[LE_CLKSYNC_CLOCK_SOURCE_MAX];

//--------------------------------------------------------------------------------------------------
/**
 * Clock sync command ID
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ClockSyncCommandEventId;

typedef enum
{
    CLOCKSYNC_COMMAND_UPDATE_CLOCK = 0        ///< Command to update clock
}
ClockSyncCommandType_t;


//--------------------------------------------------------------------------------------------------
/**
 * ClockSync command event structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    ClockSyncCommandType_t commandType;                       ///< Command
    void *context;                                            ///< Context
    le_clkSync_UpdateSystemTimeHandlerFunc_t handlerFunc;     ///< Handler function
}
ClockSyncCommand_t;


//--------------------------------------------------------------------------------------------------
/**
 * Callback handler function for a clock update execution
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_clkSync_UpdateSystemTimeHandlerFunc_t handlerFunc;
    void* contextPtr;
} ClockSyncUpdateHandler_t;

static ClockSyncUpdateHandler_t ClockSyncUpdateHandler;

#if LE_CONFIG_ENABLE_CONFIG_TREE
//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve from the config tree the configs of the given clock source in the input as
 * server name or IPv4/v6 address. There can be a max of 2 config entries, with the 1st one at
 * clockTime:/source/tp/config/1 <string> and the 2nd at clockTime:/source/tp/config/2 <string>
 *
 * @return
 *     - LE_OK: some configs of the given clock source's have been successfully retrieved
 *     - LE_NOT_FOUND: no config can be found for the given clock source
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetClockSourceConfigs
(
    le_clkSync_ClockSource_t source,
    char* config1Ptr,
    char* config2Ptr,
    size_t configPtrSize
)
{
    le_cfg_IteratorRef_t cfg;
    char configPath[LE_CFG_STR_LEN_BYTES];
    bool found = false;

    snprintf(configPath, LE_CFG_STR_LEN_BYTES, "%s/%s/%s", LE_CLKSYNC_CONFIG_TREE_ROOT_SOURCE,
             ClockSourceTypeString[source], LE_CLKSYNC_CONFIG_NODE_SOURCE_CONFIG);
    cfg = le_cfg_CreateReadTxn(configPath);
    if (!cfg)
    {
        LE_INFO("Clock source %d got no config set", source);
        return LE_NOT_FOUND;
    }

    // Seek to retrieve the 1st config
    if (!le_cfg_NodeExists(cfg, "1"))
    {
        LE_INFO("Clock source %d has no config 1", source);
    }
    else
    {
        if ((LE_OK != le_cfg_GetString(cfg, "1", config1Ptr, configPtrSize, "")) ||
            (strlen(config1Ptr) == 0))
        {
            LE_INFO("Clock source %d has no config 1", source);
            config1Ptr[0] = '\0';
        }
        else
        {
            LE_INFO("Clock source %d has config %s", source, config1Ptr);
            found = true;
        }
    }

    // Seek to retrieve the 2nd config
    if (!le_cfg_NodeExists(cfg, "2"))
    {
        LE_INFO("Clock source %d has no config 2", source);
    }
    else
    {
        if ((LE_OK != le_cfg_GetString(cfg, "2", config2Ptr, configPtrSize, "")) ||
            (strlen(config2Ptr) == 0))
        {
            LE_INFO("Clock source %d has no config 2", source);
            config2Ptr[0] = '\0';
        }
        else
        {
            LE_INFO("Clock source %d has config %s", source, config2Ptr);
            found = true;
        }
    }

    le_cfg_CancelTxn(cfg);
    return (found ? LE_OK : LE_NOT_FOUND);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Function for saving an app provided async callback function and context for posting back the
 * the result of a clock update execution when ready
 */
//--------------------------------------------------------------------------------------------------
static void le_clkSync_AddUpdateClockHandler
(
    le_clkSync_UpdateSystemTimeHandlerFunc_t handlerFunc,
    void *context
)
{
    if (!handlerFunc)
    {
        LE_DEBUG("No callback function to add");
        return;
    }

    if (ClockSyncUpdateHandler.handlerFunc)
    {
        LE_DEBUG("Callback function already registered");
        return;
    }

    ClockSyncUpdateHandler.handlerFunc = handlerFunc;
    ClockSyncUpdateHandler.contextPtr = context;
    LE_INFO("Callback function %p context %p added", handlerFunc, context);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for removing the previously prioritized clock sources from the prioritized list
 * ClockSyncSourcePrioritizedList
 */
//--------------------------------------------------------------------------------------------------
static void ClockSyncResetPrioritizedSources
(
    void
)
{
    ClockSyncUpdateDb_t *updateDbPtr;
    le_dls_Link_t *updateDbLinkPtr = le_dls_Peek(&ClockSyncSourcePrioritizedList);
    while (updateDbLinkPtr)
    {
        updateDbPtr = CONTAINER_OF(updateDbLinkPtr, ClockSyncUpdateDb_t, dbLink);
        updateDbLinkPtr = le_dls_PeekNext(&ClockSyncSourcePrioritizedList, updateDbLinkPtr);
        if (!updateDbPtr)
        {
            continue;
        }
        le_dls_Remove(&ClockSyncSourcePrioritizedList, &updateDbPtr->dbLink);
        updateDbPtr->dbLink = LE_DLS_LINK_INIT;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving clock source configs from the Clock Service's Config Tree path for the
 * given clock source type and, if configured, save them into the provided ClockSyncUpdateDb_t and
 * return to the caller its configured priority.
 * If not configured, return -1 back.
 *
 * @return
 *     -1 if the given clock source isn't configured; otherwise its configured priority
 */
//--------------------------------------------------------------------------------------------------
static int16_t ClockSyncReadSourceConfigs
(
    uint16_t source,
    ClockSyncUpdateDb_t *updateDb
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    le_cfg_IteratorRef_t cfg;
    char configPath[LE_CFG_STR_LEN_BYTES];
    bool isConfigured = false;

    if (source >= LE_CLKSYNC_CLOCK_SOURCE_MAX)
    {
        LE_ERROR("Clock source enum %d unsupported", source);
        return -1;
    }

    updateDb->source = source;
    updateDb->priority = -1;
    updateDb->updateStatus = (source == LE_CLKSYNC_CLOCK_SOURCE_GPS) ?
        LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNSUPPORTED: LE_CLKSYNC_UPDATE_SYSTEM_STATUS_NOT_CONFIGURED;
    updateDb->sourceConfig1[0] = updateDb->sourceConfig2[0] = '\0';
    updateDb->dbLink = LE_DLS_LINK_INIT;

    cfg = le_cfg_CreateReadTxn(LE_CLKSYNC_CONFIG_TREE_ROOT_SOURCE);
    if (!cfg)
    {
        LE_INFO("No clock source %d configured", source);
        return -1;
    }
    if (!le_cfg_NodeExists(cfg, ClockSourceTypeString[source]))
    {
        LE_INFO("Clock source %d not configured", source);
        le_cfg_CancelTxn(cfg);
        return -1;
    }

    // Retrieve the source's priority
    le_cfg_CancelTxn(cfg);
    snprintf(configPath, LE_CFG_STR_LEN_BYTES, "%s/%s", LE_CLKSYNC_CONFIG_TREE_ROOT_SOURCE,
             ClockSourceTypeString[source]);
    cfg = le_cfg_CreateReadTxn(configPath);
    if (!cfg)
    {
        LE_INFO("Clock source %d with no priority configured", source);
        updateDb->priority = LE_CLKSYNC_SOURCE_PRIORITY_MIN;
    }
    else
    {
        updateDb->priority = le_cfg_GetInt(cfg, LE_CLKSYNC_CONFIG_NODE_SOURCE_PRIORITY,
                                           LE_CLKSYNC_SOURCE_PRIORITY_MIN);
        isConfigured = true;
    }

    // Retrieve the source's configs
    if (LE_OK == GetClockSourceConfigs(source, updateDb->sourceConfig1, updateDb->sourceConfig2,
                                       LE_CLKSYNC_SOURCE_CONFIG_LENGTH_MAX))
    {
        LE_INFO("Clock source configs retrieved: %s and %s", updateDb->sourceConfig1,
                updateDb->sourceConfig2);
        isConfigured = true;
    }

    if (isConfigured && (source != LE_CLKSYNC_CLOCK_SOURCE_GPS))
    {
        updateDb->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_NOT_TRIED;
    }

    le_cfg_CancelTxn(cfg);
    return updateDb->priority;
#else
    LE_INFO("No clock source %d configured", source);
    return -1;
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for comparing the 2 ClockSyncUpdateDb_t's of 2 given clock sources during the step to
 * insert them into their prioritized list, ClockSyncSourcePrioritizedList, in descending order.
 *
 * @return
 *     true if node aLinkPtr has a clock source with higher priority than node bLinkPtr; false
 *     otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool UpdateDbComparePriority
(
    le_dls_Link_t* aLinkPtr,
    le_dls_Link_t* bLinkPtr
)
{
    ClockSyncUpdateDb_t *aPtr = CONTAINER_OF(aLinkPtr, ClockSyncUpdateDb_t, dbLink);
    ClockSyncUpdateDb_t *bPtr = CONTAINER_OF(bLinkPtr, ClockSyncUpdateDb_t, dbLink);
    return (aPtr->priority > bPtr->priority);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for parsing all the clock source configs on the Clock Service's config tree path and
 * save them into the data structure ClockSyncUpdateDbs[source] one by one.
 * This is done only before the Clock Service starts trying to acquire the current clock time from
 * these configured clock sources in their descending priority until success.
 * Please refer to the headline explanation for the use of ClockSyncUpdateDbs[] in the start of
 * this file for more details.
 *
 * @return
 *     Number of configured clock source(s)
 */
//--------------------------------------------------------------------------------------------------
static uint16_t ClockSyncParseSourceConfigs
(
    void
)
{
    uint16_t source, numDb = 0;
    int16_t priority;

    if (!le_dls_IsEmpty(&ClockSyncSourcePrioritizedList))
    {
        ClockSyncResetPrioritizedSources();
    }

    for (source = 0; source < LE_CLKSYNC_CLOCK_SOURCE_MAX; source++)
    {
        priority = ClockSyncReadSourceConfigs(source, &ClockSyncUpdateDbs[source]);
        if (-1 != priority)
        {
            // Insert the current element to the end of the list non-prioritized until later
            le_dls_Queue(&ClockSyncSourcePrioritizedList, &ClockSyncUpdateDbs[source].dbLink);
            numDb += 1;
        }
    }
    return numDb;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function for converting the given le_result_t result code of a current clock time read/update
 * operation into status code of enum type le_clkSync_UpdateSystemStatus. If it's a success, also
 * set the corresponding clock source into the given ClockSyncUpdateDb_t.
 */
//--------------------------------------------------------------------------------------------------
static void SetClockUpdateStatus
(
    ClockSyncUpdateDb_t *updateDbPtr,
    le_clkSync_ClockSource_t* sourceToSet,
    le_result_t result
)
{
    switch (result)
    {
        case LE_OK:
            updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_SUCCESS;
            *sourceToSet = (le_clkSync_ClockSource_t)updateDbPtr->source;
            break;
        case LE_NOT_FOUND:
        case LE_UNAVAILABLE:
            updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNAVAILABLE;
            break;
        case LE_UNSUPPORTED:
            updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNSUPPORTED;
            break;
        case LE_FAULT:
            updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_GET_ERROR;
            break;
        case LE_BAD_PARAMETER:
        default:
            updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_GENERAL_ERROR;
            break;
    }

    LE_DEBUG("Clock source %s update status retrieved: %d",
             ClockSourceTypeString[updateDbPtr->source], updateDbPtr->updateStatus);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get clock time update as configured. Depending on the input argument getOnly, update the system
 * clock time with this acquired current time or skip it.
 *
 * @return
 *     - LE_OK: succeeded
 *     - LE_UNAVAILABLE: no clock source configured
 *     - LE_FAULT: failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ClockSyncGetUpdate
(
    bool getOnly,                    ///< [IN] Get time only or also update the system clock time
    le_clkSync_ClockTime_t* timePtr, ///< [OUT] Clock time retrieved from configured source
    le_clkSync_ClockSource_t* source ///< [OUT] Clock source from which the current time is acquired
)
{
    le_dls_Link_t *updateDbLinkPtr;
    ClockSyncUpdateDb_t *updateDbPtr;
    le_result_t result = LE_FAULT;
    uint16_t numDb = ClockSyncParseSourceConfigs();

    if (numDb == 0)
    {
        LE_INFO("No clock source config for acquiring current time");
        *source = LE_CLKSYNC_CLOCK_SOURCE_MAX;
        return LE_UNAVAILABLE;
    }

    // Sort configured clock sources according to priority 1st
    LE_DEBUG("Sorting %d clock sources' priority", numDb);
    le_dls_Sort(&ClockSyncSourcePrioritizedList, UpdateDbComparePriority);

    // Try executing configured clock sources according to priority until success
    updateDbLinkPtr = le_dls_Peek(&ClockSyncSourcePrioritizedList);
    while (updateDbLinkPtr)
    {
        updateDbPtr = CONTAINER_OF(updateDbLinkPtr, ClockSyncUpdateDb_t, dbLink);
        updateDbLinkPtr = le_dls_PeekNext(&ClockSyncSourcePrioritizedList, updateDbLinkPtr);
        LE_DEBUG("Next clock source in priority: source %d, priority %d, configs %s and %s",
                 updateDbPtr->source, updateDbPtr->priority,
                 (updateDbPtr->sourceConfig1[0]!='\0') ? updateDbPtr->sourceConfig1 : "null",
                 (updateDbPtr->sourceConfig2[0]!='\0') ? updateDbPtr->sourceConfig2 : "null");

        // Execute clock update using each retrieved clock source
        switch (updateDbPtr->source)
        {
            case LE_CLKSYNC_CLOCK_SOURCE_TP:
                if (strlen(updateDbPtr->sourceConfig1))
                {
                    LE_INFO("Trying clock source TP and config %s", updateDbPtr->sourceConfig1);
                    updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_IN_PROGRESS;
                    result = pa_clkSync_GetTimeWithTimeProtocol(updateDbPtr->sourceConfig1,
                                                                getOnly, timePtr);
                    SetClockUpdateStatus(updateDbPtr, source, result);
                    if (result == LE_OK)
                    {
                        return LE_OK;
                    }
                }
                if (strlen(updateDbPtr->sourceConfig2))
                {
                    LE_INFO("Trying clock source TP and config %s", updateDbPtr->sourceConfig2);
                    updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_IN_PROGRESS;
                    result = pa_clkSync_GetTimeWithTimeProtocol(updateDbPtr->sourceConfig2,
                                                                getOnly, timePtr);
                    SetClockUpdateStatus(updateDbPtr, source, result);
                    if (result == LE_OK)
                    {
                        return LE_OK;
                    }
                }
                LE_INFO("No success in getting current time from source TP");
                break;

            case LE_CLKSYNC_CLOCK_SOURCE_NTP:
                if (strlen(updateDbPtr->sourceConfig1))
                {
                    LE_INFO("Trying clock source NTP and config %s", updateDbPtr->sourceConfig1);
                    updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_IN_PROGRESS;
                    result = pa_clkSync_GetTimeWithNetworkTimeProtocol(updateDbPtr->sourceConfig1,
                                                                       getOnly, timePtr);
                    SetClockUpdateStatus(updateDbPtr, source, result);
                    if (result == LE_OK)
                    {
                        return LE_OK;
                    }
                }
                if (strlen(updateDbPtr->sourceConfig2))
                {
                    LE_INFO("Trying clock source NTP and config %s", updateDbPtr->sourceConfig2);
                    updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_IN_PROGRESS;
                    result = pa_clkSync_GetTimeWithNetworkTimeProtocol(updateDbPtr->sourceConfig2,
                                                                       getOnly, timePtr);
                    SetClockUpdateStatus(updateDbPtr, source, result);
                    if (result == LE_OK)
                    {
                        return LE_OK;
                    }
                }
                LE_INFO("No success in getting current time from source NTP");
                break;

            case LE_CLKSYNC_CLOCK_SOURCE_GPS:
                LE_WARN("Clock source GPS not supported yet");
                updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNSUPPORTED;
                break;

            default:
                LE_ERROR("Clock source %d not supported yet", updateDbPtr->source);
                updateDbPtr->updateStatus = LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNSUPPORTED;
                break;
        }
    }
    *source = LE_CLKSYNC_CLOCK_SOURCE_MAX;
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from the time source(s) configured
 *
 * @return
 *     - LE_OK             Function successful
 *     - LE_BAD_PARAMETER  A parameter is incorrect
 *     - LE_FAULT          Function failed
 *     - LE_UNSUPPORTED    Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t clkSync_GetCurrentTime
(
    le_msg_SessionRef_t sessionRef,     ///< [IN] messaging session making request
    le_clkSync_ClockTime_t* timePtr,    ///< [OUT] Clock time retrieved from configured source
    le_clkSync_ClockSource_t* sourcePtr ///< [OUT] Clock source from which current time is acquired
)
{
    le_result_t result;
    if (!timePtr || !sourcePtr)
    {
        LE_ERROR("Bad input parameters");
        return LE_BAD_PARAMETER;
    }

    result = ClockSyncGetUpdate(true, timePtr, sourcePtr);
    if (LE_UNAVAILABLE == result)
    {
        result = LE_FAULT;
    }
    else if (LE_OK == result)
    {
        LE_DEBUG("Time retrieved from source %d: %04d-%02d-%02d %02d:%02d:%02d:%03d", *sourcePtr,
                 timePtr->year, timePtr->mon, timePtr->day, timePtr->hour, timePtr->min,
                 timePtr->sec, timePtr->msec);
        LE_INFO("Succeeded to get clock time from configured source");
    }
    else if (LE_FAULT != result)
    {
        LE_WARN("Converting unexpected error code %d to %d", result, LE_FAULT);
        result = LE_FAULT;
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from the time source(s) configured
 *
 * @return
 *     - LE_OK             Function successful
 *     - LE_BAD_PARAMETER  A parameter is incorrect
 *     - LE_FAULT          Function failed
 *     - LE_UNSUPPORTED    Function not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_clkSync_GetCurrentTime
(
    le_clkSync_ClockTime_t* timePtr,    ///< [OUT] Clock time retrieved from configured source
    le_clkSync_ClockSource_t* sourcePtr ///< [OUT] Clock source from which current time is acquired
)
{
    return clkSync_GetCurrentTime(le_clkSync_GetClientSessionRef(), timePtr, sourcePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the status of the execution of a system time update using the given source
 *
 * @return
 *      - le_clkSync_UpdateSystemStatus_t
 */
//--------------------------------------------------------------------------------------------------
le_clkSync_UpdateSystemStatus_t le_clkSync_GetUpdateSystemStatus
(
    le_clkSync_ClockSource_t source     ///< [IN] Clock source used to get the current time
)
{
    if (source >= LE_CLKSYNC_CLOCK_SOURCE_MAX)
    {
        LE_ERROR("Clock source enum %d unsupported", source);
        return LE_CLKSYNC_UPDATE_SYSTEM_STATUS_UNSUPPORTED;
    }

    // Update a source's NOT_CONFIGURED status to NOT_TRIED if its configs were installed but no
    // ClockSyncGetUpdate() thereafter has occurred to re-trigger ClockSyncParseSourceConfigs().
    // Please refer to the headline explanation for the use of ClockSyncUpdateDbs[] in the start
    // of this file for more details.
    if (ClockSyncUpdateDbs[source].updateStatus == LE_CLKSYNC_UPDATE_SYSTEM_STATUS_NOT_CONFIGURED)
    {
        (void)ClockSyncReadSourceConfigs(source, &ClockSyncUpdateDbs[source]);

    }

    LE_DEBUG("Clock source %s with status %d retrieved", ClockSourceTypeString[source],
             ClockSyncUpdateDbs[source].updateStatus);
    return ClockSyncUpdateDbs[source].updateStatus;
}


//--------------------------------------------------------------------------------------------------
/**
 * Execute clock sync update as configured, and after completion call the saved callback handler
 * to notify the initiator about it
 */
//--------------------------------------------------------------------------------------------------
static void ClockSyncExecUpdate
(
    void
)
{
    le_clkSync_ClockTime_t timePtr;
    le_clkSync_ClockSource_t source;
    le_result_t result = ClockSyncGetUpdate(false, &timePtr, &source);
    if (LE_UNAVAILABLE == result)
    {
        result = LE_FAULT;
    }
    else if (LE_OK == result)
    {
        LE_INFO("Succeeded to execute clock time update with source %d", source);
    }
    else if (LE_FAULT != result)
    {
        LE_WARN("Converting unexpected error code %d to %d", result, LE_FAULT);
        result = LE_FAULT;
    }

    if (ClockSyncUpdateHandler.handlerFunc)
    {
        LE_DEBUG("Invoking callback function %p", ClockSyncUpdateHandler.handlerFunc);
        ClockSyncUpdateHandler.handlerFunc(result, ClockSyncUpdateHandler.contextPtr);
    }
    memset(&ClockSyncUpdateHandler, 0, sizeof(ClockSyncUpdateHandler_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a Clock Sync command event. So far there's only one command type:
 * CLOCKSYNC_COMMAND_UPDATE_CLOCK available
 */
//--------------------------------------------------------------------------------------------------
static void ClockSyncCommandHandler
(
    void *commandPtr
)
{
    ClockSyncCommand_t *command = commandPtr;

    switch (command->commandType)
    {
        case CLOCKSYNC_COMMAND_UPDATE_CLOCK:
            LE_INFO("Process an update clock command");
            le_clkSync_AddUpdateClockHandler(command->handlerFunc, command->context);
            ClockSyncExecUpdate();
            break;

        default:
            LE_ERROR("Invalid command type %d", command->commandType);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Internal callback function used when the app that triggers a clock update hasn't provided one
 */
//--------------------------------------------------------------------------------------------------
void InternalCallbackFunction
(
    le_result_t status,        ///< Status of the system time update
    void* contextPtr
)
{
    LE_INFO("Clock udpate result: %d", status);
}


//--------------------------------------------------------------------------------------------------
/**
 * Send an update system clock event to the Clock Service daemon, which will run it asynchronously
 * and, when done, return the resulting status back via the provided callback
 * UpdateSystemTimeHandler
 */
//--------------------------------------------------------------------------------------------------
void le_clkSync_UpdateSystemTime
(
    le_clkSync_UpdateSystemTimeHandlerFunc_t handlerPtr, ///< [IN] requester's handler for receiving
                                                         ///< resulting status
    void *contextPtr                                     ///< [IN] requester's context
)
{
    if (ClockSyncUpdateHandler.handlerFunc)
    {
        LE_INFO("Clock update execution already in progress");
        return;
    }

    ClockSyncCommand_t cmd = {
        .commandType = CLOCKSYNC_COMMAND_UPDATE_CLOCK,
        .context = contextPtr,
        .handlerFunc = handlerPtr
    };

    le_event_Report(ClockSyncCommandEventId, &cmd, sizeof(cmd));
}


//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint16_t numDb;

    memset(&ClockSyncUpdateHandler, 0, sizeof(ClockSyncUpdateHandler_t));
    ClockSyncCommandEventId = le_event_CreateId("ClockSyncCommandEventId",
                                                sizeof(ClockSyncCommand_t));
    le_event_AddHandler("ClockSyncCommand", ClockSyncCommandEventId, ClockSyncCommandHandler);

    ClockSyncSourcePrioritizedList = LE_DLS_LIST_INIT;
    memset(&ClockSyncUpdateDbs, 0, sizeof(ClockSyncUpdateDbs));
    numDb = ClockSyncParseSourceConfigs();
    if (numDb > 0)
    {
        // Sort configured clock sources according to priority 1st
        LE_DEBUG("Sorting %d clock sources' priority", numDb);
        le_dls_Sort(&ClockSyncSourcePrioritizedList, UpdateDbComparePriority);
    }


    // Register main loop with watchdog chain
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    LE_INFO("Clock Sync Service le_clkSync is ready");
}
