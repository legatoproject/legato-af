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
 * Sets the module ID.
 *
 * @return
 *      LE_OK
 *      LE_BAD_PARAMETER
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_SetModuleId
(
    const char*     idPtr,      ///< [IN] Identifier string.
    uint64_t        keyRef      ///< [IN] Key reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the module ID.
 *
 * @return
 *      LE_OK
 *      LE_NOT_FOUND
 *      LE_BAD_PARAMETER
 *      LE_OVERFLOW
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetModuleId
(
    char*       idPtr,        ///< [OUT] Module ID buffer.
    size_t      idPtrSize     ///< [IN] Module ID buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the module ID.
 *
 * @return
 *      LE_OK
 *      LE_NOT_FOUND
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_DeleteModuleId
(
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a key.
 *
 * @return
 *      Reference to the key.
 *      0 if the key could not be found.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_GetKey
(
    const char*     keyId           ///< [IN] Identifier string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key.
 *
 * @return
 *      Reference to the key if successful.
 *      0 if the keyId is already being used or is invalid or the keyUsage is invalid.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_CreateKey
(
    const char*         keyId,      ///< [IN] Identifier string.
    uint32_t            keyUsage    ///< [IN] Key usage.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key of a specific type.
 *
 * @return
 *      Reference to the key if successful.
 *      0 if the keyId is already being used or if there was some other error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_CreateKeyByType
(
    const char*         keyId,      ///< [IN] Identifier string.
    int32_t             keyType,    ///< [IN] Key type.
    uint32_t            keySize     ///< [IN] Key size in bytes.
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
    uint64_t        updateKeyRef    ///< [IN] Reference to an update key.
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
 *      Reference to the digest.
 *      0 if the digest could not be found.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_GetDigest
(
    const char* digestId ///< [IN] Identifier string.
);


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new digest.
 *
 * @return
 *      Reference to the digest if successful.
 *      0 if there was an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_CreateDigest
(
    const char*     digestId,   ///< [IN] Identifier string.
    uint32_t        digestSize  ///< [IN] Digest size. Must be <= MAX_DIGEST_SIZE.
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
 * Get the provisioning key.
 *
 * @return
 *      LE_OK
 *      LE_OVERFLOW
 *      LE_UNSUPPORTED
 *      LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_iks_GetProvisionKey
(
    uint8_t*    bufPtr,     ///< [OUT] Buffer to hold the provisioning key.
    size_t*     bufSizePtr  ///< [INOUT] Size of the buffer to hold the provisioning key.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a session.
 *
 * @return
 *      A session reference if successful.
 *      0 if the key reference is invalid or does not contain a key value.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED uint64_t pa_iks_CreateSession
(
    uint64_t            keyRef      ///< [IN] Key reference.
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


#endif // LEGATO_PA_IOT_KEYSTORE_INCLUDE_GUARD
