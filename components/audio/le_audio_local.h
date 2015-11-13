/** @file le_audio_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
#define LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Audio stream structure.
 * Objects of this type are used to define an audio stream.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_audio_Stream {
    bool             isInput;                       ///< Flag to specify if the stream is an input
                                                    ///  or output.
    pa_audio_If_t    audioInterface;                ///< Audio interface identifier.
    uint32_t         gain;                          ///< Gain
    int32_t          fd;                            ///< Audio file descriptor for playback or
    le_hashmap_Ref_t connectorList;                 ///< capture list of connectors to which the
                                                    ///  audio stream is tied to.
    le_dls_List_t    streamRefWithEventHdlrList;    ///< List containing information related to
                                                    ///  stream event handlers
    le_event_Id_t    streamEventId;                 ///< Event ID to report stream events
    le_audio_StreamRef_t streamRef;                 ///< Stream reference
    pa_audio_SamplePcmConfig_t  samplePcmConfig;    ///< Sample PCM configuration
    le_dls_List_t    sessionRefList;                ///< Clients sessionRef list
    pa_audio_SampleAmrConfig_t  sampleAmrConfig;    ///< Sample AMR configuration
    le_audio_Format_t   encodingFormat;             ///< Audio encoding format
    le_thread_Ref_t     amrThreadRef;               ///< AMR playback/capture thread reference
} le_audio_Stream_t;

#endif // LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
