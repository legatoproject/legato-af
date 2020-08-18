/** @file iksEcc.c
 *
 * Legato APIs for performing generation/verification of signatures with ECDSA,
 * encryption/decryption of messages using ECIES and shared secret generation with ECDH.
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
 * Generate a shared secret between an ECC private key and an ECC public key.
 *
 * The private key must be of type LE_IKS_KEY_TYPE_PRIV_ECDH and the public must be of type
 * LE_IKS_KEY_TYPE_PUB_ECDH or LE_IKS_KEY_TYPE_PRIV_ECDH.
 *
 * This function may be used as part of a key exchange protocol.  The shared secret is unpredictable
 * (assuming the private portions of both keys are kept secret) but not uniformly distributed and
 * should not be used directly as a cryptographic key.
 *
 * The shared secret is in the format specified by SEC 1, that is the x component of the shared
 * point converted to an octet string.
 *
 * If the buffer is too small to hold the shared secret the shared secret will be truncated to fit.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if either key reference is invalid
 *                       or if either key type is invalid
 *                       or if two key sizes do not match
 *                       or if the secretPtr or secretSizePtr is NULL.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecdh_GetSharedSecret
(
    uint64_t privKeyRef,    ///< [IN] Private key reference.
    uint64_t pubKeyRef,     ///< [IN] Publid Key reference.
    uint8_t* secretPtr,     ///< [OUT] Buffer to hold the shared secret.
    size_t* secretSizePtr   ///< [INOUT] Shared secret size.
)
{
    return pa_iks_ecc_Ecdh_GetSharedSecret(privKeyRef, pubKeyRef, secretPtr, secretSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an ECDSA signature on the hash digest of a message.
 *
 * The key must be a LE_IKS_KEY_TYPE_PRIV_ECDSA key.
 *
 * The signature is the concatenation of the r and s values (r||s).  The size of the signature is
 * twice the key size.  For example if the key is 256 bits in size then the signature will be 64
 * bytes.  Note that when the key size is 521 bits, zero-valued high-order padding bits are added to
 * the signature values r and s resulting in a signature of 132 bytes.
 *
 * The hash function used to generate the message digest should be chosen to match the security
 * strength of the signing key.  For example, if the key size is 256 bits then SHA256 (or its
 * equivalent) should be used.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if digestPtr, signaturePtr or signatureSizePtr are NULL
 *      LE_OVERFLOW if the signature buffer is too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecdsa_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
)
{
    return pa_iks_ecc_Ecdsa_GenSig(keyRef, digestPtr, digestSize, signaturePtr, signatureSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Verifies a signature of the hash digest of a message with ECDSA.
 *
 * The key must be either a LE_IKS_KEY_TYPE_PUB_ECDSA or LE_IKS_KEY_TYPE_PRIV_ECDSA key.
 *
 * The signature is the concatenation of the r and s values (r||s).  The size of the signature is
 * twice the key size.  For example if the key is 256 bits in size then the signature will be 64
 * bytes.  Note that when the key size is 521 bits, zero-valued high-order padding bits are added to
 * the signature values r and s resulting in a signature of 132 bytes.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if either digestPtr or signaturePtr are NULL.
 *      LE_FORMAT_ERROR if signatureSize is incorrect.
 *      LE_FAULT if the signature is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecdsa_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* digestPtr,       ///< [IN] Digest of the message.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
)
{
    return pa_iks_ecc_Ecdsa_VerifySig(keyRef, digestPtr, digestSize, signaturePtr, signatureSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encrypts and integrity protects a short message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * Hybrid encryption combines an asymmetric encryption system with a symmetric encryption system to
 * encrypt messages that can only be decrypted with the holder of the private key.  Hybrid
 * encryption is usually accomplished by using a symmetric encryption system to bulk encrypt
 * the message and then using the asymmetric encryption system to encrypt the symmetric key.
 *
 * ECIES provides hybrid encryption through a method that is more efficient than manually performing
 * the two step process described above.  Broadly speaking, ECIES performs a key agreement to
 * generate a shared secret, the shared secret is then used to generate a symmetric key using a KDF
 * (Key Derivation Function).  The symmetric key is then used to bulk encrypt the message.
 *
 * This implementation of ECIES generally follows the SEC 1 standard but supports modernized
 * algorithms for the KDF and bulk encryption.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid.
 *                       or if either labelPtr, plaintextPtr, ciphertextPtr, ephemKeyPtr,
 *                         ephemKeySizePtr, tagPtr is NULL when they shouldn't be.
 *      LE_OUT_OF_RANGE if the labelSize, textSize, tagSize is invalid.
 *      LE_OVERFLOW if the ephemKeyPtr buffer is too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_EncryptPacket
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* plaintextPtr,    ///< [IN] Plaintext chunk. NULL if not used.
    size_t plaintextSize,           ///< [IN] Plaintext chunk size.
    uint8_t* ciphertextPtr,         ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextSizePtr,      ///< [INOUT] Ciphertext chunk size.
    uint8_t* ephemKeyPtr,           ///< [OUT] Serialized ephemeral public key.
    size_t* ephemKeySizePtr,        ///< [INOUT] Serialized ephemeral key size.
    uint8_t* tagPtr,                ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagSizePtr              ///< [INOUT] Tag size. Cannot be zero.
)
{
    return pa_iks_ecc_Ecies_EncryptPacket(keyRef, labelPtr, labelSize,
                                          plaintextPtr, plaintextSize,
                                          ciphertextPtr, ciphertextSizePtr,
                                          ephemKeyPtr, ephemKeySizePtr,
                                          tagPtr, tagSizePtr);

}


//--------------------------------------------------------------------------------------------------
/**
 * Decrypts and checks the integrity of a short message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * Hybrid encryption combines an asymmetric encryption system with a symmetric encryption system to
 * encrypt messages that can only be decrypted with the holder of the private key.  Hybrid
 * encryption is usually accomplished by using a symmetric encryption system to bulk encrypt
 * the message and then using the asymmetric encryption system to encrypt the symmetric key.
 *
 * ECIES provides hybrid encryption through a method that is more efficient than manually performing
 * the two step process described above.  Broadly speaking, ECIES performs a key agreement to
 * generate a shared secret, the shared secret is then used to generate a symmetric key using a KDF
 * (Key Derivation Function).  The symmetric key is then used to bulk encrypt the message.
 *
 * This implementation of ECIES generally follows the SEC 1 standard but supports modernized
 * algorithms for the KDF and bulk encryption.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key reference is invalid
 *                       or if either the ephemKeyPtr, plaintextPtr,
 *                         ciphertextPtr, tagPtr is NULL.
 *      LE_OUT_OF_RANGE if the labelSize, textSize, tagSize is invalid.
 *      LE_OVERFLOW if plaintextPtr buffer is too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_DecryptPacket
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* ephemKeyPtr,     ///< [IN] Serialized ephemeral public key.
    size_t ephemKeySize,            ///< [IN] Ephemeral public key size.
    const uint8_t* ciphertextPtr,   ///< [IN] Ciphertext chunk.
    size_t ciphertextSize,          ///< [IN] Ciphertext chunk size.
    uint8_t* plaintextPtr,          ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextSizePtr,       ///< [INOUT] Plaintext chunk size.
    const uint8_t* tagPtr,          ///< [IN] Authentication tag.
    size_t tagSize                  ///< [IN] Tag size.
)
{
    return pa_iks_ecc_Ecies_DecryptPacket(keyRef, labelPtr, labelSize,
                                          ephemKeyPtr, ephemKeySize,
                                          ciphertextPtr, ciphertextSize,
                                          plaintextPtr, plaintextSizePtr,
                                          tagPtr, tagSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to encrypt and integrity protect a message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * Hybrid encryption combines an asymmetric encryption system with a symmetric encryption system to
 * encrypt messages that can only be decrypted with the holder of the private key.  Hybrid
 * encryption is usually accomplished by using a symmetric encryption system to bulk encrypt
 * the message and then using the asymmetric encryption system to encrypt the symmetric key.
 *
 * This implementation of ECIES generally follows the SEC 1 standard but supports modernized
 * algorithms for the KDF and bulk encryption.
 *
 * To encrypt a long packet the following sequence should be used:
 *
 * Ecies_StartEncrypt()  // Start the encryption process.
 * Ecies_Encrypt()       // Call zero or more times until all plaintext is encrypted.
 * Ecies_DoneEncrypt()   // Complete the process and obtain the authentication tag.
 *
 * Calling this function will cancel any previously started process using the same session.
 *
 * The session must have been created with the public key used for encryption.
 *
 * An optional label associated with the message can be added.
 *
 * The public portion of the ephemeral key used during the encrytion process is stored in the
 * ephemKeyPtr buffer.  It is encoded as an ECPoint as described in RFC5480.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if either labelPtr, ephemKeyPtr, ephemKeySizePtr is NULL
 *                          when they shouldn't be.
 *      LE_OUT_OF_RANGE if the labelSize is invalid.
 *      LE_OVERFLOW if any of the output buffers are too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    uint8_t* ephemKeyPtr,       ///< [OUT] Serialized ephemeral public key.
    size_t* ephemKeySizePtr     ///< [INOUT] Ephemeral public key size.
)
{
    return pa_iks_ecc_Ecies_StartEncrypt(session, labelPtr, labelSize,
                                         ephemKeyPtr, ephemKeySizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext.  Ecies_StartEncrypt() must have been previously called to
 * start an encryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is too big.
 *      LE_FAULT if an encryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_Encrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* plaintextChunkPtr,   ///< [IN] Plaintext chunk. NULL if not used.
    size_t plaintextChunkSize,          ///< [IN] Plaintext chunk size.
    uint8_t* ciphertextChunkPtr,        ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextChunkSizePtr      ///< [INOUT] Ciphertext chunk size.
)
{
    return pa_iks_ecc_Ecies_Encrypt(session,
                                    plaintextChunkPtr, plaintextChunkSize,
                                    ciphertextChunkPtr, ciphertextChunkSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete encryption and calculate the authentication tag.
 *
 * The maximum tag size depends on the symmetric algorithm used.  If the supplied buffer is
 * larger than or equal to the maximum authentication tag size then the full authentication tag is
 * copied to the buffer and the rest of the buffer is left unmodified.  If the supplied buffer is
 * smaller than the maximum tag size then the tag will be truncated.  However, all tags produced
 * using the same key must use the same tag size.  It is up to the caller to ensure this.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if tagPtr is NULL.
 *      LE_OUT_OF_RANGE if tagSize if invalud.
 *      LE_OVERFLOW if the tagPtr buffer is too small.
 *      LE_FAULT if an encryption process has not started or no data has been processed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_DoneEncrypt
(
    uint64_t session,   ///< [IN] Session reference.
    uint8_t* tagPtr,    ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagSizePtr  ///< [INOUT] Tag size.
)
{
    return pa_iks_ecc_Ecies_DoneEncrypt(session, tagPtr, tagSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt and check the integrity of a message with ECIES (Elliptic Curve
 * Integrated Encryption System).
 *
 * Hybrid encryption combines an asymmetric encryption system with a symmetric encryption system to
 * encrypt (possibly long) messages that can only be decrypted with the holder of the private key.
 * Hybrid encryption is usually accomplished by using a symmetric encryption system to bulk encrypt
 * the message and then using the asymmetric encryption system to encrypt the symmetric key.
 *
 * ECIES provides hybrid encryption through a method that is more efficient than manually performing
 * the two step process described above.  Broadly speaking, ECIES performs a key agreement to
 * generate a shared secret, the shared secret is then used to generate a symmetric key using a KDF
 * (Key Derivation Function).  The symmetric key is then used to bulk encrypt the message.
 *
 * This implementation of ECIES generally follows the SEC 1 standard but supports modernized
 * algorithms for the KDF and bulk encryption.
 *
 * To decrypt a long packet the following sequence should be used:
 *
 * Ecies_StartDecrypt()  // Start the decryption process.
 * Ecies_Decrypt()       // Call zero or more times until all ciphertext is decrypted.
 * Ecies_DoneDecrypt()   // Complete the process and check the authentication tag.
 *
 * Calling this function will cancel any previously started process using the same session.
 *
 * The same label and ephemeral public key used for encryption must be provided.
 *
 * @warning
 *      While decrypting long packets in this 'streaming' fashion plaintext chunks are released to
 *      the caller before they are verified for integrity.  Ie. the caller will not know the
 *      plaintext is correct until DoneDecrypt() is called.  The caller therefore must
 *      not release or make use of any plaintext chunks until after DoneDecrypt() returns
 *      with LE_OK.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the session key type or ephemeral key is invalid
 *                       or if the ephemKeyPtr is NULL.
 *      LE_OUT_OF_RANGE if either the labelSize or ephemKeySize is invalid.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_StartDecrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    const uint8_t* ephemKeyPtr, ///< [IN] Serialized ephemeral public key.
    size_t ephemKeySize         ///< [IN] Ephemeral public key size.
)
{
    return pa_iks_ecc_Ecies_StartDecrypt(session, labelPtr, labelSize,
                                          ephemKeyPtr, ephemKeySize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext.  Ecies_StartDecrypt() must have been previously called to
 * start a decryption process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid
 *                       or if the key type is invalid
 *                       or if plaintextChunkPtr or ciphertextChunkPtr is NULL.
 *      LE_OUT_OF_RANGE if textSize is invalid.
 *      LE_FAULT if a decryption process has not started.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_Decrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* ciphertextChunkPtr,  ///< [IN] Ciphertext chunk.
    size_t ciphertextChunkSize,         ///< [IN] Ciphertext chunk size.
    uint8_t* plaintextChunkPtr,         ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextChunkSizePtr       ///< [INOUT] Plaintext chunk size.
)
{
    return pa_iks_ecc_Ecies_Decrypt(session,
                                    ciphertextChunkPtr, ciphertextChunkSize,
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
 *      LE_OUT_OF_RANGE if tagSize is invalid.
 *      LE_FAULT if a decryption process has not started or no data has been processed
 *               or the integrity check failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ecc_Ecies_DoneDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* tagPtr,  ///< [IN] Authentication tag.
    size_t tagSize          ///< [IN] Tag size.
)
{
    return pa_iks_ecc_Ecies_DoneDecrypt(session, tagPtr, tagSize);
}
