/** @file pa_audio_simu.h
 *
 * Legato @ref pa_audio_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_AUDIO_SIMU_H_INCLUDE_GUARD
#define PA_AUDIO_SIMU_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Check the audio path set.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audioSimu_CheckAudioPathSet
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Check the reseted audio path.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audioSimu_CheckAudioPathReseted
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a reception of a dtmf. Send the dtmf report.
 */
//--------------------------------------------------------------------------------------------------
void pa_audioSimu_ReceiveDtmf
(
    char dtmf
);

//--------------------------------------------------------------------------------------------------
/**
 * Set dtmf configuration.
 */
//--------------------------------------------------------------------------------------------------
void pa_audioSimu_PlaySignallingDtmf
(
    char*          dtmfPtr,
    uint32_t       duration,
    uint32_t       pause
);

#endif

