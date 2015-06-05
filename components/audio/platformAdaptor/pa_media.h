/** @file pa_media.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PAMEDIA_INCLUDE_GUARD
#define LEGATO_PAMEDIA_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Type for AMR handle decoder
 */
//--------------------------------------------------------------------------------------------------
typedef void* pa_media_AmrHandle_t;

//--------------------------------------------------------------------------------------------------
/**
 * The enumeration of AMR formats
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PA_MEDIA_AMR_WB,
    PA_MEDIA_AMR_NB,
    PA_MEDIA_AMR_MAX
}
pa_media_AmrFormat_t;

//--------------------------------------------------------------------------------------------------
/**
 * The PA AMR decoder context structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    pa_media_AmrFormat_t        format;     // Format of the AMR file
    int16_t                     bufferSize; // Size of the required buffer
    pa_media_AmrHandle_t        amrHandle;  // AMR decoder handle
}
pa_media_AmrDecoderContext_t;

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
le_result_t pa_media_DecodeAmrFrames
(
    pa_media_AmrDecoderContext_t* amrCtxPtr,
    int32_t  fd,
    uint8_t* bufferOutPtr
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
le_result_t pa_media_ReleaseAmrDecoder
(
    pa_media_AmrDecoderContext_t* amrCtxPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the PA media service.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_media_Init
(
    void
);

#endif // LEGATO_PAMEDIA_INCLUDE_GUARD
