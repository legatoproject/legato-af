/**
 * @file pa_pcm_default.c
 *
 * Default implementation of the PCM interface.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_pcm.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.  Called automatically by the application framework at process start.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Write PCM frames to sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_pcm_Write
(
    pcm_Handle_t pcmHandle,                 ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
    char* data,                             ///< [OUT] Write buffer
    uint32_t bufsize                        ///< [IN] Buffer length
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read PCM frames from sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_pcm_Read
(
    pcm_Handle_t pcmHandle,                 ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
    char* data,                             ///< [IN] Read buffer
    uint32_t bufsize                        ///< [IN] Buffer length
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_pcm_Close
(
    pcm_Handle_t pcmHandle                 ///< [IN] Handle of the audio resource given by
                                           ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                           ///< initialization functions
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the period Size from sound driver.
 *
 */
//--------------------------------------------------------------------------------------------------
uint32_t pa_pcm_GetPeriodSize
(
    pcm_Handle_t pcmHandle                  ///< [IN] Handle of the audio resource given by
                                            ///< pa_pcm_InitPlayback() / pa_pcm_InitCapture()
                                            ///< initialization functions
)
{
    LE_ERROR("Unsupported function called");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM capture.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_pcm_InitCapture
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize sound driver for PCM playback.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_pcm_InitPlayback
(
    pcm_Handle_t *pcmHandlePtr,             ///< [OUT] Handle of the audio resource, to be used on
                                            ///< further PA PCM functions call
    char* devicePtr,                        ///< [IN] Device to be initialized
    le_audio_SamplePcmConfig_t* pcmConfig   ///< [IN] Samples PCM configuration
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
