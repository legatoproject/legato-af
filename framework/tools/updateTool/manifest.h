// -------------------------------------------------------------------------------------------------
/**
 * @file manifest.h
 *
 * Provides APIs for parsing manifest of install/uninstall file. Detailed description provided in
 * manifest.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef PARSE_MANIFEST_INCLUDE_GUARD
#define PARSE_MANIFEST_INCLUDE_GUARD

/// Maximum number of tokens allowed in command field of manifest string
#define MAN_MAX_TOKENS_IN_CMD_STR         23

/// Maximum allowed size of tokens
#define MAN_MAX_CMD_TOKEN_LEN            (size_t)64
#define MAN_MAX_CMD_TOKEN_BYTES          (MAN_MAX_CMD_TOKEN_LEN + 1)

/// Reference to a manifest.
typedef struct Manifest* man_Ref_t;

//--------------------------------------------------------------------------------------------------
/**
 * Creates a manifest object.
 *
 * @return
 *      Reference to a manifest object.
 */
//--------------------------------------------------------------------------------------------------
man_Ref_t man_Create
(
    int fileDesc                          ///<[IN] File descriptor containing the update package.
);

//--------------------------------------------------------------------------------------------------
/**
 * Deletes provided manifest object.
 */
//--------------------------------------------------------------------------------------------------
void man_Delete
(
    man_Ref_t manObj                      ///<[IN] Reference to a manifest object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to get payload size from manifest.
 *
 * @return
 *      Size of PayLoad
 */
//--------------------------------------------------------------------------------------------------
size_t man_GetPayLoadSize
(
    man_Ref_t manObj                      ///<[IN] Reference to a Manifest_t object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Extracts command and its parameters from provided manifest and copy them to provided buffer.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t man_GetCmd
(
    man_Ref_t manifest,                       ///<[IN] Manifest to extract command string.
    char (*cmdList)[MAN_MAX_CMD_TOKEN_BYTES], ///<[OUT] Buffer to store command & its params.
    int *cmdParamsNum                         ///<[OUT] Destination to store no. of tokens.
);

#endif                             // Endif for PARSE_MANIFEST_INCLUDE_GUARD
