/**
 * @file sysResets.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _SYSRESETS_H
#define _SYSRESETS_H

#include <stdint.h>
#include <legato.h>

//--------------------------------------------------------------------------------------------------
/**
 * Init system resets counter
 *
 * @return
 *      - LE_OK           Init succeded
 *      - LE_FAULT        Init failed
 *      - LE_UNSUPPORTED  If not supported by the platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of expected resets
 *
 * @return
 *  - LE_OK             The function succeeded.
    - LE_UNSUPPORTED    If not supported by the platform
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_GetExpectedResetsCount
(
    uint64_t* expectedPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of unexpected resets
 *
 * @return
 *  - LE_OK             The function succeeded.
    - LE_UNSUPPORTED    If not supported by the platform
 *  - LE_FAULT          The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t sysResets_GetUnexpectedResetsCount
(
    uint64_t* unexpectedPtr
);

#endif /* _SYSRESETS_H */
