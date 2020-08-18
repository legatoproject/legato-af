//--------------------------------------------------------------------------------------------------
/**
 * @file pa_iotKeystore_default.c
 *
 * Default implementation of @ref c_pa_iotKeystore interface
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pa_iotKeystore.h"


LE_SHARED le_result_t pa_iks_GetKey
(
    const char*     keyId,          ///< [IN] Identifier string.
    uint64_t*       keyRefPtr       ///< [OUT] Key reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_CreateKey
(
    const char*         keyId,      ///< [IN] Identifier string.
    uint32_t            keyUsage,   ///< [IN] Key usage.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_CreateKeyByType
(
    const char*         keyId,      ///< [IN] Identifier string.
    int32_t             keyType,    ///< [IN] Key type.
    uint32_t            keySize,    ///< [IN] Key size in bytes.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetKeyType
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    int32_t*            keyTypePtr  ///< [OUT] Key type.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetKeySize
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint32_t*           keySizePtr  ///< [OUT] Key size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_IsKeySizeValid
(
    int32_t             keyType,    ///< [IN] Key type.
    uint32_t            keySize     ///< [IN] Key size in bytes.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_HasKeyValue
(
    uint64_t        keyRef          ///< [IN] Key reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_SetKeyUpdateKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    uint64_t        updateKeyRef    ///< [IN] Reference to an update key. 0 for not updatable.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GenKeyValue
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ProvisionKeyValue
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    const uint8_t*  provPackagePtr,     ///< [IN] Provisioning package.
    size_t          provPackageSize     ///< [IN] Provisioning package size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_SaveKey
(
    uint64_t        keyRef  ///< [IN] Key reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_DeleteKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetPubKeyValue
(
    uint64_t        keyRef,     ///< [IN] Key reference.
    uint8_t*        bufPtr,     ///< [OUT] Buffer to hold key value.
    size_t*         bufSizePtr  ///< [INOUT] Key value buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetDigest
(
    const char* digestId,       ///< [IN] Identifier string.
    uint64_t*   digestRefPtr    ///< [OUT] Digest reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_CreateDigest
(
    const char*     digestId,       ///< [IN] Identifier string.
    uint32_t        digestSize,     ///< [IN] Digest size. Must be <= MAX_DIGEST_SIZE.
    uint64_t*       digestRefPtr    ///< [OUT] Digest reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetDigestSize
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    uint32_t*           digestSizePtr   ///< [OUT] Digest size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_SetDigestUpdateKey
(
    uint64_t        digestRef,      ///< [IN] Digest reference.
    uint64_t        updateKeyRef    ///< [IN] Reference to an update key. 0 for not updatable.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ProvisionDigest
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    const uint8_t*      provPackagePtr, ///< [IN] Provisioning package.
    size_t              provPackageSize ///< [IN] Provisioning package size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_SaveDigest
(
    uint64_t            digestRef   ///< [IN] Digest reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_DeleteDigest
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    const uint8_t*      authCmdPtr, ///< [IN] Authenticated command buffer.
    size_t              authCmdSize ///< [IN] Authenticated command buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetDigestValue
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the digest value.
    size_t*             bufSizePtr  ///< [INOUT] Size of the buffer.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetUpdateAuthChallenge
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the authentication challenge.
    size_t*             bufSizePtr  ///< [INOUT] Size of the authentication challenge buffer.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_GetWrappingKey
(
    uint8_t*    bufPtr,     ///< [OUT] Buffer to hold the wrapping key.
    size_t*     bufSizePtr  ///< [INOUT] Size of the buffer to hold the wrapping key.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_CreateSession
(
    uint64_t    keyRef,         ///< [IN] Key reference.
    uint64_t*   sessionRefPtr   ///< [OUT] Session reference.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_DeleteSession
(
    uint64_t            sessionRef  ///< [IN] Session reference.
)
{
    return LE_UNSUPPORTED;
}


//========================= AES Milenage routines =====================


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
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesMilenage_GetAk
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
    uint8_t*        akPtr,          ///< [OUT] Buffer to hold the anonymity key AK.
    size_t*         akSizePtr       ///< [OUT] AK size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesMilenage_DeriveOpc
(
    uint64_t        opRef,      ///< [IN] Reference to OP key.
    const uint8_t*  kPtr,       ///< [IN] K.
    size_t          kSize,      ///< [IN] K size. Assumed to be LE_IKS_AESMILENAGE_K_SIZE.
    uint8_t*        opcPtr,     ///< [OUT] Buffer to hold the OPc value.
    size_t*         opcSize     ///< [OUT] OPc size. Assumed to be LE_IKS_AESMILENAGE_OPC_SIZE.
)
{
    return LE_UNSUPPORTED;
}

//========================= AES GCM routines =====================


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
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}

LE_SHARED le_result_t pa_iks_aesGcm_StartEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    noncePtr,       ///< [OUT] Buffer to hold the nonce.  Assumed to be
                                ///<       LE_IKS_AES_GCM_NONCE_SIZE bytes.
    size_t*     nonceSizePtr    ///< [INOUT] Nonce size.
                                ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_ProcessAad
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  aadChunkPtr,    ///< [IN] AAD chunk.
    size_t          aadChunkSize    ///< [IN] AAD chunk size.  Must be <= LE_IKS_MAX_PACKET_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_Encrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  plaintextChunkPtr,      ///< [IN] Plaintext chunk.
    size_t          plaintextChunkSize,     ///< [IN] Plaintext chunk size.
    uint8_t*        ciphertextChunkPtr,     ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t*         ciphertextChunkSizePtr  ///< [INOUT] Ciphertext chunk size.
                                            ///<         Must be >= plaintextChunkSize.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_DoneEncrypt
(
    uint64_t    session,        ///< [IN] Session reference.
    uint8_t*    tagPtr,         ///< [OUT] Buffer to hold the authentication tag.
    size_t*     tagSizePtr      ///< [INOUT] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_StartDecrypt
(
    uint64_t        session,        ///< [IN] Session reference.
    const uint8_t*  noncePtr,       ///< [IN] Nonce used to encrypt the packet.
    size_t          nonceSize       ///< [IN] Nonce size.
                                    ///<         Expected to be LE_IKS_AESGCM_NONCE_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_Decrypt
(
    uint64_t        session,                ///< [IN] Session reference.
    const uint8_t*  ciphertextChunkPtr,     ///< [IN] Ciphertext chunk.
    size_t          ciphertextChunkSize,    ///< [IN] Ciphertext chunk size.
    uint8_t*        plaintextChunkPtr,      ///< [OUT] Buffer to hold the plaintext chunk.
    size_t*         plaintextChunkSizePtr   ///< [INOUT] Plaintext chunk size.
                                            ///<         Must be >= ciphertextSize.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesGcm_DoneDecrypt
(
    uint64_t        session,    ///< [IN] Session reference.
    const uint8_t*  tagPtr,     ///< [IN] Buffer to hold the authentication tag.
    size_t          tagSize     ///< [IN] Authentication tag size.
                                ///<         Expected to be LE_IKS_AESGCM_TAG_SIZE.
)
{
    return LE_UNSUPPORTED;
}


//========================= AES CBC routines =====================


LE_SHARED le_result_t pa_iks_aesCbc_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* ivPtr,       ///< [IN] Initialization vector.
    size_t ivSize               ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
)
{
    return LE_UNSUPPORTED;
}


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

)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesCbc_StartDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* ivPtr,   ///< [IN] Initialization vector.
    size_t ivSize           ///< [IN] IV size. Assumed to be LE_IKS_AESCBC_IV_SIZE bytes.
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}


//========================= AES CMAC routines =====================


LE_SHARED le_result_t pa_iks_aesCmac_ProcessChunk
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* msgChunkPtr, ///< [IN] Message chunk.
    size_t msgChunkSize         ///< [IN] Message chunk size. Must be <= LE_IKS_MAX_PACKET_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesCmac_Done
(
    uint64_t session,       ///< [IN] Session reference.
    uint8_t* tagBufPtr,     ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagBufSizePtr   ///< [INOUT] Authentication tag buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_aesCmac_Verify
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* tagBufPtr,   ///< [IN] Authentication tag to check against.
    size_t tagBufSize           ///< [IN] Authentication tag size. Cannot be zero.
)
{
    return LE_UNSUPPORTED;
}


//========================= HMAC routines =====================


LE_SHARED le_result_t pa_iks_hmac_ProcessChunk
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* msgChunkPtr, ///< [IN] Message chunk.
    size_t msgChunkSize         ///< [IN] Message chunk size. Must be <= LE_IKS_MAX_PACKET_SIZE.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_hmac_Done
(
    uint64_t session,       ///< [IN] Session reference.
    uint8_t* tagBufPtr,     ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagBufSizePtr   ///< [INOUT] Authentication tag buffer size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_hmac_Verify
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* tagBufPtr,   ///< [IN] Authentication tag to check against.
    size_t tagBufSize           ///< [IN] Authentication tag size. Cannot be zero.
)
{
    return LE_UNSUPPORTED;
}


//========================= RSA routines =====================


LE_SHARED le_result_t pa_iks_rsa_Oaep_Encrypt
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
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_rsa_Oaep_Decrypt
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
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_rsa_Pss_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    uint32_t saltSize,          ///< [IN] Salt size.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_rsa_Pss_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    uint32_t saltSize,              ///< [IN] Salt size.
    const uint8_t* digestPtr,       ///< [IN] Digest to sign.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
)
{
    return LE_UNSUPPORTED;
}


//========================= ECC routines =====================

LE_SHARED le_result_t pa_iks_ecc_Ecdh_GetSharedSecret
(
    uint64_t privKeyRef,    ///< [IN] Private key reference.
    uint64_t pubKeyRef,     ///< [IN] Publid Key reference.
    uint8_t* secretPtr,     ///< [OUT] Buffer to hold the shared secret.
    size_t* secretSizePtr   ///< [INOUT] Shared secret size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecdsa_GenSig
(
    uint64_t keyRef,            ///< [IN] Key reference.
    const uint8_t* digestPtr,   ///< [IN] Digest to sign.
    size_t digestSize,          ///< [IN] Digest size.
    uint8_t* signaturePtr,      ///< [OUT] Buffer to hold the signature.
    size_t* signatureSizePtr    ///< [INOUT] Signature size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecdsa_VerifySig
(
    uint64_t keyRef,                ///< [IN] Key reference.
    const uint8_t* digestPtr,       ///< [IN] Digest of the message.
    size_t digestSize,              ///< [IN] Digest size.
    const uint8_t* signaturePtr,    ///< [IN] Signature of the message.
    size_t signatureSize            ///< [IN] Signature size.
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}


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
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_StartEncrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    uint8_t* ephemKeyPtr,       ///< [OUT] Serialized ephemeral public key.
    size_t* ephemKeySizePtr     ///< [INOUT] Ephemeral public key size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_Encrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* plaintextChunkPtr,   ///< [IN] Plaintext chunk. NULL if not used.
    size_t plaintextChunkSize,          ///< [IN] Plaintext chunk size.
    uint8_t* ciphertextChunkPtr,        ///< [OUT] Buffer to hold the ciphertext chunk.
    size_t* ciphertextChunkSizePtr      ///< [INOUT] Ciphertext chunk size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_DoneEncrypt
(
    uint64_t session,   ///< [IN] Session reference.
    uint8_t* tagPtr,    ///< [OUT] Buffer to hold the authentication tag.
    size_t* tagSizePtr  ///< [INOUT] Tag size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_StartDecrypt
(
    uint64_t session,           ///< [IN] Session reference.
    const uint8_t* labelPtr,    ///< [IN] Label. NULL if not used.
    size_t labelSize,           ///< [IN] Label size.
    const uint8_t* ephemKeyPtr, ///< [IN] Serialized ephemeral public key.
    size_t ephemKeySize         ///< [IN] Ephemeral public key size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_Decrypt
(
    uint64_t session,                   ///< [IN] Session reference.
    const uint8_t* ciphertextChunkPtr,  ///< [IN] Ciphertext chunk.
    size_t ciphertextChunkSize,         ///< [IN] Ciphertext chunk size.
    uint8_t* plaintextChunkPtr,         ///< [OUT] Buffer to hold the plaintext chunk.
    size_t* plaintextChunkSizePtr       ///< [INOUT] Plaintext chunk size.
)
{
    return LE_UNSUPPORTED;
}


LE_SHARED le_result_t pa_iks_ecc_Ecies_DoneDecrypt
(
    uint64_t session,       ///< [IN] Session reference.
    const uint8_t* tagPtr,  ///< [IN] Authentication tag.
    size_t tagSize          ///< [IN] Tag size.
)
{
    return LE_UNSUPPORTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
