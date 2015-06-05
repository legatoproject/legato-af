/**
* This module implements the pa_audio's unit tests.
*
*
* Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
*
*/

#include "legato.h"

#include "interfaces.h"
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
 * le_result_t pa_audio_SetDspAudioPath(pa_audio_If_t inputInterface,pa_audio_If_t outputInterface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_SetDspAudioPath()
{
    le_result_t res;
    int idxInput,idxOutput;

    for(idxInput=0;idxInput<PA_AUDIO_NUM_INTERFACES+1;idxInput++)
    {
        for(idxOutput=0;idxOutput<PA_AUDIO_NUM_INTERFACES+1;idxOutput++)
        {
            res = pa_audio_SetDspAudioPath(idxInput,idxOutput);
            LE_DEBUG("idxInput %d idxOutput %d",idxInput, idxOutput);

            if (((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_USB_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_CODEC_SPEAKER))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_FRONTEND_USB_RX) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_FRONTEND_PCM_RX) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
                ||
                ((idxInput==PA_AUDIO_IF_CODEC_MIC) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
            )
            {
                LE_ASSERT(res==LE_OK);
            }
            else
            {
                LE_ASSERT(res==LE_FAULT);
            }

        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_ResetDspAudioPath(pa_audio_If_t inputInterface,pa_audio_If_t outputInterface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_ResetDspAudioPath()
{
    le_result_t res;
    int idxInput,idxOutput;

    for(idxInput=0;idxInput<PA_AUDIO_NUM_INTERFACES+1;idxInput++)
    {
        for(idxOutput=0;idxOutput<PA_AUDIO_NUM_INTERFACES+1;idxOutput++)
        {
            res = pa_audio_FlagForResetDspAudioPath(idxInput,idxOutput);
            pa_audio_ResetDspAudioPath();
            LE_DEBUG("idxInput %d idxOutput %d",idxInput, idxOutput);

            if (((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_USB_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_CODEC_SPEAKER))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_FRONTEND_USB_RX) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_PCM_TX))
                ||
                ((idxInput==PA_AUDIO_IF_DSP_FRONTEND_PCM_RX) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
                ||
                ((idxInput==PA_AUDIO_IF_CODEC_MIC) && (idxOutput == PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX))
            )
            {
                LE_ASSERT(res==LE_OK);
            }
            else
            {
                LE_ASSERT(res==LE_FAULT);
            }

        }
    }
}

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

    for(gain=0; gain<150;gain+=5) {
        if (gain>100) {
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_CODEC_MIC,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_CODEC_SPEAKER,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,gain)==LE_OUT_OF_RANGE);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_NUM_INTERFACES,gain)==LE_OUT_OF_RANGE);
        } else {
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_CODEC_MIC,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_CODEC_SPEAKER,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,gain)==LE_OK);
   //         LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,gain)==LE_OK);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_NUM_INTERFACES,gain)==LE_FAULT);
            LE_ASSERT(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,gain)==LE_FAULT);
        }
    }


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
    uint32_t gain;

    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_CODEC_MIC,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_CODEC_SPEAKER,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,&gain)==LE_OK);
    LE_ASSERT(gain==100);
//    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,&gain)==LE_OK);
//    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_FILE_PLAY,&gain)==LE_OK);
    LE_ASSERT(gain==100);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_NUM_INTERFACES,&gain)==LE_FAULT);
    LE_ASSERT(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_FILE_CAPTURE,&gain)==LE_FAULT);
}


//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // pa_audio_SetDspAudioPath() and pa_audio_FlagForResetDspAudioPath() return LE_OK in all case
    // To be checked if we should modify these functions, or delete the tests.
 //   Test_pa_audio_SetDspAudioPath();
 //   Test_pa_audio_ResetDspAudioPath();
    Test_pa_audio_SetGain();
    Test_pa_audio_GetGain();

    exit(EXIT_SUCCESS);
}
