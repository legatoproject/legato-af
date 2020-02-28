/** @file iksAesCmac.c
 *
 * Legato IoT Keystore APIs for generating and verifying message authentication codes
 * using AES CMAC.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_iotKeystore.h"


//--------------------------------------------------------------------------------------------------
/**
 * Process message chunks.  This function may be called multiple times to process the entire
 * message but once a message has been completely processed and le_iks_aesCmac_Done() or
 * le_iks_aesCmac_Verify() has been called this function should not be called again with the same
 * session.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if msgChunkPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if no more messages can be processed, ie. le_iks_aesCmac_Done() or
 *               le_iks_aesCmac_Verify() has already been called,
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCmac_ProcessChunk
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* msgChunkPtr, ///< [IN] Message chunk.
    size_t msgChunkSize         ///< [IN] Message chunk size. Must be <= LE_IKS_MAX_PACKET_SIZE.
)
{
    return pa_iks_aesCmac_ProcessChunk(session, msgChunkPtr, msgChunkSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and get the processed message's authentication tag.
 *
 * The maximum size of the authentication tag is LE_IKS_AESCMAC_MAX_TAG_SIZE. If the supplied buffer
 * is smaller than the maximum tag size then the tag will be truncated.  However, all tags produced
 * using the same key must use the same tag size.  It is up to the caller to ensure this.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if bufPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if no message was processed or le_iks_aesCmac_Done() or
 *               le_iks_aesCmac_Verify() has already been called,
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCmac_Done
(
    uint64_t session,       ///< [IN] Session reference.
    uint8_t* tagBufPtr,     ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagBufSizePtr   ///< [INOUT] Authentication tag buffer size.
)
{
    return pa_iks_aesCmac_Done(session, tagBufPtr, tagBufSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and compare the resulting authentication tag with the supplied tag.
 *
 * The maximum size of the authentication tag is LE_IKS_AESCMAC_MAX_TAG_SIZE. If the supplied tag is
 * smaller than the maximum tag size then only the first tagSize bytes of the tag is compared.
 * However, all tags produced using the same key must use the same tag size.  It is up to the caller
 * to ensure this.
 *
 * @return
 *      LE_OK if the specified tag matches the calculated message tag.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if tagPtr is NULL or tagSize is zero.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the specified tag does not match the calculated message tag,
 *               or if no message was processed or le_iks_aesCmac_Done() or
 *               or le_iks_aesCmac_Verify() has already been called,
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCmac_Verify
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* tagBufPtr,   ///< [IN] Authentication tag to check against.
    size_t tagBufSize           ///< [IN] Authentication tag size.
)
{
    return pa_iks_aesCmac_Verify(session, tagBufPtr, tagBufSize);
}
