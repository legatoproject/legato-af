/**
* This module implements the pa_audio's unit tests.
*
*
* Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
*
*/

#include "legato.h"

#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include <alsa-intf/alsa_audio.h>


//--------------------------------------------------------------------------------------------------
/**
 * STUB FUNCTION START
 *
 */
//--------------------------------------------------------------------------------------------------

static struct mixer     Mixer;
static struct mixer_ctl MixerCtl;
static struct pcm       Pcm;

struct mixer* mixer_open(const char* path)
{
    LE_DEBUG("mixer_open");
    return &Mixer;
}

void mixer_close(struct mixer *mixer)
{
    LE_DEBUG("mixer_close");
}

struct mixer_ctl *mixer_get_control(struct mixer *mixer,const char *name, unsigned index)
{
    LE_DEBUG("mixer_get_control");
    return &MixerCtl;
}

int mixer_ctl_set_value(struct mixer_ctl *ctl, int count, char ** argv)
{
    LE_DEBUG("mixer_ctl_set_value");
    return 0;
}

int mixer_ctl_select(struct mixer_ctl *ctl, const char *value)
{
    LE_DEBUG("mixer_ctl_select");
    return 0;
}

void mixer_ctl_get(struct mixer_ctl *ctl, unsigned *value)
{
    LE_DEBUG("mixer_ctl_get");
    *value = 0;
}

struct pcm *pcm_open(unsigned flags, char *device)
{
    LE_DEBUG("pcm_open");
    return &Pcm;
}

int pcm_ready(struct pcm *pcm)
{
    LE_DEBUG("pcm_ready");
    return 1;
}

int pcm_close(struct pcm *pcm)
{
    LE_DEBUG("pcm_close");
    return 1;
}

void param_init(struct snd_pcm_hw_params *p)
{
    LE_DEBUG("param_init");
    return;
}

void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit)
{
    LE_DEBUG("param_set_mask");
    return;
}

void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned val)
{
    LE_DEBUG("param_set_min");
    return;
}

void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned val)
{
    LE_DEBUG("param_set_int");
    return;
}

int param_set_hw_refine(struct pcm *pcm, struct snd_pcm_hw_params *params)
{
    LE_DEBUG("param_set_hw_refine");
    return 0;
}

int param_set_hw_params(struct pcm *pcm, struct snd_pcm_hw_params *params)
{
    LE_DEBUG("param_set_hw_params");
    return 0;
}

int pcm_buffer_size(struct snd_pcm_hw_params *params)
{
    return 10;
}

int pcm_period_size(struct snd_pcm_hw_params *params)
{
    return 10;
}

int param_set_sw_params(struct pcm *pcm, struct snd_pcm_sw_params *sparams)
{
    LE_DEBUG("param_set_sw_params");
    return 0;
}

int pcm_prepare(struct pcm *pcm)
{
    LE_DEBUG("pcm_prepare");
    return 0;
}

int ioControl(int d, int request, ...)
{
    LE_DEBUG("ioControl");
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * STUB FUNCTION END
 *
 */
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_SetGain(pa_audio_If_t interface,uint32_t gainInDb);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_SetGain()
{
    uint32_t gain;
    le_audio_Stream_t stream;

    for(gain=0; gain<150;gain+=5)
    {
        stream.audioInterface = LE_AUDIO_IF_CODEC_MIC;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_OK);
        stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_RX;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_OK);
        stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_TX;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_OK);
        stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_OK);
        stream.audioInterface = LE_AUDIO_NUM_INTERFACES;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_FAULT);
        stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_FAULT);
    }

    for(gain=0; gain<9;gain++)
    {
        stream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;
        LE_ASSERT(pa_audio_SetGain(&stream,gain)==LE_OK);
    }

    stream.audioInterface = LE_AUDIO_IF_CODEC_MIC;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_IF_CODEC_SPEAKER;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_RX;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_TX;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
    stream.audioInterface = LE_AUDIO_NUM_INTERFACES;
    LE_ASSERT(pa_audio_SetGain(&stream,0xFFFFFFFF)==LE_OUT_OF_RANGE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_GetGain(pa_audio_If_t interface,uint32_t *gainInDbPtr);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_GetGain()
{
    int32_t gain;
    le_audio_Stream_t stream;

    stream.audioInterface = LE_AUDIO_IF_CODEC_MIC;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_RX;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_USB_TX;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    stream.audioInterface = LE_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_PLAY;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    stream.audioInterface = LE_AUDIO_NUM_INTERFACES;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_FAULT);
    stream.audioInterface = LE_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE;
    LE_ASSERT(pa_audio_GetGain(&stream,&gain)==LE_FAULT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    Test_pa_audio_SetGain();
    Test_pa_audio_GetGain();

    exit(EXIT_SUCCESS);
}
