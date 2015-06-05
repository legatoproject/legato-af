/** @file le_audio_local.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
#define LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of format stream related field.
 */
//--------------------------------------------------------------------------------------------------
#define FORMAT_NAME_MAX_LEN             30
#define FORMAT_NAME_MAX_BYTES           (FORMAT_NAME_MAX_LEN+1)

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
    char             format[FORMAT_NAME_MAX_BYTES]; ///< The name of the audio encoding as used by
                                                    ///  the Real-Time Protocol (RTP), specified by
                                                    ///  the IANA organisation.
    uint32_t         gain;                          ///< Gain
    int32_t          fd;                            ///< Audio file descriptor for playback or capture
    le_hashmap_Ref_t connectorList;                 ///< list of connectors to which the audio
                                                    ///  stream is tied to.
    le_dls_List_t    streamRefWithEventHdlrList;    ///< List containing information related to
                                                    ///  stream event handlers
    le_event_Id_t    streamEventId;                 ///< Event ID to report stream events
    le_audio_StreamRef_t streamRef;                 ///< Stream reference
    pa_audio_SamplePcmConfig_t  samplePcmConfig;    ///< Sample PCM configuration
    le_dls_List_t    sessionRefList;                ///< Clients sessionRef list

} le_audio_Stream_t;

#endif // LEGATO_LEAUDIOLOCAL_INCLUDE_GUARD
