 /**
  * Main functions to test SMS deletion from storage with multiple handler management.
  * SmsDeletion test waits for an incoming sms.
  *
  * - First handler receives sms reference and tries to delete it from storage.
  *    Sms Deletion will be delayed until no more object references exist.
  *
  * - Second handler receives sms reference and deletes it after 2 seconds.
  *
  * - Third handler receives sms reference, deletes it, creates a list wait 4 seconds and deletes it.
  *
  * - Fourth handler receives sms reference wait 6 seconds, delete reference, now SMS should be delete
  *    Automatically from storage (see traces, pa_sms_DelMsgFromMem()). All handlers are removed and
  *    application exits.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  * Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"


#define SERVICE_BASE_BINDINGS_CFG  "/users/root/bindings"

typedef void (*LegatoServiceInit_t)(void);

typedef struct {
    const char * appNamePtr;
    const char * serviceNamePtr;
    LegatoServiceInit_t serviceInitPtr;
} ServiceInitEntry_t;

#define SERVICE_ENTRY(aPP, sVC) { aPP, #sVC, sVC##_ConnectService },

typedef void (*LegatoServiceInit_t)(void);


const ServiceInitEntry_t ServiceInitEntries[] = {
    SERVICE_ENTRY("modemService", le_sms)
};


static void SetupBindings(void)
{
    int serviceIndex;
    char cfgPath[512];
    le_cfg_IteratorRef_t iteratorRef;

    /* Setup bindings */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        /* Update binding in config tree */
        LE_INFO("-> Bind %s", entryPtr->serviceNamePtr);

        snprintf(cfgPath, sizeof(cfgPath), SERVICE_BASE_BINDINGS_CFG "/%s", entryPtr->serviceNamePtr);

        iteratorRef = le_cfg_CreateWriteTxn(cfgPath);

        le_cfg_SetString(iteratorRef, "app", entryPtr->appNamePtr);
        le_cfg_SetString(iteratorRef, "interface", entryPtr->serviceNamePtr);

        le_cfg_CommitTxn(iteratorRef);
    }

    /* Tel legato to reload its bindings */
    system("sdir load");
}

static void ConnectServices(void)
{
    int serviceIndex;

    /* Init services */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        LE_INFO("-> Init %s", entryPtr->serviceNamePtr);
        entryPtr->serviceInitPtr();
    }

    LE_INFO("All services bound!");
}


static le_sms_RxMessageHandlerRef_t HandlerRef1, HandlerRef2, HandlerRef3, HandlerRef4;


//--------------------------------------------------------------------------------------------------
/**
 * This function remove the handler for message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SmsMTHandlerRemover
(
    void
)
{
    le_sms_RemoveRxMessageHandler(HandlerRef1);
    le_sms_RemoveRxMessageHandler(HandlerRef2);
    le_sms_RemoveRxMessageHandler(HandlerRef3);
    le_sms_RemoveRxMessageHandler(HandlerRef4);
    LE_INFO("All handler removed\n");
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler1
(
    le_sms_MsgRef_t  msgRef,
    void*            contextPtr
)
{
    le_result_t  res;
    LE_INFO("A New SMS1 message is received with ref.%p", msgRef);

    if (le_sms_GetFormat(msgRef) == LE_SMS_FORMAT_TEXT)
    {

        res = le_sms_DeleteFromStorage(msgRef);
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_DeleteFromStorage has failed (res.%d)!", res);
            LE_ASSERT(res != LE_OK);
        }
        else
        {
            LE_INFO("The message has been successfully deleted from storage.");
        }
    }
    else
    {
        LE_WARN("Warning! I read only Text messages!");
    }

    le_sms_Delete(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler2
(
    le_sms_MsgRef_t  msgRef,
    void*            contextPtr
)
{
    LE_INFO("A New SMS2 message is received with ref.%p", msgRef);
    sleep (2);

    le_sms_Delete(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler3
(
    le_sms_MsgRef_t  msgRef,
    void*            contextPtr
)
{
    LE_INFO("A New SMS3 message is received with ref.%p", msgRef);
    sleep(3);

     le_sms_MsgListRef_t listRef = le_sms_CreateRxMsgList();
    if (listRef == NULL)
    {
        LE_ERROR("Can't create SMS list.");
        LE_ASSERT(listRef == NULL);
    }

    sleep(1);
    if (listRef)
    {
        le_sms_DeleteList(listRef);
    }

    le_sms_Delete(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler4
(
    le_sms_MsgRef_t  msgRef,
    void*            contextPtr
)
{
    LE_INFO("A New SMS4 message is received with ref.%p", msgRef);
    sleep(6);

    le_sms_Delete(msgRef);

    SmsMTHandlerRemover();

    LE_INFO("sms Deletion test Exit\n");
    exit(0);
}


//--------------------------------------------------------------------------------------------------
/**
 *  App init
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Start Multiple SMS deletion race test!");

    SetupBindings();
    ConnectServices();

    // First handler receives sms reference and try to delete it from storage.
    // Sms Deletion will be delayed until no more object references exist.
    HandlerRef1 = le_sms_AddRxMessageHandler(RxMessageHandler1, NULL);
    if (!HandlerRef1)
    {
        LE_ERROR("le_sms_AddRxMessageHandler RxMessageHandler1 has failed!");
    }

    // Second handler receives sms reference and deletes it after 2 seconds.
    HandlerRef2 = le_sms_AddRxMessageHandler(RxMessageHandler2, NULL);
    if (!HandlerRef2)
    {
        LE_ERROR("le_sms_AddRxMessageHandler RxMessageHandler2 has failed!");
    }

    // Third handler receives sms reference, deletes it, creates a list wait 4 seconds and deletes it.
    HandlerRef3 = le_sms_AddRxMessageHandler(RxMessageHandler3, NULL);
    if (!HandlerRef3)
    {
        LE_ERROR("le_sms_AddRxMessageHandler RxMessageHandler3 has failed!");
    }

    // Fourth handler receives sms reference wait 6seconds, deletes it and removes all sms test
    // handlers.
    HandlerRef4 = le_sms_AddRxMessageHandler(RxMessageHandler4, NULL);
    if (!HandlerRef4)
    {
        LE_ERROR("le_sms_AddRxMessageHandler RxMessageHandler4 has failed!");
    }
}
