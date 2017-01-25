/** @file pa_pcm.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PAPCM_INCLUDE_GUARD
#define LEGATO_PAPCM_INCLUDE_GUARD

#include "pa_audio.h"
#include "le_audio_local.h"
#include "le_media_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * callback prototype to get/set the PCM frames for playback/capture.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*GetSetFramesFunc_t)
(
    uint8_t* bufferPtr,
    uint32_t* bufLenPtr,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * callback prototype to get the final result of playback/capture.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*ResultFunc_t)
(
    le_result_t result,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Start the playback.
 * The function is asynchronous: it starts the playback thread, then returns.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Play
(
    pcm_Handle_t pcmHandle                  ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
);

//--------------------------------------------------------------------------------------------------
/**
 * Start the recording.
 * The function is asynchronous: it starts the recording thread, then returns.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Capture
(
    pcm_Handle_t pcmHandle                  ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions

);

//--------------------------------------------------------------------------------------------------
/**
 * Set the callbacks called during a playback/recording to:
 * - get/set PCM frames thanks to getFramesFunc callback: this callback will be called by the pa_pcm
 * to get the next PCM frames to play (playback case), or to send back PCM frames to record
 * (recording case).
 * - get the playback/recording result thanks to setResultFunc callback: this callback will be
 * called by the PA_PCM to inform the caller about the status of the playback or the recording.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_SetCallbackHandlers
(
    pcm_Handle_t pcmHandle,
    GetSetFramesFunc_t framesFunc,
    ResultFunc_t setResultFunc,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Close sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_Close
(
    pcm_Handle_t pcmHandle                 ///< [IN] Handle of the audio resource given by
                                           ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                           ///< initialization functions
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the period Size from sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint32_t pa_pcm_GetPeriodSize
(
    pcm_Handle_t pcmHandle                  ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM capture.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_InitCapture
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM playback.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_pcm_InitPlayback
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
);


#endif // LEGATO_PAPCM_INCLUDE_GUARD
