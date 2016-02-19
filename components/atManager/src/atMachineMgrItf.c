/** @file atmachinemgritf.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "atMachineManager.h"
#include "atMachineMgrItf.h"
#include "atMachineCommand.h"
#include "atMachineUnsolicited.h"
#include "atMachineDevice.h"

#define THREAD_NAME_MAX 64

static le_mem_PoolRef_t  AtManagerItfPool;

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for atmgr_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtMgrItfPoolDestruct
(
    void* context   ///< atmgr_t reference
)
{
    struct atmgr* newInterfacePtr = context;

    LE_DEBUG("Destruct %s device",newInterfacePtr->atManager.curContext.atDevice.name);

    LE_DEBUG("Destruct Done");
    // No API to delete an event_Id, so nothing to do.
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create and start the ATManager + ATParser for the device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void *InitThread
(
    void* context
)
{
    le_event_HandlerRef_t   handlerRef;
    struct atmgr* newInterfacePtr = context;

    handlerRef = le_event_AddHandler("hdl_resumeInterface",
                                     newInterfacePtr->resumeInterfaceId,
                                     atmachinemanager_Resume);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    handlerRef = le_event_AddHandler("hdl_SuspendInterface",
                                     newInterfacePtr->suspendInterfaceId,
                                     atmachinemanager_Suspend);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    handlerRef = le_event_AddHandler("hdl_SubscribeUnsol",
                                     newInterfacePtr->subscribeUnsolicitedId,
                                     atmachinemanager_AddUnsolicited);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    handlerRef = le_event_AddHandler("hdl_UnSubscribeUnsol",
                                     newInterfacePtr->unSubscribeUnsolicitedId,
                                     atmachinemanager_RemoveUnsolicited);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    handlerRef = le_event_AddHandler("hdl_SendCommand",
                                     newInterfacePtr->sendCommandId,
                                     atmachinemanager_SendCommand);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    handlerRef = le_event_AddHandler("hdl_CancelCommand",
                                     newInterfacePtr->cancelCommandId,
                                     atmachinemanager_CancelCommand);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atManager));

    le_sem_Post(newInterfacePtr->waitingSemaphore);

    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(),threadName,25);
        LE_DEBUG("Start thread %s %p",threadName,le_thread_GetCurrent());
    }

    le_event_RunLoop();

    return NULL; // Should not happen
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atmanageritf
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemgritf_Init
(
    void
)
{
    AtManagerItfPool = le_mem_CreatePool("atmanageritfPool",sizeof(struct atmgr));
    le_mem_SetDestructor(AtManagerItfPool,AtMgrItfPoolDestruct);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get a atmgr_t from the Pool.
 *
 *  @return pointer to atmgr_t
 */
//--------------------------------------------------------------------------------------------------
static struct atmgr* CreateInterface
(
)
{
    struct atmgr* newInterfacePtr = le_mem_ForceAlloc(AtManagerItfPool);

    memset(&(newInterfacePtr->atManager),0,sizeof(newInterfacePtr->atManager));

    newInterfacePtr->resumeInterfaceId = le_event_CreateId("id_resumeInterface",0);

    newInterfacePtr->suspendInterfaceId = le_event_CreateId("id_suspendInterface",0);

    newInterfacePtr->subscribeUnsolicitedId = le_event_CreateIdWithRefCounting("id_SubscribeUnsol");

    newInterfacePtr->unSubscribeUnsolicitedId =
                                            le_event_CreateIdWithRefCounting("id_UnSubscribeUnsol");

    newInterfacePtr->sendCommandId = le_event_CreateIdWithRefCounting("id_SendCommand");

    newInterfacePtr->cancelCommandId = le_event_CreateIdWithRefCounting("id_CancelCommand");

    newInterfacePtr->waitingSemaphore = le_sem_Create("ItfWaitSemaphore",0);

    return newInterfacePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an interface for the given device on a device.
 *
 * @return a reference on the ATManager of this device
 */
//--------------------------------------------------------------------------------------------------
atmgr_Ref_t atmgr_CreateInterface
(
    struct atdevice* devicePtr    ///< Device
)
{
    char name[THREAD_NAME_MAX];
    le_thread_Ref_t newThreadPtr;

    struct atmgr* newInterfacePtr = CreateInterface();

    LE_DEBUG("Create a new interface for '%s'",devicePtr->name);

    memcpy(&(newInterfacePtr->atManager.curContext.atDevice),
           devicePtr,
           sizeof(newInterfacePtr->atManager.curContext.atDevice));

    snprintf(name,THREAD_NAME_MAX,"ATManager-%s",devicePtr->name);
    newThreadPtr = le_thread_Create(name,InitThread,newInterfacePtr);

    le_thread_Start(newThreadPtr);
    le_sem_Wait(newInterfacePtr->waitingSemaphore);

    return newInterfacePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start atManager on a device.
 *
 * @note
 * After this call, Unsolicited pattern can be parse,AT Command can be sent
 * on Configuration Port Handle.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_StartInterface
(
    struct atmgr* atmanageritfPtr    ///< Device
)
{
    le_event_Report(atmanageritfPtr->resumeInterfaceId,NULL,0);
    le_sem_Wait(atmanageritfPtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Stop the ATManager on a device.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_StopInterface
(
    struct atmgr* atmanageritfPtr
)
{
    le_event_Report(atmanageritfPtr->suspendInterfaceId,NULL,0);
    le_sem_Wait(atmanageritfPtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Set an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_SubscribeUnsolReq
(
    struct atmgr*   atmanageritfPtr,     ///< one ATManager interface
    le_event_Id_t    unsolicitedReportId, ///< Event Id to report to
    char            *unsolRspPtr,         ///< pattern to match
    bool             withExtraData        ///< Indicate if the unsolicited has more than one line
)
{
    atUnsolicited_t* newUnsolicitedPtr = atmachineunsolicited_Create();

    newUnsolicitedPtr->withExtraData = withExtraData;
    newUnsolicitedPtr->unsolicitedReportId = unsolicitedReportId;
    le_utf8_Copy(newUnsolicitedPtr->unsolRsp,unsolRspPtr,ATMANAGER_UNSOLICITED_SIZE,0);

    le_event_ReportWithRefCounting(
                    atmanageritfPtr->subscribeUnsolicitedId,
                    newUnsolicitedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_UnSubscribeUnsolReq
(
    struct atmgr*      atmanageritfPtr,        ///< one ATManager interface
    le_event_Id_t           unsolicitedReportId,    ///< Event Id to report to
    char *                  unsolRspPtr             ///< pattern to match
)
{
    atUnsolicited_t* newUnsolicitedPtr = atmachineunsolicited_Create();

    newUnsolicitedPtr->unsolicitedReportId = unsolicitedReportId;
    le_utf8_Copy(newUnsolicitedPtr->unsolRsp,unsolRspPtr,ATMANAGER_UNSOLICITED_SIZE,0);

    le_event_ReportWithRefCounting(
                    atmanageritfPtr->unSubscribeUnsolicitedId,
                    newUnsolicitedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Send an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_SendCommandRequest
(
    struct atmgr*      atmanageritfPtr,     ///< one ATManager interface
    struct atcmd *    atcommandToSendPtr   ///< AT Command Reference
)
{
    le_mem_AddRef(atcommandToSendPtr);
    le_event_ReportWithRefCounting(
                    atmanageritfPtr->sendCommandId,
                    atcommandToSendPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Cancel an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_CancelCommandRequest
(
    struct atmgr          *atmanageritfPtr,     ///< one ATManager interface
    struct atcmd      *atcommandToCancelRef ///< AT Command Reference
)
{
    le_mem_AddRef(atcommandToCancelRef);
    le_event_ReportWithRefCounting(
                    atmanageritfPtr->cancelCommandId,
                    atcommandToCancelRef);
}
