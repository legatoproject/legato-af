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
 *      - Number of expected resets
 */
//--------------------------------------------------------------------------------------------------
int64_t sysResets_GetExpectedResetsCount
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of unexpected resets
 *
 * @return
 *      - Number of unexpected resets
 */
//--------------------------------------------------------------------------------------------------
int64_t sysResets_GetUnexpectedResetsCount
(
    void
);

#endif /* _SYSRESETS_H */
