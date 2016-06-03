/**
 * @file pa_pcm_simu.c
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

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static uint8_t* DataPtr = NULL;
static uint32_t DataLen=0;
static uint32_t DataIndex=0;
static intptr_t PcmHandle = 0xBADCAFE;
static le_sem_Ref_t*    RecSemaphorePtr = NULL;

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set the semaphore to unlock the test thread.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_pcmSimu_SetSemaphore
(
    le_sem_Ref_t*    semaphorePtr
)
{
    RecSemaphorePtr = semaphorePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init the data buffer with the correct size.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_pcmSimu_InitData
(
    uint32_t len
)
{
    DataPtr = malloc(len);
    memset(DataPtr,0,len);
    DataLen = len;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release the data buffer.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_pcmSimu_ReleaseData
(
    void
)
{
    free(DataPtr);
    DataLen = 0;
    DataIndex = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the data buffer.
 *
 */
//--------------------------------------------------------------------------------------------------
uint8_t* pa_pcmSimu_GetDataPtr
(
    void
)
{
    return DataPtr;
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
    LE_ASSERT(pcmHandle == (pcm_Handle_t) PcmHandle);

    if (DataPtr && DataLen && ((DataIndex + bufsize) <= DataLen))
    {
        memcpy(DataPtr + DataIndex, data, bufsize );
        DataIndex += bufsize;
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
    LE_ASSERT(pcmHandle == (pcm_Handle_t) PcmHandle);

    if (DataPtr && DataLen && data && bufsize)
    {
        uint32_t len = (DataLen - DataIndex) >= bufsize ? bufsize : DataLen - DataIndex;

        memcpy(data, DataPtr+DataIndex, len);
        DataIndex += len;

        if (len < bufsize)
        {
            memcpy(data, DataPtr, bufsize-len);
            DataIndex = bufsize-len;
        }

        if ( DataIndex == DataLen )
        {
            DataIndex = 0;

            if (RecSemaphorePtr != NULL)
            {
                le_sem_Post(*RecSemaphorePtr);
                RecSemaphorePtr = NULL;
            }
        }

        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
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
    return 10;
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
    *pcmHandlePtr = (pcm_Handle_t) PcmHandle;
    return LE_OK;
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
    *pcmHandlePtr = (pcm_Handle_t) PcmHandle;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the pa_pcm simu.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_pcmSimu_Init
(
    void
)
{
    return;
}
