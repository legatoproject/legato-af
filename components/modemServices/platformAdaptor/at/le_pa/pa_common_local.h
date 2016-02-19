/** @file pa_common_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PACOMMONLOCAL_INCLUDE_GUARD
#define LEGATO_PACOMMONLOCAL_INCLUDE_GUARD


#include "legato.h"

#define FIND_STRING(stringToFind, intoString) \
            (strncmp(stringToFind,intoString,sizeof(stringToFind)-1)==0)

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mcc module
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_common_Init
(
    void
);

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
);

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
);

#endif // LEGATO_PACOMMONLOCAL_INCLUDE_GUARD
