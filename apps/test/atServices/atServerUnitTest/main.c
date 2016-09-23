/**
 * This module implements the unit tests for AT server API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "log.h"

#define PARAM_MAX 10
#define DESC_MAX_LENGTH 512

static le_sem_Ref_t    ThreadSemaphore;
static le_sem_Ref_t    HandlerSemaphore;

extern void le_dev_Init(void);
extern void le_dev_NewData(char* stringPtr,uint32_t len);
void le_dev_WaitSemaphore(void);
void le_dev_ExpectedResponse(char* rspPtr);
void le_dev_Done(void);

static uint32_t ExpectedParametersNumber = 0;
static char* ExpectedAtCommandPtr;
static le_atServer_Type_t ExpectedType;
static char* ExpectedParamPtr[PARAM_MAX] = {0};
static bool SendRspCustom = false;
static char* CustomFinalRspPtr = NULL;
static bool ExpectedIntermediateRsp = false;
static char* IntermediateRspPtr = NULL;
static le_atServer_FinalRsp_t FinalRsp = LE_ATSERVER_OK;

//--------------------------------------------------------------------------------------------------
/**
 * Set an expected response
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetExpectedResponse
(
    char * rspPtr
)
{
    char rsp[LE_ATSERVER_COMMAND_MAX_BYTES];
    memset(rsp,0,LE_ATSERVER_COMMAND_MAX_BYTES);
    snprintf(rsp, LE_ATSERVER_COMMAND_MAX_BYTES, "\r\n%s\r\n", rspPtr);
    le_dev_ExpectedResponse(rsp);
}

//--------------------------------------------------------------------------------------------------
/**
 * AT command handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_INFO("commandRef %p", commandRef);

    le_sem_Wait(ThreadSemaphore);

    LE_ASSERT(ExpectedParametersNumber == parametersNumber);

    char atCommandName[LE_ATSERVER_COMMAND_MAX_BYTES];
    memset(atCommandName,0,LE_ATSERVER_COMMAND_MAX_BYTES);
    LE_ASSERT(le_atServer_GetCommandName(commandRef,atCommandName,LE_ATSERVER_COMMAND_MAX_BYTES)
                                                                                          == LE_OK);
    LE_ASSERT(strncmp(atCommandName, ExpectedAtCommandPtr, strlen(atCommandName))==0);
    LE_INFO("AT command name %s", atCommandName);

    LE_ASSERT(ExpectedType == type);
    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    char param[LE_ATSERVER_PARAMETER_MAX_BYTES];

    if (parametersNumber)
    {
        int i = 0;

        for (; i < parametersNumber; i++)
        {
            memset(param,0,LE_ATSERVER_PARAMETER_MAX_BYTES);
            LE_ASSERT(le_atServer_GetParameter( commandRef,
                                                i,
                                                param,
                                                LE_ATSERVER_PARAMETER_MAX_BYTES) == LE_OK);
            LE_INFO("param %d %s", i, param);
            if (i < PARAM_MAX)
            {
                LE_ASSERT(strncmp(param, ExpectedParamPtr[i], strlen(param)) == 0);
            }
            else
            {
                LE_ASSERT(0);
            }
        }
    }

    LE_ASSERT(le_atServer_GetParameter( commandRef,
                                        parametersNumber+1,
                                        param,
                                        LE_ATSERVER_PARAMETER_MAX_BYTES ) == LE_BAD_PARAMETER);

    if (ExpectedIntermediateRsp)
    {
        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, IntermediateRspPtr) == LE_OK);
    }

    if (SendRspCustom)
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef,
                                                FinalRsp,
                                                true,
                                                CustomFinalRspPtr) == LE_OK);
    }
    else
    {
        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, FinalRsp, false, "") == LE_OK);
    }

    le_sem_Post(HandlerSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a command to the AT parser
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddAtCmdHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_atServer_CmdRef_t atCmdRef = param1Ptr;
    le_atServer_CommandHandlerFunc_t cmdHandlerPtr = param2Ptr;

    le_atServer_AddCommandHandler(atCmdRef, cmdHandlerPtr, NULL);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * AT command thread handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AtCmdThreadHandler
(
    void* ctxPtr
)
{
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestHandler
(
    void* ctxPtr
)
{
    le_atServer_CmdRef_t atCmdRef = NULL;
    le_atServer_CmdRef_t atRef = NULL;
    le_atServer_CmdRef_t atARef = NULL;
    le_atServer_CmdRef_t atAndFRef = NULL;
    le_atServer_CmdRef_t atSRef = NULL;
    le_atServer_CmdRef_t atVRef = NULL;
    le_atServer_CmdRef_t atAndCRef = NULL;
    le_atServer_CmdRef_t atAndDRef = NULL;
    le_atServer_CmdRef_t atERef = NULL;
    le_atServer_CmdRef_t atDlRef = NULL;

    le_thread_Ref_t AtCmdThreadRef = le_thread_Create("TestThread", AtCmdThreadHandler, NULL);
    le_thread_Start(AtCmdThreadRef);

    // Unit test, we are using a fake device
    le_atServer_DeviceRef_t devRef = le_atServer_Start(1);

    LE_ASSERT(devRef != NULL);

    struct
    {
        const char* atCmdPtr;
        le_atServer_CmdRef_t cmdRef;
    } atCmdCreation[] =     {
                                { "AT+ABCD",    atCmdRef    },
                                { "AT",         atRef       },
                                { "ATA",        atARef      },
                                { "AT&F",       atAndFRef   },
                                { "ATS",        atSRef      },
                                { "ATV",        atVRef      },
                                { "AT&C",       atAndCRef   },
                                { "AT&D",       atAndDRef   },
                                { "ATE",        atERef      },
                                { "ATDL",       atDlRef     },
                                { NULL,         NULL        }
                            };

    int i=0;

    // AT commands subscriptions
    while ( atCmdCreation[i].atCmdPtr != NULL )
    {
        atCmdCreation[i].cmdRef = le_atServer_Create(atCmdCreation[i].atCmdPtr);
        LE_ASSERT(atCmdCreation[i].cmdRef != NULL);

        le_event_QueueFunctionToThread( AtCmdThreadRef,
                                        AddAtCmdHandler,
                                        atCmdCreation[i].cmdRef,
                                        AtCmdHandler );

        le_sem_Wait(ThreadSemaphore);

        i++;
    }

    // Send "AT" command
    ExpectedParametersNumber = 0;
    ExpectedAtCommandPtr = "AT";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    SendRspCustom = false;
    ExpectedIntermediateRsp = false;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("AT\r", 3);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    // Send ATA command, with one parameter
    ExpectedParametersNumber = 1;
    ExpectedAtCommandPtr = "ATA";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParamPtr[0] = "1";
    SendRspCustom = true;
    CustomFinalRspPtr = "ATA_REC";
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "ATA_INTERMEDIATE_RSP";

    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse(IntermediateRspPtr);
    le_dev_NewData("ATA1\r", 6);
    le_dev_WaitSemaphore();
    SetExpectedResponse(CustomFinalRspPtr);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    // Send ATAB command: ATA should be parsed correctly, ATB is unknown => error expected
    ExpectedParametersNumber = 0;
    ExpectedAtCommandPtr = "ATA";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "ATA_INTERMEDIATE_RSP";
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse(IntermediateRspPtr);
    le_dev_NewData("atab\r", 6);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("ERROR");
    le_dev_WaitSemaphore();
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT&F
    ExpectedParametersNumber = 0;
    ExpectedAtCommandPtr = "AT&F";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedIntermediateRsp = false;
    SendRspCustom = false;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("at&f\r", 5);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send ATS
    ExpectedParametersNumber = 2;
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParamPtr[0] = "45";
    ExpectedParamPtr[1] = "95";
    ExpectedIntermediateRsp = false;
    SendRspCustom = false;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("ATS45=95\r", 9);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
    LE_ASSERT(le_sem_GetValue(HandlerSemaphore) == 0);

    // Send ATS0?
    ExpectedParametersNumber = 1;
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedParamPtr[0] = "0";
    ExpectedIntermediateRsp = false;
    SendRspCustom = false;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("ATS0?\r", 9);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
    LE_ASSERT(le_sem_GetValue(HandlerSemaphore) == 0);

    // Send AT&FE0V1&C1&D2S95=47S0=0
    ExpectedParametersNumber = 0;
    ExpectedIntermediateRsp = false;
    SendRspCustom = false;
    ExpectedAtCommandPtr = "AT&F";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("AT&FE0V1&C1&D2S95=47S0=0\r", 25);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATE";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "0";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATV";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "1";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "AT&C";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "1";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "AT&D";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "2";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 2;
    ExpectedParamPtr[0] = "95";
    ExpectedParamPtr[1] = "47";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 2;
    ExpectedParamPtr[0] = "0";
    ExpectedParamPtr[1] = "0";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
    LE_ASSERT(le_sem_GetValue(HandlerSemaphore) == 0);

    // Send AT+ABCD=0,,1,2,"3", command - Send a second command during the parsing
    ExpectedParametersNumber = 6;
    ExpectedAtCommandPtr = "AT+ABCD";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParamPtr[0] = "0";
    ExpectedParamPtr[1] = "";
    ExpectedParamPtr[2] = "1";
    ExpectedParamPtr[3] = "2";
    ExpectedParamPtr[4] = "3";
    ExpectedParamPtr[5] = "";
    SendRspCustom = false;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = ExpectedAtCommandPtr;

    SetExpectedResponse("ERROR");
    le_dev_NewData("blabla", 6);
    le_dev_NewData("ABCDAT+AB", 9);
    le_dev_NewData("CD=0,,1,2,\"3\",", 14);
    le_dev_NewData("\r", 1);
    le_dev_NewData("AT+TOTO\r", 8);
    le_dev_WaitSemaphore();
    SetExpectedResponse("AT+ABCD");
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT+ABCD=,,1,,;+ABCD=,,;+ABCD=,,1 commandd
    ExpectedParametersNumber = 5;
    ExpectedAtCommandPtr = "AT+ABCD";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParamPtr[0] = "";
    ExpectedParamPtr[1] = "";
    ExpectedParamPtr[2] = "1";
    ExpectedParamPtr[3] = "";
    ExpectedParamPtr[4] = "";
    SendRspCustom = false;
    ExpectedIntermediateRsp = false;
    SetExpectedResponse("OK");
    le_sem_Post(ThreadSemaphore);
    le_dev_NewData("AT+ABCD=,,1,,;+ABCD=,,;+ABCD=,,1\r", 33);
    le_sem_Wait(HandlerSemaphore);

    ExpectedParametersNumber = 3;
    ExpectedParamPtr[0] = "";
    ExpectedParamPtr[1] = "";
    ExpectedParamPtr[2] = "";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);

    ExpectedParametersNumber = 3;
    ExpectedParamPtr[0] = "";
    ExpectedParamPtr[1] = "";
    ExpectedParamPtr[2] = "1";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT+ABCD=? command
    ExpectedParametersNumber = 0;
    ExpectedType = LE_ATSERVER_TYPE_TEST;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "+ABCD: TEST";
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse(IntermediateRspPtr);
    le_dev_NewData("AT+ABCD=?\r", 10);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT+ABCD? command
    ExpectedParametersNumber = 0;
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "+ABCD: READ";
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse(IntermediateRspPtr);
    le_dev_NewData("AT+ABCD?\r", 9);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT+ABCD? command
    ExpectedParametersNumber = 0;
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "+ABCD: ACT";
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse(IntermediateRspPtr);
    le_dev_NewData("AT+ABCD\r",8);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);

    // Send AT+ABCD;+ABCD=?;+ABCD? concatenated command
    ExpectedParametersNumber = 0;
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedIntermediateRsp = true;
    IntermediateRspPtr = "+ABCD: ACT";
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);

    le_dev_NewData("AT+ABCD;+ABCD=?;+ABCD?\r", 23);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    ExpectedType = LE_ATSERVER_TYPE_TEST;
    IntermediateRspPtr = "+ABCD: TEST";
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    IntermediateRspPtr = "+ABCD: READ";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
    LE_ASSERT(le_sem_GetValue(HandlerSemaphore) == 0);

    // Send AT+ABCD;+ABCD=?;+ABCD? concatenated command (lower case)
    IntermediateRspPtr = "+ABCD: ACT";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);
    le_dev_NewData("at+abcd;+abcd=?;+abcd?\r", 23);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    ExpectedType = LE_ATSERVER_TYPE_TEST;
    IntermediateRspPtr = "+ABCD: TEST";
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();

    IntermediateRspPtr = "+ABCD: READ";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    SetExpectedResponse(IntermediateRspPtr);
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("OK");
    le_dev_WaitSemaphore();

    // Try to send intermediate whereas no AT command is in progress
    LE_ASSERT(le_atServer_SendIntermediateResponse(atCmdRef, "Blabla") == LE_FAULT);

    // Send unsolicited
    SetExpectedResponse("+UNSOL: 1");
    le_atServer_SendUnsolicitedResponse("+UNSOL: 1", LE_ATSERVER_ALL_DEVICES, 0);
    le_dev_WaitSemaphore();
    SetExpectedResponse("+UNSOL: 2");
    le_atServer_SendUnsolicitedResponse("+UNSOL: 2", LE_ATSERVER_SPECIFIC_DEVICE, devRef);
    le_dev_WaitSemaphore();

    // send unsolicited when at command in progress
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedIntermediateRsp = false;
    le_sem_Post(ThreadSemaphore);
    SetExpectedResponse("OK");
    le_dev_NewData("AT+ABCD?\r", 9);
    le_atServer_SendUnsolicitedResponse("+UNSOL: 1", LE_ATSERVER_ALL_DEVICES, 0);
    le_atServer_SendUnsolicitedResponse("+UNSOL: 2", LE_ATSERVER_SPECIFIC_DEVICE, devRef);
    le_sem_Wait(HandlerSemaphore);
    le_dev_WaitSemaphore();
    SetExpectedResponse("+UNSOL: 1");
    le_dev_WaitSemaphore();
    SetExpectedResponse("+UNSOL: 2");
    le_dev_WaitSemaphore();

    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
    LE_ASSERT(le_sem_GetValue(HandlerSemaphore) == 0);

    // Send ATE0S3?;+ABCD?;S0?S0=2E1;V0DLS0=\"3\"
    ExpectedParametersNumber = 1;
    ExpectedIntermediateRsp = false;
    SendRspCustom = false;
    ExpectedAtCommandPtr = "ATE";
    ExpectedParamPtr[0] = "0";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    le_sem_Post(ThreadSemaphore);

    SetExpectedResponse("OK");
    le_dev_NewData("ATE0S3?;+ABCD?;S0?S0=2E1;V0DLS0=\"3\"+ABCD\r", 41);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "3";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "AT+ABCD";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedParametersNumber = 0;
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_READ;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "0";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 2;
    ExpectedParamPtr[0] = "0";
    ExpectedParamPtr[1] = "2";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATE";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "1";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATV";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 1;
    ExpectedParamPtr[0] = "0";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATDL";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedParametersNumber = 0;
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "ATS";
    ExpectedType = LE_ATSERVER_TYPE_PARA;
    ExpectedParametersNumber = 2;
    ExpectedParamPtr[0] = "0";
    ExpectedParamPtr[1] = "3";
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);
    ExpectedAtCommandPtr = "AT+ABCD";
    ExpectedType = LE_ATSERVER_TYPE_ACT;
    ExpectedParametersNumber = 0;
    le_sem_Post(ThreadSemaphore);
    le_sem_Wait(HandlerSemaphore);

    le_dev_WaitSemaphore();

    le_dev_Done();
    // test deletion command
    // delete created commands
    i = 0;
    while ( atCmdCreation[i].atCmdPtr != NULL )
    {
        LE_ASSERT(le_atServer_Delete(atCmdCreation[i].cmdRef) == LE_OK);
        i++;
    }
    // test stopping the server
    LE_ASSERT(le_atServer_Stop(devRef) == LE_OK);

    exit(0);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    le_log_SetFilterLevel(LE_LOG_DEBUG);

    // Create a semaphore to coordinate the test
    ThreadSemaphore = le_sem_Create("ThreadSem",0);
    HandlerSemaphore = le_sem_Create("HandlerSem",0);

    le_dev_Init();

    le_thread_Ref_t appThreadRef = le_thread_Create("TestThread", TestHandler, NULL);
    le_thread_Start(appThreadRef);
}
