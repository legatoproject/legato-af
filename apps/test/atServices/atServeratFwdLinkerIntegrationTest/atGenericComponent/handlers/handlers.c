/**
 * This module implements handlers functions for AT commands server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "handlers.h"

//--------------------------------------------------------------------------------------------------
/**
 * Standard final response when switching to the command mode
 */
//--------------------------------------------------------------------------------------------------
#define ATSERVERUTIL_OK "\r\nOK\r\n"

//--------------------------------------------------------------------------------------------------
/**
 * Standard response when switching to the data mode
 */
//--------------------------------------------------------------------------------------------------
#define ATSERVERUTIL_CONNECT "\r\nCONNECT\r\n"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of parameters
 */
//--------------------------------------------------------------------------------------------------
#define PARAM_MAX 10

//--------------------------------------------------------------------------------------------------
/**
 * Constants of the calc command handler
 */
//--------------------------------------------------------------------------------------------------
#define CCALC_PARAM_MIN 2
#define CCALC_PARAM_MAX 3
#define CCALC_OP_ADD  0
#define CCALC_OP_SUB  1
#define CCALC_OP_INC  2
#define CCALC_OP_DEC  3
#define CCALC_MAX_OP_NAME_BUF_LENGTH 4


//--------------------------------------------------------------------------------------------------
/**
 * Generic command handler which outputs the type of AT command. This can be mapped to any white
 * listed AT command in the modem. Example:
 * AT+KFTPCFG?         --> READ
 * AT+KFTPCFG=?        --> TEST
 * AT+KFTPCFG=1,2,3,4  --> PARA
 * AT+KFTPCFG          --> ACTION
 */
//--------------------------------------------------------------------------------------------------
void GenericCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
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

    /* Send Final response */
    LE_ASSERT(
        le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0)
        == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * calc command handler
 *
 * The calc command handler will perform the specified mathematical operation on operands. This
 * handler is used for the end-to-end testing and as a minimalist template for other AT command
 * handlers.
 *
 * The generic command handler (GenericCmdHandler) is also used for testing purposes. However, the
 * difference between the calc command handler and the generic command handler is that this handler
 * has strict syntax rules and will report error when the input command doesn't follow the syntax,
 * but the generic command handler always returns OK. If you need to test whether your app can
 * handle error responses correctly, you need to use the calc command handler.
 *
 * This command handler can be bound to any AT command name, and it can correctly report its
 * command name in responses. The following +CCALC command name is an example, which can be
 * replaced by any AT command name in the whitelist of the modem firmware.
 *
 *
 * SYNTAX               RESPONSE
 * ======               ========
 * Test Command:        +CCALC: (list of supported <op>s),(possible values of <operand_1>),
 * AT+CCALC=?                   [(possible values of <operand_2>)]
 *
 * Read Command:        +CCALC: <op>,<operand_1>,<operand_2>
 * AT+CCALC?
 *
 * Write Command:       OK
 * AT+CCALC=<op>,
 * <operand_1>,         Parameters
 * [<operand_2>]        ----------
 *                      <op>   Mathematical operation
 *                      "ADD"  Add <operand_1> and <operand_2>
 *                      "SUB"  Subtract <operand_2> from <operand_1>
 *                      "MUL"  Multiply <operand_1> by <operand_2>
 *                      "INC"  Add one to <operand_1>
 *                      "DEC"  Subtract one from <operand_1>
 *
 * Execution Command:   <result>
 * AT+CCALC             OK
 */
//--------------------------------------------------------------------------------------------------
void CalcCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    char opName[CCALC_MAX_OP_NAME_BUF_LENGTH] = {0};
    static int op = 0;
    static int operand_1 = 0;
    static int operand_2 = 0;

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);
    LE_ASSERT(le_atServer_GetCommandName(
        commandRef, atCommandName, LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            if (parametersNumber < CCALC_PARAM_MIN || parametersNumber > CCALC_PARAM_MAX)
            {
                memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                        "%s: Wrong number of parameters.", atCommandName + 2);
                LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0)
                        == LE_OK);
                return;
            }

            for (int i = 0; i < parametersNumber; i++)
            {
                memset(param, 0, LE_ATDEFS_PARAMETER_MAX_BYTES);
                LE_ASSERT(
                    le_atServer_GetParameter(commandRef,
                                             i,
                                             param,
                                             LE_ATDEFS_PARAMETER_MAX_BYTES)
                    == LE_OK);

                if (i == 0)
                {
                    if (strcasecmp(param, "ADD") == 0)
                    {
                        op = CCALC_OP_ADD;
                    }
                    else if (strcasecmp(param, "SUB") == 0)
                    {
                        op = CCALC_OP_SUB;
                    }
                    else if (strcasecmp(param, "INC") == 0)
                    {
                        op = CCALC_OP_INC;
                    }
                    else if (strcasecmp(param, "DEC") == 0)
                    {
                        op = CCALC_OP_DEC;
                    }
                    else
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                "%s: Unknown operator: '%s'", atCommandName + 2, param);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR,
                                                                  "", 0) == LE_OK);
                        return;
                    }

                }
                else if (i == 1 || i == 2)
                {
                    long operand = strtol(param, NULL, 0);
                    if (operand == 0)
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                "%s: Cannot convert '%s' to a number", atCommandName + 2, param);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR,
                                                                  "", 0) == LE_OK);
                        return;
                    }
                    else if ((operand == LONG_MAX || operand == LONG_MIN) && errno == ERANGE)
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                 "%s: '%s' is out of range (%d to %d)",
                                 atCommandName + 2, param, SCHAR_MIN, SCHAR_MAX);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR,
                                                                  "", 0) == LE_OK);
                        return;
                    }
                    else if (operand < SCHAR_MIN || operand > SCHAR_MAX)
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                 "%s: '%d' is out of range (%d to %d)",
                                 atCommandName + 2, operand, SCHAR_MIN, SCHAR_MAX);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR,
                                                                  "", 0) == LE_OK);
                        return;
                    }
                    else
                    {
                        if (i == 1)
                        {
                            operand_1 = operand;
                        }
                        else if (i == 2)
                        {
                            operand_2 = operand;
                        }
                    }
                }
            }
            break;

        case LE_ATSERVER_TYPE_TEST:
            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                    "%s: (\"ADD\",\"SUB\",\"INC\",\"DEC\"),(%d-%d),[(%d-%d)]",
                    atCommandName + 2, SCHAR_MIN, SCHAR_MAX, SCHAR_MIN, SCHAR_MAX);
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
            break;

        case LE_ATSERVER_TYPE_READ:
            memset(opName, 0, CCALC_MAX_OP_NAME_BUF_LENGTH);
            switch (op)
            {
                case CCALC_OP_ADD:
                    snprintf(opName, CCALC_MAX_OP_NAME_BUF_LENGTH, "ADD");
                    break;
                case CCALC_OP_SUB:
                    snprintf(opName, CCALC_MAX_OP_NAME_BUF_LENGTH, "SUB");
                    break;
                case CCALC_OP_INC:
                    snprintf(opName, CCALC_MAX_OP_NAME_BUF_LENGTH, "INC");
                    break;
                case CCALC_OP_DEC:
                    snprintf(opName, CCALC_MAX_OP_NAME_BUF_LENGTH, "DEC");
                    break;
            }

            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s: %s,%d,%d",
                     atCommandName + 2, opName, operand_1, operand_2);
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
            break;

        case LE_ATSERVER_TYPE_ACT:
        {
            long v = 0;
            switch (op)
            {
                case CCALC_OP_ADD:
                    v = operand_1 + operand_2;
                    break;
                case CCALC_OP_SUB:
                    v = operand_1 - operand_2;
                    break;
                case CCALC_OP_INC:
                    v = operand_1 + 1;
                    break;
                case CCALC_OP_DEC:
                    v = operand_1 - 1;
                    break;
            }
            memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%d", v);
            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
            break;

        }
        default:
            LE_ASSERT(0);
        break;
    }

    LE_ASSERT(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0) == LE_OK);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sending response command handler for testing sending intermediate, unsolicited, and final
 * responses.
 */
//--------------------------------------------------------------------------------------------------
void SendResponseCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES] = {0};

    /* Get command name */
    LE_ASSERT(
        le_atServer_GetCommandName(commandRef, atCommandName, LE_ATDEFS_COMMAND_MAX_BYTES)
        == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    switch (type)
    {
        case LE_ATSERVER_TYPE_ACT:
        {
            char resp[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};

            memset(resp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
            snprintf(resp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                     "Sending intermediate response for %s AT COMMAND ",
                     atCommandName+2);

            char urcResp[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};

            LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, resp) == LE_OK);

            /* Send unsolicited response in between intermediate and final response */
            for (int i = 1; i <= 2; i++)
            {
                memset(urcResp, 0, sizeof(urcResp));
                snprintf(urcResp, sizeof(urcResp), "%s %d", "Sending URC before final response",
                         1234567+i);

                LE_ASSERT(
                    le_atServer_SendUnsolicitedResponse(urcResp, LE_ATSERVER_ALL_DEVICES, NULL)
                    == LE_OK);
            }

            LE_ASSERT(
                le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0)
                == LE_OK);

            /* Send unsolicited responses, testing string with ':' */
            for (int i = 1; i <= 10; i++)
            {
                memset(urcResp, 0, sizeof(urcResp));
                snprintf(urcResp, sizeof(urcResp), "%s : %d", "Sending URC", i);

                LE_ASSERT(
                    le_atServer_SendUnsolicitedResponse(urcResp, LE_ATSERVER_ALL_DEVICES, NULL)
                    == LE_OK);
            }

            /* Send unsolicited responses, testing string without ':' */
            for (int i = 1; i <= 10; i++)
            {
                memset(urcResp, 0, sizeof(urcResp));
                snprintf(urcResp, sizeof(urcResp), "%s %d", "Sending URC", 10+i);

                LE_ASSERT(
                    le_atServer_SendUnsolicitedResponse(urcResp, LE_ATSERVER_ALL_DEVICES, NULL)
                    == LE_OK);
            }

            LE_ASSERT(
                     le_atServer_SendUnsolicitedResponse("OK", LE_ATSERVER_ALL_DEVICES, NULL)
                     == LE_OK);
            break;
        }

        case LE_ATSERVER_TYPE_TEST:
        {
            LE_ASSERT(
                le_atServer_SendIntermediateResponse(commandRef, "Send final non-empty patternPtr")
                == LE_OK);
            LE_ASSERT(
              le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_NO_DIALTONE, "NO DIALTONE", 3)
              == LE_OK);
            break;
        }

        default:
        {
            LE_ASSERT(
                   le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "+CME ERROR: ", 4)
                   == LE_OK);
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Data mode command handler
 *
 * This command handler is used to test the Switch API and Data Forward API.
 *
 */
//--------------------------------------------------------------------------------------------------
void DataModeCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    // Get command name
    LE_ASSERT_OK(le_atServer_GetCommandName(commandRef, atCommandName,
                 LE_ATDEFS_COMMAND_MAX_BYTES));

    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName + 2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            le_utf8_Append(rsp, "PARA", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            le_utf8_Append(rsp, "TEST", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            le_utf8_Append(rsp, "READ", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            le_utf8_Append(rsp, "ACT", LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);
        break;

        default:
            LE_TEST_INFO("AT command type is not proper!");
            LE_ERROR("AT command type is not proper!");
            pthread_exit(NULL);
        break;
    }

    if (type != LE_ATSERVER_TYPE_ACT)
    {
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0));
        return;
    }

    le_atServer_DeviceRef_t atServerDevRef = NULL;
    le_result_t result = le_atServer_GetDevice(commandRef, &atServerDevRef);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get device information! Result: %d", result);
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    le_port_DeviceRef_t devRefPtr = NULL;
    result = le_port_GetPortReference(atServerDevRef, &devRefPtr);
    if (LE_OK != result)
    {
        LE_ERROR("Cannot get port reference! Result: %d", result);
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    int atSockFd = -1;
    result = le_port_SetDataMode(devRefPtr, &atSockFd);
    if (LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    if(-1 == le_fd_Write(atSockFd, ATSERVERUTIL_CONNECT, strlen(ATSERVERUTIL_CONNECT)))
    {
        LE_ERROR("CONNECT write error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    char buf[LE_CONFIG_PIPE_BLOCK_SIZE * 3] = {0};
    for (;;)
    {
        memset(buf, 0, sizeof(buf));
        ssize_t ret = le_fd_Read(atSockFd, buf, sizeof(buf));
        if (ret <= 0)
        {
            LE_TEST_INFO("Fail to read raw data from MCU: %d", errno);
            LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
            return;
        }
        else if (ret > 0)
        {
            if (strcmp(buf, "+++") == 0)
            {
                break;
            }

            size_t writeSize = 0;
            ssize_t ws = 0;
            do
            {
                ws = le_fd_Write(atSockFd, buf + writeSize, ret - writeSize);
                if (ws == -1 && errno != EAGAIN)
                {
                    break;
                }
                else if (ws > 0)
                {
                    writeSize += ws;
                }
            }
            while (writeSize < ret);

            continue;
        }
    }

    // Close data port
    LE_INFO("Switch back to the command mode");
    le_fd_Close(atSockFd);
    result = le_port_SetCommandMode(devRefPtr, &atServerDevRef);
    if(LE_OK != result)
    {
        LE_ERROR("le_port_SetDataMode API usage error");
        LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_ERROR, "", 0));
        return;
    }

    LE_ASSERT_OK(le_atServer_SendFinalResultCode(commandRef, LE_ATSERVER_OK, "", 0));
}
