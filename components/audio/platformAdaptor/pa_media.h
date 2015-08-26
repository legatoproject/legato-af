/** @file pa_media.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAMEDIA_INCLUDE_GUARD
#define LEGATO_PAMEDIA_INCLUDE_GUARD

#include "pa_audio.h"
#include "le_media_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * The PA AMR decoder context structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_media_Format_t           format;     ///< Format of the AMR file
    int16_t                     bufferSize; ///< Size of the required buffer
    void*                       amrHandle;  ///< AMR decoder handle
}
pa_media_AmrDecoderContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * The PA AMR encoder context structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_media_Format_t            format;                ///< Encoding format
    pa_audio_SampleAmrConfig_t*  sampleAmrConfigPtr;    ///< Sample AMR configuration
    void *                       amrHandle;             ///< AMR encoder handle
    uint32_t                     mode;                  ///< Encoding AMR mode (bitrate enum)
    int16_t                     bufferSize;             ///< Size of the required buffer
}
pa_media_AmrEncoderContext_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialization of AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_InitAmrDecoder
(
    le_media_Format_t               format,     ///< [IN] Decoding format
    pa_media_AmrDecoderContext_t**  amrCtxPtr   ///< [OUT] AMR decoder context
);

//--------------------------------------------------------------------------------------------------
/**
 *Decode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_DecodeAmrFrames
(
    pa_media_AmrDecoderContext_t* amrCtxPtr, ///< [IN] AMR decoder context
    int32_t  fd,                             ///< [IN] file descriptor input
    uint8_t* bufferOutPtr                    ///< [OUT] Decoding samples buffer output
);

//--------------------------------------------------------------------------------------------------
/**
 * Release AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_ReleaseAmrDecoder
(
    pa_media_AmrDecoderContext_t* amrCtxPtr ///< [IN] AMR decoder context
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialization of AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_InitAmrEncoder
(
    pa_audio_SampleAmrConfig_t*  sampleAmrConfigPtr,    ///< [IN] Sample AMR configuration
    pa_media_AmrEncoderContext_t**  amrCtxPtr           ///< [OUT] AMR encoder context
);

//--------------------------------------------------------------------------------------------------
/**
 * Encode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_EncodeAmrFrames
(
    pa_media_AmrEncoderContext_t* amrCtxPtr, ///< [IN] AMR encoder context
    uint8_t* inputDataPtr,                   ///< [IN] input AMR samples buffer
    uint32_t inputDataLen,                   ///< [IN] input buffer length
    uint8_t* outputDataPtr,                  ///< [OUT] output PCM samples buffer
    uint32_t* outputDataLen                  ///< [OUT] output PCM buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Release AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_media_ReleaseAmrEncoder
(
    pa_media_AmrEncoderContext_t* amrCtxPtr ///< [IN] AMR encoder context
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the PA media service.
 *
 * @note This is not supposed to be called from outside the audio PA.  The Audio PA will call it.
 *
 * @todo Move this to a different (internal) header file.
 */
//--------------------------------------------------------------------------------------------------
void pa_media_Init
(
    void
);

#endif // LEGATO_PAMEDIA_INCLUDE_GUARD
