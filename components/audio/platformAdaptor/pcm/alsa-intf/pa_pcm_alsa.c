/**
 * @file pa_media.c
 *
 * This file contains the source code of the low level Audio API for playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include "pa_pcm.h"

#include <alsa-intf/alsa_audio.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for set ALSA parameters function.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*SetPcmParamsFunc_t)(struct pcm *pcm);


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set "playback" internal PCM parameter for the Qualcomm alsa
 * driver
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetPcmParamsPlayback
(
    struct pcm* pcmPtr
)
{
    struct snd_pcm_hw_params *params;
    struct snd_pcm_sw_params *sparams;
    uint32_t samplingRes;

    int channels = (pcmPtr->flags & PCM_MONO) ? 1 : ((pcmPtr->flags & PCM_5POINT1)? 6 : 2 );

    // can't use le_mem service, the memory is released in pcm_close()
    params = (struct snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));

    if (!params)
    {
        LE_ERROR("Failed to allocate ALSA hardware parameters!");
        return LE_FAULT;
    }

    param_init(params);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                   (pcmPtr->flags & PCM_MMAP)?
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED :
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT, pcmPtr->format);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);

    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 10);

    switch(pcmPtr->format)
    {
        case SNDRV_PCM_FORMAT_S8:
        samplingRes=8;
        break;

        case SNDRV_PCM_FORMAT_S16_LE:
        samplingRes=16;
        break;

        case SNDRV_PCM_FORMAT_S24_LE:
        samplingRes=24;
        break;

        case SNDRV_PCM_FORMAT_S32_LE:
        samplingRes=32;
        break;

        default:
        LE_ERROR("Unsupported sampling resolution (%d)!", pcmPtr->format);
        free(params);
        return LE_FAULT;
    }

    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, samplingRes);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,pcmPtr->channels * samplingRes);

    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS,pcmPtr->channels);
    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, pcmPtr->rate);

    if (param_set_hw_params(pcmPtr, params))
    {
        LE_ERROR("Cannot set hw params");
        return LE_FAULT;
    }

    pcmPtr->buffer_size = pcm_buffer_size(params);
    pcmPtr->period_size = pcm_period_size(params);
    pcmPtr->period_cnt = pcmPtr->buffer_size/pcmPtr->period_size;

    LE_DEBUG("buffer_size %d period_size %d period_cnt %d", pcmPtr->buffer_size, pcmPtr->period_size,
                                                            pcmPtr->period_cnt);

    // can't use le_mem service, the memory is released in pcm_close()
    sparams = (struct snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));

    if (!sparams)
    {
        LE_ERROR("Failed to allocate ALSA software parameters!");
        return LE_FAULT;
    }

    // Get the current software parameters
    sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    sparams->period_step = 1;

    sparams->avail_min = pcmPtr->period_size/(channels * 2) ;
    sparams->start_threshold =  pcmPtr->period_size/(channels * 2) ;
    sparams->stop_threshold =  pcmPtr->buffer_size ;
    sparams->xfer_align =  pcmPtr->period_size/(channels * 2) ; /* needed for old kernels */

    sparams->silence_size = 0;
    sparams->silence_threshold = 0;

    if (param_set_sw_params(pcmPtr, sparams))
    {
        LE_ERROR("Cannot set sw params");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set "capture" internal PCM parameter for the Qualcomm alsa driver
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetPcmParamsCapture
(
    struct pcm* pcmPtr
)
{
    struct snd_pcm_hw_params *params;
    struct snd_pcm_sw_params *sparams;
    uint32_t samplingRes;

    // can't use le_mem service, the memory is released by pcm_close()
    params = (struct snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));

    if (!params)
    {
        LE_ERROR("Failed to allocate ALSA hardware parameters!");
        return LE_FAULT;
    }

    param_init(params);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                   (pcmPtr->flags & PCM_MMAP)?
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED :
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT, pcmPtr->format);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);

    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 10);

    switch(pcmPtr->format)
    {
        case SNDRV_PCM_FORMAT_S8:
        samplingRes=8;
        break;

        case SNDRV_PCM_FORMAT_S16_LE:
        samplingRes=16;
        break;

        case SNDRV_PCM_FORMAT_S24_LE:
        samplingRes=24;
        break;

        case SNDRV_PCM_FORMAT_S32_LE:
        samplingRes=32;
        break;

        default:
        LE_ERROR("Unsupported sampling resolution (%d)!", pcmPtr->format);
        free(params);
        return LE_FAULT;
    }

    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, samplingRes);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,pcmPtr->channels * samplingRes);

    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS, pcmPtr->channels);
    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, pcmPtr->rate);

    param_set_hw_refine(pcmPtr, params);

    if (param_set_hw_params(pcmPtr, params))
    {
        LE_ERROR("Cannot set hw params");
        return LE_FAULT;
    }

    pcmPtr->buffer_size = pcm_buffer_size(params);
    pcmPtr->period_size = pcm_period_size(params);
    pcmPtr->period_cnt = pcmPtr->buffer_size/pcmPtr->period_size;

    // can't use le_mem service, the memory is released in pcm_close()
    sparams = (struct snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));

    if (!sparams)
    {
        LE_ERROR("Failed to allocate ALSA software parameters!");
        return LE_FAULT;
    }

    sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    sparams->period_step = 1;

    if (pcmPtr->flags & PCM_MONO)
    {
        sparams->avail_min = pcmPtr->period_size/2;
        sparams->xfer_align = pcmPtr->period_size/2;
    }
    else if (pcmPtr->flags & PCM_QUAD)
    {
        sparams->avail_min = pcmPtr->period_size/8;
        sparams->xfer_align = pcmPtr->period_size/8;
    }
    else if (pcmPtr->flags & PCM_5POINT1)
    {
        sparams->avail_min = pcmPtr->period_size/12;
        sparams->xfer_align = pcmPtr->period_size/12;
    }
    else
    {
        sparams->avail_min = pcmPtr->period_size/4;
        sparams->xfer_align = pcmPtr->period_size/4;
    }

    sparams->start_threshold = 1;
    sparams->stop_threshold = INT_MAX;
    sparams->silence_size = 0;
    sparams->silence_threshold = 0;

    if (param_set_sw_params(pcmPtr, sparams))
    {
        LE_ERROR("Cannot set sw params");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init Alsa driver for pcm playback/capture.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitPcmPlaybackCapture
(
    pcm_Handle_t *pcmHandlePtr,
    const char* devicePtr,
    le_audio_SamplePcmConfig_t* pcmConfig,
    uint32_t flags,
    SetPcmParamsFunc_t paramsFunc
)
{
    uint32_t format;

    flags |= PCM_NMMAP;

    if (pcmConfig->channelsCount == 1)
    {
        flags |= PCM_MONO;
    }
    else if (pcmConfig->channelsCount == 2)
    {
        flags |= PCM_STEREO;
    }
    else if (pcmConfig->channelsCount == 4)
    {
        flags |= PCM_QUAD;
    }
    else if (pcmConfig->channelsCount == 6)
    {
        flags |= PCM_5POINT1;
    }
    else
    {
        flags |= PCM_MONO;
    }

    switch(pcmConfig->bitsPerSample)
    {
        case 8:
        format = SNDRV_PCM_FORMAT_S8;
        break;

        case 16:
        format = SNDRV_PCM_FORMAT_S16_LE;
        break;

        case 24:
        format = SNDRV_PCM_FORMAT_S24_LE;
        break;

        case 32:
        format = SNDRV_PCM_FORMAT_S32_LE;
        break;

        default:
        LE_ERROR("Unsupported sampling resolution (%d)!", pcmConfig->bitsPerSample);
        return LE_FAULT;
    }


    struct pcm *myPcmPtr = pcm_open(flags, (char*) devicePtr);

    if (myPcmPtr < 0)
    {
        return LE_FAULT;
    }

    myPcmPtr->channels = pcmConfig->channelsCount;
    myPcmPtr->rate     = pcmConfig->sampleRate;
    myPcmPtr->flags    = flags;
    myPcmPtr->format   = format;

    if (pcm_ready(myPcmPtr))
    {
        myPcmPtr->channels = pcmConfig->channelsCount;
        myPcmPtr->rate     = pcmConfig->sampleRate;
        myPcmPtr->flags    = flags;
        myPcmPtr->format   = format;

        if (paramsFunc(myPcmPtr)==LE_OK)
        {
            if (pcm_prepare(myPcmPtr))
            {
                LE_ERROR("Failed in pcm_prepare");
                return LE_FAULT;
            }
        }
        else
        {
            LE_ERROR("Failed in set_params");
            return LE_FAULT;
        }
    }
    else
    {
        LE_ERROR("PCM is not ready (pcm error: %s)", pcm_error(myPcmPtr));
        return LE_FAULT;
    }

    *pcmHandlePtr = (pcm_Handle_t) myPcmPtr;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

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
    struct pcm *pcm = (struct pcm *) pcmHandle;
    int myErrno;

    if ( (myErrno = pcm_write(pcm, data, bufsize)) != 0 )
    {
        LE_ERROR("Could not write %d void bytes! errno %d", bufsize, -myErrno);
        return LE_FAULT;

    }
    return LE_OK;
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
    struct pcm *pcm = (struct pcm *) pcmHandle;
    int myErrno;

    if ( (myErrno = pcm_read(pcm, data, bufsize)) != 0 )
    {
        LE_ERROR("Could not write %d void bytes! errno %d", bufsize, -myErrno);
        return LE_FAULT;

    }
    return LE_OK;
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
    struct pcm *pcm = (struct pcm *) pcmHandle;

    LE_DEBUG("Call pcm_close");
    if  (pcm_close(pcm) == 0)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
    struct pcm *pcm = (struct pcm *) pcmHandle;
    return pcm->period_size;
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
    uint32_t flags = PCM_IN;

    return InitPcmPlaybackCapture( pcmHandlePtr,
                                   devicePtr,
                                   pcmConfig,
                                   flags,
                                   SetPcmParamsCapture );
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
    uint32_t flags = PCM_OUT;

    return InitPcmPlaybackCapture( pcmHandlePtr,
                                   devicePtr,
                                   pcmConfig,
                                   flags,
                                   SetPcmParamsPlayback );
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.  Called automatically by the application framework at process start.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
