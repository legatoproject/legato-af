/**
 * This module implements some stubs for the voice call service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * le_audio_OpenModemVoiceTx() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceTx
(
    void
)
{
    return (le_audio_StreamRef_t) 0x100A;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_audio_OpenModemVoiceRx() stub.
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t le_audio_OpenModemVoiceRx
(
    void
)
{
    return (le_audio_StreamRef_t) 0x100B;
}
