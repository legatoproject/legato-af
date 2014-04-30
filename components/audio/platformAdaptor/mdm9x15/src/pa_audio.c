/**
 * @file pa_audio_qmi.c
 *
 * QMI implementation of pa_audio API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "pa_audio.h"
#include "pa_audio_local.h"
#include <alsa-intf/alsa_audio.h>
#include <sys/ioctl.h>


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Define the Audio device.
 */
//--------------------------------------------------------------------------------------------------
#define AUDIO_QUALCOMM_DEVICE_PATH  "/dev/snd/controlC0"

//--------------------------------------------------------------------------------------------------
/**
 * Capture and playback threads flags
 */
//--------------------------------------------------------------------------------------------------
static bool CaptureIsOn;
static bool PlaybackIsOn;

//--------------------------------------------------------------------------------------------------
/**
 * Capture thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  CaptureThreadRef=NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Playback thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t  PlaybackThreadRef=NULL;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * The PCM structure associated to the Capture thread.
 */
//--------------------------------------------------------------------------------------------------
struct audioThreadParameter {
    uint32_t        nbChannel;          ///< Number of channel
    uint32_t        rate;               ///< rate
    uint32_t        format;             ///< format for the driver
    le_sem_Ref_t    threadSemaphore;    ///< semaphore to check starting
};

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set "playback" internal PCM parameter
 * for the Qualcomm alsa driver
 *
 */
//--------------------------------------------------------------------------------------------------
static int SetPCMParamsPlayback
(
    struct pcm *pcm
)
{
    struct snd_pcm_hw_params *params;
    struct snd_pcm_sw_params *sparams;

    int channels = (pcm->flags & PCM_MONO) ? 1 : ((pcm->flags & PCM_5POINT1)? 6 : 2 );

    params = (struct snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params) {
        LE_FATAL("Failed to allocate ALSA hardware parameters!");
    }

    param_init(params);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                   (pcm->flags & PCM_MMAP)?
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED :
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT, pcm->format);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);

    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 10);
    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,pcm->channels * 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS,pcm->channels);
    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, pcm->rate);

    if (param_set_hw_params(pcm, params)) {
        LE_FATAL("cannot set hw params\n");
    }

    pcm->buffer_size = pcm_buffer_size(params);
    pcm->period_size = pcm_period_size(params);
    pcm->period_cnt = pcm->buffer_size/pcm->period_size;

    sparams = (struct snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));
    if (!sparams) {
        LE_FATAL("Failed to allocate ALSA software parameters!");
    }
    // Get the current software parameters
    sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    sparams->period_step = 1;

    sparams->avail_min = pcm->period_size/(channels * 2) ;
    sparams->start_threshold =  pcm->period_size/(channels * 2) ;
    sparams->stop_threshold =  pcm->buffer_size ;
    sparams->xfer_align =  pcm->period_size/(channels * 2) ; /* needed for old kernels */

    sparams->silence_size = 0;
    sparams->silence_threshold = 0;

    if (param_set_sw_params(pcm, sparams)) {
        LE_FATAL("cannot set sw params");
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set "capture" internal PCM parameter
 * for the Qualcomm alsa driver
 *
 */
//--------------------------------------------------------------------------------------------------
static int SetPCMParamsCapture
(
    struct pcm *pcm
)
{
    struct snd_pcm_hw_params *params;
    struct snd_pcm_sw_params *sparams;

    params = (struct snd_pcm_hw_params*) calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params) {
        LE_FATAL("Failed to allocate ALSA hardware parameters!");
    }

    param_init(params);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_ACCESS,
                   (pcm->flags & PCM_MMAP)?
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED :
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT, pcm->format);
    param_set_mask(params, SNDRV_PCM_HW_PARAM_SUBFORMAT, SNDRV_PCM_SUBFORMAT_STD);

    param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 10);

    param_set_int(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_FRAME_BITS,pcm->channels * 16);
    param_set_int(params, SNDRV_PCM_HW_PARAM_CHANNELS,pcm->channels);
    param_set_int(params, SNDRV_PCM_HW_PARAM_RATE, pcm->rate);

    param_set_hw_refine(pcm, params);

    if (param_set_hw_params(pcm, params)) {
        LE_FATAL("cannot set hw params");
    }

    pcm->buffer_size = pcm_buffer_size(params);
    pcm->period_size = pcm_period_size(params);
    pcm->period_cnt = pcm->buffer_size/pcm->period_size;

    sparams = (struct snd_pcm_sw_params*) calloc(1, sizeof(struct snd_pcm_sw_params));
    if (!sparams) {
        LE_FATAL("Failed to allocate ALSA software parameters!");
    }

    sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
    sparams->period_step = 1;

    if (pcm->flags & PCM_MONO) {
        sparams->avail_min = pcm->period_size/2;
        sparams->xfer_align = pcm->period_size/2;
    } else if (pcm->flags & PCM_QUAD) {
        sparams->avail_min = pcm->period_size/8;
        sparams->xfer_align = pcm->period_size/8;
    } else if (pcm->flags & PCM_5POINT1) {
        sparams->avail_min = pcm->period_size/12;
        sparams->xfer_align = pcm->period_size/12;
    } else {
        sparams->avail_min = pcm->period_size/4;
        sparams->xfer_align = pcm->period_size/4;
    }

    sparams->start_threshold = 1;
    sparams->stop_threshold = INT_MAX;
    sparams->silence_size = 0;
    sparams->silence_threshold = 0;

    if (param_set_sw_params(pcm, sparams)) {
        LE_FATAL("cannot set sw params");
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Playback/Capture thread destructor
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyThread
(
    void *contextPtr
)
{
    struct pcm *pcmPtr = (struct pcm *)contextPtr;

    if (pcmPtr) {
        pcm_close(pcmPtr);
    }
    LE_DEBUG("Thread stopped");
}

//--------------------------------------------------------------------------------------------------
/**
 * Playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* PlaybackThread
(
    void* contextPtr
)
{
    struct audioThreadParameter* audioParamPtr = (struct audioThreadParameter*)contextPtr;

    uint32_t openFlags;
    if (audioParamPtr->nbChannel > 1 ) {
        openFlags = PCM_NMMAP|PCM_OUT|PCM_STEREO;
    } else {
        openFlags = PCM_NMMAP|PCM_OUT|PCM_MONO;
    }

    struct pcm *pcmPtr = pcm_open(openFlags, "hw:0,2");
    if (pcmPtr < 0) {
        LE_FATAL("PCM cannot be open");
    }

    le_thread_AddDestructor(DestroyThread,pcmPtr);

    if (!pcm_ready(pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("PCM is not ready");
    }

    pcmPtr->channels    = audioParamPtr->nbChannel;
    pcmPtr->rate        = audioParamPtr->rate;
    pcmPtr->flags       = openFlags;
    pcmPtr->format      = audioParamPtr->format;

    if (SetPCMParamsPlayback (pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("params setting failed\n");
    }

    if (pcm_prepare(pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("Failed in pcm_prepare\n");
    }

    if (ioctl(pcmPtr->fd, SNDRV_PCM_IOCTL_START)) {
        pcm_close(pcmPtr);
        LE_FATAL("Hostless IOCTL_START Error no %d \n", errno);
    }

    LE_DEBUG("Thread Playback Started: channel[%d], rate[%d], format[%d]",
             pcmPtr->channels,pcmPtr->rate,pcmPtr->format);

    le_sem_Post(audioParamPtr->threadSemaphore);
    le_event_RunLoop();
    // Should never happened
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Capture thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* CaptureThread
(
    void* contextPtr
)
{
    struct audioThreadParameter* audioParamPtr = (struct audioThreadParameter*)contextPtr;

    uint32_t openFlags;
    if (audioParamPtr->nbChannel > 1 ) {
        openFlags = PCM_NMMAP|PCM_IN|PCM_STEREO;
    } else {
        openFlags = PCM_NMMAP|PCM_IN|PCM_MONO;
    }

    struct pcm *pcmPtr = pcm_open(openFlags, "hw:0,2");
    if (pcmPtr < 0) {
        LE_FATAL("PCM cannot be open");
    }

    le_thread_AddDestructor(DestroyThread,pcmPtr);

    if (!pcm_ready(pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("PCM is not ready");
    }

    pcmPtr->channels    = audioParamPtr->nbChannel;
    pcmPtr->rate        = audioParamPtr->rate;
    pcmPtr->flags       = openFlags;
    pcmPtr->format      = audioParamPtr->format;

    if (SetPCMParamsCapture (pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("params setting failed\n");
    }

    if (pcm_prepare(pcmPtr)) {
        pcm_close(pcmPtr);
        LE_FATAL("Failed in pcm_prepare\n");
    }

    if (ioctl(pcmPtr->fd, SNDRV_PCM_IOCTL_START)) {
        pcm_close(pcmPtr);
        LE_FATAL("Hostless IOCTL_START Error no %d \n", errno);
    }

    LE_DEBUG("Thread Capture Started: channel[%d], rate[%d], format[%d]",
             pcmPtr->channels,pcmPtr->rate,pcmPtr->format);
    le_sem_Post(audioParamPtr->threadSemaphore);
    le_event_RunLoop();
    // Should never happened
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a playback thread
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The playback format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartPlayback
(
    const char* formatPtr,      ///< [IN] Playback format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    PlaybackIsOn = true;

    struct audioThreadParameter audioParam;

    LE_ASSERT(formatPtr);

    LE_DEBUG("Create Playback thread '%s'",formatPtr);

    if (strcmp("L16-8K",formatPtr)==0) {
        audioParam.nbChannel = channelCount;
        audioParam.rate = 8000;
        audioParam.format = SNDRV_PCM_FORMAT_S16_LE;
        audioParam.threadSemaphore = le_sem_Create("PlaybackSem",0);
    } else {
        LE_ERROR("This format '%s' is not supported",formatPtr);
        return LE_BAD_PARAMETER;
    }

    if (PlaybackThreadRef) {
        LE_ERROR("Playback thread is already started");
        return LE_DUPLICATE;
    }

    PlaybackThreadRef = le_thread_Create("Audio-Playback",
                                         PlaybackThread,
                                         &audioParam);

    le_thread_SetJoinable(PlaybackThreadRef);
    le_thread_Start(PlaybackThreadRef);

    le_sem_Wait(audioParam.threadSemaphore);
    le_sem_Delete(audioParam.threadSemaphore);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a capture thread
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The capture format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartCapture
(
    const char* formatPtr,      ///< [IN] Capture format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    struct audioThreadParameter audioParam;

    LE_ASSERT(formatPtr);

    LE_DEBUG("Create Capture thread '%s'",formatPtr);

    if (strcmp("L16-8K",formatPtr)==0)
    {
        audioParam.nbChannel = channelCount;
        audioParam.rate = 8000;
        audioParam.format = SNDRV_PCM_FORMAT_S16_LE;
        audioParam.threadSemaphore = le_sem_Create("CaptureSem",0);
    }
    else
    {
        LE_ERROR("This format '%s' is not supported",formatPtr);
        return LE_BAD_PARAMETER;
    }

    if (CaptureThreadRef)
    {
        LE_ERROR("Capture thread is already started");
        return LE_DUPLICATE;
    }

    CaptureThreadRef = le_thread_Create("Audio-Capture",
                                        CaptureThread,
                                        &audioParam);

    le_thread_SetJoinable(CaptureThreadRef);
    le_thread_Start(CaptureThreadRef);

    le_sem_Wait(audioParam.threadSemaphore);
    le_sem_Delete(audioParam.threadSemaphore);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the playback and record threads
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The capture format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartThreads
(
    const char* formatPtr,      ///< [IN] Capture format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    le_result_t res;

    LE_ASSERT(formatPtr);

    if (PlaybackIsOn && CaptureIsOn)
    {
        if((res=StartPlayback(formatPtr, channelCount)) != LE_OK)
        {
            return res;
        }
        if((res=StartCapture(formatPtr, channelCount)) != LE_OK)
        {
            return res;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set a mixer value
 *
 */
//--------------------------------------------------------------------------------------------------
void SetMixerParameter
(
    const char* namePtr,
    const char* valuePtr
)
{
    struct mixer *mixerPtr;
    struct mixer_ctl *ctlPtr;

    LE_DEBUG("Set '%s' with value '%s'",namePtr,valuePtr);

    mixerPtr = mixer_open(AUDIO_QUALCOMM_DEVICE_PATH);
    if (mixerPtr==NULL)
    {
        LE_FATAL("Cannot open <%s>",AUDIO_QUALCOMM_DEVICE_PATH);
    }

    ctlPtr = mixer_get_control(mixerPtr, namePtr, 0);
    if (ctlPtr==NULL)
    {
        LE_FATAL("Cannot get mixer controler <%s>", namePtr);
    }

    if (isdigit(valuePtr[0]))
    {
        if ( mixer_ctl_set_value(ctlPtr,1, (char**)&valuePtr ) )
        {
            LE_FATAL("Cannot set the value <%s>",valuePtr);
        }
    } else
    {
        if ( mixer_ctl_select(ctlPtr, valuePtr ) )
        {
            LE_FATAL("Cannot select the value <%s>",valuePtr);
        }
    }
    mixer_close(mixerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get a mixer value
 *
 */
//--------------------------------------------------------------------------------------------------
void GetMixerParameter
(
    const char* namePtr,
    unsigned*   valuePtr
)
{
    struct mixer *mixerPtr;
    struct mixer_ctl *ctlPtr;

    mixerPtr = mixer_open(AUDIO_QUALCOMM_DEVICE_PATH);
    if (mixerPtr==NULL)
    {
        LE_FATAL("Cannot open <%s>",AUDIO_QUALCOMM_DEVICE_PATH);
    }

    ctlPtr = mixer_get_control(mixerPtr, namePtr, 0);
    if (ctlPtr==NULL)
    {
        LE_FATAL("Cannot get mixer controler <%s>", namePtr);
    }

    mixer_ctl_get(ctlPtr,valuePtr);

    mixer_close(mixerPtr);
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeslot number of a PCM interface.
 *
 * @return LE_FAULT         The function failed to set the timeslot number.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetPcmTimeSlot
(
    pa_audio_If_t interface,
    uint32_t      timeslot
)
{
    LE_DEBUG("Use timeslot.%d for interface.%d", interface, timeslot);

    switch (interface)
    {
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        {
            if (timeslot > 0)
            {
                LE_ERROR("Timeslot %zu is out of range (>0).", timeslot);
                return LE_FAULT;
            }
            else
            {
                return LE_OK;
            }
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the channel mode of an I2S interface.
 *
 * @return LE_FAULT         The function failed to set the channel mode.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetI2sChannelMode
(
    pa_audio_If_t           interface,
    le_audio_I2SChannel_t  mode
)
{
    LE_DEBUG("Use channel mode.%d for interface.%d", mode, interface);

    switch (interface)
    {
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        {
             return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to configure an interface as a Master.
 *
 * @return LE_FAULT         The function failed to configure the interface as a Master.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetMasterMode
(
    pa_audio_If_t interface
)
{
    LE_DEBUG("Configure interface.%d as a Master", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        {
            SetMixerParameter("AUX PCM Sync", "1");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to configure an interface as a Slave.
 *
 * @return LE_FAULT         The function failed to configure the interface as a Slave.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_SetSlaveMode
(
    pa_audio_If_t interface
)
{
    LE_DEBUG("Configure interface.%d as a Slave", interface);

    switch (interface)
    {
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_PCM_TX:
        {
            SetMixerParameter("AUX PCM Sync", "0");
            return LE_OK;
        }
        case PA_AUDIO_IF_CODEC_SPEAKER:
        case PA_AUDIO_IF_CODEC_MIC:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_USB_TX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX:
        case PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_RX:
        case PA_AUDIO_IF_DSP_FRONTEND_I2S_TX:
        case PA_AUDIO_IF_FILE_PLAYING:
        case PA_AUDIO_IF_END:
        {
            break;
        }
    }
    LE_ERROR("This interface (%d) is not supported",interface);
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to ask for a playback thread starting
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The playback format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartPlayback
(
    const char* formatPtr,      ///< [IN] Playback format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    LE_ASSERT(formatPtr);

    PlaybackIsOn = true;

    return (StartThreads(formatPtr, channelCount));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a playback thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopPlayback
(
    void
)
{
    PlaybackIsOn = false;

    if (PlaybackThreadRef)
    {
        le_thread_Cancel(PlaybackThreadRef);
        le_thread_Join(PlaybackThreadRef,NULL);
        PlaybackThreadRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to ask for a capture thread starting
 *
 * @return LE_OK            The thread is started
 * @return LE_BAD_PARAMETER The capture format is not valid
 * @return LE_DUPLICATE     The thread is already started
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_audio_StartCapture
(
    const char* formatPtr,      ///< [IN] Capture format
    uint32_t    channelCount    ///< [IN] Number of channel
)
{
    LE_ASSERT(formatPtr);

    CaptureIsOn = true;

    return (StartThreads(formatPtr, channelCount));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop a capture thread
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_audio_StopCapture
(
    void
)
{
    CaptureIsOn = false;

    if (CaptureThreadRef)
    {
        le_thread_Cancel(CaptureThreadRef);
        le_thread_Join(CaptureThreadRef,NULL);
        CaptureThreadRef = NULL;
    }
}
