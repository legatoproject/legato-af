/**
 * @file pa_amr_default.c
 *
 * Default implementation of the AMR interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_amr.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.  Called automatically by the application framework at process start.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_DecodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,   ///< [IN] Media thread context
    uint8_t*                       bufferOutPtr,  ///< [OUT] Decoding samples buffer output
    uint32_t*                      readLenPtr     ///< [OUT] Length of the read data
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
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
    le_audio_Stream_t*               streamPtr,      ///< [IN] Stream object
    le_audio_MediaThreadContext_t*   mediaCtxPtr     ///< [IN] Media thread context
)
{
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
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
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
