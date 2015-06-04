//--------------------------------------------------------------------------------------------------
/**
 * @file mkCommon.h
 *
 * Functions shared by different command-line tools.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Run the Ninja build tool.  Executes the build.ninja script in the root of the working directory
 * tree, if it exists.
 *
 * If the build.ninja file exists, this function will never return.  If the build.ninja file doesn't
 * exist, then this function WILL return.
 */
//--------------------------------------------------------------------------------------------------
void RunNinja
(
    const mk::BuildParams_t& buildParams
);


#endif // LEGATO_MKTOOLS_MK_COMMON_H_INCLUDE_GUARD
