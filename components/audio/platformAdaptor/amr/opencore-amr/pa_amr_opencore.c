/**
 * @file pa_amr.c
 *
 * This file contains the source code of the low level Audio API for AMR playback / capture
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_audio_local.h"
#include "pa_audio.h"
#include "pa_amr.h"

#include <opencore-amrwb/dec_if.h>
#include <opencore-amrnb/interf_dec.h>
#include <opencore-amrnb/interf_enc.h>
#include <vo-amrwbenc/enc_if.h>



//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

/* From pvamrwbdecoder_api.h, by dividing by 8 and rounding up */
const int AmrWbSizes[] = { 17, 23, 32, 36, 40, 46, 50, 58, 60, 5, -1, -1, -1, -1, -1, 0 };
/* From WmfDecBytesPerFrame in dec_input_format_tab.cpp */
const int AmrNbSizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

/* enum found in voAMRWB.h */
typedef enum {
    VOAMRWB_MDNONE      = -1,   ///< Invalid mode
    VOAMRWB_MD66        = 0,    ///<  6.60kbps
    VOAMRWB_MD885       = 1,    ///<  8.85kbps
    VOAMRWB_MD1265      = 2,    ///<  12.65kbps
    VOAMRWB_MD1425      = 3,    ///<  14.25kbps
    VOAMRWB_MD1585      = 4,    ///<  15.85bps
    VOAMRWB_MD1825      = 5,    ///<  18.25bps
    VOAMRWB_MD1985      = 6,    ///<  19.85kbps
    VOAMRWB_MD2305      = 7,    ///<  23.05kbps
    VOAMRWB_MD2385      = 8,    ///<  23.85kbps
} VoAmrWbMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * Define the buffers size.
 */
//--------------------------------------------------------------------------------------------------

#define AMR_DECODER_BUFFER_LEN  500
#define AMR_WB_BUFFER_SIZE      640
#define AMR_NB_BUFFER_SIZE      320

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for codec parameters pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t CodecParamsPool;

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Prototypes for AMR-WB/NB decoding functions
 */
//--------------------------------------------------------------------------------------------------
typedef void* (*AmrInitFunc_t) (void);
typedef void (*AmrDecodeFunc_t) (void* state, const unsigned char* in, short* out, int bfi);
typedef void (*AmrExitFunc_t)(void* state);

//--------------------------------------------------------------------------------------------------
/**
 * AMR decoder parameters
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    AmrInitFunc_t   amrInitFunc;    ///< AMR decoder initialization
    AmrDecodeFunc_t amrDecodeFunc;  ///< AMR decoder function
    AmrExitFunc_t   amrExitFunc;    ///< AMR decoder ending
    const int*      amrSizes;       ///< AMR decoder frames length
}
pa_amr_AmrDecoderParam_t;

//--------------------------------------------------------------------------------------------------
/**
 * Codec parameters structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_audio_SampleAmrConfig_t*  sampleAmrConfigPtr; ///< AMR samples configuration
    void*                        amrHandlePtr;       ///< AMR decoder handle
    uint32_t                     mode;               ///< Encoding AMR mode (bitrate enum)
}
CodecParams_t;

//--------------------------------------------------------------------------------------------------
/**
 * Encoding mode structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_audio_AmrMode_t        amrMode;      ///< AMR mode
    enum Mode                 amrNbMode;    ///< AMR Narrowband mode
    VoAmrWbMode_t             amrWbMode;    ///< AMR Wideband mode
}
EncodingMode_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

static EncodingMode_t EncodingMode[] =
{
    { LE_AUDIO_AMR_NB_4_75_KBPS,    MR475, VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_5_15_KBPS,    MR515, VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_5_9_KBPS,     MR59,  VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_6_7_KBPS,     MR67,  VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_7_4_KBPS,     MR74,  VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_7_95_KBPS,    MR795, VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_10_2_KBPS,    MR102, VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_NB_12_2_KBPS,    MR122, VOAMRWB_MDNONE  },
    { LE_AUDIO_AMR_WB_6_6_KBPS,     -1,    VOAMRWB_MD66    },
    { LE_AUDIO_AMR_WB_8_85_KBPS,    -1,    VOAMRWB_MD885   },
    { LE_AUDIO_AMR_WB_12_65_KBPS,   -1,    VOAMRWB_MD1265  },
    { LE_AUDIO_AMR_WB_14_25_KBPS,   -1,    VOAMRWB_MD1425  },
    { LE_AUDIO_AMR_WB_15_85_KBPS,   -1,    VOAMRWB_MD1585  },
    { LE_AUDIO_AMR_WB_18_25_KBPS,   -1,    VOAMRWB_MD1825  },
    { LE_AUDIO_AMR_WB_19_85_KBPS,   -1,    VOAMRWB_MD1985  },
    { LE_AUDIO_AMR_WB_23_05_KBPS,   -1,    VOAMRWB_MD2305  },
    { LE_AUDIO_AMR_WB_23_85_KBPS,   -1,    VOAMRWB_MD2385  }
};


//--------------------------------------------------------------------------------------------------
/**
 * AMR decoder parameters
 */
//--------------------------------------------------------------------------------------------------
static const pa_amr_AmrDecoderParam_t AmrDecoderParam[LE_AUDIO_FILE_MAX] =
{
    // WAVE parameters
    {
        .amrInitFunc    = NULL,
        .amrDecodeFunc  = NULL,
        .amrExitFunc    = NULL,
        .amrSizes       = NULL,
    },
    // AMR-NB parameters
    {
        .amrInitFunc    = Decoder_Interface_init,
        .amrDecodeFunc  = Decoder_Interface_Decode,
        .amrExitFunc    = Decoder_Interface_exit,
        .amrSizes       = AmrNbSizes,
    },
    // AMR-WB parameters
    {
        .amrInitFunc    = D_IF_init,
        .amrDecodeFunc  = D_IF_decode,
        .amrExitFunc    = D_IF_exit,
        .amrSizes       = AmrWbSizes,
    }
};

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StartDecoder
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Decoding format
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
)
{
    if (mediaCtxPtr->format >= LE_AUDIO_FILE_MAX)
    {
        return LE_FAULT;
    }

    CodecParams_t* codecParamPtr = le_mem_ForceAlloc(CodecParamsPool);

    if (codecParamPtr == NULL)
    {
        LE_ERROR("Error in memory allocation");
        return LE_FAULT;
    }

    codecParamPtr->amrHandlePtr = AmrDecoderParam[mediaCtxPtr->format].amrInitFunc();
    mediaCtxPtr->codecParams = (le_audio_Codec_t) codecParamPtr;

    if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_WB )
    {
        mediaCtxPtr->bufferSize = AMR_WB_BUFFER_SIZE;
    }
    else if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_NB )
    {
        mediaCtxPtr->bufferSize = AMR_NB_BUFFER_SIZE;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *Decode AMR frames
 *
 * @return LE_FAULT         Incorrect input parameters.
 * @return LE_UNDERFLOW     Not enough read data.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_DecodeFrames
(
    le_audio_MediaThreadContext_t*   mediaCtxPtr,   ///< [IN] Media thread context
    uint8_t*                         bufferOutPtr,  ///< [OUT] Decoding samples buffer output
    uint32_t*                        readLenPtr     ///< [OUT] Length of the read data
)
{
    if ((mediaCtxPtr == NULL) || (bufferOutPtr == NULL))
    {
        return LE_FAULT;
    }

    CodecParams_t* codecParamPtr = (CodecParams_t*) mediaCtxPtr->codecParams;

    if (codecParamPtr == NULL)
    {
        LE_ERROR("Bad input");
        return LE_FAULT;
    }

    int16_t tmpBuff[mediaCtxPtr->bufferSize/2];
    uint8_t readBuff[AMR_DECODER_BUFFER_LEN];

    int i,n,size;

    /* Read the mode byte */
    n = read(mediaCtxPtr->fd_in, readBuff, 1);
    if (n <= 0)
    {
        return LE_UNDERFLOW;
    }

    /* Find the packet size */
    size = (AmrDecoderParam[mediaCtxPtr->format].amrSizes)[(readBuff[0] >> 3) & 0x0f];

    if ((size < 0) || (size > (AMR_DECODER_BUFFER_LEN-1)))
    {
        return LE_FAULT;
    }

    n = read(mediaCtxPtr->fd_in, readBuff + 1, size);
    if (n != size)
    {
        return LE_UNDERFLOW;
    }

    /* Decode the packet */
    AmrDecoderParam[mediaCtxPtr->format].amrDecodeFunc(
                    codecParamPtr->amrHandlePtr,
                    readBuff,
                    tmpBuff,
                    0);


    /* Convert to little endian and write to wav */
    for (i = 0; i < mediaCtxPtr->bufferSize/2; i++) {
        *bufferOutPtr++ = (tmpBuff[i] >> 0) & 0xff;
        *bufferOutPtr++ = (tmpBuff[i] >> 8) & 0xff;
    }

    *readLenPtr = mediaCtxPtr->bufferSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR decoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StopDecoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
)
{
    if (!mediaCtxPtr || (mediaCtxPtr->format >= LE_AUDIO_FILE_MAX))
    {
        LE_ERROR("Bad format %d", mediaCtxPtr->format);
        return LE_FAULT;
    }

    CodecParams_t* codecParamPtr = (CodecParams_t*) mediaCtxPtr->codecParams;

    if (codecParamPtr == NULL)
    {
        LE_ERROR("Bad input");
        return LE_FAULT;
    }

    AmrDecoderParam[mediaCtxPtr->format].amrExitFunc(codecParamPtr->amrHandlePtr);

    le_mem_Release(codecParamPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StartEncoder
(
    le_audio_Stream_t*               streamPtr,    ///< [IN] Decoding format
    le_audio_MediaThreadContext_t*   mediaCtxPtr   ///< [IN] Media thread context
)
{
    le_result_t res;

    if (mediaCtxPtr == NULL)
    {
        LE_ERROR("Error in memory allocation");
        return LE_FAULT;
    }

    CodecParams_t* codecParamPtr = le_mem_ForceAlloc(CodecParamsPool);

    le_audio_SampleAmrConfig_t*  sampleAmrConfigPtr = &streamPtr->sampleAmrConfig;

    mediaCtxPtr->codecParams = (le_audio_Codec_t) codecParamPtr;

    int i;
    int i_last = sizeof(EncodingMode)/sizeof(EncodingMode_t);
    for (i = 0; i < i_last; i++)
    {
        if ( sampleAmrConfigPtr->amrMode == EncodingMode[i].amrMode )
        {
            codecParamPtr->sampleAmrConfigPtr = sampleAmrConfigPtr;

            if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_NB )
            {
                codecParamPtr->mode = EncodingMode[i].amrNbMode;
                codecParamPtr->amrHandlePtr = Encoder_Interface_init(sampleAmrConfigPtr->dtx);
                mediaCtxPtr->bufferSize = AMR_NB_BUFFER_SIZE;

                res = LE_OK;
                i = i_last;
            }
            else if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_WB )
            {
                codecParamPtr->mode = EncodingMode[i].amrWbMode;
                codecParamPtr->amrHandlePtr = E_IF_init();
                mediaCtxPtr->bufferSize = AMR_WB_BUFFER_SIZE;

                res = LE_OK;
                i = i_last;
            }
        }
    }

    int32_t wroteLen = 0;

    if ( res == LE_OK )
    {
        if (mediaCtxPtr->format == LE_AUDIO_FILE_AMR_WB)
        {
            wroteLen = write(mediaCtxPtr->fd_out, "#!AMR-WB\n", 9);
        }
        else
        {
            wroteLen = write(mediaCtxPtr->fd_out, "#!AMR\n", 6);
        }

        if (wroteLen <= 0)
        {
            LE_ERROR("Failed to write in given fd");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode AMR frames
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_EncodeFrames
(
    le_audio_MediaThreadContext_t* mediaCtxPtr,///< [IN] Media thread context
    uint8_t* inputDataPtr,                     ///< [IN] input AMR samples buffer
    uint32_t inputDataLen,                     ///< [IN] input buffer length
    uint8_t* outputDataPtr,                    ///< [OUT] output PCM samples buffer
    uint32_t* outputDataLen                    ///< [OUT] output PCM buffer length
)
{
    uint32_t bufLen = mediaCtxPtr->bufferSize/2;
    CodecParams_t* codecParamPtr = NULL;

    if (mediaCtxPtr)
    {
        codecParamPtr = (CodecParams_t*) mediaCtxPtr->codecParams;
    }

    if (codecParamPtr == NULL)
    {
        LE_ERROR("Bad input");
        return LE_FAULT;
    }

    short buf[bufLen];
    int i;

    inputDataLen /= 2;

    if (inputDataLen < bufLen)
    {
        return LE_UNDERFLOW;
    }

    for (i = 0; i < bufLen; i++)
    {
        const uint8_t* in = &inputDataPtr[2*i];
        buf[i] = in[0] | (in[1] << 8);
    }

    if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_NB )
    {
        *outputDataLen = Encoder_Interface_Encode(codecParamPtr->amrHandlePtr,
                                                  codecParamPtr->mode,
                                                  buf,
                                                  outputDataPtr,
                                                  0);
    }
    else
    {
        *outputDataLen = E_IF_encode(codecParamPtr->amrHandlePtr,
                                     codecParamPtr->mode,
                                     buf,
                                     outputDataPtr,
                                     codecParamPtr->sampleAmrConfigPtr->dtx);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop AMR encoder
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_amr_StopEncoder
(
    le_audio_MediaThreadContext_t*    mediaCtxPtr    ///< [IN] Media thread context
)
{
    CodecParams_t* codecParamPtr = NULL;

    if (mediaCtxPtr)
    {
        codecParamPtr = (CodecParams_t*) mediaCtxPtr->codecParams;
    }

    if (codecParamPtr == NULL)
    {
        LE_ERROR("Bad input");
        return LE_FAULT;
    }

    if ( mediaCtxPtr->format == LE_AUDIO_FILE_AMR_NB )
    {
        Encoder_Interface_exit(codecParamPtr->amrHandlePtr);
    }
    else
    {
        E_IF_exit(codecParamPtr->amrHandlePtr);
    }

    le_mem_Release(codecParamPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    CodecParamsPool = le_mem_CreatePool( "CodecParamsPool",
                                               sizeof(CodecParams_t) );
}
