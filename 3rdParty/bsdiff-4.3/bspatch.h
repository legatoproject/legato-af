/**
 * @file bspatch.h
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#ifndef BSPATCH_INCLUDE_GUARD
#define BSPATCH_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * This function apply a delta patch to an origin image and write it to a destination image
 *
 * @return
 *      - LE_OK       Patch is successfully applied
 */
//--------------------------------------------------------------------------------------------------
le_result_t bsPatch
(
    pa_patch_Context_t *patchContextPtr,
                            ///< [IN] Context for the patch
    char *patchfile,        ///< [IN] File containing the patch
    uint32_t *crc32Ptr,     ///< [OUT] Pointer to return the CRC32 of the patch applied
    bool lastPatch,         ///< [IN] True if this is the last patch in this context
    bool forceClose         ///< [IN] Force close of device and resources
);

#endif // BSPATCH_INCLUDE_GUARD
