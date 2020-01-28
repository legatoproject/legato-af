/** @file iksAesMilenage.c
 *
 * Legato API for performing Milenage authentication and key derivation using AES as
 * the PRP.
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
 * Calculates the network authentication code MAC-A using the Milenage algorithm set with AES-128 as
 * the block cipher.  Implements the Milenage function f1.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if either K or OPc reference is invalid
 *                       or if either K or OPc key type is invalid
 *                       or if either randPtr, amfPtr, sqnPtr, or macaPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesMilenage_GetMacA
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_RAND_SIZE.
    const uint8_t*  amfPtr,         ///< [IN] Authentication management field, AMF.
    size_t          amfSize,        ///< [IN] AMF size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_AMF_SIZE.
    const uint8_t*  sqnPtr,         ///< [IN] Sequence number, SQN.
    size_t          sqnSize,        ///< [IN] SQN size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_SQN_SIZE.
    uint8_t*        macaPtr,        ///< [OUT] Buffer to hold the network authentication code.
    size_t*         macaSizePtr     ///< [OUT] Network authentication code size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_MACA_SIZE.
)
{
    return pa_iks_aesMilenage_GetMacA(kRef, opcRef, randPtr, randSize, amfPtr, amfSize,
                                      sqnPtr, sqnSize, macaPtr, macaSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Calculates the re-synchronisation authentication code MAC-S using the Milenage algorithm set with
 * AES-128 as the block cipher.  Implements the Milenage function f1*.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if either K or OPc reference is invalid
 *                       or if either K or OPc key type is invalid
 *                       or if either randPtr, amfPtr, sqnPtr, or macsPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesMilenage_GetMacS
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_RAND_SIZE.
    const uint8_t*  amfPtr,         ///< [IN] Authentication management field, AMF.
    size_t          amfSize,        ///< [IN] AMF size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_AMF_SIZE.
    const uint8_t*  sqnPtr,         ///< [IN] Sequence number, SQN.
    size_t          sqnSize,        ///< [IN] SQN size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_SQN_SIZE.
    uint8_t*        macsPtr,        ///< [OUT] Buffer to hold the re-sync authentication code.
    size_t*         macsSizePtr     ///< [OUT] Re-sync authentication code size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_MACS_SIZE.
)
{
    return pa_iks_aesMilenage_GetMacS(kRef, opcRef, randPtr, randSize, amfPtr, amfSize,
                                      sqnPtr, sqnSize, macsPtr, macsSizePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Derives authentication response and keys using the Milenage algorithm set with AES-128 as the
 * block cipher.  Implements the Milenage functions f2, f3, f4, f5.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if either K or OPc reference is invalid
 *                       or if either K or OPc key type is invalid
 *                       or if either randPtr, resPtr, ckPtr, ikPtr or akPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesMilenage_GetKeys
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_RAND_SIZE.
    uint8_t*        resPtr,         ///< [OUT] Buffer to hold the authentication response RES.
    size_t*         resSize,        ///< [OUT] RES size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_RES_SIZE.
    uint8_t*        ckPtr,          ///< [OUT] Buffer to hold the confidentiality key CK.
    size_t*         ckSize,         ///< [OUT] CK size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_CK_SIZE.
    uint8_t*        ikPtr,          ///< [OUT] Buffer to hold the integrity key IK.
    size_t*         ikSize,         ///< [OUT] IK size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_IK_SIZE.
    uint8_t*        akPtr,          ///< [OUT] Buffer to hold the anonymity key AK.
    size_t*         akSize          ///< [OUT] AK size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_AK_SIZE.
)
{
    return pa_iks_aesMilenage_GetKeys(kRef, opcRef, randPtr, randSize, resPtr, resSize,
                                      ckPtr, ckSize, ikPtr, ikSize, akPtr, akSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Derives the anonymity key for the re-synchronisation message using the Milenage algorithm set
 * with AES-128 as the block cipher.  Implements the Milenage functions f5*.
 *
 * @return
 *      LE_OK if successful.
 *      LE_BAD_PARAMETER if either K or OPc reference is invalid
 *                       or if either K or OPc key type is invalid
 *                       or if either randPtr or akPtr is NULL.
 *      LE_UNSUPPORTED if underlying resource does not support this operation.
 *      LE_FAULT if there was an internal error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_iks_aesMilenage_GetAk
(
    uint64_t        kRef,           ///< [IN] Reference to K.
    uint64_t        opcRef,         ///< [IN] Reference to OPc.
    const uint8_t*  randPtr,        ///< [IN] RAND challenge.
    size_t          randSize,       ///< [IN] RAND size.
                                    ///<      Expected to be LE_IKS_AESMILENAGE_RAND_SIZE.
    uint8_t*        akPtr,          ///< [OUT] Buffer to hold the anonymity key AK.
    size_t*         akSize          ///< [OUT] AK size.
                                    ///<       Expected to be LE_IKS_AESMILENAGE_AK_SIZE.
)
{
    return pa_iks_aesMilenage_GetAk(kRef, opcRef, randPtr, randSize, akPtr, akSize);
}
