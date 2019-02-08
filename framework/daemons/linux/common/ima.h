//--------------------------------------------------------------------------------------------------
/**
 * @file ima.h
 *
 * Provides interfaces that can be used to import IMA keys (into the kernel keyring) and
 * verify IMA signatures.
 *
 * This file exposes interfaces that are for use by highly privileged framework daemons
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_IMA_H_INCLUDE_GUARD
#define LEGATO_IMA_H_INCLUDE_GUARD



//--------------------------------------------------------------------------------------------------
/**
 * Name of IMA public certificate.
 */
//--------------------------------------------------------------------------------------------------
#define PUB_CERT_NAME    "ima_pub.cert"

//--------------------------------------------------------------------------------------------------
/**
 * Smack label used for protecting data files
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_CONFIG_IMA_SMACK
#   define LE_CONFIG_IMA_SMACK  "imaLegato"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Check whether current linux kernel is IMA-enabled or not.
 *
 * @return
 *      - true if IMA is enabled.
 *      - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool ima_IsEnabled
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Import IMA public certificate to linux keyring. Public certificate must be signed by system
 * private key to import it properly.
 *
 * @return
 *      - LE_OK if imports properly
 *      - LE_FAULT if fails to import
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_ImportPublicCert
(
    const char * certPath
);


//--------------------------------------------------------------------------------------------------
/**
 * Verify a file IMA signature against provided public certificate path
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t  ima_VerifyFile
(
    const char * filePath,
    const char * certPath
);


//--------------------------------------------------------------------------------------------------
/**
 * Recursively traverse the directory and verify each file IMA signature against provided public
 * certificate path
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_VerifyDir
(
    const char * dirPath,
    const char * certPath
);

#endif // LEGATO_IMA_H_INCLUDE_GUARD
