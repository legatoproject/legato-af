/**
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of parameters
 */
//--------------------------------------------------------------------------------------------------
#ifdef MK_CONFIG_PARAMETER_LIST_MAX
#   define PARAM_MAX            MK_CONFIG_PARAMETER_LIST_MAX
#else
#   define PARAM_MAX            10
#endif

#define TIME_TO_WAIT_SEC        3

//--------------------------------------------------------------------------------------------------
/**
 * AtCmd_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char*                      cmdPtr;
    le_atServer_CmdRef_t             cmdRef;
    le_atServer_CommandHandlerFunc_t handlerPtr;
    void*                            contextPtr;
}
AtCmd_t;

//--------------------------------------------------------------------------------------------------
/**
 * Generic command handler which outputs the type of AT command. This can be mapped to any white
 * listed AT command in the modem. Example:
 * AT+HELLOWORLD?         --> READ
 * AT+HELLOWORLD=?        --> TEST
 * AT+HELLOWORLD=1,2,3,4  --> PARA
 * AT+HELLOWORLD          --> ACTION
 */
//--------------------------------------------------------------------------------------------------
void HelloWorldCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    LE_UNUSED(contextPtr);

    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    /* Get command name */
    LE_ASSERT(le_atServer_GetCommandName(
        commandRef,atCommandName,LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s AT COMMAND TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            le_utf8_Append(rsp, "PARA", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_TEST:
            le_utf8_Append(rsp, "TEST", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_READ:
            le_utf8_Append(rsp, "READ", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_ACT:
            le_utf8_Append(rsp, "ACT", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    /* Send the command type into an intermediate response */
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

    /* Send parameters into an intermediate response */
    for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        LE_ASSERT(
            le_atServer_GetParameter(commandRef,
                                    i,
                                    param,
                                    LE_ATDEFS_PARAMETER_MAX_BYTES)
            == LE_OK);

        snprintf(rsp,LE_ATDEFS_RESPONSE_MAX_BYTES,
            "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(
            le_atServer_SendIntermediateResponse(commandRef, rsp)
            == LE_OK);

        LE_INFO("param %d \"%s\"", i, param);
    }

    LE_INFO("Sleep for a few seconds to observe the intermediate reponse before final response");
    le_thread_Sleep(TIME_TO_WAIT_SEC);

    /* Send Final response */
    LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0)== LE_OK);
    LE_INFO("Command handler completed");
}


//--------------------------------------------------------------------------------------------------
/**
 * AT Commands definition
 *
 */
//--------------------------------------------------------------------------------------------------
static AtCmd_t AtCmdCreation[] =
{
    {
        .cmdPtr = "AT+HELLOWORLD",
        .cmdRef = NULL,
        .handlerPtr = HelloWorldCmdHandler,
        .contextPtr = NULL
    },
};

static le_result_t InstallCmdHandler
(
    AtCmd_t* atCmdPtr               ///< [IN] AT command structure list
)
{
    atCmdPtr->cmdRef = le_atServer_Create(atCmdPtr->cmdPtr);
    if (NULL == atCmdPtr->cmdRef)
    {
        LE_ERROR("Cannot create a command: atCmdPtr->cmdRef is NULL!");
        return LE_FAULT;
    }
    le_atServer_AddCommandHandler(atCmdPtr->cmdRef, atCmdPtr->handlerPtr, atCmdPtr->contextPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("============== AT command initialization starts =================");

    int i = 0;
    le_result_t result = LE_OK;

    while (i < NUM_ARRAY_MEMBERS(AtCmdCreation))
    {
        result = InstallCmdHandler(&AtCmdCreation[i]);
        if(LE_OK != result)
        {
            LE_ERROR("Handler subscription failed. Return value: %d", result);
            return;
        }
        i++;
    }
}
