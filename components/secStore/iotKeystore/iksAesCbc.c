/** @file iksAesCbc.c
 *
 * Legato IoT Keystore APIs for performing AES encryption in CBC mode.
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
 * Starts a process to encrypt a message with AES in CBC mode.  Calling this function will cancel
 * any previously started process using the same session.
 *
 * To encrypt a message the following sequence should be used:
 *
 * le_iks_aesCbc_StartEncrypt() // Start the encryption process.
 * le_iks_aesCbc_Encrypt()      // Call zero or more times until all plaintext is encrypted.
 *
 * The initialization vector, IV, does not need to be kept secret but must be unpredictable.  Thus
 * the IV must be generated from a well seeded CPRNG each time this function is called.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if ivPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCbc_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* ivPtr,       ///< [IN] Initialization vector.
    size_t ivSize               ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
)
{
    return pa_iks_aesCbc_StartEncrypt(session, ivPtr, ivSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext. le_iks_aesCbc_StartEncrypt() must have been previously called. The
 * plaintest must be a multiple of the block size.  It is up to the caller to pad the plaintext as
 * needed.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an encryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCbc_Encrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* plaintextChunkPtr,   ///< [IN] Plaintext chunk.
    size_t plaintextChunkSize,          ///< [IN] Plaintext chunk size.
                                        ///<      Must be <= LE_IKS_MAX_PACKET_SIZE and
                                        ///<      a multiple of LE_IKS_AES_BLOCK_SIZE.
    uint8_t* ciphertextChunkPtr,        ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextChunkSizePtr      ///< [INOUT] Ciphertext chunk size.
                                        ///<         Must be >= plaintextChunkSize.

)
{
    return pa_iks_aesCbc_Encrypt(session, plaintextChunkPtr, plaintextChunkSize,
                                 ciphertextChunkPtr, ciphertextChunkSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt a message with AES in CBC mode.  Calling this function will cancel
 * any previously started process using the same session.
 *
 * To decrypt a message the following sequence should be used:
 *
 * le_iks_aesCbc_StartDecrypt() // Start the decryption process.
 * le_iks_aesCbc_Decrypt()      // Call zero or more times until all ciphertext is decrypted.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if ivPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCbc_StartDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* ivPtr,   ///< [IN] Initialization vector.
    size_t ivSize           ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
)
{
    return pa_iks_aesCbc_StartDecrypt(session, ivPtr, ivSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext. le_iks_aesCbc_StartDecrypt() must have been previously called to
 * start a decryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid.
 *                       or if the key type is invalid.
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if a decryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesCbc_Decrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* ciphertextChunkPtr,  ///< [IN] Ciphertext chunk.
    size_t ciphertextChunkSize,         ///< [IN] Ciphertext chunk size.
                                        ///<      Must be <= LE_IKS_MAX_PACKET_SIZE and
                                        ///<      a multiple of LE_IKS_AES_BLOCK_SIZE.
    uint8_t* plaintextChunkPtr,         ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextChunkSizePtr       ///< [INOUT] Plaintext buffer size.
                                        ///<         Must be >= ciphertextChunkSize.
)
{
    return pa_iks_aesCbc_Decrypt(session, ciphertextChunkPtr, ciphertextChunkSize,
                                 plaintextChunkPtr, plaintextChunkSizePtr);
}
