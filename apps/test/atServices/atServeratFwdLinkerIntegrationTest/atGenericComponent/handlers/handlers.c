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
        le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "")
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
                LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "")
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
                        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR,
                                                                false, "") == LE_OK);
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
                        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR,
                                                                false, "") == LE_OK);
                        return;
                    }
                    else if ((operand == LONG_MAX || operand == LONG_MIN) && errno == ERANGE)
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                 "%s: '%s' is out of range (%d to %d)",
                                 atCommandName + 2, param, SCHAR_MIN, SCHAR_MAX);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR,
                                                                false, "") == LE_OK);
                        return;
                    }
                    else if (operand < SCHAR_MIN || operand > SCHAR_MAX)
                    {
                        memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
                        snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES,
                                 "%s: '%d' is out of range (%d to %d)",
                                 atCommandName + 2, operand, SCHAR_MIN, SCHAR_MAX);
                        LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);
                        LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR,
                                                                false, "") == LE_OK);
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

    LE_ASSERT(le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "") == LE_OK);
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
    if (type != LE_ATSERVER_TYPE_ACT)
    {
        LE_ASSERT(
            le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_ERROR, false, "")
            == LE_OK);
        return;
    }

    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES] = {0};

    /* Get command name */
    LE_ASSERT(
        le_atServer_GetCommandName(commandRef, atCommandName, LE_ATDEFS_COMMAND_MAX_BYTES)
        == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    char resp[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};

    memset(resp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(resp, LE_ATDEFS_RESPONSE_MAX_BYTES, "Sending intermediate response for %s AT COMMAND ",
             atCommandName+2);

    char urcResp[LE_ATDEFS_RESPONSE_MAX_BYTES] = {0};

    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, resp) == LE_OK);

    /* Send unsolicited response in between intermediate and final response */
    for (int i = 1; i <= 2; i++)
    {
        memset(urcResp, 0, sizeof(urcResp));
        snprintf(urcResp, sizeof(urcResp), "%s %d", "Sending URC before final response", 1234567+i);

        LE_ASSERT(
            le_atServer_SendUnsolicitedResponse(urcResp, LE_ATSERVER_ALL_DEVICES, NULL)
            == LE_OK);
    }

    LE_ASSERT(
        le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "")
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

    LE_ASSERT(le_atServer_SendUnsolicitedResponse("OK", LE_ATSERVER_ALL_DEVICES, NULL) == LE_OK);
}
