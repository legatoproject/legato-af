/** @file pa_mcc_simu.h
 *
 * Legato @ref pa_mcc_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_MCC_SIMU_H_INCLUDE_GUARD
#define PA_MCC_SIMU_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * simu init
 *
 **/
//--------------------------------------------------------------------------------------------------
le_result_t mcc_simu_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Report the Call event
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_ReportCallEvent
(
    const char*     phoneNumPtr, ///< [IN] The simulated telephone number
    le_mcc_Event_t  event        ///< [IN] The simulated call event
);

//--------------------------------------------------------------------------------------------------
/**
 * Report the Call termination reason
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_ReportCallTerminationReason
(
    const char*     phoneNumPtr, ///< [IN] The simulated telephone number
    le_mcc_TerminationReason_t  term,
    int32_t                     termCode
);

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the voice dial status.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mccSimu_SetVoiceDialResult
(
    le_result_t voiceDialResult
);

#endif // PA_MCC_SIMU_H_INCLUDE_GUARD

