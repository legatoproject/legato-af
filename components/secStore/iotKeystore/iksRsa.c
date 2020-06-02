/** @file iksRsa.c
 *
 * Legato IoT Keystore APIs for performing generation/verification of signatures with RSA-PSS as
 * well as encryption/decryption of short messages using RSA OAEP.
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
 * Encrypts a message with RSAES-OAEP (RSA Encryption Scheme - Optimal Asymmetric Encryption
 * Padding).
 *
 * The maximum plaintext size (pLen bytes) depends on the key size (kLen bytes) and the hash digest
 * size (hLen bytes) according to the equation:  pLen = kLen - 2*hLen - 2
 * For example, with a 2048 bit key using SHA-224 the maximum plaintext size is 226 bytes.
 *
 * An optional label associated with the message can be added.  The label is restricted to less than
 * or equal to MAX_LABEL_SIZE.  The same label must be provided during decryption.
 *
 * The ciphertext size is always kLen bytes (key size) and the ciphertextPtr buffer should be large
 * enough to hold the ciphertext.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       of if the key type is invalid
 *                       or if plaintextPtr, ciphertextPtr or ciphertextSizePtr is NULL.
 *      LE_OUT_OF_RANGE if either the labelSize or the plaintextSize is too big.
 *      LE_OVERFLOW if the ciphertext buffer is too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_rsa_Oaep_Encrypt
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* plaintextPtr,    ///< [IN] Plaintext. NULL if not used.
    size_t plaintextSize,           ///< [IN] Plaintext size.
    uint8_t* ciphertextPtr,         ///< [OUT] Buffer to hold the ciphertext.
    size_t* ciphertextSizePtr       ///< [INOUT] Ciphertext buffer size.
)
{
    return pa_iks_rsa_Oaep_Encrypt(keyRef, labelPtr, labelSize, plaintextPtr, plaintextSize,
                                   ciphertextPtr, ciphertextSizePtr);
}


/**
 * Decrypts a message with RSAES-OAEP (RSA Encryption Scheme - Optimal Asymmetric Encryption
 * Padding).
 *
 * The maximum plaintext size (pLen bytes) depends on the key size (kLen bytes) and the hash digest
 * size (hLen bytes) according to the equation:  pLen = kLen - 2*hLen - 2
 * For example, with a 2048 bit key using SHA-224 the maximum plaintext size is 226 bytes.
 * The plaintextPtr buffer is assumed to be large enough to hold the plaintext.  A safe size for
 * this buffer is kLen.
 *
 * The optional label associated with the message is restricted to less than or equal to
 * MAX_LABEL_SIZE and should be the same label used for encryption.
 *
 * The ciphertext size is expected to be the same as the key size (kLen).
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if the either the ciphertextPtr or plaintextSizePtr is NULL.
 *      LE_OUT_OF_RANGE if the labelSize is too big.
 *      LE_FORMAT_ERROR if the ciphertextSize does not match the key size.
 *      LE_OVERFLOW if the plaintextSizePtr is too small to hold the plaintext.
 *      LE_FAULT if the decryption failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_rsa_Oaep_Decrypt
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* ciphertextPtr,   ///< [IN] Ciphertext.
    size_t ciphertextSize,          ///< [IN] Ciphertext size.
    uint8_t* plaintextPtr,          ///< [OUT] Buffer to hold the plaintext.
    size_t* plaintextSizePtr        ///< [INOUT] Plaintext size.
)
{
    return pa_iks_rsa_Oaep_Decrypt(keyRef, labelPtr, labelSize, ciphertextPtr, ciphertextSize,
                                   plaintextPtr, plaintextSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates a signature on the hash digest of a message with RSASSA-PSS (RSA Signature Scheme with
 * Appendix - Probabilistic Signature Scheme).
 *
 * Signatures are generally only created on a hash of a message rather than directly on the message
 * itself this function follows this paradigm.  However, the same hash function used to create the
 * signature must be used to create the digest of the message.  For example, if the key type is
 * LE_IKS_KEY_TYPE_PRIV_RSASSA_PSS_SHA512 then SHA512 muust be used to create the digest for the
 * message.  The digest size should be the output size of the hash function being used.
 *
 * The salt size should generally be small between 8 and 16 bytes.  Strictly, it must be less than
 * keySize - hLen - 2 where hLen is the output size of the hash function used to create the
 * signature.
 *
 * The signature size is always the size of the key.  The signature buffer should be large enough to
 * hold the signature.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if digestPtr, signaturePtr or signatureSizePtr are NULL.
 *      LE_OUT_OF_RANGE if either the saltSize or the digestSize is too big.
 *      LE_OVERFLOW if the signature buffer is too small.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_rsa_Pss_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    uint32_t saltSize,          ///< [IN] Salt size.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
)
{
    return pa_iks_rsa_Pss_GenSig(keyRef, saltSize, digestPtr, digestSize,
                                 signaturePtr, signatureSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Verifies a signature of the hash digest of a message with RSASSA-PSS (RSA Signature Scheme with
 * Appendix - Probabilistic Signature Scheme).
 *
 * Signatures are generally only created on a hash of a message rather than directly on the message
 * itself this function follows this paradigm.  However, the same hash function used to create the
 * signature must be used to create the digest of the message.  For example, if the key type is
 * LE_IKS_KEY_TYPE_PRIV_RSASSA_PSS_SHA512 then SHA512 muust be used to create the digest for the
 * message.  The digest size should be the output size of the hash function being used.
 *
 * The salt size should generally be small between 8 and 16 bytes.  Strictly, it must be less than
 * keySize - hLen - 2 where hLen is the output size of the hash function used to create the
 * signature.
 *
 * The signature size should always the size of the key.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key type is invalid
 *                       or if either digestPtr or signaturePtr are NULL.
 *      LE_OUT_OF_RANGE if either the saltSize or the digestSize is too big.
 *      LE_FORMAT_ERROR if signatureSize does not match the key size.
 *      LE_FAULT if the signature is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_rsa_Pss_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    uint32_t saltSize,              ///< [IN] Salt size.
    const uint8_t* digestPtr,       ///< [IN] Digest to sign.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
)
{
    return pa_iks_rsa_Pss_VerifySig(keyRef, saltSize, digestPtr, digestSize,
                                    signaturePtr, signatureSize);
}
