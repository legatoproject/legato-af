/**
 * @file pa_media.c
 *
 * This file contains the source code of the low level Audio API for playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
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
    pa_media_AmrFormat_t            format,
    pa_media_AmrDecoderContext_t**  amrCtxPtr
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
    pa_media_AmrDecoderContext_t* amrCtxPtr,
    int32_t  fd,
    uint8_t* bufferOutPtr
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
    pa_media_AmrDecoderContext_t* amrCtxPtr
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









