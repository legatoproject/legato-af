//--------------------------------------------------------------------------------------------------
/**
 * @file streamMedia.c
 *
 * This file contains the source code of the high level Media Stream API.
 *
 * @warning This sample includes GPLv2 code (PJSIP library). Use of a GPLv2 library results in this
 * sample app being GPLv2 also.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "pjmedia.h"
#include "pjlib-util.h"
#include "pjlib.h"
#include "pjsip.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  All IP addresses on the local machine.
 */
// ------------------------------------------------------------------------------------------------
#define ALL_ADDRESSES "0.0.0.0"

// -------------------------------------------------------------------------------------------------
/**
 *  Local IP address.
 */
// ------------------------------------------------------------------------------------------------
#define LOCAL_ADDRESS "127.0.0.1"

//--------------------------------------------------------------------------------------------------
/**
 * Symbols related to RTCP packets type.
 */
//--------------------------------------------------------------------------------------------------
#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203

//--------------------------------------------------------------------------------------------------
/**
 * Maximum RTP/RTCP Packet Size.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_RTP_PACKET_SIZE 1500

//--------------------------------------------------------------------------------------------------
/**
 * Maximum audio sample size.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_AUDIO_SAMPLE_SIZE 1280

//--------------------------------------------------------------------------------------------------
/**
 * Codec related.
 */
//--------------------------------------------------------------------------------------------------
#define STREAMMEDIA_CLOCK_RATE 8000
#define STREAMMEDIA_BITS_PER_SAMPLE 16
#define STREAMMEDIA_SAMPLE_PER_FRAME 160

//--------------------------------------------------------------------------------------------------
/**
 * RTP session variables. These variables should be allocated dynamically when connecting a RTP
 * stream.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t           streamEventId;

    le_audio_StreamRef_t    receptionPlayerRef; // RTP Reception stream reference.
    le_audio_StreamRef_t    transmissionRecorderRef; // RTP Transmission stream reference.

    pjmedia_rtp_session     pjInRtpSess;        // RTP reception session information.
    pjmedia_rtp_session     pjOutRtpSess;       // RTP transmission session information.
    pjmedia_rtcp_session    pjRtcpSess;         // RTCP session information.

    pjmedia_transport*      pjTransportPtr;     // Transport layer used to send/receive packets.

    pj_caching_pool         pjCp;               // PJMedia caching pool.

    pjmedia_endpt*          pjMedEndptPtr;      // PJMedia endpoint.

    int32_t                 rxPipefd[2];        // Reception pipe, written in by the RTP stream and
                                                // read by Alsa which is routed to the output
                                                // interface.
    int32_t                 txPipefd[2];        // Transmission pipe, written in by Alsa hwich is
                                                // routed to the input interface and read by the RTP
                                                // stream.

    le_thread_Ref_t         TransmitRtpThreadRef; // Transmission RTP thread, sends audio samples.

    bool                    isInit;             // True when the RTP sockets are created.
    bool                    rxOn;               // True when reception is ON, e.g. the received
                                                // samples are transmitted to the output interface.
    bool                    txOn;               // True when transmission is ON, e.g. the data from
                                                // the input interface is sent trhough RTP.
}RtpSessionCtx_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Static declaration of the structure that contains every RTP related variable.
 */
//--------------------------------------------------------------------------------------------------
static RtpSessionCtx_t RtpSession;

//--------------------------------------------------------------------------------------------------
/**
 *  Play Samples thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TransmitRtpSamplesThread
(
    void* contextPtr
)
{
    int32_t len = MAX_AUDIO_SAMPLE_SIZE;
    char data[len];
    int32_t readLen;

    const void*             headerBufferPtr;
    int                     headerLen;
    char                    packet[MAX_RTP_PACKET_SIZE];
    pj_ssize_t              size;
    pj_status_t             status;

    // The audio recorder writes audio samples from its connected interface to a pipe.
    // Read audio samples from the pipe filled by the media player, and send it.
    while ((readLen = read(RtpSession.txPipefd[0], data, len)))
    {
        if (readLen < 0)
        {
            // Exit RTP transmission.
            LE_ERROR("Error reading from transmission pipe : %d. err %m",RtpSession.txPipefd[0]);
            break;
        }
        if (RtpSession.txOn)
        {
            // Build RTP header
            status = pjmedia_rtp_encode_rtp(&RtpSession.pjOutRtpSess, // RTP session
                    0, // Payload type
                    0, // Marker flag
                    readLen, // Payload length
                    STREAMMEDIA_SAMPLE_PER_FRAME, // timestamp length
                    &headerBufferPtr, // RTP packet header
                    &headerLen); // Size of RTP packet header
            if (PJ_SUCCESS == status)
            {
                // Send RTP packet
                pj_memcpy(packet, headerBufferPtr, headerLen);
                pj_memcpy(packet+headerLen, data, readLen);
                size = (pj_ssize_t) (headerLen + readLen);
                status = pjmedia_transport_send_rtp(RtpSession.pjTransportPtr, // Transport
                        packet, // packet
                        size); // size of packet
                if (PJ_SUCCESS != status)
                {
                    LE_ERROR("Error sending RTP packet. readLen:%d", readLen);
                }
            }
            else
            {
                LE_ERROR("Error encoding RTP header.");
            }
        }
    }

    LE_INFO("Exiting RTP transmission thread.");

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * RTP reception handler function.
 * This function decodes the RTP header from the received packet and writes the audio sample into
 * a pipe. This pipe is read by the audio player that is connected to another audio interface.
 *
 */
//--------------------------------------------------------------------------------------------------
static void OnRxRtp
(
    void*       userDataPtr,
    void*       pkt,
    pj_ssize_t  size
)
{
    pj_status_t status;
    const pjmedia_rtp_hdr* headerPtr;
    const void* payloadPtr;
    unsigned payloadLen;

    // Discard packets if RTP reception is off.
    if (!RtpSession.rxOn)
    {
        return;
    }

    status = pjmedia_rtp_decode_rtp(&RtpSession.pjInRtpSess,
                                    pkt, (int) size,
                                    &headerPtr, &payloadPtr, &payloadLen);
    if (PJ_SUCCESS != status)
    {
        LE_ERROR("RTP decoding failed.");
        return;
    }

    if ( write(RtpSession.rxPipefd[1], (char*)payloadPtr, payloadLen) < 0 )
    {
        LE_ERROR("Cannot write in Alsa reception pipe %d. err %m", RtpSession.rxPipefd[1]);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * RTCP reception handler function.
 * Send the event to any RTCP handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void OnRxRtcp
(
    void*       userDataPtr,
    void*       pkt,
    pj_ssize_t  size
)
{
    pj_uint8_t* p;
    pj_uint8_t* pEnd;
    streamMedia_RtcpEvent_t rtcpEvent;

    pjmedia_rtcp_rx_rtcp(&RtpSession.pjRtcpSess, pkt, size);

    p = (pj_uint8_t*)pkt;
    pEnd = p + size;
    while (p < pEnd)
    {
        pjmedia_rtcp_common* common = (pjmedia_rtcp_common*)p;
        unsigned len;

        len = (pj_ntohs((pj_uint16_t)common->length)+1) * 4;
        switch(common->pt)
        {
            case RTCP_SR:
                rtcpEvent = STREAMMEDIA_RTCP_SR;
                le_event_Report(RtpSession.streamEventId, &rtcpEvent,
                                sizeof(rtcpEvent));
                break;
            case RTCP_RR:
                rtcpEvent = STREAMMEDIA_RTCP_RR;
                le_event_Report(RtpSession.streamEventId, &rtcpEvent,
                                sizeof(rtcpEvent));
                break;
            case RTCP_SDES:
                rtcpEvent = STREAMMEDIA_RTCP_SDES;
                le_event_Report(RtpSession.streamEventId, &rtcpEvent,
                                sizeof(rtcpEvent));
                break;
            case RTCP_BYE:
                rtcpEvent = STREAMMEDIA_RTCP_BYE;
                le_event_Report(RtpSession.streamEventId, &rtcpEvent,
                                sizeof(rtcpEvent));
                break;
            default:
                LE_INFO("Received unknown RTCP packet type=%d\n", common->pt);
                break;
        }
        p += len;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer File Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerStreamEventHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    streamMedia_RtcpEvent_t* rtcpEventPtr = (streamMedia_RtcpEvent_t*) reportPtr;

    if (!rtcpEventPtr)
    {
        LE_ERROR("Invalid reference provided!");
        return;
    }

    streamMedia_RtcpEvent_t rtcpEvent = *rtcpEventPtr;
    streamMedia_RtcpHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;
    clientHandlerFunc(RtpSession.receptionPlayerRef,
                      rtcpEvent,
                      le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * This function initializes a RTP session.
 *
 * Initializes PJSIP media endpoint and creates UDP sockets.
 * The UDP sockets are attached to the RTP reception handlers and to the remote peer address.
 *
 * Since both the handlers and the remote address must be specified when attaching the sockets, the
 * remote address is set to localhost when no remote address has been passed in argument.
 * This happens when this function is called by streamMedia_OpenAudioRtpRx() to which no remote
 * address is passed.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t InitRtp
(
    int32_t             localPort,      ///< [IN] Local UDP port to use for RTP
    const char*         remoteAddrPtr,  ///< [IN] Remote address to send RTP
    int32_t             remotePort      ///< [IN] repote RTP port
)
{
    pj_status_t status;
    le_result_t res = LE_OK;
    pj_sockaddr_in remoteAddr;
    pj_str_t hostnameStr;
    pj_str_t remoteAddressStr;

    hostnameStr = pj_str(ALL_ADDRESSES);
    if (NULL == remoteAddrPtr)
    {
        remoteAddressStr = pj_str(LOCAL_ADDRESS);
    }
    else
    {
        remoteAddressStr = pj_str((char*)remoteAddrPtr);
    }

    pj_sockaddr_in_init(&remoteAddr, &remoteAddressStr, remotePort);

    if (!RtpSession.isInit)
    {
        status = pj_init();
        if (PJ_SUCCESS != status)
        {
            LE_ERROR("pjmedia init failed");
            return LE_FAULT;
        }

        pj_caching_pool_init(&RtpSession.pjCp, &pj_pool_factory_default_policy, 0);

        status = pjmedia_endpt_create(&RtpSession.pjCp.factory, NULL, 1, &RtpSession.pjMedEndptPtr);
        if (PJ_SUCCESS != status)
        {
            LE_ERROR("media endpoint creation failed.");
            return LE_FAULT;
        }

        pjmedia_rtp_session_init(&RtpSession.pjOutRtpSess,
                                 0, // payload type
                                 pj_rand()); // sender ssrc
        pjmedia_rtp_session_init(&RtpSession.pjInRtpSess,
                                 0,
                                 0);
        pjmedia_rtcp_init(&RtpSession.pjRtcpSess, "rtcp",
                          STREAMMEDIA_CLOCK_RATE, // clock rate
                          STREAMMEDIA_SAMPLE_PER_FRAME, // sample per frame
                          0); // ssrc

        // Create UDP Socket and bind it to addr.
        status = pjmedia_transport_udp_create2(RtpSession.pjMedEndptPtr,
                                               "rtp",
                                               &hostnameStr,
                                               localPort, // RTP port
                                               0,
                                               &RtpSession.pjTransportPtr );// transport instance
        if (PJ_SUCCESS != status)
        {
            LE_ERROR("UDP socket creation failed.");
            return LE_FAULT;
        }

        // Attach socket to callback functions for reception of RTP/RTCP packet.
        status = pjmedia_transport_attach(RtpSession.pjTransportPtr,
                                          NULL, // user data
                                          &remoteAddr, //remote RTP address
                                          NULL, // remote RTCP address
                                          sizeof(pj_sockaddr_in),
                                          &OnRxRtp, // callback function for RTP reception
                                          &OnRxRtcp); // callback function for RTCP reception
        if (PJ_SUCCESS != status)
        {
            LE_ERROR("UDP socket attachement failed.");
            return LE_FAULT;
        }

        // Open the pipes that are will be used to read and write audio samples from and to the
        // audio player and recorder.
        if (pipe(RtpSession.rxPipefd) == -1)
        {
            LE_ERROR("Failed to create the pipe");
        }
        if (pipe(RtpSession.txPipefd) == -1)
        {
            LE_ERROR("Failed to create the pipe");
        }

        RtpSession.isInit = true;
    }
    else if (RtpSession.isInit && NULL != remoteAddrPtr)
    {
        LE_DEBUG("Setting remote address to %s", remoteAddrPtr);
        // Re-attach socket to callback functions for reception of RTP/RTCP packet.
        pjmedia_transport_detach(RtpSession.pjTransportPtr, NULL);
        status = pjmedia_transport_attach(RtpSession.pjTransportPtr,
                                          NULL, // user data
                                          &remoteAddr, //remote RTP address
                                          NULL, // remote RTCP address
                                          sizeof(pj_sockaddr_in),
                                          &OnRxRtp, // callback function for RTP reception
                                          &OnRxRtcp); // callback function for RTCP reception
        if (PJ_SUCCESS != status)
        {
            LE_ERROR("UDP socket attachement failed.");
            return LE_FAULT;
        }
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a RTCP Session Description packet (SDES)
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t streamMedia_SendRtcpSdes
(
    le_audio_StreamRef_t       streamRef,   ///< [IN] Audio stream reference.
    const char*                cnamePtr,    ///< [IN] The optional source canonical name.
    const char*                namePtr,        ///< [IN] The optional source name.
    const char*                emailPtr,    ///< [IN] The optional source email.
    const char*                phonePtr,    ///< [IN] The optional source phone.
    const char*                locPtr,      ///< [IN] The optional source location.
    const char*                toolPtr,     ///< [IN] The optional source tool.
    const char*                notePtr      ///< [IN] The optional source note.
)
{
    pj_status_t status;
    void* sdesPkt[MAX_RTP_PACKET_SIZE];
    pj_size_t sdesPktLen = MAX_RTP_PACKET_SIZE;
    pjmedia_rtcp_sdes sdes;

    sdes.cname = pj_str((char*)cnamePtr);
    sdes.name = pj_str((char*)namePtr);
    sdes.email = pj_str((char*)emailPtr);
    sdes.phone = pj_str((char*)phonePtr);
    sdes.loc = pj_str((char*)locPtr);
    sdes.tool = pj_str((char*)toolPtr);
    sdes.note = pj_str((char*)notePtr);
    status = pjmedia_rtcp_build_rtcp_sdes(&RtpSession.pjRtcpSess, // RTCP session
                                 sdesPkt, // buffer to receive RTCP SDES packet.
                                 &sdesPktLen, // length of that buffer. On return, length of SDES
                                                // packet
                                 &sdes); // Session description;
    if (PJ_SUCCESS != status)
    {
        LE_ERROR("Error building RTCP SDES packet.");
        return LE_FAULT;
    }

    status = pjmedia_transport_send_rtcp(RtpSession.pjTransportPtr, sdesPkt, sdesPktLen);
    if (PJ_SUCCESS != status)
    {
        LE_ERROR("Error sending RTCP SDES packet.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a RTCP BYE packet
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t streamMedia_SendRtcpBye
(
    le_audio_StreamRef_t  streamRef, ///< [IN] Audio stream reference.
    const char*           reason     ///< [IN] The optional BYE reason.
)
{
    pj_status_t status;
    char  rtcpByePacket[MAX_RTP_PACKET_SIZE];
    pj_size_t byeLen = MAX_RTP_PACKET_SIZE;
    pj_str_t pjReason;

    pjReason = pj_str((char*)reason);
    status = pjmedia_rtcp_build_rtcp_bye(&RtpSession.pjRtcpSess, // RTCP session
                                rtcpByePacket, // buffer to receive RTCP BYE packet
                                &byeLen,   // size of the buffer, on return size of the RTCP BYE
                                           // packet
                                &pjReason); // optional reason
    if (PJ_SUCCESS != status)
    {
        LE_ERROR("Error building RTCP BYE packet.");
        return LE_FAULT;
    }

    status = pjmedia_transport_send_rtcp(RtpSession.pjTransportPtr, rtcpByePacket, byeLen);
    if (PJ_SUCCESS != status)
    {
        LE_ERROR("Error sending RTCP BYE packet.");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'streamMedia_RtcpEvent_t'
 *
 * This event provides information on RTCP packet reception events.
 */
//--------------------------------------------------------------------------------------------------
streamMedia_RtcpHandlerRef_t streamMedia_AddRtcpHandler
(
    le_audio_StreamRef_t                streamRef,  ///< [IN] The audio stream reference.
    streamMedia_RtcpHandlerFunc_t       handlerPtr, ///< [IN]
    void*                               contextPtr  ///< [IN]
)
{
    le_event_HandlerRef_t  handlerRef;

    if (streamRef != RtpSession.receptionPlayerRef)
    {
        LE_KILL_CLIENT("Invalid stream reference (%p) provided!", streamRef);
        return NULL;
    }

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    RtpSession.streamEventId = le_event_CreateId("RtcpEvent", sizeof(streamMedia_RtcpEvent_t));

    handlerRef = le_event_AddLayeredHandler("RtcpEventHandler",
                                            RtpSession.streamEventId,
                                            FirstLayerStreamEventHandler,
                                            handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (streamMedia_RtcpHandlerRef_t) handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'streamMedia_RtcpEvent_t'
 */
//--------------------------------------------------------------------------------------------------
void streamMedia_RemoveRtcpHandler
(
    streamMedia_RtcpHandlerRef_t handlerRef ///< [IN]
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the received audio stream of a RTP session.
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t streamMedia_OpenAudioRtpRx
(
    int localPort
)
{
    InitRtp(localPort, NULL, 0);

    // The audio player is used to send audio samples to the audio interface connected to this RTP
    // reception interface.
    RtpSession.receptionPlayerRef = le_audio_OpenPlayer();

    // Set the player samples type.
    // Currently we only support 8000 Hz 16 bits per sample PCM audio data.
    if (LE_OK != le_audio_SetSamplePcmChannelNumber(RtpSession.receptionPlayerRef, 1))
    {
        LE_ERROR("Error setting sample pcm channel number.");
    }
    if (LE_OK != le_audio_SetSamplePcmSamplingRate(RtpSession.receptionPlayerRef,
                                                          STREAMMEDIA_CLOCK_RATE))
    {
        LE_ERROR("Error setting sample pcm sampling rate.");
    }
    if (LE_OK != le_audio_SetSamplePcmSamplingResolution(RtpSession.receptionPlayerRef,
                                                                STREAMMEDIA_BITS_PER_SAMPLE))
    {
        LE_ERROR("Error setting sample pcm sampling resolution.");
    }

    LE_DEBUG("RTP Reception stream opened.");
    return RtpSession.receptionPlayerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the transmitted audio stream of a RTP session.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
le_audio_StreamRef_t streamMedia_OpenAudioRtpTx
(
    int             localPort,
    const char*     remoteAddress,
    int             remotePort
)
{
    InitRtp(localPort, remoteAddress, remotePort);

    // The audio recorder is used to get audio samples from the audio interface that is connecter to
    // this RTP transmission interface.
    RtpSession.transmissionRecorderRef = le_audio_OpenRecorder();

    // Set the player samples type.
    // Currently we only support 8000 Hz 16 bits per sample PCM audio data.
    if (LE_OK != le_audio_SetSamplePcmChannelNumber(RtpSession.transmissionRecorderRef, 1))
    {
        LE_ERROR("Error setting sample pcm channel number.");
    }
    if (LE_OK != le_audio_SetSamplePcmSamplingRate(RtpSession.transmissionRecorderRef,
                                                          STREAMMEDIA_CLOCK_RATE))
    {
        LE_ERROR("Error setting sample pcm sampling rate.");
    }
    if (LE_OK != le_audio_SetSamplePcmSamplingResolution(RtpSession.transmissionRecorderRef,
                                                                STREAMMEDIA_BITS_PER_SAMPLE))
    {
        LE_ERROR("Error setting sample pcm sampling resolution.");
    }

    LE_DEBUG("RTP Transmission opened.");
    return RtpSession.transmissionRecorderRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start RTP. This must be done only after the stream is connected.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t streamMedia_Start
(
    le_audio_StreamRef_t streamRef
)
{
    if (streamRef == RtpSession.receptionPlayerRef)
    {
        LE_DEBUG("Starting RTP Reception.");
        // Play audio from the reception pipe that is filled with audio samples received in RTP
        // packets.
        if (LE_OK != le_audio_PlaySamples(streamRef, RtpSession.rxPipefd[0]))
        {
            LE_ERROR("Cannot start RTP reception : cannot play samples.");
            return LE_FAULT;
        }
        RtpSession.rxOn = true;
    }
    else if (streamRef == RtpSession.transmissionRecorderRef)
    {
        LE_DEBUG("Starting RTP Transmission.");
        // Start transmission thread that reads audio samples from the transmission pipe and sends
        // it in RTP packets.
        RtpSession.TransmitRtpThreadRef = le_thread_Create("TransmitSamples",
                                                           TransmitRtpSamplesThread, streamRef);
        le_thread_Start(RtpSession.TransmitRtpThreadRef);
        // Write audio samples from the connected audio interface to the transmission pipe.
        if (LE_OK != le_audio_GetSamples(streamRef, RtpSession.txPipefd[1]))
        {
            LE_ERROR("Cannot start RTP transmission : cannot get samples.");
            return LE_FAULT;
        }
        RtpSession.txOn = true;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop RTP.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t streamMedia_Stop
(
    le_audio_StreamRef_t streamRef
)
{
    if(streamRef == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", streamRef);
        return LE_FAULT;
    }

    if ((streamRef == RtpSession.receptionPlayerRef) && RtpSession.rxOn)
    {
        LE_DEBUG("Stop RTP Reception.");
        RtpSession.rxOn = false;
        if (LE_OK != le_audio_Stop(streamRef))
        {
            LE_ERROR("Cannot stop RTP Reception");
            return LE_FAULT;
        }
    }
    else if ((streamRef == RtpSession.transmissionRecorderRef) && RtpSession.txOn)
    {
        LE_DEBUG("Stop RTP Transmission.");
        RtpSession.txOn = false;
        le_thread_Cancel(RtpSession.TransmitRtpThreadRef);
        if (LE_OK != le_audio_Stop(streamRef))
        {
            LE_ERROR("Cannot stop RTP Transmission");
            return LE_FAULT;
        }
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Close all RTP.
 *
 */
//--------------------------------------------------------------------------------------------------
void streamMedia_Close
(
    le_audio_StreamRef_t streamRef
)
{
    if(streamRef == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided!", streamRef);
        return;
    }

    if (streamRef == RtpSession.receptionPlayerRef)
    {
        RtpSession.rxOn = false;

        le_audio_Close(streamRef);

        if (RtpSession.rxPipefd[0] > 0)
        {
            // Closing RtpSession.rxPipefd[0] is unnecessary since the messaging infrastructure
            // underneath the le_audio_PlaySamples API that use it would close it.

            RtpSession.rxPipefd[0] = -1;
        }
        if (RtpSession.rxPipefd[1] > 0)
        {
            close(RtpSession.rxPipefd[1]);
            RtpSession.rxPipefd[1] = -1;
        }

        RtpSession.receptionPlayerRef = NULL;

        LE_INFO("RTP RX session closed.");
    }
    else if (streamRef == RtpSession.transmissionRecorderRef)
    {
        RtpSession.txOn = false;

        le_audio_Close(streamRef);

        le_thread_Cancel(RtpSession.TransmitRtpThreadRef);
        if (RtpSession.txPipefd[0] > 0)
        {
            close(RtpSession.txPipefd[0]);
            RtpSession.txPipefd[0] = -1;
        }
        if (RtpSession.txPipefd[1] > 0)
        {
            // Closing RtpSession.txPipefd[1] is unnecessary since the messaging infrastructure
            // underneath le_audio_GetSamples API that use it would close it.

            RtpSession.txPipefd[1] = -1;
        }

        RtpSession.transmissionRecorderRef = NULL;

        LE_INFO("RTP TX session closed.");
    }

    if ((NULL == RtpSession.receptionPlayerRef) &&
        (NULL == RtpSession.transmissionRecorderRef) && RtpSession.isInit)
    {
        if (RtpSession.pjTransportPtr)
        {
            pjmedia_transport_detach(RtpSession.pjTransportPtr, NULL);
            pjmedia_transport_close(RtpSession.pjTransportPtr);
            RtpSession.pjTransportPtr = NULL;
        }

        pjmedia_endpt_destroy(RtpSession.pjMedEndptPtr);
        RtpSession.pjMedEndptPtr = NULL;

        pj_caching_pool_destroy(&RtpSession.pjCp);

        pj_shutdown();

        RtpSession.isInit = false;
        LE_INFO("RTP session closed.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the media stream service.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Starting streamMedia");

    RtpSession.isInit = false;
}
