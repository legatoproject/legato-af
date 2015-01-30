/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa_common.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "atManager/inc/atCmdSync.h"
#include "atManager/inc/atPorts.h"

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the common module.
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_common_Init
(
    void
)
{
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_WARN("Common Module is not initialize in this session");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the SierraWireless proprietary indicator +WIND
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_common_GetWindIndicator
(
    uint32_t*  windPtr
)
{
    le_result_t result = LE_OK;
    atcmdsync_ResultRef_t  resRef = NULL;
    const char* interRespPtr[] = {"+WIND:",NULL};

    LE_ASSERT(windPtr);

    result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                    "AT+WIND?",
                                    &resRef,
                                    interRespPtr,
                                    30000);

    if ( result == LE_OK )
    {
        char* line = atcmdsync_GetLine(resRef,0);
        if ( sscanf(line,"+WIND: %d",windPtr) != 1)
        {
            LE_DEBUG("cannot qet wind indicator");
            result = LE_FAULT;
        }
    }

    le_mem_Release(resRef);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SierraWireless proprietary indicator +WIND
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_common_SetWindIndicator
(
    uint32_t  wind
)
{
    atcmdsync_ResultRef_t  resRef = NULL;
    char atcommand[ATCOMMAND_SIZE];

    atcmdsync_PrepareString(atcommand,ATCOMMAND_SIZE,"AT+WIND=%d",wind);

    le_result_t result = atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                atcommand,
                                                &resRef,
                                                NULL,
                                                30000);

    le_mem_Release(resRef);
    return result;
}
