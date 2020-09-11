/** @file iksKeyManagement.c
 *
 * Legato API for IoT KeyStore's key and digest management routines.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_iotKeystore.h"
#include "secStoreServer.h"


//--------------------------------------------------------------------------------------------------
/**
 * Convert Key ID by adding a prefix representing app or user name.
 * keyId -> <clientName>.keyId
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the client name.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConvertKeyId
(
    const char* src,    ///< [IN] Original Key ID
    char* dst,          ///< [OUT] Converted Key ID
    size_t dstSize      ///< [IN] Destination buffer size
)
{
    le_result_t result;
    size_t offset = 0;

    result = secStoreServer_GetClientName(le_iks_GetClientSessionRef(), dst, dstSize, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("Could not get the client's name.");
        return result;
    }

    offset = strlen(dst);
    // Must fit delimiter, at least one-character key ID, and terminating 0.
    if (offset + 3 > dstSize)
    {
        LE_ERROR("Buffer too small to contain the client name: offset %" PRIuS " size %" PRIuS,
                 offset, dstSize);
        return LE_OVERFLOW;
    }

    dst[offset] = '.';
    offset++;

    result = le_utf8_Copy(dst + offset, src, dstSize - offset, NULL);
    if (result != LE_OK)
    {
        LE_ERROR("Could not copy KeyId to the buffer: %s", LE_RESULT_TXT(result));
        return result;
    }

    LE_DEBUG("Converted Key Id '%s'", dst);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a key.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the keyId is invalid or keyRef is NULL.
 *      LE_NOT_FOUND if the key does not exist.
 *      LE_NO_MEMORY if there is not enough memory to retrieve the key.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetKey
(
    const char*     keyId,          ///< [IN] Identifier string.
    uint64_t*       keyRefPtr       ///< [OUT] Key reference.
)
{
    char fullKeyIdBuf[LE_IKS_MAX_KEY_ID_BYTES] = {0};

    if (LE_OK != ConvertKeyId(keyId, fullKeyIdBuf, sizeof(fullKeyIdBuf)))
    {
        LE_ERROR("Error converting key '%s'", keyId);
        return LE_FAULT;
    }
    return pa_iks_GetKey(fullKeyIdBuf, keyRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key.
 *
 * This is a convenient way to create a key for a specific usage.  This function will choose a
 * default key type to satisfy the specified usage.
 *
 * New keys initially have no value and cannot be used.  Key values can be set using either
 * le_iks_GenKeyValue() or le_iks_ProvisionKeyValue().
 *
 * Created keys initially only exist in non-persistent memory, call le_iks_SaveKey() to save
 * the key to persistent memory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the keyId or keyUsage is invalid or if keyRef is NULL.
 *      LE_DUPLICATE if the keyId is already being used.
 *      LE_NO_MEMORY if there is not enough memory to create the key.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_CreateKey
(
    const char*         keyId,      ///< [IN] Identifier string.
    le_iks_KeyUsage_t   keyUsage,   ///< [IN] Key usage.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
)
{
    char fullKeyIdBuf[LE_IKS_MAX_KEY_ID_BYTES] = {0};

    if (LE_OK != ConvertKeyId(keyId, fullKeyIdBuf, sizeof(fullKeyIdBuf)))
    {
        LE_ERROR("Error converting key '%s'", keyId);
        return LE_FAULT;
    }

    return pa_iks_CreateKey(fullKeyIdBuf, keyUsage, keyRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new key of a specific type.
 *
 * New keys initially have no value and cannot be used.  Key values can be set using either
 * le_iks_GenKeyValue() or le_iks_ProvisionKeyValue().
 *
 * Created keys initially only exist in non-persistent memory, call le_iks_SaveKey() to save
 * the key to persistent memory.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the keyId or keyType is invalid or if keyRef is NULL.
 *      LE_DUPLICATE if the keyId is already being used.
 *      LE_OUT_OF_RANGE if the key size is invalid.
 *      LE_NO_MEMORY if there is not enough memory to create the key.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_CreateKeyByType
(
    const char*         keyId,      ///< [IN] Identifier string.
    le_iks_KeyType_t    keyType,    ///< [IN] Key type.
    uint32_t            keySize,    ///< [IN] Key size in bytes.
    uint64_t*           keyRefPtr   ///< [OUT] Key reference.
)
{
    char fullKeyIdBuf[LE_IKS_MAX_KEY_ID_BYTES] = {0};

    if (LE_OK != ConvertKeyId(keyId, fullKeyIdBuf, sizeof(fullKeyIdBuf)))
    {
        LE_ERROR("Error converting key '%s'", keyId);
        return LE_FAULT;
    }

    return pa_iks_CreateKeyByType(fullKeyIdBuf, keyType, keySize, keyRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the key type.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetKeyType
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    le_iks_KeyType_t*   keyTypePtr  ///< [OUT] Key type.
)
{
    int32_t keyType = 0;

    LE_ASSERT(keyTypePtr != NULL);
    le_result_t rc = pa_iks_GetKeyType(keyRef, &keyType);
    *keyTypePtr = (le_iks_KeyType_t) keyType;

    return rc;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the key size in bytes.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetKeySize
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint32_t*           keySizePtr  ///< [OUT] Key size.
)
{
    return pa_iks_GetKeySize(keyRef, keySizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the key size is valid.
 *
 * @return
 *      LE_OK if the key size is valid.
 *      LE_OUT_OF_RANGE if the key size is invalid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_IsKeySizeValid
(
    le_iks_KeyType_t    keyType,    ///< [IN] Key type.
    uint32_t            keySize     ///< [IN] Key size in bytes.
)
{
    return pa_iks_IsKeySizeValid(keyType, keySize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the key has a value.
 *
 * @return
 *      LE_OK if the key has a value.
 *      LE_BAD_PARAMETER if the key reference is invalid.
 *      LE_NOT_FOUND if the key has no value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_HasKeyValue
(
    uint64_t    keyRef          ///< [IN] Key reference.
)
{
    return pa_iks_HasKeyValue(keyRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set an update key for the specified key.  The update key must be of type KEY_TYPE_KEY_UPDATE.
 * The update key can be used at a later time to perform authenticated updates of the specified key.
 * The same update key may be used for multiple keys and digests.
 *
 * The key can be made not updatable by setting the updateKeyRef parameter to zero.  Warning, this
 * is a one-time, one-way operation.
 *
 * @note
 *      Once an update key is assigned the key parameters can no longer be modified except through
 *      an authenticated update process.
 *
 *      Update keys can be assigned to themselves or other update keys.
 *
 * @warning
 *      It is strongly recommended to save the update key before assigning it to other keys/digests.
 *      Otherwise a sudden power loss could leave the update key reference pointing to a
 *      non-existing update key allowing a new update key to be created with the same ID but a
 *      different (unintended) value.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the update key reference is invalid or does not have a value.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an update key has already been set
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_SetKeyUpdateKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    uint64_t        updateKeyRef    ///< [IN] Reference to an update key. 0 for not updatable.
)
{
    return pa_iks_SetKeyUpdateKey(keyRef, updateKeyRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a key value.
 *
 * If the specified key has an assigned update key then the authCmdPtr must contain a generate key
 * command and a valid authentication challenge, obtained by le_iks_GetUpdateAuthChallenge(),
 * and is signed with the update private key.  If the command is valid and authentic then a new key
 * value is generated replacing the old value.
 *
 * If the specified key does not have an update key then the authCmdPtr is ignored.
 *
 * Public keys cannot be generated using this function.  They must be provisioned using
 * le_iks_ProvisionKeyValue().
 *
 * @note
 *      See the comment block at the top of this page for the authenticated command format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key is a public key.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there is an update key set and the authCmdPtr does not contain a
 *               valid authenticated command
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GenKeyValue
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
)
{
    return pa_iks_GenKeyValue(keyRef, authCmdPtr, authCmdSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Provision a key value.
 *
 * The provisioning package, provPackagePtr, must contain the key value to provision.
 *
 * Private key provisioning is not currently supported.
 *
 * If the key is a symmetric then the key value must be encrypted with the provisioning key.  If the
 * key is a public key the key value must be provided in plaintext.
 *
 * If the specified key does not have an assigned update key then the provPackagePtr is treated as a
 * buffer containing the key value.
 *
 * If the specified key has an assigned update key then the provPackagePtr must also contain a valid
 * authentication challenge and be signed with the assigned update key.
 *
 * @note
 *      See the comment block at the top of this page for the provisioning package format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the provPackagePtr is not validly encrypted and/or signed
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ProvisionKeyValue
(
    uint64_t        keyRef,             ///< [IN] Key reference.
    const uint8_t*  provPackagePtr,     ///< [IN] Provisioning package.
    size_t          provPackageSize     ///< [IN] Provisioning package size.
)
{
    return pa_iks_ProvisionKeyValue(keyRef, provPackagePtr, provPackageSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Saves a key to persistent storage.
 *
 * @note
 *      Previously saved keys that have been updated do not need to be re-saved.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the key is already in persistent storage
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_SaveKey
(
    uint64_t        keyRef  ///< [IN] Key reference.
)
{
    return pa_iks_SaveKey(keyRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete key.
 *
 * If the specified key has an assigned update key then the authCmdPtr must contain a delete key
 * command and a valid authentication challenge, obtained by le_iks_GetUpdateAuthChallenge(), and is
 * signed with the update private key.  If the command is valid and authentic then the key will be
 * deleted.
 *
 * If the specified key does not have an assigned update key then then authCmdPtr is ignored.
 *
 * @warning
 *      When deleting an update key, it is a good idea to delete all keys that depend on the update
 *      key first.  Otherwise the dependent keys will be left non-updatable.
 *
 * @note
 *      See the comment block at the top of this page for the authenticated command format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the key has an update key and the authCmdPtr is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_DeleteKey
(
    uint64_t        keyRef,         ///< [IN] Key reference.
    const uint8_t*  authCmdPtr,     ///< [IN] Authenticated command buffer.
    size_t          authCmdSize     ///< [IN] Authenticated command buffer size.
)
{
    return pa_iks_DeleteKey(keyRef, authCmdPtr, authCmdSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the public portion of an asymmetric key.
 *
 * The output will be in:
 *      - PKCS #1 format (DER encoded) for RSA keys.
 *      - ECPoint format defined in RFC5480 for ECC keys.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key is not an asymmetric key.
 *      LE_NOT_FOUND if the key reference does not have a value.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_OVERFLOW if the supplied buffer is too small to hold the key value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetPubKeyValue
(
    uint64_t        keyRef,     ///< [IN] Key reference.
    uint8_t*        bufPtr,     ///< [OUT] Buffer to hold key value.
    size_t*         bufSizePtr  ///< [INOUT] Key value buffer size.
)
{
    return pa_iks_GetPubKeyValue(keyRef, bufPtr, bufSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to a digest.
 *
 * Digest IDs may only consist of alphanumeric characters, the underscore '_' and hyphen '-'.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digestId is invalid or if digestRef is NULL.
 *      LE_NOT_FOUND if the digest does not exist.
 *      LE_NO_MEMORY if there is not enough memory to retrieve the digest.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetDigest
(
    const char* digestId,       ///< [IN] Identifier string.
    uint64_t*   digestRefPtr    ///< [OUT] Digest reference.
)
{
    return pa_iks_GetDigest(digestId, digestRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new digest.
 *
 * New digests initially have no value.  Digest values can be set using le_iks_ProvisionDigest().
 *
 * Created digests initially only exist in non-persistent memory, call le_iks_SaveDigest() to save
 * to persistent storage.
 *
 * Digest IDs may only consist of alphanumeric characters, the underscore '_' and hyphen '-'.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digestId is invalid or if digestRef is NULL.
 *      LE_DUPLICATE if the digestId is already being used.
 *      LE_OUT_OF_RANGE if the digest size is invalid.
 *      LE_NO_MEMORY if there is not enough memory to create the digest.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_CreateDigest
(
    const char*     digestId,       ///< [IN] Identifier string.
    uint32_t        digestSize,     ///< [IN] Digest size. Must be <= MAX_DIGEST_SIZE.
    uint64_t*       digestRefPtr    ///< [OUT] Digest reference.
)
{
    return pa_iks_CreateDigest(digestId, digestSize, digestRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the digest size in bytes.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetDigestSize
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    uint32_t*           digestSizePtr   ///< [OUT] Digest size.
)
{
    return pa_iks_GetDigestSize(digestRef, digestSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set an update key for the specified digest.  The update key must be of type
 * KEY_TYPE_KEY_UPDATE.  The update key can be used at a later time to perform authenticated
 * updates of the specified digest.  The same update key may be used for multiple keys and digests.
 *
 * The digest can be made not updatable by setting the updateKeyRef parameter to zero.  Warning,
 * this is a one-time, one-way operation.
 *
 * @note
 *      Once an update key is assigned the digest parameters can no longer be modified except
 *      through an authenticated update process.
 *
 * @warning
 *      It is strongly recommended to save the update key before assigning it to other keys/digests.
 *      Otherwise a sudden power loss could leave the update key reference pointing to a
 *      non-existing update key allowing a new update key to be created with the same ID but a
 *      different (unintended) value.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid
 *                       or if the update key reference is invalid or does not have a value.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if an update key has already been set
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_SetDigestUpdateKey
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    uint64_t            updateKeyRef    ///< [IN] Reference to an update key. 0 for not updatable.
)
{
    return pa_iks_SetDigestUpdateKey(digestRef, updateKeyRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Provision a digest value.
 *
 * The provisioning package, provPackagePtr, must contain the digest value to provision.
 *
 * If the specified digest does not have an assigned update key then the provPackagePtr is treated
 * as a buffer containing the digest value.
 *
 * If the specified digest has an assigned update key then the provPackagePtr must also contain a
 * valid authentication challenge and be signed with the assigned update key.
 *
 * @note
 *      See the comment block at the top of this page for the provisioning package format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid
 *                       or if the digest value is too long.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the provPackagePtr does not have a valid signature
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_ProvisionDigest
(
    uint64_t            digestRef,      ///< [IN] Digest reference.
    const uint8_t*      provPackagePtr, ///< [IN] Provisioning package.
    size_t              provPackageSize ///< [IN] Provisioning package size.
)
{
    return pa_iks_ProvisionDigest(digestRef, provPackagePtr, provPackageSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Saves a digest to persistent storage.
 *
 * @note
 *      Previously saved digests that have been updated do not need to be re-saved.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the digest is already in persistent storage
 *               or if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_SaveDigest
(
    uint64_t            digestRef   ///< [IN] Digest reference.
)
{
    return pa_iks_SaveDigest(digestRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a digest.
 *
 * If the specified digest has an assigned update key then the authCmdPtr must contain a delete
 * digest command and a valid authentication challenge, obtained by le_iks_GetUpdateAuthChallenge()
 * and is signed with the update private key.  If the command is valid and authentic then the digest
 * will be deleted.
 *
 * If the specified digest does not have an assigned update key then then authCmdPtr is ignored.
 *
 * @note
 *      See the comment block at the top of this page for the authenticated command format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if the digest has an update key and the authCmdPtr is not valid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_DeleteDigest
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    const uint8_t*      authCmdPtr, ///< [IN] Authenticated command buffer.
    size_t              authCmdSize ///< [IN] Authenticated command buffer size.
)
{
    return pa_iks_DeleteDigest(digestRef, authCmdPtr, authCmdSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the digest value.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the digest reference is invalid.
 *      LE_NOT_FOUND if the digest reference does not have a value.
 *      LE_OVERFLOW if the supplied buffer is too small to hold the digest value.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetDigestValue
(
    uint64_t            digestRef,  ///< [IN] Digest reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the digest value.
    size_t*             bufSizePtr  ///< [INOUT] Size of the buffer.
)
{
    return pa_iks_GetDigestValue(digestRef, bufPtr, bufSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get update authentication challenge.
 *
 * This challenge code must be included in any update commands created using the specified update
 * key.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the update key reference is invalid.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there is an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetUpdateAuthChallenge
(
    uint64_t            keyRef,     ///< [IN] Key reference.
    uint8_t*            bufPtr,     ///< [OUT] Buffer to hold the authentication challenge.
    size_t*             bufSizePtr  ///< [INOUT] Size of the authentication challenge buffer.
)
{
    return pa_iks_GetUpdateAuthChallenge(keyRef, bufPtr, bufSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the wrapping key.  This is a public key that is internally generated by the IOT Key Store
 * and used to encrypt (wrap) symmetric and private keys for provisioning into the IOT Key Store.
 * This key can only be used for this purpose.
 *
 * @note
 *      The key is provided in ASN.1 structured DER encoded format.  Refer to the comment at the
 *      top of this file for details of the file format.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the supplied buffer is too small.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there is an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_GetWrappingKey
(
    uint8_t*    bufPtr,     ///< [OUT] Buffer to hold the wrapping key.
    size_t*     bufSizePtr  ///< [INOUT] Size of the buffer to hold the wrapping key.
)
{
    return pa_iks_GetWrappingKey(bufPtr, bufSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a session.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the key reference is invalid
 *                       or if the key does not contain a key value or sessionRef is NULL.
 *      LE_NO_MEMORY if there is not enough memory to create the session.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_CreateSession
(
    uint64_t    keyRef,         ///< [IN] Key reference.
    uint64_t*   sessionRefPtr   ///< [OUT] Session reference.
)
{
    return pa_iks_CreateSession(keyRef, sessionRefPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a session.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if the session reference is invalid.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_DeleteSession
(
    uint64_t            sessionRef  ///< [IN] Session reference.
)
{
    return pa_iks_DeleteSession(sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
