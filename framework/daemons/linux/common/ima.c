//--------------------------------------------------------------------------------------------------
/**
 * @file ima.c
 *
 * This file implements functions that can be used to import IMA keys (into the kernel keyring) and
 * verify IMA signatures.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "file.h"
#include "fileDescriptor.h"
#include "sysPaths.h"
#include "ima.h"
#include "file.h"
#include "smack.h"

#if LE_CONFIG_ENABLE_IMA

#include <openssl/x509.h>


//--------------------------------------------------------------------------------------------------
/**
 * Max size of IMA command
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CMD_BYTES   4096


//--------------------------------------------------------------------------------------------------
/**
 * Path to evmctl tool. It can be used for producing and verifying IMA signatures. It can be also
 * used to import keys into the kernel keyring.
 */
//--------------------------------------------------------------------------------------------------
#define EVMCTL_PATH   "/usr/bin/evmctl"


//--------------------------------------------------------------------------------------------------
/**
 * Evmctl option to check certificate expiry
 */
//--------------------------------------------------------------------------------------------------
#define CHECK_EXPIRY_OPTION   "--check_expiry"


//--------------------------------------------------------------------------------------------------
/**
 * Verify a file IMA signature against provided public certificate path
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VerifyFile
(
    const char * filePath,
    const char * certPath
)
{
    char cmd[MAX_CMD_BYTES] = "";

    snprintf(cmd, sizeof(cmd), "%s ima_verify %s -k %s ", EVMCTL_PATH, filePath, certPath);

    LE_DEBUG("Verify file command: %s", cmd);

    int exitCode = system(cmd);

    if (WIFEXITED(exitCode) && (0 == WEXITSTATUS(exitCode)))
    {
        LE_DEBUG("Verified file: '%s' successfully", filePath);
        return LE_OK;
    }
    else
    {
        LE_ERROR("Failed to verify file '%s' with certificate '%s', exitCode: %d",
                 filePath, certPath, WEXITSTATUS(exitCode));
        return LE_FAULT;
    }
}


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
static le_result_t VerifyDir
(
    const char * dirPath,
    const char * certPath
)
{
    char* pathArrayPtr[] = {(char *)dirPath,
                                    NULL};

    // Open the directory tree to traverse.
    FTS* ftsPtr = fts_open(pathArrayPtr,
                           FTS_PHYSICAL,
                           NULL);

    if (NULL == ftsPtr)
    {
        LE_CRIT("Could not access dir '%s'.  %m.", pathArrayPtr[0]);
        return LE_FAULT;
    }

    // Traverse through the directory tree.
    FTSENT* entPtr;
    while (NULL != (entPtr = fts_read(ftsPtr)))
    {
        LE_DEBUG("Filename: %s, filePath: %s, rootPath: %s, fts_info: %d", entPtr->fts_name,
                                entPtr->fts_accpath,
                                entPtr->fts_path,
                                entPtr->fts_info);

        switch (entPtr->fts_info)
        {
            case FTS_SL:
            case FTS_SLNONE:
                break;

            case FTS_F:
                if (0 != strcmp(entPtr->fts_name, PUB_CERT_NAME ))
                {
                    if (LE_OK != VerifyFile(entPtr->fts_accpath, certPath))
                    {
                        LE_CRIT("Failed to verify file '%s' with public certificate '%s'",
                                entPtr->fts_accpath,
                                certPath);
                        fts_close(ftsPtr);
                        return LE_FAULT;
                    }
                }
                break;
        }

    }

    fts_close(ftsPtr);

    return LE_OK;
}


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
static le_result_t ImportPublicCert
(
    const char * certPath,
    bool checkExpiry
)
{

    char cmd[MAX_CMD_BYTES] = "";
    char checkExpiryOption[LIMIT_MAX_ARGS_STR_BYTES] = "";

    if (checkExpiry)
    {
        strncpy(checkExpiryOption, CHECK_EXPIRY_OPTION, sizeof(checkExpiryOption)- 1);
    }

    snprintf(cmd, sizeof(cmd), "SECFS=/sys/kernel/security && "
             "grep -q  $SECFS /proc/mounts || mount -n -t securityfs securityfs $SECFS && "
             "ima_id=\"`awk '/\\.ima/ { printf \"%%d\", \"0x\"$1; }' /proc/keys`\" && "
             "%s %s import %s $ima_id", EVMCTL_PATH, checkExpiryOption, certPath);

    LE_DEBUG("cmd: %s", cmd);

    int pid = fork();

    if (pid == 0)
    {
        // Import the keys as '_' label
        smack_SetMyLabel("_");

        int exitCode = system(cmd);

        if (WIFEXITED(exitCode) && (0 == WEXITSTATUS(exitCode)))
        {
            LE_DEBUG("Installed certificate: '%s' successfully", certPath);
            exit(EXIT_SUCCESS);
        }
        else
        {
            LE_ERROR("Failed to import certificate '%s', exitCode: %d",
                     certPath,
                     WEXITSTATUS(exitCode));
            exit(EXIT_FAILURE);
        }
    }

    int status;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == EXIT_FAILURE)
    {
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify a file IMA signature against provided public certificate path
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_VerifyFile
(
    const char * filePath,
    const char * certPath
)
{
    return VerifyFile(filePath, certPath);
}


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
)
{
    return VerifyDir(dirPath, certPath);
}


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
)
{

    int exitCode = system("(zcat /proc/config.gz | grep CONFIG_IMA=y) &&"
                          " (cat /proc/cmdline | grep \"ima_appraise=enforce\")");

    return WIFEXITED(exitCode) && (0 == WEXITSTATUS(exitCode));
}


//--------------------------------------------------------------------------------------------------
/**
 * Import IMA public certificate to linux keyring. Public certificate must be signed by system
 * private key to import it properly. Only privileged process with right permission and smack
 * label will be able to do that.
 *
 * @return
 *      - LE_OK if imports properly
 *      - LE_FAULT if fails to import
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_ImportPublicCert
(
    const char * certPath
)
{
    if (LE_OK != ImportPublicCert(certPath, true))
    {
        // Importing certificate with check_expiry option failed. Now try to import certificate
        // without check_expiry option.
        FILE *fp;
        X509 *crt = NULL;
        int checkBefore = 0;
        int checkAfter = 0;

        fp = fopen(certPath, "r");
        if (!fp) {
            LE_ERROR("Failed to open certificate: %s\n", certPath);
            return LE_FAULT;
        }

        crt = d2i_X509_fp(fp, NULL);
        if (!crt) {
            LE_ERROR("d2i_X509_fp() failed\n");
            fclose(fp);
            return LE_FAULT;
        }

        checkBefore = X509_cmp_current_time(X509_get_notBefore(crt));
        checkAfter = X509_cmp_current_time(X509_get_notAfter(crt));
        X509_free(crt);
        fclose(fp);

        if (checkBefore > 0 || checkAfter < 0)
        {
            LE_WARN("Certificate '%s' expired. Retrying without %s option",
                     certPath, CHECK_EXPIRY_OPTION);
            return ImportPublicCert(certPath, false);
        }

        return LE_FAULT;
    }

    return LE_OK;
}

#else /* !LE_CONFIG_ENABLE_IMA */

//--------------------------------------------------------------------------------------------------
/**
 * Verify a file IMA signature against provided public certificate path
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_VerifyFile
(
    const char * filePath,
    const char * certPath
)
{
    return LE_FAULT;
}

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
)
{
    return LE_FAULT;
}

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
)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Import IMA public certificate to linux keyring. Public certificate must be signed by system
 * private key to import it properly. Only privileged process with right permission and smack
 * label will be able to do that.
 *
 * @return
 *      - LE_OK if imports properly
 *      - LE_FAULT if fails to import
 */
//--------------------------------------------------------------------------------------------------
le_result_t ima_ImportPublicCert
(
    const char * certPath
)
{
    return LE_FAULT;
}

#endif /* LE_CONFIG_ENABLE_IMA */
