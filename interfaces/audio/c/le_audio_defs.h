/** @page c_audio_apis Audio Service
 *
 * This file contains common definitions for the Audio APIs and Audio Platform Adapter APIs.
 *
 * High-Level Audio APIs @subpage c_audio
 *
 */

/** @file le_audio_defs.h
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved.
 */

#ifndef LEGATO_AUDIODEFS_INCLUDE_GUARD
#define LEGATO_AUDIODEFS_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * I2S channel mode.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_AUDIO_I2S_LEFT    = 0, ///< Left channel.
    LE_AUDIO_I2S_RIGHT   = 1, ///< Right channel.
    LE_AUDIO_I2S_MONO    = 2, ///< Mono mode.
    LE_AUDIO_I2S_STEREO  = 3, ///< Stereo mode.
    LE_AUDIO_I2S_REVERSE = 4, ///< Reverse mode (left & right reversed).
}
le_audio_I2SChannel_t;

#endif // LEGATO_AUDIODEFS_INCLUDE_GUARD
