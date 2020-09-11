/**
 * @page c_pa_iotKeystore IoT KeyStore PA Interface
 *
 * This module contains a platform independent API for IoT Keystore.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_IOT_KEYSTORE_INCLUDE_GUARD
#define LEGATO_PA_IOT_KEYSTORE_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NOT_FOUND
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetKey
(
    const char*     keyId,          ///< [IN] Identifier string.
    uint64_t*       keyRefPtr       ///< [OUT] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_DUPLICATE
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_CreateKey
(
    const char*         keyId,      ///< [IN] Identifier string.
    uint32_t            keyUsage,   ///< [IN] Key usage.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key of a specific type.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_DUPLICATE
 *      LE_OUT_OF_RANGE
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_CreateKeyByType
(
    const char*         keyId,      ///< [IN] Identifier string.
    int32_t             keyType,    ///< [IN] Key type.
    uint32_t            keySize,    ///< [IN] Key size in bytes.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the key type.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetKeyType
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    int32_t*            keyTypePtr  ///< [OUT] Key type.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the key size in bytes.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetKeySize
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint32_t*           keySizePtr  ///< [OUT] Key size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the key size is valid.
 *
 * @return
 *      LE_OK
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_IsKeySizeValid
(
    int32_t             keyType,    ///< [IN] Key type.
    uint32_t            keySize     ///< [IN] Key size in bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the key has a value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NOT_FOUND
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_HasKeyValue
(
    uint64_t        keyRef          ///< [IN] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set an update key for the specified key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_SetKeyUpdateKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    uint64_t        updateKeyRef    ///< [IN] Reference to an update key. 0 for not updatable.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate a key value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GenKeyValue
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Provision a key value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ProvisionKeyValue
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    const uint8_t*  provPackagePtr,     ///< [IN] Provisioning package.
    size_t          provPackageSize     ///< [IN] Provisioning package size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Saves a key to persistent storage.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_SaveKey
(
    uint64_t        keyRef  ///< [IN] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_DeleteKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the public portion of an asymmetric key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NOT_FOUND
 *      LE_UNSUPPORTED
 *      LE_OVERFLOW
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetPubKeyValue
(
    uint64_t        keyRef,     ///< [IN] Key reference.
    uint8_t*        bufPtr,     ///< [OUT] Buffer to hold key value.
    size_t*         bufSizePtr  ///< [INOUT] Key value buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a digest.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NOT_FOUND
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetDigest
(
    const char* digestId,       ///< [IN] Identifier string.
    uint64_t*   digestRefPtr    ///< [OUT] Digest reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new digest.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_DUPLICATE
 *      LE_OUT_OF_RANGE
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_CreateDigest
(
    const char*     digestId,       ///< [IN] Identifier string.
    uint32_t        digestSize,     ///< [IN] Digest size. Must be <= MAX_DIGEST_SIZE.
    uint64_t*       digestRefPtr    ///< [OUT] Digest reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the digest size in bytes.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetDigestSize
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    uint32_t*           digestSizePtr   ///< [OUT] Digest size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set an update key for the specified digest.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_SetDigestUpdateKey
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    uint64_t            updateKeyRef    ///< [IN] Reference to an update key.
);


//--------------------------------------------------------------------------------------------------
/**
 * Provision a digest value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ProvisionDigest
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    const uint8_t*      provPackagePtr, ///< [IN] Provisioning package.
    size_t              provPackageSize ///< [IN] Provisioning package size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Saves a digest to persistent storage.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_SaveDigest
(
    uint64_t            digestRef   ///< [IN] Digest reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete a digest.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_DeleteDigest
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    const uint8_t*      authCmdPtr, ///< [IN] Authenticated command buffer.
    size_t              authCmdSize ///< [IN] Authenticated command buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the digest value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NOT_FOUND
 *      LE_OVERFLOW
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetDigestValue
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the digest value.
    size_t*             bufSizePtr  ///< [INOUT] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get update authentication challenge.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetUpdateAuthChallenge
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the authentication challenge.
    size_t*             bufSizePtr  ///< [INOUT] Size of the authentication challenge buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the wrapping key.
 *
 * @return
 *      LE_OK
 *      LE_OVERFLOW
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetWrappingKey
(
    uint8_t*    bufPtr,     ///< [OUT] Buffer to hold the wrapping key.
    size_t*     bufSizePtr  ///< [INOUT] Size of the buffer to hold the wrapping key.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a session.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_NO_MEMORY
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_CreateSession
(
    uint64_t    keyRef,         ///< [IN] Key reference.
    uint64_t*   sessionRefPtr   ///< [OUT] Session reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete a session.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_DeleteSession
(
    uint64_t            sessionRef  ///< [IN] Session reference.
);


//========================= AES Milenage routines =====================


//--------------------------------------------------------------------------------------------------
/**
 * Calculates the network authentication code MAC-A using the Milenage algorithm set with AES-128 as
 * the block cipher.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesMilenage_GetMacA
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
    const uint8_t*  amfPtr,         ///< [IN] Authentication management field, AMF.
    size_t          amfSize,        ///< [IN] AMF size.
    const uint8_t*  sqnPtr,         ///< [IN] Sequence number, SQN.
    size_t          sqnSize,        ///< [IN] SQN size.
    uint8_t*        macaPtr,        ///< [OUT] Buffer to hold the network authentication code.
    size_t*         macaSizePtr     ///< [OUT] Network authentication code size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Calculates the re-synchronisation authentication code MAC-S using the Milenage algorithm set with
 * AES-128 as the block cipher.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesMilenage_GetMacS
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
    const uint8_t*  amfPtr,         ///< [IN] Authentication management field, AMF.
    size_t          amfSize,        ///< [IN] AMF size.
    const uint8_t*  sqnPtr,         ///< [IN] Sequence number, SQN.
    size_t          sqnSize,        ///< [IN] SQN size.
    uint8_t*        macsPtr,        ///< [OUT] Buffer to hold the re-sync authentication code.
    size_t*         macsSizePtr     ///< [OUT] Re-sync authentication code size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Derives authentication response and keys using the Milenage algorithm set with AES-128 as the
 * block cipher.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesMilenage_GetKeys
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
    uint8_t*        resPtr,         ///< [OUT] Buffer to hold the authentication response RES.
    size_t*         resSizePtr,     ///< [OUT] RES size.
    uint8_t*        ckPtr,          ///< [OUT] Buffer to hold the confidentiality key CK.
    size_t*         ckSizePtr,      ///< [OUT] CK size.
    uint8_t*        ikPtr,          ///< [OUT] Buffer to hold the integrity key IK.
    size_t*         ikSizePtr,      ///< [OUT] IK size.
    uint8_t*        akPtr,          ///< [OUT] Buffer to hold the anonymity key AK.
    size_t*         akSizePtr       ///< [OUT] AK size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Derives the anonymity key for the re-synchronisation message using the Milenage algorithm set
 * with AES-128 as the block cipher.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesMilenage_GetAk
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
    uint8_t*        akPtr,          ///< [OUT] Buffer to hold the anonymity key AK.
    size_t*         akSizePtr       ///< [OUT] AK size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Derive an OPc value from the specified K and the internal OP value.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesMilenage_DeriveOpc
(
    uint64_t        opRef,      ///< [IN] Reference to OP key.
    const uint8_t*  kPtr,       ///< [IN] K.
    size_t          kSize,      ///< [IN] K size. Assumed to be LE_IKS_AESMILENAGE_K_SIZE.
    uint8_t*        opcPtr,     ///< [OUT] Buffer to hold the OPc value.
    size_t*         opcSize     ///< [OUT] OPc size. Assumed to be LE_IKS_AESMILENAGE_OPC_SIZE.
);

//========================= AES GCM routines =====================

//--------------------------------------------------------------------------------------------------
/**
 * Encrypt and integrity protect a packet with AES in GCM mode.
 *
 * @return
 *      LE_OK
 *      LE_OUT_OF_RANGE
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_EncryptPacket
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    uint8_t*        noncePtr,           ///< [OUT] Buffer to hold the nonce.
    size_t*         nonceSizePtr,       ///< [INOUT] Nonce size.
    const uint8_t*  aadPtr,             ///< [IN] Additional authenticated data (AAD).
    size_t          aadSize,            ///< [IN] AAD size.
    const uint8_t*  plaintextPtr,       ///< [IN] Plaintext. NULL if not used.
    size_t          plaintextSize,      ///< [IN] Plaintext size.
    uint8_t*        ciphertextPtr,      ///< [OUT] Buffer to hold the ciphertext.
    size_t*         ciphertextSizePtr,  ///< [INOUT] Ciphertext size.
    uint8_t*        tagPtr,             ///< [OUT] Buffer to hold the authentication tag.
    size_t*         tagSizePtr          ///< [INOUT] Authentication tag size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt and verify the integrity of a packet with AES in GCM mode.
 *
 * @return
 *      LE_OK
 *      LE_OUT_OF_RANGE
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_DecryptPacket
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    const uint8_t*  noncePtr,           ///< [IN] Nonce used to encrypt the packet.
    size_t          nonceSize,          ///< [IN] Nonce size.
    const uint8_t*  aadPtr,             ///< [IN] Additional authenticated data (AAD).
    size_t          aadSize,            ///< [IN] AAD size.
    const uint8_t*  ciphertextPtr,      ///< [IN] Ciphertext. NULL if not used.
    size_t          ciphertextSize,     ///< [IN] Ciphertext size.
    uint8_t*        plaintextPtr,       ///< [OUT] Buffer to hold the plaintext.
    size_t*         plaintextSizePtr,   ///< [INOUT] Plaintext size.
    const uint8_t*  tagPtr,             ///< [IN] Buffer to hold the authentication tag.
    size_t          tagSize             ///< [IN] Authentication tag size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to encrypt and integrity protect a long packet with AES in GCM mode.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_StartEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    noncePtr,       ///< [OUT] Buffer to hold the nonce.  Assumed to be
                                ///<       LE_IKS_AES_GCM_NONCE_SIZE bytes.
    size_t*     nonceSizePtr    ///< [INOUT] Nonce size.
                                ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Process a chunk of AAD (Additional Authenticated Data).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_ProcessAad
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  aadChunkPtr,    ///< [IN] AAD chunk.
    size_t          aadChunkSize    ///< [IN] AAD chunk size.  Must be <= LE_IKS_MAX_PACKET_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_Encrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  plaintextChunkPtr,      ///< [IN] Plaintext chunk.
    size_t          plaintextChunkSize,     ///< [IN] Plaintext chunk size.
    uint8_t*        ciphertextChunkPtr,     ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t*         ciphertextChunkSizePtr  ///< [INOUT] Ciphertext chunk size.
                                            ///<         Must be >= plaintextChunkSize.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete encryption and calculate the authentication tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_DoneEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    tagPtr,         ///< [OUT] Buffer to hold the authentication tag.
    size_t*     tagSizePtr      ///< [INOUT] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt and verify the integrity of a long packet with AES in GCM mode.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_StartDecrypt
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  noncePtr,       ///< [IN] Nonce used to encrypt the packet.
    size_t          nonceSize       ///< [IN] Nonce size.
                                    ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_Decrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  ciphertextChunkPtr,     ///< [IN] Ciphertext chunk.
    size_t          ciphertextChunkSize,    ///< [IN] Ciphertext chunk size.
    uint8_t*        plaintextChunkPtr,      ///< [OUT] Buffer to hold the plaintext chunk.
    size_t*         plaintextChunkSizePtr   ///< [INOUT] Plaintext chunk size.
                                            ///<         Must be >= ciphertextSize.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete decryption and verify the integrity.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesGcm_DoneDecrypt
(
    uint64_t        session,    ///< [IN] Session reference.
    const uint8_t*  tagPtr,     ///< [IN] Buffer to hold the authentication tag.
    size_t          tagSize     ///< [IN] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to encrypt a message with AES in CBC mode.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCbc_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* ivPtr,       ///< [IN] Initialization vector.
    size_t ivSize               ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCbc_Encrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* plaintextChunkPtr,   ///< [IN] Plaintext chunk.
    size_t plaintextChunkSize,          ///< [IN] Plaintext chunk size.
                                        ///<      Must be <= LE_IKS_MAX_PACKET_SIZE and
                                        ///<      a multiple of LE_IKS_AES_BLOCK_SIZE.
    uint8_t* ciphertextChunkPtr,        ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextChunkSizePtr      ///< [INOUT] Ciphertext chunk size.
                                        ///<         Must be >= plaintextChunkSize.

);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt a message with AES in CBC mode.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCbc_StartDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* ivPtr,   ///< [IN] Initialization vector.
    size_t ivSize           ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCbc_Decrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* ciphertextChunkPtr,  ///< [IN] Ciphertext chunk.
    size_t ciphertextChunkSize,         ///< [IN] Ciphertext chunk size.
                                        ///<      Must be <= LE_IKS_MAX_PACKET_SIZE and
                                        ///<      a multiple of LE_IKS_AES_BLOCK_SIZE.
    uint8_t* plaintextChunkPtr,         ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextChunkSizePtr       ///< [INOUT] Plaintext buffer size.
                                        ///<         Must be >= ciphertextChunkSize.
);


//--------------------------------------------------------------------------------------------------
/**
 * Process message chunks.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCmac_ProcessChunk
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* msgChunkPtr, ///< [IN] Message chunk.
    size_t msgChunkSize         ///< [IN] Message chunk size. Must be <= LE_IKS_MAX_PACKET_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and get the processed message's authentication tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCmac_Done
(
    uint64_t session,       ///< [IN] Session reference.
    uint8_t* tagBufPtr,     ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagBufSizePtr   ///< [INOUT] Authentication tag buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and compare the resulting authentication tag with the supplied tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_aesCmac_Verify
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* tagBufPtr,   ///< [IN] Authentication tag to check against.
    size_t tagBufSize           ///< [IN] Authentication tag size. Cannot be zero.
);


//--------------------------------------------------------------------------------------------------
/**
 * Process message chunks.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_hmac_ProcessChunk
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* msgChunkPtr, ///< [IN] Message chunk.
    size_t msgChunkSize         ///< [IN] Message chunk size. Must be <= LE_IKS_MAX_PACKET_SIZE.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and get the processed message's authentication tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_hmac_Done
(
    uint64_t session,       ///< [IN] Session reference.
    uint8_t* tagBufPtr,     ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagBufSizePtr   ///< [INOUT] Authentication tag buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete message processing and compare the resulting authentication tag with the supplied tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_hmac_Verify
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* tagBufPtr,   ///< [IN] Authentication tag to check against.
    size_t tagBufSize           ///< [IN] Authentication tag size. Cannot be zero.
);


//========================= RSA routines =====================

//--------------------------------------------------------------------------------------------------
/**
 * Encrypts a message with RSAES-OAEP (RSA Encryption Scheme - Optimal Asymmetric Encryption
 * Padding).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_rsa_Oaep_Encrypt
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* plaintextPtr,    ///< [IN] Plaintext. NULL if not used.
    size_t plaintextSize,           ///< [IN] Plaintext size.
    uint8_t* ciphertextPtr,         ///< [OUT] Buffer to hold the ciphertext.
    size_t* ciphertextSizePtr       ///< [INOUT] Ciphertext buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypts a message with RSAES-OAEP (RSA Encryption Scheme - Optimal Asymmetric Encryption
 * Padding).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FORMAT_ERROR
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_rsa_Oaep_Decrypt
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* labelPtr,        ///< [IN] Label. NULL if not used.
    size_t labelSize,               ///< [IN] Label size.
    const uint8_t* ciphertextPtr,   ///< [IN] Ciphertext.
    size_t ciphertextSize,          ///< [IN] Ciphertext size.
    uint8_t* plaintextPtr,          ///< [OUT] Buffer to hold the plaintext.
    size_t* plaintextSizePtr        ///< [INOUT] Plaintext size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generates a signature on the hash digest of a message with RSASSA-PSS (RSA Signature Scheme with
 * Appendix - Probabilistic Signature Scheme).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_rsa_Pss_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    uint32_t saltSize,          ///< [IN] Salt size.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Verifies a signature of the hash digest of a message with RSASSA-PSS (RSA Signature Scheme with
 * Appendix - Probabilistic Signature Scheme).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FORMAT_ERROR
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_rsa_Pss_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    uint32_t saltSize,              ///< [IN] Salt size.
    const uint8_t* digestPtr,       ///< [IN] Digest to sign.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
);


//========================= ECC routines =====================

//--------------------------------------------------------------------------------------------------
/**
 * Generate a shared secret between an ECC private key and an ECC public key.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecdh_GetSharedSecret
(
    uint64_t privKeyRef,    ///< [IN] Private key reference.
    uint64_t pubKeyRef,     ///< [IN] Publid Key reference.
    uint8_t* secretPtr,     ///< [OUT] Buffer to hold the shared secret.
    size_t* secretSizePtr   ///< [INOUT] Shared secret size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate an ECDSA signature on the hash digest of a message.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecdsa_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Verifies a signature of the hash digest of a message with ECDSA.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_FORMAT_ERROR
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecdsa_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* digestPtr,       ///< [IN] Digest of the message.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Encrypts and integrity protects a short message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_EncryptPacket
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
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypts and checks the integrity of a short message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_DecryptPacket
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
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to encrypt and integrity protect a message with ECIES (Elliptic Curve Integrated
 * Encryption System).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    uint8_t* ephemKeyPtr,       ///< [OUT] Serialized ephemeral public key.
    size_t* ephemKeySizePtr     ///< [INOUT] Ephemeral public key size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Encrypt a chunk of plaintext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_Encrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* plaintextChunkPtr,   ///< [IN] Plaintext chunk. NULL if not used.
    size_t plaintextChunkSize,          ///< [IN] Plaintext chunk size.
    uint8_t* ciphertextChunkPtr,        ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextChunkSizePtr      ///< [INOUT] Ciphertext chunk size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete encryption and calculate the authentication tag.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_OVERFLOW
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_DoneEncrypt
(
    uint64_t session,   ///< [IN] Session reference.
    uint8_t* tagPtr,    ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagSizePtr  ///< [INOUT] Tag size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts a process to decrypt and check the integrity of a message with ECIES (Elliptic Curve
 * Integrated Encryption System).
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_StartDecrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    const uint8_t* ephemKeyPtr, ///< [IN] Serialized ephemeral public key.
    size_t ephemKeySize         ///< [IN] Ephemeral public key size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrypt a chunk of ciphertext.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_Decrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* ciphertextChunkPtr,  ///< [IN] Ciphertext chunk.
    size_t ciphertextChunkSize,         ///< [IN] Ciphertext chunk size.
    uint8_t* plaintextChunkPtr,         ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextChunkSizePtr       ///< [INOUT] Plaintext chunk size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete decryption and verify the integrity.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_OUT_OF_RANGE
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_ecc_Ecies_DoneDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* tagPtr,  ///< [IN] Authentication tag.
    size_t tagSize          ///< [IN] Tag size.
);

#endif // LEGATO_PA_IOT_KEYSTORE_INCLUDE_GUARD
