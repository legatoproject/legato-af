/**
 * @file atProxyEmux.c
 *
 * AT Proxy Emux implementation for atProxyRemote.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "atProxyRemote.h"
#include "emux.h"
#include "atProxySerialUart.h"
#include "le_atServer_common.h"
#include "atProxy.h"
#include "atProxyCmdHandler.h"
#include "atProxyUnsolicitedRsp.h"

// AT proxy Emux channel
#define AT_PROXY_EMUX_CH    2

// Buffer size for cache the final response string
#define AT_PROXY_FINAL_RESPONSE_MAX_LEN     25

typedef enum le_atProxy_RespType
{
    LE_AT_PROXY_FINAL_RESP,
    LE_AT_PROXY_DATA_MODE,
    LE_AT_PROXY_OTHER_RESP
} le_atProxy_RespType_t;

// Final response strings
static const char* RespFinalList[] = {
    LE_AT_PROXY_OK,
    LE_AT_PROXY_ERROR,
    LE_AT_PROXY_NO_CARRIER,
    LE_AT_PROXY_NO_DIALTONE,
    LE_AT_PROXY_BUSY,
    LE_AT_PROXY_NO_ANSWER
};

// +CME and +CMS final response strings
static const char* RespFinalList2[] = {
    LE_AT_PROXY_CME_ERROR,
    LE_AT_PROXY_CMS_ERROR
};

static char RespFinal[AT_PROXY_FINAL_RESPONSE_MAX_LEN];

static int RespInd;

// Emux handle to AT proxy channel
static emux_handle_t EmuxHandle;


//--------------------------------------------------------------------------------------------------
/**
 * Check whether current RespFinal includes a final response
 *
 * @return true if RespFinal has a final response, false otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool CheckFinalRespList
(
    const char** strList,       ///< [IN] Point to a string array of final responses
    int num,                    ///< [IN] Number of strings in strList
    int indStart                ///< [IN] Index of buffer (RespFinal) to start check if >= 0
)
{
    int i, j, lenToken, ind;
    const char* ptrToken;

    for (i = 0; i < num; i++)
    {
        ptrToken = strList[i];
        lenToken = strnlen(ptrToken, AT_PROXY_FINAL_RESPONSE_MAX_LEN);

        if (indStart >= 0)
        {
            ind = indStart;
        }
        else
        {
            ind = (RespInd - lenToken) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
        }

        for (j = 0; j < lenToken; j++, ptrToken++)
        {
            if (RespFinal[ind] != *ptrToken)
            {
                break;
            }
            ind = (ind + 1) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
        }

        if (j == lenToken)
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if there is an intermediate response denoting data mode in buffer
 *
 * @return true if "\r\nCONNECT\r\n" is found, false otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool CheckDataModeResp
(
    void
)
{
    int j, lenToken, ind;
    const char* ptrToken;

    ptrToken = LE_AT_PROXY_CONNECT;
    lenToken = strnlen(ptrToken, AT_PROXY_FINAL_RESPONSE_MAX_LEN);

    ind = (RespInd - lenToken) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;

    for (j = 0; j < lenToken; j++, ptrToken++)
    {
        if (RespFinal[ind] != *ptrToken)
        {
            break;
        }
        ind = (ind + 1) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
    }

    if (j == lenToken)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if there is a final or interested intermediate response in buffer
 *
 * @return
 *      - LE_AT_PROXY_DATA_MODE     if found "\r\nCONNECT\r\n"
 *      - LE_AT_PROXY_FINAL_RESP    if found any final response
 *      - LE_AT_PROXY_OTHER_RESP    otherwise
 */
//--------------------------------------------------------------------------------------------------
static le_atProxy_RespType_t FindResponse
(
    void
)
{
    int lenToken, ind, numFinalResp;
    const char* ptrToken;

    // Check '\r', as all AT responses should have '\r' before '\n'
    ind = (RespInd - 2) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
    if (RespFinal[ind] != '\r')
    {
        return LE_AT_PROXY_OTHER_RESP;
    }

    // Check if it is a response indicating data mode
    if (CheckDataModeResp())
    {
        return LE_AT_PROXY_DATA_MODE;
    }

    // Check if it is one of the final respones in the list RespFinalList
    numFinalResp = sizeof(RespFinalList) / sizeof(RespFinalList[0]);
    if (CheckFinalRespList(RespFinalList, numFinalResp, -1))
    {
        return LE_AT_PROXY_FINAL_RESP;
    }

    // Check the other two special final response: +CME_ERROR: and +CMS_ERROR:, which have
    // different format.
    ptrToken = LE_AT_PROXY_CME_ERROR;

    // Longest error code has 3 digits + 2 bytes of '\r\n', so + 5
    lenToken = strnlen(ptrToken, AT_PROXY_FINAL_RESPONSE_MAX_LEN) + 5;

    ind = (RespInd - lenToken) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
    int count = 0;
    while (RespFinal[ind] != ptrToken[0])
    {
        // Looks for the index of first character of the token, if we cannot find it in less than 3
        // comparisons (because there are at most 3 digits to skip), then it won't be the a
        // +CME/+CMS final response.
        ind = (ind + 1) % AT_PROXY_FINAL_RESPONSE_MAX_LEN;
        count++;
        if (count >= 3) // impossible to have a +CME/+CMS response
        {
            return LE_AT_PROXY_OTHER_RESP;
        }
    }

    numFinalResp = sizeof(RespFinalList2) / sizeof(RespFinalList2[0]);
    if (CheckFinalRespList(RespFinalList2, numFinalResp, ind))
    {
        return LE_AT_PROXY_FINAL_RESP;
    }

    return LE_AT_PROXY_OTHER_RESP;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback function that will be called when there's data on Emux channel to be read (from MAP)
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
static void MUXRxReceived
(
    uint8_t c,              ///< [IN] Data received from eMUX channel
    void *param             ///< [IN] Parameter during registration of this callback (unused)
)
{
    LE_UNUSED(param);
    char ch = (char)c;

    // If the current active command is local, then the received data here is unsolicted message
    // from MAP
    if (atProxyCmdHandler_isLocalSessionActive())
    {
        atProxyUnsolicitedRsp_parse(ch);
        return;
    }

    atProxySerialUart_write(&ch, 1);

    RespFinal[(RespInd++) % AT_PROXY_FINAL_RESPONSE_MAX_LEN] = ch;

    if (ch == '\n' && atProxyCmdHandler_isActive())
    {
        switch (FindResponse())
        {
            case LE_AT_PROXY_FINAL_RESP:
                LE_DEBUG("Final response detected!");
                memset(RespFinal, 0, AT_PROXY_FINAL_RESPONSE_MAX_LEN);
                atProxyCmdHandler_complete();
            break;

            case LE_AT_PROXY_DATA_MODE:
                LE_DEBUG("Intermediate response CONNECT detected!");
                atProxyCmdHandler_startDataMode();
            break;

            case LE_AT_PROXY_OTHER_RESP:
                LE_DEBUG("Normal data or response.");
            break;

            default:
                LE_ERROR("Unrecognized response!");
            break;
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Process unsolicited data/messages if there are any.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void atProxyRemote_processUnsolicitedMsg
(
    void
)
{
    atProxyUnsolicitedRsp_output();
}

//--------------------------------------------------------------------------------------------------
/**
 * Send AT command or data to remote end
 *
 * @return
 *      - LE_OK         Successfully send data
 *      - LE_FAULT      Sending failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t atProxyRemote_send
(
    char* data,         ///< [IN] Data to be sent
    int len             ///< [IN] Length of data in byte
)
{
    int res;

    res = MUX_Send(EmuxHandle, (const uint8_t *)data, len);

    if (res != emux_Transfer_Success)
    {
        LE_ERROR("MUX_Send failed!\n");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the remote end
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void atProxyRemote_init
(
    void
)
{
    EmuxHandle = MUX_Init(0, AT_PROXY_EMUX_CH, NULL, 0);

    if (EmuxHandle == NULL)
    {
        LE_ERROR("Error in opening at proxy emux channel %d", AT_PROXY_EMUX_CH);
        return;
    }

    MUX_StartRxDataEventCallback(EmuxHandle, MUXRxReceived, NULL);

    // Disable Echo Commmand on Remote Server
    atProxyRemote_send("ATE0\r", strlen("ATE0\r"));
}
