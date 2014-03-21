/** @file pa_audio_local.h
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PAAUDIOLOCAL_INCLUDE_GUARD
#define LEGATO_PAAUDIOLOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Define the Audio device.
 */
//--------------------------------------------------------------------------------------------------
#define AUDIO_QUALCOMM_DEVICE_PATH  "/dev/snd/controlC0"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set a mixer value
 *
 */
//--------------------------------------------------------------------------------------------------
void SetMixerParameter
(
    const char* namePtr,
    const char* valuePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get a mixer value
 *
 */
//--------------------------------------------------------------------------------------------------
void GetMixerParameter
(
    const char* namePtr,
    unsigned*   valuePtr
);

#endif // LEGATO_PAAUDIOLOCAL_INCLUDE_GUARD
