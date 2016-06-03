/** @file le_dev.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_atClient.h"

#define ATCOMMANDCLIENT_UNSOLICITED_SIZE  256
#define ATFSMPARSER_BUFFER_MAX            1024
#define ATPARSER_LINE_MAX                 (36+150)*2
#define LE_ATCLIENT_DATA_SIZE             (36+140)*2

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the atCommandClient interface.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_mgr* DevRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT Command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_cmd
{
    uint32_t                    commandId;                               ///< Id for the command
    uint8_t                     command[LE_ATCLIENT_CMD_SIZE_MAX_LEN+1]; ///< Command string to execute
    size_t                      commandSize;                             ///< strlen of the command
    uint8_t                     data[LE_ATCLIENT_DATA_SIZE+1];           ///< Data to send if needed
    size_t                      dataSize;                                ///< sizeof data to send
    le_dls_List_t               intermediateResp;                        ///< List of string pattern for intermediate
    le_event_Id_t               intermediateId;                          ///< Event Id to report to when intermediateResp is found
    le_dls_List_t               finaleResp;                              ///< List of string pattern for final (ends the command)
    le_event_Id_t               finalId;                                 ///< Event Id to report to when finalResp is found
    bool                        withExtra;                               ///< Intermediate response have two lines
    bool                        waitExtra;                               ///< Internal use, to get the two lines
    uint32_t                    timer;                                   ///< timer value in miliseconds (30s -> 30000)
    le_timer_ExpiryHandler_t    timerHandler;                            ///< timer handler
    le_dls_Link_t               link;                                    ///< used to add command in a wainting list
}
le_atClient_cmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * typedef of le_atClient_machinestring_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_string
{
    char            line[LE_ATCLIENT_CMD_SIZE_MAX_BYTES]; ///< string value
    le_dls_Link_t   link;                                 ///< link for list (intermediate, final or unsolicited)
} le_atClient_machinestring_t;

/*
 * ATParser State Machine reference
 */
typedef struct le_atClient_ParserStateMachine* le_atClient_ParserStateMachineRef_t;

/*
 * atCommandClient State Machine reference
 */
typedef struct le_atClient_ManagerStateMachine* le_atClient_ManagerStateMachineRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the device of one atCommandClient
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_device* le_atClient_DeviceRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * typedef of le_atClient_CmdResponse_t
 *
 * This struct is used the send a line when an Intermediate or a final pattern matched.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_response
{
    le_atClient_CmdRef_t    fromWhoRef;
    char                    line[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
}
le_atClient_CmdResponse_t;


//--------------------------------------------------------------------------------------------------
/**
 * ATParser structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct ATParser
{
    uint8_t  buffer[ATFSMPARSER_BUFFER_MAX];    ///< buffer read
    int32_t  idx;                               ///< index of parsing the buffer
    size_t   endbuffer;                         ///< index where the read was finished (idx < endbuffer)
    int32_t  idx_lastCRLF;                      ///< index where the last CRLF has been found
}
ATParser_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of Event for ATParser
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EVENT_PARSER_CHAR=0,    ///< Any character except CRLF('\r\n') or PROMPT('>')
    EVENT_PARSER_CRLF,      ///< CRLF ('\r\n')
    EVENT_PARSER_PROMPT,    ///< PROMPT ('>')
    EVENT_PARSER_MAX        ///< unused
}
EIndicationATParser;

/*
 * ATParserStateProc_func_t is the function pointer type that
 * is used to represent each state of our machine.
 * The le_atClient_ParserStateMachineRef_t *sm argument holds the current
 * information about the machine most importantly the
 * current state.  A ATParserStateProc_func_t function may receive
 * input that forces a change in the current state.  This
 * is done by setting the curState member of the StateMachine.
 */
typedef void (*ATParserStateProc_func_t)(le_atClient_ParserStateMachineRef_t sm, EIndicationATParser input);

/*
 * Now that ATParserStateProc_func_t is defined, we can define the
 * actual layout of StateMachine.  Here we have the
 * current state of the ATParser StateMachine.
 */
typedef struct le_atClient_ParserStateMachine
{
    ATParserStateProc_func_t              prevState;          ///< Previous state for debuging purpose
    ATParserStateProc_func_t              curState;           ///< Current state
    EIndicationATParser                   lastEvent;          ///< Last event received for debugging purpose
    ATParser_t                            curContext;         ///< ATParser structure
    le_atClient_ManagerStateMachineRef_t  atCommandClientPtr; ///< Reference of which atCommandClient it belongs
}
le_atClient_ParserStateMachine_t;


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of Event for atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EVENT_MANAGER_SENDCMD=0,    ///< Send command
    EVENT_MANAGER_SENDDATA,     ///< Send data
    EVENT_MANAGER_PROCESSLINE,  ///< Process line event
    EVENT_MANAGER_CANCELCMD,    ///< Cancel command event
    EVENT_MANAGER_MAX           ///< Unused
}
EIndicationATCommandClient;

/*
 * ATCommandClientStateProc_Func_t is the function pointer type that
 * is used to represent each state of our machine.
 * The le_atClient_ManagerStateMachineRef_t *smRef argument holds the current
 * information about the machine most importantly the
 * current state.  A ATCommandClientStateProc_Func_t function may receive
 * input that forces a change in the current state.  This
 * is done by setting the curState member of the atCommandClient StateMachine.
 */
typedef void (*ATCommandClientStateProc_Func_t)(le_atClient_ManagerStateMachineRef_t smRef, EIndicationATCommandClient input);


//--------------------------------------------------------------------------------------------------
/**
 * typedef of le_atClient_device_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_device
{
    char               name[LE_ATCLIENT_CMD_SIZE_MAX_LEN]; ///< Name of the device
    char               path[LE_ATCLIENT_CMD_SIZE_MAX_LEN]; ///< Path of the device
    uint32_t           handle;                             ///< Handle of the device.
    le_fdMonitor_Ref_t fdMonitor;                          ///< fd event monitor associated to Handle
}
le_atClient_device_t;

//--------------------------------------------------------------------------------------------------
/**
 * typedef of atUnsolicited_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t unsolicitedReportId;                        ///< Event Id to report to
    char          unsolRsp[ATCOMMANDCLIENT_UNSOLICITED_SIZE]; ///< pattern to match
    bool          withExtraData;                              ///< Indicate if the unsolicited has more than one line
    bool          waitForExtraData;                           ///< Indicate if this is the extra data to send
    le_dls_Link_t link;                                       ///< Used to link in Unsolicited List
}
atUnsolicited_t;

/**
 * atCommandClient structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atCommandClient
{
    le_atClient_ParserStateMachine_t atParser;                  ///< AT Machine Parser
    struct le_atClient_device        le_atClient_device;        ///< AT device
    char                             atLine[ATPARSER_LINE_MAX]; ///< Last line retreived in atParser
    le_atClient_CmdRef_t             atCommandInProgressRef;    ///< current command executed
    le_dls_List_t                    atCommandList;             ///< List of command waiting for execution
    le_timer_Ref_t                   atCommandTimer;            ///< command timer
    le_dls_List_t                    atUnsolicitedList;         ///< List of unsolicited pattern
}
ATCommandClient_t;

/*
 * Now that ATCommandClientStateProc_Func_t is defined, we can define the
 * actual layout of StateMachine.  Here we have the
 * current state of the atCommandClient StateMachine.
 */
typedef struct le_atClient_ManagerStateMachine
{
    ATCommandClientStateProc_Func_t prevState;      ///< Previous state for debugging purpose
    ATCommandClientStateProc_Func_t curState;       ///< Current state
    EIndicationATCommandClient      lastEvent;      ///< Last event received for debugging purpose
    ATCommandClient_t               curContext;     ///< atCommandClient structure
}
le_atClient_ManagerStateMachine_t;

//--------------------------------------------------------------------------------------------------
/**
 * atCommandClient interface structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_atClient_mgr
{
    struct le_atClient_ManagerStateMachine atCommandClient; ///< FSM
    le_event_Id_t resumeInterfaceId;                        ///< event to start an interface
    le_event_Id_t suspendInterfaceId;                       ///< event to stop an interface
    le_event_Id_t subscribeUnsolicitedId;                   ///< event to add unsolicited to the FSM
    le_event_Id_t unSubscribeUnsolicitedId;                 ///< event to remove unsoilicited for the FSM
    le_event_Id_t sendCommandId;                            ///< event to send a command
    le_event_Id_t cancelCommandId;                          ///< event to cancel a command
    le_sem_Ref_t  waitingSemaphore;                         ///< semaphore used to synchronize le_atClient_mgr_ API
}
le_atClient_mgr_t;


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atstring, the atcommandclientitf and the atunsolicited
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add new string into a le_dls_List_t.
 *
 * @note
 * pList must be finished by a NULL.
 *
 * @return nothing
 */
//--------------------------------------------------------------------------------------------------
void le_dev_AddInList
(
    le_dls_List_t* list,          ///< List of le_atClient_machinestring_t
    const char**   patternListPtr ///< List of pattern
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release all string list.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ReleaseFromList
(
    le_dls_List_t* pList
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the ID of a command
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t le_dev_GetId
(
    le_atClient_CmdRef_t atCommandRef  ///< AT Command
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the command string
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_dev_GetCommand
(
    le_atClient_CmdRef_t  atCommandRef,  ///< AT Command
    char*                 commandPtr,    ///< [IN/OUT] Command buffer
    size_t                commandSize    ///< [IN] Size of commandPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an interface for the given device on a device.
 *
 * @return a reference on the atCommandClient of this device
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED DevRef_t le_dev_CreateInterface
(
    le_atClient_DeviceRef_t deviceRef    ///< Device
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to save the line to process, and execute the atCommandClient FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ProcessLine
(
    le_atClient_ManagerStateMachineRef_t  smRef,
    char*                                 linePtr,
    size_t                                lineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to resume the current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Resume
(
    void* report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to suspend the current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Suspend
(
    void* report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to add unsolicited string into current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_AddUnsolicited
(
    void* report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to remove unsolicited string from current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_RemoveUnsolicited
(
    void* report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to send a new AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_SendCommand
(
    void* report    ///< Callback parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is to cancel an AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_CancelCommand
(
    void* report    ///< Callback parameter
);


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_InitializeState
(
    le_atClient_ParserStateMachineRef_t smRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read and send event to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ReadBuffer
(
    le_atClient_ParserStateMachineRef_t smRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete character that where already read.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ResetBuffer
(
    le_atClient_ParserStateMachineRef_t smRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Read
(
    le_atClient_DeviceRef_t  deviceRef, ///< device pointer
    uint8_t*                 rxDataPtr, ///< Buffer where to read
    uint32_t                 size       ///< size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
void  le_dev_Write
(
    le_atClient_DeviceRef_t  deviceRef, ///< device pointer
    uint8_t*                 rxDataPtr, ///< Buffer to write
    uint32_t                 size       ///< size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_PrintBuffer
(
    char*    namePtr,       ///< buffer name
    uint8_t* bufferPtr,     ///< the buffer to print
    uint32_t bufferSize     ///< Number of element to print
);


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of intermediate string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_CheckIntermediate
(
    le_atClient_CmdRef_t atCommandRef,
    char*                atLinePtr,
    size_t               atLineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of final string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_dev_CheckFinal
(
    le_atClient_CmdRef_t atCommandRef,
    char*                atLinePtr,
    size_t               atLineSize
);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to prepare the atcommand:
 *  - adding CR at the end of the command
 *  - adding Ctrl-Z for data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Prepare
(
    le_atClient_CmdRef_t atCommandRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an atUnsolicited_t struct
 *
 * @return pointer on the structure
 */
//--------------------------------------------------------------------------------------------------
atUnsolicited_t* le_dev_Create
(
    void
);