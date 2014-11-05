/**
 * This module is for unit testing of the DTMF Audio service.
 *
 * TODO: This module only tests DTMF decoding. DTMF encoding tests will come in a future release.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>


#include "interfaces.h"

static bool isIncoming=false;
static le_mcc_call_ObjRef_t TestCallRef;

static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t FeInRef = NULL;
static le_audio_StreamRef_t FeOutRef = NULL;

static le_audio_ConnectorRef_t AudioInputConnectorRef = NULL;
static le_audio_ConnectorRef_t AudioOutputConnectorRef = NULL;

static char  DestinationNumber[LE_MDMDEFS_PHONE_NUM_MAX_LEN];

static le_audio_DtmfDetectorHandlerRef_t DtmfHandlerRef1 = NULL;
static le_audio_DtmfDetectorHandlerRef_t DtmfHandlerRef2 = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Bindings functions.
 *
 */
//--------------------------------------------------------------------------------------------------
#define SERVICE_BASE_BINDINGS_CFG  "/users/root/bindings"

typedef void (*LegatoServiceInit_t)(void);

typedef struct {
    const char * appNamePtr;
    const char * serviceNamePtr;
    LegatoServiceInit_t serviceInitPtr;
} ServiceInitEntry_t;

#define SERVICE_ENTRY(aPP, sVC) { aPP, #sVC, sVC##_ConnectService },

const ServiceInitEntry_t ServiceInitEntries[] = {
    SERVICE_ENTRY("modemService", le_mcc_profile)
    SERVICE_ENTRY("modemService", le_mcc_call)
    SERVICE_ENTRY("audioService", le_audio)
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

static void ConnectAudioToPcm
(
    le_mcc_call_ObjRef_t callRef
)
{
    le_result_t res;

    MdmRxAudioRef = le_mcc_call_GetRxAudioStream(callRef);
    LE_ERROR_IF((MdmRxAudioRef==NULL), "GetRxAudioStream returns NULL!");
    MdmTxAudioRef = le_mcc_call_GetTxAudioStream(callRef);
    LE_ERROR_IF((MdmTxAudioRef==NULL), "GetTxAudioStream returns NULL!");

    // Redirect audio to the PCM interface.
    FeOutRef = le_audio_OpenPcmTx(0);
    LE_ERROR_IF((FeOutRef==NULL), "OpenPcmTx returns NULL!");
    FeInRef = le_audio_OpenPcmRx(0);
    LE_ERROR_IF((FeInRef==NULL), "OpenPcmRx returns NULL!");

    AudioInputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioInputConnectorRef==NULL), "AudioInputConnectorRef is NULL!");
    AudioOutputConnectorRef = le_audio_CreateConnector();
    LE_ERROR_IF((AudioOutputConnectorRef==NULL), "AudioOutputConnectorRef is NULL!");

    if (MdmRxAudioRef && MdmTxAudioRef && FeOutRef && FeInRef &&
        AudioInputConnectorRef && AudioOutputConnectorRef)
    {
        res = le_audio_Connect(AudioInputConnectorRef, FeInRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM RX on Input connector!");
        res = le_audio_Connect(AudioInputConnectorRef, MdmTxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmTx on Input connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, FeOutRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect PCM TX on Output connector!");
        res = le_audio_Connect(AudioOutputConnectorRef, MdmRxAudioRef);
        LE_ERROR_IF((res!=LE_OK), "Failed to connect mdmRx on Output connector!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect audio.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectAllAudio
(
    le_mcc_call_ObjRef_t callRef
)
{
    LE_INFO("delete DTMF handler\n");
    le_audio_RemoveDtmfDetectorHandler(DtmfHandlerRef1);
    sleep(1);
    LE_INFO("delete DTMF handler2\n");
    le_audio_RemoveDtmfDetectorHandler(DtmfHandlerRef2);

    if (AudioInputConnectorRef)
    {
        if (FeInRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FeInRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, FeInRef);
        }
        if(MdmTxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", MdmTxAudioRef, AudioInputConnectorRef);
            le_audio_Disconnect(AudioInputConnectorRef, MdmTxAudioRef);
        }
    }
    if(AudioOutputConnectorRef)
    {
        if(FeOutRef)
        {
            LE_INFO("Disconnect %p from connector.%p", FeOutRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, FeOutRef);
        }
        if(MdmRxAudioRef)
        {
            LE_INFO("Disconnect %p from connector.%p", MdmRxAudioRef, AudioOutputConnectorRef);
            le_audio_Disconnect(AudioOutputConnectorRef, MdmRxAudioRef);
        }
    }

    if(AudioInputConnectorRef)
    {
        le_audio_DeleteConnector(AudioInputConnectorRef);
        AudioInputConnectorRef = NULL;
    }
    if(AudioOutputConnectorRef)
    {
        le_audio_DeleteConnector(AudioOutputConnectorRef);
        AudioOutputConnectorRef = NULL;
    }

    if(FeInRef)
    {
        le_audio_Close(FeInRef);
        FeInRef = NULL;
    }
    if(FeOutRef)
    {
        le_audio_Close(FeOutRef);
        FeOutRef = NULL;
    }
    if(MdmRxAudioRef)
    {
        le_audio_Close(MdmRxAudioRef);
        FeOutRef = NULL;
    }
    if(MdmTxAudioRef)
    {
        le_audio_Close(MdmTxAudioRef);
        FeOutRef = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler functions for DTMF Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyDtmfDetectorHandler1
(
    le_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler1 detects %c", dtmf);
}

static void MyDtmfDetectorHandler2
(
    le_audio_StreamRef_t streamRef,
    char  dtmf,
    void* contextPtr
)
{
    LE_INFO("MyDtmfDetectorHandler2 detects %c", dtmf);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Call Event Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void MyCallEventHandler
(
    le_mcc_call_ObjRef_t   callRef,
    le_mcc_call_Event_t    callEvent,
    void*                  contextPtr
)
{
    le_result_t         res;

    if (callEvent == LE_MCC_CALL_EVENT_ALERTING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_ALERTING.");
    }
    else if (callEvent == LE_MCC_CALL_EVENT_CONNECTED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_CONNECTED.");
        ConnectAudioToPcm(callRef);
        DtmfHandlerRef1 = le_audio_AddDtmfDetectorHandler(MdmRxAudioRef, MyDtmfDetectorHandler1, NULL);
        DtmfHandlerRef2 = le_audio_AddDtmfDetectorHandler(MdmRxAudioRef, MyDtmfDetectorHandler2, NULL);
    }
    else if (callEvent == LE_MCC_CALL_EVENT_TERMINATED)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_TERMINATED.");
        le_mcc_call_TerminationReason_t term = le_mcc_call_GetTerminationReason(callRef);
        switch(term)
        {
            case LE_MCC_CALL_TERM_NETWORK_FAIL:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NETWORK_FAIL");
                break;

            case LE_MCC_CALL_TERM_BAD_ADDRESS:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BAD_ADDRESS");
                break;

            case LE_MCC_CALL_TERM_BUSY:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_BUSY");
                break;

            case LE_MCC_CALL_TERM_LOCAL_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_LOCAL_ENDED");
                break;

            case LE_MCC_CALL_TERM_REMOTE_ENDED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_REMOTE_ENDED");
                break;

            case LE_MCC_CALL_TERM_NOT_DEFINED:
                LE_INFO("Termination reason is LE_MCC_CALL_TERM_NOT_DEFINED");
                break;

            default:
                LE_INFO("Termination reason is %d", term);
                break;
        }
        DisconnectAllAudio(callRef);
        le_mcc_call_Delete(callRef);
    }
    else if (callEvent == LE_MCC_CALL_EVENT_INCOMING)
    {
        LE_INFO("Call event is LE_MCC_CALL_EVENT_INCOMING.");
        isIncoming=true;
        res = le_mcc_call_Answer(callRef);
        if (res != LE_OK)
        {
            LE_INFO("Failed to answer the call.");
        }
    }
    else
    {
        LE_INFO("Unknowm Call event.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler's thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* HandlerThread
(
    void* contextPtr
)
{
    le_mcc_profile_ObjRef_t profileRef = contextPtr;

    le_mcc_profile_ConnectService();
    le_mcc_call_ConnectService();
    le_audio_ConnectService();
    le_mcc_profile_AddCallEventHandler(profileRef, MyCallEventHandler, NULL);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t GetTel(void)
{
    char *strPtr;

    memset(DestinationNumber, 0, LE_MDMDEFS_PHONE_NUM_MAX_LEN);

    do
    {
        fprintf(stderr, "Please enter the destination's telephone number to start a call or 'stop' to exit: \n");
        strPtr=fgets ((char*)DestinationNumber, 16, stdin);
    }while (strlen(strPtr) == 0);

    DestinationNumber[strlen(DestinationNumber)-1]='\0';

    if (!strcmp(DestinationNumber, "stop"))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_mcc_profile_ObjRef_t profileRef;
    int32_t                 stopTest = 0;

    SetupBindings();
    ConnectServices();

    profileRef=le_mcc_profile_GetByName("Modem-Sim1");
    if ( profileRef == NULL )
    {
        LE_INFO("Unable to get the Call profile reference");
        exit(1);
    }

    // Start the handler thread to monitor the call for the just created profile.
    le_thread_Start(le_thread_Create("MCC", HandlerThread, profileRef));

    while(!stopTest)
    {
        stopTest = GetTel();
        if (!stopTest)
        {
            isIncoming=false;
            TestCallRef=le_mcc_profile_CreateCall(profileRef, DestinationNumber);
            le_mcc_call_Start(TestCallRef);
        }
        else
        {
            LE_INFO("Exit AudioDtmf Test!");
            exit(0);
        }
    }

}

