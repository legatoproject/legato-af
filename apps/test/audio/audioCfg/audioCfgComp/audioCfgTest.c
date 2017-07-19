/**
 * This module is for unit testing of the configuration Audio service.
 *
 * On the target, you must issue the following commands:
 * $ app runProc audioCfgTest --exe=audioCfgTest [-- <audio profile>]
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


static le_audio_StreamRef_t MdmRxAudioRef = NULL;
static le_audio_StreamRef_t MdmTxAudioRef = NULL;
static le_audio_StreamRef_t I2sRxAudioRef = NULL;
static le_audio_StreamRef_t I2sTxAudioRef = NULL;
static uint32_t             ErrorCount;
static uint32_t             AudioProfile;

static void TestAudioCfgParamCheck
(
    void
)
{
{
    le_result_t          res;

    LE_INFO("Start TestAudioCfgParamCheck.");

    // Enable Noise suppressor
    if ((res = le_audio_EnableNoiseSuppressor(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_EnableNoiseSuppressor parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableNoiseSuppressor parameter check succeed");
    }

    // Enable Echo Canceller
    if ((res = le_audio_EnableEchoCanceller(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_EnableEchoCanceller parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableEchoCanceller parameter check succeed");
    }

    // Enable FIR and IIR filters
    if ((res = le_audio_EnableFirFilter(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_EnableFirFilter parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableFirFilter parameter check succeed");
    }

    if ((res = le_audio_EnableIirFilter(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_EnableIirFilter parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableIirFilter parameter check succeed");
    }

    // Enable automatic gain control
    if ((res = le_audio_EnableAutomaticGainControl(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_EnableAutomaticGainControl parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableAutomaticGainControl parameter check succeed");
    }


    // Disable Noise suppressor
    if ((res = le_audio_DisableNoiseSuppressor(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_DisableNoiseSuppressor parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableNoiseSuppressor parameter check succeed");
    }

    // Disable Echo Canceller
    if ((res = le_audio_DisableEchoCanceller(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_DisableEchoCanceller parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableEchoCanceller parameter check succeed");
    }

    // Disable FIR and IIR filters
    if ((res = le_audio_DisableFirFilter(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_DisableFirFilter parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableFirFilter parameter check succeed");
    }

    if ((res = le_audio_DisableIirFilter(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_DisableIirFilter parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableIirFilter parameter check succeed");
    }

    // Disable automatic gain control
    if ((res = le_audio_DisableAutomaticGainControl(I2sRxAudioRef)) != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_audio_DisableAutomaticGainControl parameter check failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableAutomaticGainControl parameter check succeed");
    }

    LE_INFO("End TestAudioCfgParamCheck.");
}
}

static void TestAudioCfgEnable
(
    void
)
{
    le_result_t          res;
    uint32_t profil = 0;

    LE_INFO("Start TestAudioCfgEnable.");

    if (le_audio_SetProfile(AudioProfile) != LE_OK)
    {
        LE_ERROR("le_audio_SetProfile failed!");
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_SetProfile returns successfuly (%d)", AudioProfile);
    }

    // Get/Set profile
    if (le_audio_GetProfile(&profil) != LE_OK)
    {
        LE_ERROR("le_audio_GetProfile failed!");
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_GetProfile returns successfuly (%d)", AudioProfile);
    }

    if (profil != AudioProfile)
    {
        LE_ERROR("audio profil doesn't match!");
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_GetProfile matches with le_audio_SetProfile (%d)", profil);
    }

    if (le_audio_SetProfile(1) != LE_OK)
    {
        LE_ERROR("le_audio_SetProfile failed!");
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_SetProfile returns successfuly");
    }

    // Enable Noise suppressor
    if ((res = le_audio_EnableNoiseSuppressor(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableNoiseSuppressor failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableNoiseSuppressor returns successfuly");
    }

    // Enable Echo Canceller
    if ((res = le_audio_EnableEchoCanceller(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableEchoCanceller failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableEchoCanceller returns successfuly");
    }

    // Enable FIR and IIR filters
    if ((res = le_audio_EnableFirFilter(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableFirFilter failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableFirFilter returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_EnableFirFilter(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableFirFilter failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableFirFilter returns successfuly on Mdm Rx");
    }

    if ((res = le_audio_EnableIirFilter(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableIirFilter failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableIirFilter returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_EnableIirFilter(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableIirFilter failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableIirFilter returns successfuly on Mdm Rx");
    }

    // Enable automatic gain control
    if ((res = le_audio_EnableAutomaticGainControl(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableAutomaticGainControl failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableAutomaticGainControl returns successfuly on Mdm Rx");
    }

    if ((res = le_audio_EnableAutomaticGainControl(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableAutomaticGainControl failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableAutomaticGainControl returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_EnableAutomaticGainControl(I2sTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_EnableAutomaticGainControl failed on I2S Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_EnableAutomaticGainControl returns successfuly on I2S Tx");
    }

    LE_INFO("End TestAudioCfgEnable.");
}


static void TestAudioCfgDisable
(
    void
)
{
    le_result_t          res;

    LE_INFO("Start TestAudioCfgDisable.");

    // Enable Noise suppressor
    if ((res = le_audio_DisableNoiseSuppressor(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableNoiseSuppressor failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableNoiseSuppressor returns successfuly");
    }

    // Enable Echo Canceller
    if ((res = le_audio_DisableEchoCanceller(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableEchoCanceller failed! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableEchoCanceller returns successfuly");
    }

    // Enable FIR and IIR filters
    if ((res = le_audio_DisableFirFilter(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableFirFilter failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableFirFilter returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_DisableFirFilter(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableFirFilter failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableFirFilter returns successfuly on Mdm Rx");
    }

    if ((res = le_audio_DisableIirFilter(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableIirFilter failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableIirFilter returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_DisableIirFilter(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableIirFilter failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableIirFilter returns successfuly on Mdm Rx");
    }

    // Enable automatic gain control
    if ((res = le_audio_DisableAutomaticGainControl(MdmRxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableAutomaticGainControl failed on Mdm Rx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableAutomaticGainControl returns successfuly on Mdm Rx");
    }

    if ((res = le_audio_DisableAutomaticGainControl(MdmTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableAutomaticGainControl failed on Mdm Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableAutomaticGainControl returns successfuly on Mdm Tx");
    }

    if ((res = le_audio_DisableAutomaticGainControl(I2sTxAudioRef)) != LE_OK)
    {
        LE_ERROR("le_audio_DisableAutomaticGainControl failed on I2S Tx! (res.%d)", res);
        ErrorCount++;
    }
    else
    {
        LE_INFO("le_audio_DisableAutomaticGainControl returns successfuly on I2S Tx");
    }

    LE_INFO("End TestAudioCfgDisable.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    if (le_arg_NumArgs() == 1)
    {
        const char* audioProfileStr = le_arg_GetArg(0);
        if (NULL == audioProfileStr)
        {
            LE_ERROR("audioProfileStr is NULL");
            exit(1);
        }
        int value = atoi(audioProfileStr);
        if (value < 0)
        {
            AudioProfile = 1;
        }
        else
        {
            AudioProfile = value;
        }
    }
    else
    {
        AudioProfile = 1;
    }

    LE_INFO("Start AudioConfiguration Test audio profile %d!", AudioProfile);

    ErrorCount = 0;

    MdmRxAudioRef = le_audio_OpenModemVoiceRx();
    LE_FATAL_IF((MdmRxAudioRef==NULL), "le_audio_OpenModemVoiceRx returns NULL!");
    MdmTxAudioRef = le_audio_OpenModemVoiceTx();
    LE_FATAL_IF((MdmTxAudioRef==NULL), "le_audio_OpenModemVoiceTx returns NULL!");
    I2sRxAudioRef = le_audio_OpenI2sRx(LE_AUDIO_I2S_STEREO);
    LE_FATAL_IF((I2sRxAudioRef==NULL), "le_audio_OpenI2sRx returns NULL!");
    I2sTxAudioRef = le_audio_OpenI2sTx(LE_AUDIO_I2S_STEREO);
    LE_FATAL_IF((I2sTxAudioRef==NULL), "le_audio_OpenI2sTx returns NULL!");

    TestAudioCfgParamCheck();
    TestAudioCfgEnable();
    TestAudioCfgDisable();

    if (ErrorCount == 0)
    {
        LE_INFO("AudioConfiguration test succeed.");
        exit(0);
    }
    else
    {
        LE_ERROR("AudioConfiguration test failed: found %d failures, check the logs!", ErrorCount);
        exit(1);
    }
}

