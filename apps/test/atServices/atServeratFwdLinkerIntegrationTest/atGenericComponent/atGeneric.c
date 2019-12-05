/**
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "handlers.h"
#include "atServerUtil.h"

//--------------------------------------------------------------------------------------------------
/**
 * AT Commands definition
 *
 */
//--------------------------------------------------------------------------------------------------
static atServerUtil_AtCmd_t AtCmdCreation[] =
{
    {
        .cmdPtr = "AT+KFTPCFG",
        .cmdRef = NULL,
        .handlerPtr = GenericCmdHandler,
        .contextPtr = NULL
    },
    {
        .cmdPtr = "AT+KFTPLS",
        .cmdRef = NULL,
        .handlerPtr = CalcCmdHandler,
        .contextPtr = NULL
    },
    {
        .cmdPtr = "AT+KFTPSND",
        .cmdRef = NULL,
        .handlerPtr = SendResponseCmdHandler,
        .contextPtr = NULL
    },
    {
        .cmdPtr = "AT+KFTPCNX",
        .cmdRef = NULL,
        .handlerPtr = DataModeCmdHandler,
        .contextPtr = NULL
    },
};


//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

    LE_INFO("============== AT generic command initialization starts =================");

    int i = 0;
    le_result_t result = LE_OK;

    while (i < NUM_ARRAY_MEMBERS(AtCmdCreation))
    {
        result = atServerUtil_InstallCmdHandler(&AtCmdCreation[i]);
        if(LE_OK != result)
        {
            LE_ERROR("Handler subscription failed. Return value: %d", result);
            return;
        }
        i++;
    }
}
