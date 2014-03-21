/**
 * @page c_atmgr AT manager
 *
 * This module provide AT Command Managment.
 *
 * @section atmgr_toc Table of Contents
 *
 * - @ref atmgr_intro
 * - @ref atmgr_rational
 *  - @ref atmgr_start
 *  - @ref atmgr_configport
 *  - @ref atmgr_unsolicited
 *  - @ref atmgr_atcommand
 *
 * @section atmgr_intro Introduction
 *
 * The main idea is to provide the possibility to subscribe to unsolicited string pattern and
 * to send AT Command.
 *
 *
 * @section atmgr_rational Rational
 *
 * @subsection atmgr_start Module Start
 * *
 * After calling @ref atmgr_Start, the ATManager is launch.
 * This is a new thread.
 * So all interface command are available:
 *  - @ref atmgr_StartInterface
 *  - @ref atmgr_StopInterface
 *  - @ref atmgr_SubscribeUnsolReq
 *  - @ref atmgr_UnSubscribeUnsolReq
 *  - @ref atmgr_SendCommandRequest
 *  - @ref atmgr_CancelCommandRequest
 *
 * @subsection atmgr_configport Configuration Context
 *
 * @ref atmgr_StartInterface and @ref atmgr_StopInterface are used to attach a new
 * @ref atDevice_t where AT Command can be sent or Unsolicited AT Pattern can be catch.
 *
@code
#define UART_PORT "/dev/ttyUSB0"

atdevice_Ref_t deviceRef;
deviceRef->handle = le_uart_open(UART_PORT)
deviceRef->deviceItf.read = (le_da_DeviceReadFunc_t)le_uart_read;
deviceRef->deviceItf.write = (le_da_DeviceWriteFunc_t)le_uart_write);
deviceRef->deviceItf.io_control = (le_da_DeviceIoControlFunc_t)le_uart_ioctl);
deviceRef->deviceItf.close = (le_da_DeviceCloseFunc_t)le_uart_close);

atmgr_Ref_t interfacePtr = atmgr_StartInterface(deviceRef);

...

atmgr_StopInterface(interfacePtr);
@endcode
 *
 * @subsection atmgr_unsolicited Subscribe Unsolicited pattern
 *
 * To subscibe to an unsolicited pattern, we need to add a Handler to the @ref le_event_Id_t that
 * will be subscribe with @ref atmgr_SubscribeUnsolReq
 *
 * @code
atmgr_SubscribeUnsolReq(interfacePtr,
                            ID_ATUnsolicited,
                            "+CMNI",
                            false
);
atmgr_SubscribeUnsolReq(interfacePtr,
                            ID_ATUnsolicited,
                            "+CREG",
                            false
);
atmgr_SubscribeUnsolReq(interfacePtr,
                            ID_ATUnsolicited,
                            "+WIND",
                            false
);
 * @endcode

 * To unsubscribe to an unsolicited pattern, we need to tell which @ref le_event_Id_t and string
 * that will be unsubscribe with @ref atmgr_SubscribeUnsolReq

 * @code
atmgr_UnSubscribeUnsolReq(interfacePtr,
                              ID_ATUnsolicited,
                              "+CREG"
);

 * @endcode
 *
 * Example of a CallBack for unsolicited pattern.
 * The @ref report pointer will be @ref atmgr_UnsolResponse_t in this callback.
 * @code
static void ATUnsolicited(void* report)
{
    atmgr_UnsolResponse_t* unsolPtr = report;

    fprintf(stdout,"\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    fprintf(stdout,"Unsolicited : -%s-\n",unsolPtr->line);
    fprintf(stdout,"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n\n");

}
 @endcode
 *
 * @subsection atmgr_atcommand Sending AT Command
 *
 * As this module use @ref c_eventLoop, all answer are asynchronous. The developer is in charge
 * of doing these call synchronous.
 *
 * Here is an example of doing this
 * @warning In this example there is no check if the answer is currently receive for the
 * AT command sent.
 *
 * @code
#define ATCMDRESPONSE_SIZE  1024
static char     ATCmdResponse[ATCMDRESPONSE_SIZE];
static sem_t MySignal;

static void ATCommandResponseInter(void *report)
{
    uint32_t size;
    atcmd_Response_t* respPtr = report;

    size = ATCMDRESPONSE_SIZE - strlen(respPtr->line) - strlen(ATCmdResponse);

    fprintf(stdout,"\n############################################################\n");
    fprintf(stdout,"Intermediate Response : -%s-\n",respPtr->line);
    fprintf(stdout,"############################################################\n\n");

    strncat(ATCmdResponse,respPtr->line,size);
    ATCmdResponse[strlen(ATCmdResponse)]='\n';
}

static void ATCommandResponseFinal(void *report)
{
    uint32_t size;
    atcmd_Response_t* respPtr = report;

    size = ATCMDRESPONSE_SIZE - strlen(respPtr->line) - strlen(ATCmdResponse);

    fprintf(stdout,"\n############################################################\n");
    fprintf(stdout,"Final Response : -%s-\n",respPtr->line);
    fprintf(stdout,"############################################################\n\n");

    strncat(ATCmdResponse,respPtr->line,size);
    ATCmdResponse[strlen(ATCmdResponse)]='\n';

    sem_post(&MySignal);
}

static char* SendAndWaitForResponse(atmgr_Ref_t interfacePtr,atcmd_Ref_t atCmdRef)
{
    memset(ATCmdResponse,0,sizeof(ATCmdResponse));

    atmgr_SendCommandRequest(interfacePtr,atCmdRef);
    sem_wait(&MySignal);

    return ATCmdResponse;
}
@endcode
 *
 * Before using @ref atmgr_SendCommandRequest, the developer should create
 * @ref ATCommand_t
 *
 * For doing this, we provide these API:
 *  - @ref atcmd_Create
 *  - @ref atcmd_AddIntermediateResp
 *  - @ref atcmd_AddFinalResp
 *  - @ref atcmd_AddCommand
 *  - @ref atcmd_AddData
 *  - @ref atcmd_SetTimer
 *
 * Example for sending an SMS
@code

static void timerHandler(le_timer_Ref_t timerRef)
{
    // timer code
}

...

atcmd_Ref_t newATCmdRef = atcmd_Create();
char* interRespPtr[] = {"+CMGS:",NULL}; // Add intermediate string matching
char* finalRespPtr[] = {"OK","ERROR","+CME ERROR:",NULL}; // String pattern for ending an AT Command
char* respPtr = NULL;  // Response
char  cmd[] = "AT+CMGS=\"XXXXXXXXXX\"";  // the AT Command
char  data[] = "Test sending message\n"; // Data to send when prompt is received

atcmd_AddIntermediateResp(newATCmdRef,ID_ATCmdResp_Inter,interRespPtr); // link intermediate string matching to le_event_Id_t
atcmd_AddFinalResp(newATCmdRef,ID_ATCmdResp_Final,finalRespPtr); // link final string matching to le_event_Id_t
atcmd_AddCommand(newATCmdRef,cmd,false); // set the AT Command with no-extra data
atcmd_AddData(newATCmdRef,data,strlen(data)); // set data to send when a prompt will be available
atcmd_SetTimer(newATCmdRef,500,timerHandler); // set the timer with his handler

respPtr = SendAndWaitForResponse(interfacePtr, newATCmdRef); // Send the command and wait for answer. This is a blocking call

le_mem_Release(newATCmdRef);
@endcode
 * Example for listing all message
@code
atcmd_Ref_t newATCmdRef = atcmd_Create();
char* interRespPtr[] = {"+CMGL:",NULL}; // Add intermediate string matching
char* finalRespPtr[] = {"OK","ERROR","+CME ERROR:",NULL}; // String pattern for ending an AT Command
char* respPtr = NULL; // Response
char  cmd[] = ""AT+CMGL=\"ALL\"";  // the AT Command

atcmd_AddIntermediateResp(newATCmdRef,ID_ATCmdResp_Inter,interRespPtr); // link intermediate string matching to le_event_Id_t
atcmd_AddFinalResp(newATCmdRef,ID_ATCmdResp_Final,finalRespPtr); // link final string matching to le_event_Id_t
atcmd_AddCommand(newATCmdRef,cmd,false); // set the AT Command with no-extra data
atcmd_AddData(newATCmdRef,NULL,0); // no data to send, and no prompt checking
atcmd_SetTimer(newATCmdRef,500,timerHandler); // set the port-id where to execute the AT Command

respPtr = SendAndWaitForResponse(interfacePtr, newATCmdRef); // Send the command and wait for answer. This is a blocking call

le_mem_Release(newATCmdRef);
@endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file atMgr.h
 *
 * Legato @ref c_atmgr include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMGR_INCLUDE_GUARD
#define LEGATO_ATMGR_INCLUDE_GUARD

#include "legato.h"
#include "atCmd.h"
#include "atDevice.h"

#define ATMGR_RESPONSELINE_SIZE             512

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the atmanager interface.
 */
//--------------------------------------------------------------------------------------------------
typedef struct atmgr* atmgr_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * This struct is used the send a line when an unsolicited pattern matched.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    char            line[ATMGR_RESPONSELINE_SIZE];
} atmgr_UnsolResponse_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function init the ATManager.
 */
//--------------------------------------------------------------------------------------------------
void atmgr_Start();

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check it the ATManager is started
 *
 * @return true if the ATManager is started, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool atmgr_IsStarted();

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an interface for the given device on a device.
 *
 * @return a reference on the ATManager of this device
 */
//--------------------------------------------------------------------------------------------------
atmgr_Ref_t atmgr_CreateInterface
(
    atdevice_Ref_t deviceRef    ///< Device
);

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
    atmgr_Ref_t atmanageritfPtr    ///< Device
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Stop the ATManager on a device.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_StopInterface
(
    atmgr_Ref_t atmanageritfPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Set an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_SubscribeUnsolReq
(
    atmgr_Ref_t     atmanageritfPtr,     ///< one ATManager interface
    le_event_Id_t   unsolicitedReportId, ///< Event Id to report to
    char           *unsolRspPtr,         ///< pattern to match
    bool            withExtraData        ///< Indicate if the unsolicited has more than one line
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_UnSubscribeUnsolReq
(
    atmgr_Ref_t          atmanageritfPtr,        ///< one ATManager interface
    le_event_Id_t        unsolicitedReportId,    ///< Event Id to report to
    char *               unsolRspPtr             ///< pattern to match
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Send an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_SendCommandRequest
(
    atmgr_Ref_t          atmanageritfPtr,     ///< one ATManager interface
    atcmd_Ref_t       atcommandToSendRef   ///< AT Command Reference
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Cancel an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
void atmgr_CancelCommandRequest
(
    atmgr_Ref_t          atmanageritfPtr,     ///< one ATManager interface
    atcmd_Ref_t       atcommandToCancelRef ///< AT Command Reference
);

#endif /* LEGATO_ATMGR_INCLUDE_GUARD */
