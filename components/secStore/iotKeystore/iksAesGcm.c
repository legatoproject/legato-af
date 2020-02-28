/** @file iksAesGcm.c
 *
 * Legato API for IoT KeyStore's AES GCM routines.
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
 * Encrypt and integrity protect a packet with AES in GCM mode.
 *
 * GCM is an AEAD (Authenticated Encryption with Associated Data) which means that it provides both
 * confidentiality and integrity protection for plaintext data and provides integrity protection for
 * associated data.  The associated data, also referred to as Additional Authenticated Data (AAD),
 * is not encrypted but is integrity protected.  The output of the encryption is a randomly chosen
 * nonce, the ciphertext corresponding to the plaintext and an authentication tag.  The
 * authentication tag integrity protects the nonce, AAD and the ciphertext.
 *
 * ______________________
 * |   AAD, plaintext   |
 * ----------------------
 *           |
 *           V
 * ______________________________
 * |   nonce, ciphertext, tag   |
 * ------------------------------
 *
 * This is especially useful in communication protocols where a packet's payload needs to be secret
 * but the packet's header must be readable.  In this case the packet's header is the AAD.
 *
 * The AAD and plaintext are optional but they cannot both be omitted.  If the AAD is omitted then
 * confidentiality and integrity is provided for just the plaintext.  If the plaintext is omitted
 * then integrity protection is provided for just the AAD.
 *
 * The ciphertext size is the same as the plaintext size and it is assumed that the ciphertextPtr
 * buffer is at least plaintextSize bytes long.
 *
 * The tag size is always LE_IKS_AES_GCM_TAG_SIZE bytes and it is assumed that the tagPtr buffer is
 * large enough to hold the tag.
 *
 * A random nonce is chosen for each invocation of this function.  The nonce is passed out to the
 * caller via noncePtr and is assumed to always be LE_IKS_AES_GCM_NONCE_SIZE bytes.
 * The nonce does not need to be kept secret and can be passed in the clear.
 *
 * Nonce values must be unique for each invocation for the lifetime of the key.  In other words a
 * (key, nonce) pair must be unique for every invocation for all time and for all users in the
 * world.  This is a critical security requirement but can be difficult to satisfy that is why
 * keys should be rotated frequently.
 *
 * Repeated nonces in GCM are particularly problematic as they can be used to recover the integrity
 * key.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OUT_OF_RANGE if either the  aadSize or plaintextSize is the wrong size.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if either noncePtr, tagPtr, plaintextPtr or ciphertextPtr is NULL when
 *                        they shouldn't be.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_EncryptPacket
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    uint8_t*        noncePtr,           ///< [OUT] Buffer to hold the nonce.
    size_t*         nonceSizePtr,       ///< [INOUT] Nonce size.
                                        ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
    const uint8_t*  aadPtr,             ///< [IN] Additional authenticated data (AAD).
    size_t          aadSize,            ///< [IN] AAD size.
    const uint8_t*  plaintextPtr,       ///< [IN] Plaintext. NULL if not used.
    size_t          plaintextSize,      ///< [IN] Plaintext size.
    uint8_t*        ciphertextPtr,      ///< [OUT] Buffer to hold the ciphertext.
    size_t*         ciphertextSizePtr,  ///< [INOUT] Ciphertext size.
                                        ///<         Must be >= plaintextSize.
    uint8_t*        tagPtr,             ///< [OUT] Buffer to hold the authentication tag.
    size_t*         tagSizePtr          ///< [INOUT] Authentication tag size.
                                        ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return pa_iks_aesGcm_EncryptPacket(keyRef, noncePtr, nonceSizePtr, aadPtr, aadSize,
                                       plaintextPtr, plaintextSize,
                                       ciphertextPtr, ciphertextSizePtr,
                                       tagPtr, tagSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt and verify the integrity of a packet with AES in GCM mode.
 *
 * This function performs an integrity check of the AAD and the ciphertext and if the integrity
 * passes provides the decrypted plaintext.
 *
 * The plaintext size is the same as the ciphertext size and it is assumed that the plaintextPtr
 * buffer is at least ciphertextSize bytes long.
 *
 * The nonce, AAD, ciphertext and tag must be the values produced during encryption.
 *
 * ___________________________________
 * |   nonce, AAD, ciphertext, tag   |
 * -----------------------------------
 *                  |
 *                  V
 *         _________________
 *         |   plaintext   |
 *         -----------------
 *
 * @return
 *      LE_OK if successful.
 *      LE_OUT_OF_RANGE if either the aadSize or ciphertextSize is too large.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if either noncePtr, tagPtr, plaintextPtr or ciphertextPtr is NULL when
 *                        they shouldn't be.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the integrity check failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_DecryptPacket
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    const uint8_t*  noncePtr,           ///< [IN] Nonce used to encrypt the packet.
    size_t          nonceSize,          ///< [IN] Nonce size.
                                        ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
    const uint8_t*  aadPtr,             ///< [IN] Additional authenticated data (AAD).
    size_t          aadSize,            ///< [IN] AAD size.
    const uint8_t*  ciphertextPtr,      ///< [IN] Ciphertext. NULL if not used.
    size_t          ciphertextSize,     ///< [IN] Ciphertext size.
    uint8_t*        plaintextPtr,       ///< [OUT] Buffer to hold the plaintext.
    size_t*         plaintextSizePtr,   ///< [INOUT] Plaintext size.
                                        ///<         Must be >= ciphertextSize.
    const uint8_t*  tagPtr,             ///< [IN] Buffer to hold the authentication tag.
    size_t          tagSize             ///< [IN] Authentication tag size.
                                        ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return pa_iks_aesGcm_DecryptPacket(keyRef, noncePtr, nonceSize, aadPtr, aadSize,
                                       ciphertextPtr, ciphertextSize,
                                       plaintextPtr, plaintextSizePtr,
                                       tagPtr, tagSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to encrypt and integrity protect a long packet with AES in GCM mode.  This
 * function is useful for encrypting and integrity protecting packets that are larger than
 * LE_IKS_MAX_PACKET_SIZE.  Calling this function will cancel any previously started process using
 * the same session.
 *
 * To encrypt a long packet the following sequence should be used:
 *
 * le_iks_aesGcm_StartEncrypt() // Start the encryption process.
 * le_iks_aesGcm_ProcessAad()   // Call zero or more times until all AAD data is processed.
 * le_iks_aesGcm_Encrypt()      // Call zero or more times until all plaintext is encrypted.
 * le_iks_aesGcm_DoneEncrypt()  // Complete process and obtain authentication tag.
 *
 * All AAD must be processed before plaintext processing begins.
 *
 * A random nonce is chosen for each invocation of this function.  The nonce is passed out to the
 * caller via noncePtr and is assumed to always be LE_IKS_AES_GCM_NONCE_SIZE bytes.
 * The nonce does not need to be kept secret and can be passed in the clear.
 *
 * Nonce values must be unique for each invocation for the lifetime of the key.  In other words a
 * (key, nonce) pair must be unique for every invocation for all time and for all users in the
 * world.  This is a critical security requirement but can be difficult to satisfy.  Therefore keys
 * should be rotated frequently.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if noncePtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_StartEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    noncePtr,       ///< [OUT] Buffer to hold the nonce.  Assumed to be
                                ///<       LE_IKS_AES_GCM_NONCE_SIZE bytes.
    size_t*     nonceSizePtr    ///< [INOUT] Nonce size.
                                ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
)
{
    return pa_iks_aesGcm_StartEncrypt(session, noncePtr, nonceSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a chunk of AAD (Additional Authenticated Data).  Either le_iks_aesGcm_StartEncrypt() or
 * le_iks_aesGcm_StartDecrypt() must have been previously called to start either an encryption or
 * decryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if aadChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if aadChunkSize is too big.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an encryption or decryption process was not started or
 *                            plaintext/ciphertext processing has already started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_ProcessAad
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  aadChunkPtr,    ///< [IN] AAD chunk.
    size_t          aadChunkSize    ///< [IN] AAD chunk size.  Must be <= LE_IKS_MAX_PACKET_SIZE.
)
{
    return pa_iks_aesGcm_ProcessAad(session, aadChunkPtr, aadChunkSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext.  le_iks_aesGcm_StartEncrypt() must have been previously called to
 * start an encryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is too big.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an encryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_Encrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  plaintextChunkPtr,      ///< [IN] Plaintext chunk.
    size_t          plaintextChunkSize,     ///< [IN] Plaintext chunk size.
    uint8_t*        ciphertextChunkPtr,     ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t*         ciphertextChunkSizePtr  ///< [INOUT] Ciphertext chunk size.
                                            ///<         Must be >= plaintextChunkSize.
)
{
    return pa_iks_aesGcm_Encrypt(session, plaintextChunkPtr, plaintextChunkSize,
                                 ciphertextChunkPtr, ciphertextChunkSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete encryption and calculate the authentication tag.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if tagPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an encryption process has not started or no data
 *                            (AAD and plaintext) has been processed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_DoneEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    tagPtr,         ///< [OUT] Buffer to hold the authentication tag.
    size_t*     tagSizePtr      ///< [INOUT] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return pa_iks_aesGcm_DoneEncrypt(session, tagPtr, tagSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt and verify the integrity of a long packet with AES in GCM mode.  This
 * function is useful for decrypting and verifying packets that are larger than
 * LE_IKS_MAX_PACKET_SIZE.
 * Calling this function will cancel any previously started process using the same session.
 *
 * To decrypt a long packet the following sequence should be used:
 *
 * le_iks_aesGcm_StartDecrypt() // Start the decryption process.
 * le_iks_aesGcm_ProcessAad()   // Call zero or more times until all AAD data is processed.
 * le_iks_aesGcm_Decrypt()      // Call zero or more times until all ciphertext is decrypted.
 * le_iks_aesGcm_DoneDecrypt()  // Complete decryption process.
 *
 * @warning
 *      While decrypting long packets in this 'streaming' fashion plaintext chunks are released to
 *      the caller before they are verified for integrity.  Ie. the caller will not know the
 *      plaintext is correct until le_iks_aesGcm_DoneDecrypt() is called.  The caller therefore must
 *      not release or make use of any plaintext chunks until after
 *      le_iks_aesGcm_DoneDecrypt() returns with LE_OK.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is not valid
 *                       or if noncePtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_StartDecrypt
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  noncePtr,       ///< [IN] Nonce used to encrypt the packet.
    size_t          nonceSize       ///< [IN] Nonce size.
                                    ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
)
{
    return pa_iks_aesGcm_StartDecrypt(session, noncePtr, nonceSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext.  le_iks_aesGcm_StartDecrypt() must have been previously called to
 * start either a decryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is too big.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if a decryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_Decrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  ciphertextChunkPtr,     ///< [IN] Ciphertext chunk.
    size_t          ciphertextChunkSize,    ///< [IN] Ciphertext chunk size.
    uint8_t*        plaintextChunkPtr,      ///< [OUT] Buffer to hold the plaintext chunk.
    size_t*         plaintextChunkSizePtr   ///< [INOUT] Plaintext chunk size.
                                            ///<         Must be >= ciphertextSize.
)
{
    return pa_iks_aesGcm_Decrypt(session, ciphertextChunkPtr, ciphertextChunkSize,
                                 plaintextChunkPtr, plaintextChunkSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete decryption and verify the integrity.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if tagPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if a decryption process has not started
 *               or no data (AAD and ciphertext) has been processed
 *               or the integrity check failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesGcm_DoneDecrypt
(
    uint64_t        session,    ///< [IN] Session reference.
    const uint8_t*  tagPtr,     ///< [IN] Buffer to hold the authentication tag.
    size_t          tagSize     ///< [IN] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return pa_iks_aesGcm_DoneDecrypt(session, tagPtr, tagSize);
}
