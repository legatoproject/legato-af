/**
* This module implements the pa_audio's unit tests.
*
*
* Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
*
*/

#include "legato.h"

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

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
 * le_result_t pa_audio_EnableCodecInput(pa_audio_If_t interface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_EnableCodecInput()
{
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_CODEC_MIC),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_CODEC_SPEAKER),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_DSP_FRONTEND_USB_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_DSP_FRONTEND_USB_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_FILE_PLAYING),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecInput(PA_AUDIO_IF_END),LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_DisableCodecInput(pa_audio_If_t interface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_DisableCodecInput()
{
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_CODEC_MIC),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_CODEC_SPEAKER),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_DSP_FRONTEND_USB_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_DSP_FRONTEND_USB_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_FILE_PLAYING),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecInput(PA_AUDIO_IF_END),LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_EnableCodecOutput(pa_audio_If_t interface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_EnableCodecOutput()
{
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_CODEC_MIC),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_CODEC_SPEAKER),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_DSP_FRONTEND_USB_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_DSP_FRONTEND_USB_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_FILE_PLAYING),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_EnableCodecOutput(PA_AUDIO_IF_END),LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_DisableCodecOutput(pa_audio_If_t interface);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_DisableCodecOutput()
{
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_CODEC_MIC),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_CODEC_SPEAKER),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_DSP_FRONTEND_USB_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_DSP_FRONTEND_USB_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_FILE_PLAYING),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_DisableCodecOutput(PA_AUDIO_IF_END),LE_FAULT);
}

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

    for(idxInput=0;idxInput<PA_AUDIO_IF_END+1;idxInput++)
    {
        for(idxOutput=0;idxOutput<PA_AUDIO_IF_END+1;idxOutput++)
        {
            res = pa_audio_SetDspAudioPath(idxInput,idxOutput);

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
                ||
                ((idxInput==PA_AUDIO_IF_FILE_PLAYING) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_USB_TX))
                ||
                ((idxInput==PA_AUDIO_IF_FILE_PLAYING) && (idxOutput == PA_AUDIO_IF_CODEC_SPEAKER))
            )
            {
                CU_ASSERT_EQUAL(res,LE_OK);
            }
            else
            {
                CU_ASSERT_EQUAL(res,LE_FAULT);
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

    for(idxInput=0;idxInput<PA_AUDIO_IF_END+1;idxInput++)
    {
        for(idxOutput=0;idxOutput<PA_AUDIO_IF_END+1;idxOutput++)
        {
            res = pa_audio_ResetDspAudioPath(idxInput,idxOutput);

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
                ||
                ((idxInput==PA_AUDIO_IF_FILE_PLAYING) && (idxOutput == PA_AUDIO_IF_DSP_FRONTEND_USB_TX))
                ||
                ((idxInput==PA_AUDIO_IF_FILE_PLAYING) && (idxOutput == PA_AUDIO_IF_CODEC_SPEAKER))
            )
            {
                CU_ASSERT_EQUAL(res,LE_OK);
            }
            else
            {
                CU_ASSERT_EQUAL(res,LE_FAULT);
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
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_CODEC_MIC,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_CODEC_SPEAKER,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_FILE_PLAYING,gain),LE_OUT_OF_RANGE);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_END,gain),LE_OUT_OF_RANGE);
        } else {
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_CODEC_MIC,gain),LE_OK);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_CODEC_SPEAKER,gain),LE_OK);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,gain),LE_FAULT);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,gain),LE_FAULT);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,gain),LE_FAULT);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,gain),LE_FAULT);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_FILE_PLAYING,gain),LE_FAULT);
            CU_ASSERT_EQUAL(pa_audio_SetGain(PA_AUDIO_IF_END,gain),LE_FAULT);
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

    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_CODEC_MIC,&gain),LE_OK);
    CU_ASSERT_EQUAL(gain,0);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_CODEC_SPEAKER,&gain),LE_OK);
    CU_ASSERT_EQUAL(gain,0);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_RX,&gain),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_DSP_FRONTEND_USB_TX,&gain),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_RX,&gain),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_DSP_BACKEND_MODEM_VOICE_TX,&gain),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_FILE_PLAYING,&gain),LE_FAULT);
    CU_ASSERT_EQUAL(pa_audio_GetGain(PA_AUDIO_IF_END,&gain),LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_StartPlayback(const char* formatPtr,uint32_t channelCount);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_StartPlayback()
{
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("",0),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("",1),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("",2),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("L16-16K",1),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("L16-8K",1),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_StartPlayback("L16-8K",1),LE_DUPLICATE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * void pa_audio_StopPlayback(void);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_StopPlayback()
{
    pa_audio_StopPlayback();
    pa_audio_StopPlayback();
    CU_PASS("Test_pa_audio_StopPlayback");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * le_result_t pa_audio_StartCapture(const char* formatPtr,uint32_t channelCount);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_StartCapture()
{
    CU_ASSERT_EQUAL(pa_audio_StartCapture("",0),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartCapture("",1),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartCapture("",2),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartCapture("L16-16K",1),LE_BAD_PARAMETER);
    CU_ASSERT_EQUAL(pa_audio_StartCapture("L16-8K",1),LE_OK);
    CU_ASSERT_EQUAL(pa_audio_StartCapture("L16-8K",1),LE_DUPLICATE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *
 * void pa_audio_StopCapture(void);
 *
 */
//--------------------------------------------------------------------------------------------------
void Test_pa_audio_StopCapture()
{
    pa_audio_StopCapture();
    pa_audio_StopCapture();
    CU_PASS("Test_pa_audio_StopCapture");
}



//--------------------------------------------------------------------------------------------------
/**
 * Main.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* test(void* context)
{
    // Init the test case / test suite data structures
    CU_TestInfo audiotest[] =
    {
        { "Test pa_audio_EnableCodecInput()"    , Test_pa_audio_EnableCodecInput },
        { "Test pa_audio_DisableCodecInput()"   , Test_pa_audio_DisableCodecInput },
        { "Test pa_audio_EnableCodecOutput()"   , Test_pa_audio_EnableCodecOutput },
        { "Test pa_audio_DisableCodecOutput()"  , Test_pa_audio_DisableCodecOutput },
        { "Test pa_audio_SetDspAudioPath()"     , Test_pa_audio_SetDspAudioPath },
        { "Test pa_audio_ResetDspAudioPath()"   , Test_pa_audio_ResetDspAudioPath },
        { "Test pa_audio_SetGain()"             , Test_pa_audio_SetGain },
        { "Test pa_audio_GetGain()"             , Test_pa_audio_GetGain },
        { "Test pa_audio_StartPlayback()"       , Test_pa_audio_StartPlayback },
        { "Test pa_audio_StopPlayback()"        , Test_pa_audio_StopPlayback },
        { "Test pa_audio_StartCapture()"        , Test_pa_audio_StartCapture },
        { "Test pa_audio_StopCapture()"         , Test_pa_audio_StopCapture },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "PA Audio tests",                NULL, NULL, audiotest },
        CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

static void init()
{
    pa_audio_Init();

    test(NULL);

}

LE_EVENT_INIT_HANDLER
{
    init();
}
