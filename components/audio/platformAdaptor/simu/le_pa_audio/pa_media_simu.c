/**
 * @file pa_media.c
 *
 * This file contains the source code of the low level Audio API for playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_audio.h"
#include "pa_media.h"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialization of AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_InitAmrDecoder
(
    le_media_Format_t               format,     ///< [IN] Decoding format
    pa_media_AmrDecoderContext_t**  amrCtxPtr   ///< [OUT] AMR decoder context
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 *Decode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_DecodeAmrFrames
(
    pa_media_AmrDecoderContext_t* amrCtxPtr, ///< [IN] AMR decoder context
    int32_t  fd,                             ///< [IN] file descriptor input
    uint8_t* bufferOutPtr                    ///< [OUT] Decoding samples buffer output
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_ReleaseAmrDecoder
(
    pa_media_AmrDecoderContext_t* amrCtxPtr ///< [IN] AMR decoder context
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization of AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_InitAmrEncoder
(
    pa_audio_SampleAmrConfig_t*  sampleAmrConfigPtr,    ///< [IN] Sample AMR configuration
    pa_media_AmrEncoderContext_t**  amrCtxPtr           ///< [OUT] AMR encoder context
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_EncodeAmrFrames
(
    pa_media_AmrEncoderContext_t* amrCtxPtr, ///< [IN] AMR encoder context
    uint8_t* inputDataPtr,                   ///< [IN] input AMR samples buffer
    uint32_t inputDataLen,                   ///< [IN] input buffer length
    uint8_t* outputDataPtr,                  ///< [OUT] output PCM samples buffer
    uint32_t* outputDataLen                  ///< [OUT] output PCM buffer length
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_media_ReleaseAmrEncoder
(
    pa_media_AmrEncoderContext_t* amrCtxPtr ///< [IN] AMR encoder context
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the PA media service.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_media_Init
(
    void
)
{
    return;
}
