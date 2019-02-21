//--------------------------------------------------------------------------------------------------
/**
 * @file mkTools.h
 *
 * Header that includes all the other common header files used by multiple parts of the mk tools.
 *
 * This is done partly as a convenience for the programmer (so they don't have to include as many
 * files) and partly as a performance improvement measure (because this file can be pre-compiled).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_H_INCLUDE_GUARD

#include "defTools.h"

#include "buildScriptGenerator/buildScriptGenerator.h"
#include "codeGenerator/codeGenerator.h"
#include "configGenerator/configGenerator.h"
#include "adefGenerator/exportedAdefGenerator.h"
#include "generators.h"
#include "targetInfo/targetInfo.h"

#endif  // LEGATO_MKTOOLS_H_INCLUDE_GUARD
