/**
 * @file pa_amr.c
 *
 * This file contains the source code of the low level Audio API for AMR playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include "pa_amr.h"

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StartDecoder
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
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
le_result_t pa_amr_DecodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,    ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr,   ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr      ///< [OUT] Length of the read data
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StopDecoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StartEncoder
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
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
le_result_t pa_amr_EncodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,///< [IN] Media thread context
    uint8_t* inputDataPtr,                     ///< [IN] input AMR samples buffer
    uint32_t inputDataLen,                     ///< [IN] input buffer length
    uint8_t* outputDataPtr,                    ///< [OUT] output PCM samples buffer
    uint32_t* outputDataLen                    ///< [OUT] output PCM buffer length
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StopEncoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
)
{
    return LE_FAULT;
}
