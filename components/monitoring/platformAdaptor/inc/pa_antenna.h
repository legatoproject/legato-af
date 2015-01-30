/** @file pa_antenna.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAANTENNA_INCLUDE_GUARD
#define LEGATO_PAANTENNA_INCLUDE_GUARD


typedef struct
{
    le_antenna_Type_t    antennaType;
    le_antenna_Status_t  status;

} pa_antenna_StatusInd_t;

typedef void (*pa_antenna_StatusIndHandlerFunc_t)
(
    pa_antenna_StatusInd_t* msgRef
);

le_event_HandlerRef_t* pa_antenna_AddStatusHandler
(
    pa_antenna_StatusIndHandlerFunc_t   msgHandler
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the short limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_SetShortLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t                shortLimit
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the short limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_GetShortLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t*               shortLimitPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the open limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_SetOpenLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t                openLimit
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the open limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_GetOpenLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t*               openLimitPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the antenna status
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_GetStatus
(
    le_antenna_Type_t    antennaType,
    le_antenna_Status_t* statusPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the status indication on a specific antenna
 *
 * * @return
 * - LE_OK on success
 * - LE_BUSY if the status indication is already set for the requested antenna
 * - LE_FAULT on other failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_SetStatusIndication
(
    le_antenna_Type_t antennaType
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to remove the status indication on a specific antenna
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_RemoveStatusIndication
(
    le_antenna_Type_t antennaType
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add a status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t* pa_antenna_AddStatusHandler
(
    pa_antenna_StatusIndHandlerFunc_t   msgHandler
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to intialize the pa_mon_antenna.c
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_antenna_Init
(
    void
);


#endif // LEGATO_PAANTENNA_INCLUDE_GUARD
