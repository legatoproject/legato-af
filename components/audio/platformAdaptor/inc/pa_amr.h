/** @file pa_amr.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PAAMR_INCLUDE_GUARD
#define LEGATO_PAAMR_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_amr_StartDecoder
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
);

//--------------------------------------------------------------------------------------------------
/**
 * Decode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_amr_DecodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,   ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr,  ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr     ///< [OUT] Length of the read data
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_amr_StopDecoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
);

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_amr_StartEncoder
(
    le_audio_Stream_t*               streamPtr,      ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr     ///< [IN] Media thread context
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
LE_SHARED le_result_t pa_amr_EncodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,///< [IN] Media thread context
    uint8_t* inputDataPtr,                     ///< [IN] input AMR samples buffer
    uint32_t inputDataLen,                     ///< [IN] input buffer length
    uint8_t* outputDataPtr,                    ///< [OUT] output PCM samples buffer
    uint32_t* outputDataLen                    ///< [OUT] output PCM buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_amr_StopEncoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
);


#endif // LEGATO_PAAMR_INCLUDE_GUARD
