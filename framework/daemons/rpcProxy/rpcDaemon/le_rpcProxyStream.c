/**
 * @file le_rpcProxyStream.c
 *
 * @ref stream_intro <br>
 * @ref stream_send <br>
 * @ref stream_receive <br>

 * @section stream_intro        Introduction
 *
 * This file contains the source code for streaming messages out and receiving streams.
 *
 * There are two type of streams that this file handles, outgoing streams and incoming streams.
 *
 * An outgoing stream is an IPC message that must be parsed, repacked, and sent to the remote side.
 * An incoming stream is an RPC message that is coming from the remote side. It must be parsed, and
 * repacked as an IPC message suitable for a local client or sever.
 *
 * The client-server concepts are independent of outgoing and incoming message types. Consider the
 * below diagram:
@verbatim


             ALICE                 │            BOB
                                   │
                                   │
                                   │
                                   │
  ┌───────┐        ┌───────┐       │      ┌───────┐         ┌────────┐
  │       ├──1───► │ rpc   ├────2──┼────► │ rpc   ├───3────►│        │
  │client │        │       │       │      │       │         │ server │
  │       │◄──6─── │server │ ◄─────┤5─────┤ client│◄───4────┤        │
  └───────┘        └───────┘       │      └───────┘         └────────┘

@endverbatim

 * IPC message 1 is a client request, for Alice's rpc engine, this is an outgoing message.
 * It will be repacked into message 2, which is considered an incoming message for Bob's RPC engine.
 * Bob converts 3 into an IPC client request and sends it to the server. The server response,
 * message 4 is then considered an outgoing message by Bob's rpc engine. It will convert it to an
 * RPC message, message 5, which is seen as an incoming message by Alice's rpc engine. Finally,
 * Alice's rpc engine converts message 5 to message 6, a server response IPC message.
 *
 * This file provides two major API's used by the rest of RPC,
 * @ref rpcProxy_SendVariableLengthMsgBody and @ref rpcProxy_RecvStream. Outgoing messages,
 * messages that are supposed to be sent, are handled by @ref rpcProxy_SendVariableLengthMsgBody
 * and incoming message, messages that we are receiving from a remote side are handled by
 * @ref rpcProxy_RecvStream.
 *
 *
 * @section stream_send Send Logic
 *
 * The @ref rpcProxy_SendVariableLengthMsgBody function must handle two types of message,
 * file stream messages and IPC messages. File stream messages are passed to
 * @ref rpcProxy_SendFileStreamMessageBody, which uses @c cbor_encode_* APIs to send file message
 * components one by one. IPC messages are passed to @ref rpcProxy_SendIpcMessageBody which uses
 * @c cbor_stream_decode API to decode the IPC message buffer. @c cbor_stream_decode expects a list
 * of callbacks for different CBOR data types. Because at every state of the message, only specific
 * CBOR types are acceptable, there is a different callback structure for every send state.
 * During the transition to a new state we will also update the callback structure. Send states are
 * laid out in @ref SendState and @ref StateToCallbacksTable attributed a specific
 * callback structure to each state. In each state's callback structure, expected CBOR types are
 * given an appropriate callback and unexpected CBOR types are given a callback that if called,
 * raises an error.
 *
 * @section stream_receive Receive Logic
 *
 * Receiving an RPC message is driven by the fdmonitor handler given to @c le_comm. This handler is
 * called whenever new bytes are received. RPC proxy then receives the new bytes and processes them
 * accordingly. If a full message is received, it will be processed and if RPC expected more bytes
 * for the current message, it waits for following calls to the receiver handler.
 *
 * An RPC message is handled in two state machines. The message header is handled in
 * @ref le_rpcProxy.c's @ref RecvRpcMsg. After receiving the header, the first layer state machine
 * goes to the stream state. It then calls @ref rpcProxy_InitializeStreamState to initialize the
 * second layer state machine according to the message type found in the header.
 *
 * From then on, @ref rpcProxy_RecvStream is called with new bytes to process. Every state has
 * an expected number of bytes. The state machine will buffer the messages in its destination buffer
 * until it has received the expected number of bytes for the current state, it can then process
 * that state and move on to the next.
 *
 * For constant size messages, the state machine starts by expecting the size of message body and
 * is finished once it receives that many bytes. For variable length messages, the state machine
 * expects to receive the message item by item and at each state, it will receive enough to parse
 * only one item. In the case of CBOR items, the state machine first receives the CBOR header and
 * then decides what to do depending on the CBOR item type. The @ref DispatchTable provides a handler
 * function to be called for any CBOR <tag, item> pair.
 *
 *
 * @subsection stream_dest Where is the destination buffer?
 *
 * The destination buffer is set dynamically according to the type of message and type of CBOR item
 * that is being received. For constant size messages, the rpcProxy message pointer provided to us
 * has enough room for the message body. For IPC and file stream messages the destination is set
 * according to the stream state and CBOR item. For the CBOR header and CBOR small items like
 * integers destination is the @c workBuffer of state machine. For larger CBOR items like strings or
 * byte strings, the destination buffer is set to the IPC message buffer or the payload buffer of
 * file stream message.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "le_rpcProxy.h"
#include "le_rpcProxyNetwork.h"
#include "le_rpcProxyConfig.h"
#include "le_rpcProxyFileStream.h"
#include "le_rpcProxyEventHandler.h"

#include "cbor.h"

//--------------------------------------------------------------------------------------------------
/**
 * macros and defines:
 */
//--------------------------------------------------------------------------------------------------
#define RPC_PROXY_LARGE_OUT_PARAMETER_MAX_NUM  LE_CONFIG_RPC_PROXY_MSG_LARGE_OUT_PARAMETER_MAX_NUM
#define RPC_PROXY_SMALL_OUT_PARAMETER_MAX_NUM  LE_CONFIG_RPC_PROXY_MSG_SMALL_OUT_PARAMETER_MAX_NUM
#define RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM    (RPC_PROXY_LARGE_OUT_PARAMETER_MAX_NUM + \
                                                RPC_PROXY_SMALL_OUT_PARAMETER_MAX_NUM)
#define IPC_MSG_ID_SIZE                        sizeof(uint32_t)
// Initial number of bytes expected to parse an async (event) message:
// 4 for id, 1 for indef array header, 1 for async handler tag, 2 for async handler tag value
#define ASYNC_MSG_INITIAL_EXPECTED_SIZE        IPC_MSG_ID_SIZE + 1 + 1 + 2

//Type that defines send state:
typedef enum SendState
{
    SEND_INITIAL_STATE = 0,
    SEND_NORMAL_STATE = 1,
    SEND_EXPECTING_REFERENCE = 2,
#ifdef RPC_PROXY_LOCAL_SERVICE
    SEND_EXPECTING_OPT_STR_HDR = 3,
    SEND_EXPECTING_OPT_STR_SIZE = 4,
    SEND_EXPECTING_OPT_STR_POINTER = 5,
    SEND_EXPECTING_OPT_BSTR_RESPONSE_SIZE = 6,
#endif
    SEND_NUM_STATES
} SendState_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Structure holding the context during send
 */
//--------------------------------------------------------------------------------------------------
typedef struct SendContext
{
    void* handle;                   ///< Handle to use for writing to le_comm
    SendState_t state;              ///< Send State
    bool squelchThisItem;           ///< Do not send the last parsed value
    rpcProxy_Message_t* messagePtr; ///< Pointer to proxy message being streamed
    unsigned int lastTag;           ///< Last observed semantic tag
    unsigned int collectionLayer;   ///< how many array layers have we seen so far.
    le_result_t lastCallbackRes;    ///< holds result of last callback

#ifdef RPC_PROXY_LOCAL_SERVICE
    size_t lastLength;              ///< Length of current optimized string
    uint8_t slotIndex;              ///< Used to keep track of optimized strings and arrays
#endif
} SendContext_t;


//--------------------------------------------------------------------------------------------------
/**
 * libcbor callback prototypes:
 */
//--------------------------------------------------------------------------------------------------
static void SemanticTagCallback(void* context, uint64_t value);
static void IndefArrayStartCallback(void* context);
static void IndefEndCallback(void* context);
static void ReferenceCallback(void* context, uint64_t value);
#ifdef RPC_PROXY_LOCAL_SERVICE
static void OptStringHeaderCallback(void* context, size_t size);
static void OptStringSizeCallback(void* context, uint64_t value);
static void OptStringPointerCallback(void* context, uint64_t value);
static void OptByteStringResponseCallback(void* context, uint64_t value);
#endif

//--------------------------------------------------------------------------------------------------
/**
 * libcbor callbacks that will cause a format error in send
 */
//--------------------------------------------------------------------------------------------------
static void SimpleErrorCallback(void *context)
{
    LE_ERROR("Unexpected CBOR Item in outgoing message");
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->lastCallbackRes = LE_FORMAT_ERROR;
}

static void Uint8ErrorCallback(void * context, uint8_t value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void Uint16ErrorCallback(void *context, uint16_t value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void Uint32ErrorCallback(void *context, uint32_t value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void Uint64ErrorCallback(void *context, uint64_t value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void CollectionStartErrorCallback(void *context, size_t value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void FloatErrorCallback(void *context, float value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void DoubleErrorCallback(void *context, double value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void BooleanErrorCallback(void *context, bool value)
{
    LE_UNUSED(value);
    SimpleErrorCallback(context);
}

static void StringErrorCallback(void *context, cbor_data data, size_t value)
{
    LE_UNUSED(value);
    LE_UNUSED(data);
    SimpleErrorCallback(context);
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callbacks that will treat a positive integer as reference
 */
//--------------------------------------------------------------------------------------------------
static void Uint8ReferenceCallback(void* context, uint8_t value)
{
    ReferenceCallback(context, value);
}

static void Uint16ReferenceCallback(void* context, uint16_t value)
{
    ReferenceCallback(context, value);
}

static void Uint32ReferenceCallback(void* context, uint32_t value)
{
    ReferenceCallback(context, value);
}

static void Uint64ReferenceCallback(void* context, uint64_t value)
{
    ReferenceCallback(context, value);
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callbacks that will treat a positive integer as size of an optimized string:
 */
//--------------------------------------------------------------------------------------------------
static void OptStringUint8SizeCallback(void* context, uint8_t value)
{
    OptStringSizeCallback(context, value);
}

static void OptStringUint16SizeCallback(void* context, uint16_t value)
{
    OptStringSizeCallback(context, value);
}

static void OptStringUint32SizeCallback(void* context, uint32_t value)
{
    OptStringSizeCallback(context, value);
}

static void OptStringUint64SizeCallback(void* context, uint64_t value)
{
    OptStringSizeCallback(context, value);
}


//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callbacks that will treat a positive integer as size of an optimized string:
 */
//--------------------------------------------------------------------------------------------------
static void OptBStrRespUint8SizeCallback(void* context, uint8_t value)
{
    OptByteStringResponseCallback(context, value);
}

static void OptBStrRespUint16SizeCallback(void* context, uint16_t value)
{
    OptByteStringResponseCallback(context, value);
}

static void OptBStrRespUint32SizeCallback(void* context, uint32_t value)
{
    OptByteStringResponseCallback(context, value);
}

static void OptBStrRespUint64SizeCallback(void* context, uint64_t value)
{
    OptByteStringResponseCallback(context, value);
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callbacks that will treat a positive integer as pointer for an optimized string:
 */
//--------------------------------------------------------------------------------------------------
static void OptStrUint8PointerCallback(void* context, uint8_t value)
{
    OptStringPointerCallback(context, value);
}

static void OptStrUint16PointerCallback(void* context, uint16_t value)
{
    OptStringPointerCallback(context, value);
}

static void OptStrUint32PointerCallback(void* context, uint32_t value)
{
    OptStringPointerCallback(context, value);
}

static void OptStrUint64PointerCallback(void* context, uint64_t value)
{
    OptStringPointerCallback(context, value);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the initial state:
 *  in the initial state only indef array header is expected. Everything else is considered
 *  a format error.
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks InitialStateCallbacks =
{
  .indef_array_start = IndefArrayStartCallback,
  .uint8 = Uint8ErrorCallback,
  .uint16 = Uint16ErrorCallback,
  .uint32 = Uint32ErrorCallback,
  .uint64 = Uint64ErrorCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = CollectionStartErrorCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the normal state:
 *  Note: A cbor_null_* callback is an empty callback that does nothing.
 *  In normal state many types of items are passed as is, therefore they have an empty callback.
 *  Some items types like MAP are not supported and therefore will throw an error.
 *  Item types that need specific processing are:
 *  Indef Array begin, Indef Array End, Tags
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks NormalStateCallbacks =
{
  .indef_array_start = IndefArrayStartCallback,
  .uint8 = cbor_null_uint8_callback,
  .uint16 = cbor_null_uint16_callback,
  .uint32 = cbor_null_uint32_callback,
  .uint64 = cbor_null_uint64_callback,
  .negint64 = cbor_null_negint64_callback,
  .negint32 = cbor_null_negint32_callback,
  .negint16 = cbor_null_negint16_callback,
  .negint8 = cbor_null_negint8_callback,
  .byte_string_start = cbor_null_byte_string_start_callback,
  .byte_string = cbor_null_byte_string_callback,
  .string = cbor_null_string_callback,
  .string_start = cbor_null_string_start_callback,
  .array_start = cbor_null_array_start_callback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = SemanticTagCallback,
  .float2 = cbor_null_float2_callback,
  .float4 = cbor_null_float4_callback,
  .float8 = cbor_null_float8_callback,
  .undefined = cbor_null_null_callback,
  .null = cbor_null_undefined_callback,
  .boolean = cbor_null_boolean_callback,
  .indef_break = IndefEndCallback,
};

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the expecting reference state:
 *
 *  The only expected item in this state is a positive integer that holds the reference value
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks ExpectingReferenceStateCallbacks =
{
  .indef_array_start = SimpleErrorCallback,
  .uint8 = Uint8ReferenceCallback,
  .uint16 = Uint16ReferenceCallback,
  .uint32 = Uint32ReferenceCallback,
  .uint64 = Uint64ReferenceCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = CollectionStartErrorCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the expecting opt string header state:
 *  in the expecting opt string header state only array header is expected.
 *  Everything else is considered a format error.
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks ExpectingOptStringHeaderStateCallbacks =
{
  .indef_array_start = SimpleErrorCallback,
  .uint8 = Uint8ErrorCallback,
  .uint16 = Uint16ErrorCallback,
  .uint32 = Uint32ErrorCallback,
  .uint64 = Uint64ErrorCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = OptStringHeaderCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the expecting opt string size state:
 *  in the expecting opt string size state only a positive integer is expected.
 *  Everything else is considered a format error.
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks ExpectingOptStringSizeStateCallbacks =
{
  .indef_array_start = SimpleErrorCallback,
  .uint8 = OptStringUint8SizeCallback,
  .uint16 = OptStringUint16SizeCallback,
  .uint32 = OptStringUint32SizeCallback,
  .uint64 = OptStringUint64SizeCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = CollectionStartErrorCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the expecting opt string header state:
 *  in the expecting opt string header state only array header is expected.
 *  Everything else is considered a format error.
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks ExpectingOptStringPointerStateCallbacks =
{
  .indef_array_start = SimpleErrorCallback,
  .uint8 = OptStrUint8PointerCallback,
  .uint16 = OptStrUint16PointerCallback,
  .uint32 = OptStrUint32PointerCallback,
  .uint64 = OptStrUint64PointerCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = CollectionStartErrorCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};

//--------------------------------------------------------------------------------------------------
/**
 *  List of callbacks defining expected items in the expecting opt byte string response state:
 *  in the expecting opt string size state only a positive integer is expected.
 *  Everything else is considered a format error.
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks ExpectingOptByteStringRespStateCallbacks =
{
  .indef_array_start = SimpleErrorCallback,
  .uint8 = OptBStrRespUint8SizeCallback,
  .uint16 = OptBStrRespUint16SizeCallback,
  .uint32 = OptBStrRespUint32SizeCallback,
  .uint64 = OptBStrRespUint64SizeCallback,
  .negint64 = Uint64ErrorCallback,
  .negint32 = Uint32ErrorCallback,
  .negint16 = Uint16ErrorCallback,
  .negint8 = Uint8ErrorCallback,
  .byte_string_start = SimpleErrorCallback,
  .byte_string = StringErrorCallback,
  .string = StringErrorCallback,
  .string_start = SimpleErrorCallback,
  .array_start = CollectionStartErrorCallback,
  .indef_map_start = SimpleErrorCallback,
  .map_start = CollectionStartErrorCallback,
  .tag = Uint64ErrorCallback,
  .float2 = FloatErrorCallback,
  .float4 = FloatErrorCallback,
  .float8 = DoubleErrorCallback,
  .undefined = SimpleErrorCallback,
  .null = SimpleErrorCallback,
  .boolean = BooleanErrorCallback,
  .indef_break = SimpleErrorCallback,
};
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  This table gives pointer to callback structures for every send state:
 *
 *  These callback structures are to be given to the cbor_stream_decode api depending on the send
 *  state
 */
//--------------------------------------------------------------------------------------------------
static const struct cbor_callbacks* StateToCallbacksTable[SEND_NUM_STATES] =
{
    [SEND_INITIAL_STATE] = &InitialStateCallbacks,
    [SEND_NORMAL_STATE] = &NormalStateCallbacks,
    [SEND_EXPECTING_REFERENCE] = &ExpectingReferenceStateCallbacks,
#ifdef RPC_PROXY_LOCAL_SERVICE
    [SEND_EXPECTING_OPT_STR_HDR] = &ExpectingOptStringHeaderStateCallbacks,
    [SEND_EXPECTING_OPT_STR_SIZE] = &ExpectingOptStringSizeStateCallbacks,
    [SEND_EXPECTING_OPT_STR_POINTER] = &ExpectingOptStringPointerStateCallbacks,
    [SEND_EXPECTING_OPT_BSTR_RESPONSE_SIZE] = &ExpectingOptByteStringRespStateCallbacks,
#endif
};

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for local message linked list.
 * Initialized in rpcProxy_InitializeStreamingMemPools().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LocalBufferPool,
                          RPC_PROXY_LARGE_OUT_PARAMETER_MAX_NUM,
                          sizeof(rpcProxy_LocalBuffer_t) + RPC_LOCAL_MAX_LARGE_OUT_PARAMETER_SIZE);
static le_mem_PoolRef_t LocalBufferPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Structure of response pointers provided by the client.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ResponseParameterArray
{
    uintptr_t pointer[RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM];
}
ResponseParameterArray_t;
//--------------------------------------------------------------------------------------------------
/**
 * Hash Map to store Response "out" parameter pointers (value), using the Proxy Message ID (key).
 * Initialized in rpcProxy_InitializeStreamingMemPools().
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(ResponseParameterArrayHashMap, RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM);
static le_hashmap_Ref_t ResponseParameterArrayByProxyId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * This pool is used to allocate memory for the Response "out" parameter response array.
 * Initialized in rpcProxy_InitializeStreamingMemPools().
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ResponseParameterArrayPool,
                          RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM,
                          sizeof(ResponseParameterArray_t));
static le_mem_PoolRef_t ResponseParameterArrayPoolRef = NULL;

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Helper functions for checking tags:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Checks whether a tag is for local service optimization:
 *
 * @return
 *      - true if it's a local service optimization tag
 *      - false otherwise
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsTagLocalServiceOpt
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
   return (tag == LE_PACK_IN_STRING_POINTER || tag == LE_PACK_OUT_STRING_POINTER ||
       tag == LE_PACK_IN_BYTE_STR_POINTER  || tag == LE_PACK_OUT_BYTE_STR_POINTER);
}

static inline bool IsTagLocalStrResponse
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return tag == LE_PACK_OUT_STRING_RESPONSE;
}

static inline bool IsTagLocalByteStrResponse
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return tag == LE_PACK_OUT_BYTE_STR_RESPONSE;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Checks whether a tag is an optimized string
 *  @return
 *      - true if tag is for an optimizes string
 *      - false otherwise
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsTagOptimizedString
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return (tag == LE_PACK_IN_STRING_POINTER || tag == LE_PACK_OUT_STRING_POINTER);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Checks whether a tag is for an event handler.
 *  @return
 *      - true if tag is for an async event
 *      - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsTagEventHandler
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return (tag == LE_PACK_CONTEXT_PTR_REFERENCE || tag == LE_PACK_ASYNC_HANDLER_REFERENCE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Is tag for file stream?
 * @return
 *      - true if tag is for file stream
 *      - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsTagFileStream
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return (tag == LE_PACK_FILESTREAM_ID || tag == LE_PACK_FILESTREAM_FLAG ||
            tag == LE_PACK_FILESTREAM_REQUEST_SIZE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Is tag for an out paramater's size?
 * @return
 *      - true if tag is for an out parameters size
 *      - false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsTagOutParamSize
(
    le_pack_SemanticTag_t tag                  ///< [IN] Semantic tag value
)
{
    return (tag == LE_PACK_OUT_STRING_SIZE || tag == LE_PACK_OUT_BYTE_STR_SIZE);
}


/**
 * Get the direction an array parameter is traveling from the last seen semantic tag.
 */
static inline rpcProxy_Direction_t GetParamDirection
(
    le_pack_SemanticTag_t tag
)
{
    if (tag == LE_PACK_OUT_STRING_SIZE || tag == LE_PACK_OUT_BYTE_STR_SIZE)
    {
        return DIR_OUT;
    }
    else
    {
        return DIR_IN;
    }
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 *  Functions for handling optimized arrays and strings in local service mode:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Function to store a response memory buffer.
 * Helper function for facilitating un-rolling optimized data before it is sent over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackStoreResponsePointer
(
    SendContext_t* sendContextPtr, ///< [IN] pointer to send context structure
    uintptr_t pointer              ///< [IN] Pointer to the response buffer
)
{
    rpcProxy_Message_t* proxyMessagePtr = sendContextPtr->messagePtr;
    uint8_t* slotIndexPtr = &(sendContextPtr->slotIndex);
    // Retrieve existing array pointer, if it exists
    ResponseParameterArray_t* arrayPtr =
        le_hashmap_Get(
            ResponseParameterArrayByProxyId,
            (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    if (arrayPtr == NULL)
    {
        // Allocate the response parameter array, in which to
        // store the response pointers
        arrayPtr = le_mem_Alloc(ResponseParameterArrayPoolRef);
        memset(arrayPtr, 0, sizeof(ResponseParameterArray_t));
    }

    if (*slotIndexPtr == RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM)
    {
        LE_ERROR("Response array overflow error - out of array elements");
        return LE_OVERFLOW;
    }

    // If the item is passed in, store it in the response pointer array.
    // If the item is NULL, just ignore it as it will not be received in the
    // response message
    if (pointer)
    {
        // Store the response pointer in the array, using the slot Id
        arrayPtr->pointer[*slotIndexPtr] = pointer;

        LE_DEBUG("Storing response pointer, proxy id [%" PRIu32 "], "
                 "slot id [%" PRIu8 "], pointer [%" PRIiPTR "]",
                 proxyMessagePtr->commonHeader.id,
                 *slotIndexPtr,
                 pointer);

        // Store the array of memory pointer until
        // the server response is received, using the proxy message Id
        le_hashmap_Put(ResponseParameterArrayByProxyId,
                       (void*)(uintptr_t) proxyMessagePtr->commonHeader.id,
                       arrayPtr);

        // Increment the slotIndex
        *slotIndexPtr = *slotIndexPtr + 1;
    }
    else
    {
        LE_DEBUG("Discarding null response pointer, proxy id [%" PRIu32 "], "
                 "slot id [%" PRIu8 "]",
                 proxyMessagePtr->commonHeader.id,
                 *slotIndexPtr);
    }

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Function for Cleaning-up Local Message Memory Pool resources that have been allocated
 * in order to facilitate string and array memory optimizations
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_CleanUpLocalMessageResources
(
    uint32_t proxyMsgId                  ///< [IN] proxy message id field in the common header.
)
{
    //
    // Clean-up the Response "out" parameter hashmap
    //

    // Retrieve the Response out" parameter hashmap entry
    ResponseParameterArray_t* arrayPtr =
        le_hashmap_Get(ResponseParameterArrayByProxyId, (void*) proxyMsgId);

    if (arrayPtr != NULL)
    {
        LE_DEBUG("Releasing response parameter array, proxy id [%" PRIu32 "]", proxyMsgId);

        // Free memory allocate for the Response "out" parameter array
        le_mem_Release(arrayPtr);

        // Delete Response "out" parameter hashmap entry
        le_hashmap_Remove(ResponseParameterArrayByProxyId, (void*) proxyMsgId);
    }
}

#endif

//--------------------------------------------------------------------------------------------------
/**
 *  Print IPC message information:
 */
//--------------------------------------------------------------------------------------------------
static void PrintIpcMessageInfo
(
    rpcProxy_Message_t* proxyMessagePtr,   ///< [IN] pointer to rpc message
    uint32_t ipcMsgId                      ///< [IN] ipc message Id
)
{
    LE_DEBUG("IPC MsgId: %"PRIu32", ServiceID: %"PRIu32"", ipcMsgId,
            proxyMessagePtr->commonHeader.serviceId);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Helper functions used during send:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Write the metadata of proxy message
 */
//--------------------------------------------------------------------------------------------------
static void WriteMetaData
(
    SendContext_t* sendContextPtr                 ///< [IN] pointer to send state structure
)
{
    if (sendContextPtr->messagePtr->metaData.isFileStreamValid)
    {
        uint8_t tempBuff[1 + sizeof(uint64_t)];
        size_t encoded_size = cbor_encode_tag(LE_PACK_FILESTREAM_ID, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);

        encoded_size = cbor_encode_uint(sendContextPtr->messagePtr->metaData.fileStreamId, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);

        // pack flags:
        encoded_size = cbor_encode_tag(LE_PACK_FILESTREAM_FLAG, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);

        encoded_size = cbor_encode_uint(sendContextPtr->messagePtr->metaData.fileStreamFlags, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);
    }
}

#ifdef RPC_PROXY_LOCAL_SERVICE
/**
 * Get the next output parameter for this message.
 */
static rpcProxy_LocalBuffer_t *PopNextOutputParameter
(
    uint32_t serviceId
)
{
    le_dls_Link_t *paramItem = NULL;
    rpcProxy_LocalBuffer_t *paramBuffer = NULL;

    // Find the next output parameter
    do
    {
        paramItem = rpcProxy_PopNextParameter(serviceId);
        if (!paramItem)
        {
            return NULL;
        }

        paramBuffer = CONTAINER_OF(paramItem,
                                   rpcProxy_LocalBuffer_t,
                                   link);

        if (paramBuffer->dir == DIR_IN)
        {
            // Just free input parameters.
            // Since function has returned, we don't need them anymore.
            le_mem_Release(paramBuffer);
            paramItem = NULL;
        }
    } while (!paramItem);

    return paramBuffer;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Write a size as string header to the le_comm
 */
//--------------------------------------------------------------------------------------------------
static void WriteStringHeader
(
    SendContext_t* sendContextPtr,        ///< [IN] pointer to send state structure
    uint64_t length                       ///< [IN] length of string.
)
{
    uint8_t tempBuff [1 + sizeof(uint64_t)];
    size_t encoded_size = cbor_encode_string_start(length, tempBuff, sizeof(tempBuff));
    le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write byte string header
 */
//--------------------------------------------------------------------------------------------------
static void WriteByteStringHeader
(
    SendContext_t* sendContextPtr,        ///< [IN] pointer to send state structure
    uint64_t byteCount                    ///< [IN] length of string.
)
{
    uint8_t tempBuff [1 + sizeof(uint64_t)];
    size_t encoded_size = cbor_encode_bytestring_start(byteCount, tempBuff, sizeof(tempBuff));
    le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Write a string size as an out parameter tag
 */
//--------------------------------------------------------------------------------------------------
static void WriteOutStringSize
(
    SendContext_t* sendContextPtr,        ///< [IN] pointer to send state structure
    size_t length,                        ///< [IN] length of string.
    uint32_t tag                          ///< [IN] tag to encode
)
{
    // This is when we're writing the size for the outstring:
    uint8_t tempBuff [1 + sizeof(uint64_t)];
    size_t encoded_size = cbor_encode_tag(tag, tempBuff, sizeof(tempBuff));
    le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);

    encoded_size = cbor_encode_uint(length, tempBuff, sizeof(tempBuff));
    le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Write data that is buffered in a pointer directly to le_comm
 */
//--------------------------------------------------------------------------------------------------
static void WriteBufferedData
(
    SendContext_t* sendContextPtr,        ///< [IN] pointer to send state structure
    uintptr_t pointer,                    ///< [IN] pointer to buffer
    size_t length                         ///< [IN] size of data in buffer
)
{

    uint8_t* buff = (uint8_t*) pointer;
    le_comm_Send(sendContextPtr->handle, buff, length);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the out string data over the wire.
 *
 * This occurs in response to seeing a semantic tag indicating an optimized string has been removed
 * at this point.
 */
//--------------------------------------------------------------------------------------------------
static void WriteStringResponse
(
    SendContext_t* sendContextPtr         ///< [IN] pointer to send state structure
)
{
    size_t length, encodedSize;
    rpcProxy_LocalBuffer_t *paramBuffer =
        PopNextOutputParameter(sendContextPtr->messagePtr->commonHeader.id);
    uint8_t tempBuff [1 + sizeof(uint64_t)];

    LE_ASSERT(paramBuffer);

    // skip NULL output parameters
    if (paramBuffer->dataSz)
    {
        length = strnlen((char *)paramBuffer->bufferData, paramBuffer->dataSz);

        LE_DEBUG("Writing string '%s': max len %d, actual len %d",
                 (char *)paramBuffer->bufferData,
                 paramBuffer->dataSz,
                 length);

        encodedSize = cbor_encode_tag(LE_PACK_OUT_STRING_RESPONSE, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encodedSize);
        WriteStringHeader(sendContextPtr, length);
        WriteBufferedData(sendContextPtr, (uintptr_t)paramBuffer->bufferData, length);
    }

    le_mem_Release(paramBuffer);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  Libcbor callbacks:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for semantic tags:
 */
//--------------------------------------------------------------------------------------------------
static void SemanticTagCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    uint64_t value           ///< [IN] Semantic tag value
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    le_pack_SemanticTag_t tagId = (le_pack_SemanticTag_t) value;

    sendContextPtr->lastTag = tagId;
    if (IsTagLocalServiceOpt(tagId))
    {
        sendContextPtr->state = SEND_EXPECTING_OPT_STR_HDR;
        sendContextPtr->squelchThisItem = true;
    }
    else if (IsTagLocalStrResponse(tagId))
    {

        WriteStringResponse(sendContextPtr);
        sendContextPtr->squelchThisItem = true;

        // clear the tag now:
        sendContextPtr->lastTag = 0;
        sendContextPtr->state = SEND_NORMAL_STATE;
    }
    else if (IsTagLocalByteStrResponse(tagId))
    {
        sendContextPtr->state = SEND_EXPECTING_OPT_BSTR_RESPONSE_SIZE;
    }
    else if (IsTagEventHandler(tagId))
    {
        sendContextPtr->state = SEND_EXPECTING_REFERENCE;
    }
    else
    {
        // Unrecognized tag -- pass through
        sendContextPtr->lastTag = 0;
        sendContextPtr->state = SEND_NORMAL_STATE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for indefinite array header
 */
//--------------------------------------------------------------------------------------------------
static void IndefArrayStartCallback
(
    void* context            ///< [IN] libcbor context, holds pointer to send state structure
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->collectionLayer++;
    if (sendContextPtr->state == SEND_INITIAL_STATE)
    {
        sendContextPtr->state = SEND_NORMAL_STATE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for break item.
 */
//--------------------------------------------------------------------------------------------------
static void IndefEndCallback
(
    void* context            ///< [IN] libcbor context, holds pointer to send state structure
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->collectionLayer--;

    if (sendContextPtr->collectionLayer == 0)
    {
        // we're finished: send out metadata and possible out arrays before the break:
        WriteMetaData(sendContextPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for references:
 */
//--------------------------------------------------------------------------------------------------
static void ReferenceCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    uint64_t value           ///< [IN] the integer value
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    if (IsTagEventHandler(sendContextPtr->lastTag))
    {
        // We need to generate a different reference based on this and send that one instead:
        sendContextPtr->squelchThisItem = true;
        void* newContext;
        uint32_t contextPtrValue = (uint32_t) value;

        rpcEventHandler_RepackOutgoingContext(sendContextPtr->lastTag, (void*) contextPtrValue,
                                              &newContext,
                                              sendContextPtr->messagePtr);
        //new write the new context:
        uint8_t tempBuff[1 + sizeof(uint64_t)];
        size_t encoded_size = cbor_encode_uint((uintptr_t)newContext, tempBuff, sizeof(tempBuff));
        le_comm_Send(sendContextPtr->handle, tempBuff, encoded_size);
    }
    // clear the tag now:
    sendContextPtr->lastTag = 0;
    sendContextPtr->state = SEND_NORMAL_STATE;
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * libcbor callback for header of an optimized string:
 */
//--------------------------------------------------------------------------------------------------
static void OptStringHeaderCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    size_t size              ///< [IN] number of items in the array
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    LE_ASSERT(IsTagLocalServiceOpt(sendContextPtr->lastTag));
    if (size != 2)
    {
        LE_ERROR("Optimized string is an array of more than two items is unexpected");
        sendContextPtr->lastCallbackRes = LE_FORMAT_ERROR;
        return;
    }
    sendContextPtr->squelchThisItem = true;
    sendContextPtr->state = SEND_EXPECTING_OPT_STR_SIZE;
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for size of an optimized string(text string or byte string):
 */
//--------------------------------------------------------------------------------------------------
static void OptStringSizeCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    uint64_t value           ///< [IN] the integer value
)
{
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->squelchThisItem = true;
    sendContextPtr->lastLength = (size_t) value;
    sendContextPtr->state = SEND_EXPECTING_OPT_STR_POINTER;
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for pointer of an optimized string(text string or byte string):
 */
//--------------------------------------------------------------------------------------------------
static void OptStringPointerCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    uint64_t value           ///< [IN] the integer value
)
{
    // Value is pointer:
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->squelchThisItem = true;
    if (sendContextPtr->lastTag == LE_PACK_IN_STRING_POINTER)
    {
        // for [IN] parameters, we just need to unroll the string
        // write the header:
        WriteStringHeader(sendContextPtr, sendContextPtr->lastLength);
        // Write data:
        WriteBufferedData(sendContextPtr, value, sendContextPtr->lastLength);
    }
    else if (sendContextPtr->lastTag == LE_PACK_IN_BYTE_STR_POINTER)
    {
        // for [IN] parameters, we just need to unroll the byte string
        // write the header:
        WriteByteStringHeader(sendContextPtr, sendContextPtr->lastLength);
        // Write data:
        WriteBufferedData(sendContextPtr, value, sendContextPtr->lastLength);
    }
    else if ((sendContextPtr->lastTag == LE_PACK_OUT_STRING_POINTER) ||
             (sendContextPtr->lastTag == LE_PACK_OUT_BYTE_STR_POINTER))
    {
        // for [OUT] parameters, we also need to keep track of the pointer
        uint32_t outTag;
        if (sendContextPtr->lastTag == LE_PACK_OUT_STRING_POINTER)
        {
            outTag = LE_PACK_OUT_STRING_SIZE;
        }
        else
        {
            outTag = LE_PACK_OUT_BYTE_STR_SIZE;
        }

        RepackStoreResponsePointer(sendContextPtr, value);
        WriteOutStringSize(sendContextPtr, sendContextPtr->lastLength, outTag);
    }
    else
    {
        LE_EMERG("OptStringPointerCallback is called but last tag is not an optimized string");
    }
    // clear the tag now:
    sendContextPtr->lastTag = 0;
    sendContextPtr->state = SEND_NORMAL_STATE;
}

//--------------------------------------------------------------------------------------------------
/**
 *  libcbor callback for result size of an optimized byte string.
 *
 *  Called when we see a result byte string size in a response message to insert the byte string
 *  data into the message.
 */
//--------------------------------------------------------------------------------------------------
static void OptByteStringResponseCallback
(
    void* context,           ///< [IN] libcbor context, holds pointer to send state structure
    uint64_t value           ///< [IN] the integer value
)
{
    // Value is pointer:
    SendContext_t* sendContextPtr = (SendContext_t*) context;
    sendContextPtr->squelchThisItem = true;

    rpcProxy_LocalBuffer_t *paramBuffer =
        PopNextOutputParameter(sendContextPtr->messagePtr->commonHeader.id);

    LE_ASSERT(paramBuffer);

    // Skip NULL output parameters
    if (paramBuffer->dataSz)
    {
        LE_FATAL_IF(value > paramBuffer->dataSz,
                    "Returned byte array size %"PRIu64" larger than buffer %"PRIuS,
                    value, paramBuffer->dataSz);

        WriteByteStringHeader(sendContextPtr, value);
        WriteBufferedData(sendContextPtr, (uintptr_t)paramBuffer->bufferData, value);
    }

    le_mem_Release(paramBuffer);

    // clear the tag now:
    sendContextPtr->lastTag = 0;
    sendContextPtr->state = SEND_NORMAL_STATE;
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Send out a file stream message body
 * @return
 *      - LE_OK transmission was successful.
 *      - LE_FAULT an error happened during transmission.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t rpcProxy_SendFileStreamMessageBody
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    rpcProxy_FileStreamMessage_t* messagePtr ///< [IN] Void pointer to the message buffer
)
{
    le_result_t ret = LE_OK;
    if (messagePtr->metaData.isFileStreamValid)
    {
        uint8_t tempBuff[1 + sizeof(uint64_t)];
        size_t encoded_size = cbor_encode_indef_array_start(tempBuff, sizeof(tempBuff));
        ret = le_comm_Send(handle, tempBuff, encoded_size);

        // pack the stream id:
        if (ret == LE_OK)
        {
            encoded_size = cbor_encode_tag(LE_PACK_FILESTREAM_ID, tempBuff, sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
        }

        if (ret == LE_OK)
        {
            encoded_size = cbor_encode_uint(messagePtr->metaData.fileStreamId, tempBuff,
                                              sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
        }

        // pack flags:
        if (ret == LE_OK)
        {
            encoded_size = cbor_encode_tag(LE_PACK_FILESTREAM_FLAG, tempBuff, sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
        }

        if (ret == LE_OK)
        {
            encoded_size = cbor_encode_uint(messagePtr->metaData.fileStreamFlags, tempBuff,
                                            sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
        }

        // pack data as byte string:
        if (ret == LE_OK && messagePtr->payloadSize != 0)
        {
            encoded_size = cbor_encode_bytestring_start(messagePtr->payloadSize, tempBuff,
                                                        sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
            if (ret == LE_OK)
            {
                ret = le_comm_Send(handle, messagePtr->payload, messagePtr->payloadSize);
            }
        }

        if (ret == LE_OK && messagePtr->requestedSize != 0)
        {
            encoded_size = cbor_encode_tag(LE_PACK_FILESTREAM_REQUEST_SIZE, tempBuff, sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);

            if (ret == LE_OK)
            {
                encoded_size = cbor_encode_uint(messagePtr->requestedSize, tempBuff,
                                                  sizeof(tempBuff));
                ret = le_comm_Send(handle, tempBuff, encoded_size);
            }
        }
        //pack break:
        if (ret == LE_OK)
        {
            encoded_size = cbor_encode_break(tempBuff, sizeof(tempBuff));
            ret = le_comm_Send(handle, tempBuff, encoded_size);
        }
    }
    else
    {
        LE_ERROR("Asked to send a file stream message that does not have valid metadata");
        ret = LE_FAULT;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Stream an outgoing message
 *  @return
 *      - LE_OK if message was transmitted successfully
 *      - LE_FAULT in case of error in transmission.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t rpcProxy_SendIpcMessageBody
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    rpcProxy_Message_t* messagePtr ///< [IN] Void pointer to the message buffer
)
{
    SendContext_t context;
    memset(&context, 0, sizeof(context));
    context.handle = handle;
    context.messagePtr = messagePtr;
    context.lastCallbackRes = LE_OK;
    context.state = SEND_INITIAL_STATE;

    void* msgBuff = le_msg_GetPayloadPtr(messagePtr->msgRef);
    size_t maxLength = le_msg_GetMaxPayloadSize(messagePtr->msgRef);

    uint32_t id = 0;
    memcpy((uint8_t*) &id, msgBuff, IPC_MSG_ID_SIZE);
    id = htobe32(id);
    le_comm_Send(context.handle, (uint8_t*) &id, sizeof(uint32_t));
    msgBuff += sizeof(uint32_t);
    maxLength -= sizeof(uint32_t);

    struct cbor_decoder_result decode_result;
    size_t bytes_read = 0;

    le_result_t ret = LE_OK;
    while (bytes_read < maxLength) {
        decode_result = cbor_stream_decode(msgBuff + bytes_read, maxLength - bytes_read,
                                       StateToCallbacksTable[context.state], &context);
        // first checking status:
        if (decode_result.status != CBOR_DECODER_FINISHED || context.lastCallbackRes != LE_OK)
        {
            LE_ERROR("Detected error during a proxy message transmission");
            ret = LE_FAULT;
            break;
        }
        // check whether this needs to be sent:
        if (!context.squelchThisItem)
        {
#if RPC_PROXY_HEX_DUMP
            LE_INFO("RPC Sending:");
            LE_LOG_DUMP(LE_LOG_INFO, msgBuff+bytes_read, decode_result.read);
#endif
            if (le_comm_Send(context.handle, msgBuff+bytes_read, decode_result.read) != LE_OK)
            {
                ret = LE_COMM_ERROR;
                break;
            }
        }

        // check whether we're done:
        if (context.collectionLayer == 0)
        {
            //we're done here:
            ret = LE_OK;
            break;
        }

        //advance in the buffer:
        bytes_read += decode_result.read;
        context.squelchThisItem = false;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Send out the body of a variable length message
 *  @return
 *      - LE_OK if message was transmitted successfully
 *      - LE_FAULT in case of error in transmission.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_SendVariableLengthMsgBody
(
    void* handle, ///< [IN] Opaque handle to the le_comm communication channel
    void* messagePtr ///< [IN] Void pointer to the message buffer
)
{
    rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) messagePtr;
    if (commonHeaderPtr->type == RPC_PROXY_FILESTREAM_MESSAGE)
    {
        return rpcProxy_SendFileStreamMessageBody(handle, (rpcProxy_FileStreamMessage_t*)messagePtr);
    }
    else
    {
        return rpcProxy_SendIpcMessageBody(handle, (rpcProxy_Message_t*) messagePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize streaming memory pools and hash tables
 */
//--------------------------------------------------------------------------------------------------
void rpcProxy_InitializeOnceStreamingMemPools()
{
#ifdef RPC_PROXY_LOCAL_SERVICE
    le_mem_PoolRef_t parentPoolRef = le_mem_InitStaticPool(LocalBufferPool,
                                                           RPC_PROXY_LARGE_OUT_PARAMETER_MAX_NUM,
                                                           sizeof(rpcProxy_LocalBuffer_t) +
                                                           RPC_LOCAL_MAX_LARGE_OUT_PARAMETER_SIZE);
    LocalBufferPoolRef = le_mem_CreateReducedPool(parentPoolRef, "LocalBufferPool",
                                                  RPC_PROXY_SMALL_OUT_PARAMETER_MAX_NUM,
                                                  sizeof(rpcProxy_LocalBuffer_t) +
                                                  RPC_LOCAL_MAX_SMALL_OUT_PARAMETER_SIZE);


    ResponseParameterArrayPoolRef = le_mem_InitStaticPool(
                                        ResponseParameterArrayPool,
                                        RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM,
                                        sizeof(ResponseParameterArray_t));


    // Create hash map for response "OUT" parameter pointers, using the Proxy Message ID (key).
    ResponseParameterArrayByProxyId = le_hashmap_InitStatic(
                                          ResponseParameterArrayHashMap,
                                          RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM,
                                          le_hashmap_HashVoidPointer,
                                          le_hashmap_EqualsVoidPointer);
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 *  Receive state machine:
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Function prototypes:
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConnectServiceStreamInitializer(StreamState_t*, void*);
static le_result_t ClientRequestStreamInitializer(StreamState_t*, void*);
static le_result_t ServerResponseStreamInitializer(StreamState_t*, void*);
static le_result_t KeepAliveStreamInitializer(StreamState_t*, void*);
static le_result_t EventMessageStreamInitializer(StreamState_t*, void*);
static le_result_t FileStreamMessageStreamInitializer(StreamState_t*, void*);

static le_result_t HandleIndefEnd(StreamState_t*, void*, void**);
static le_result_t HandleSemanticTag(StreamState_t*, void*, void**);
static le_result_t HandleStringHeader(StreamState_t*, void*, void**);
static le_result_t HandleArrayHeader(StreamState_t*, void*, void**);
static le_result_t HandleFileStreamMetadata(StreamState_t*, void*, void**);
static le_result_t HandleOutputSize(StreamState_t*, void*, void**);
static le_result_t HandleReference(StreamState_t*, void*, void**);
static le_result_t HandleAsError(StreamState_t*, void*, void**);
static le_result_t HandleWithDirectCopy(StreamState_t*, void*, void**);

//--------------------------------------------------------------------------------------------------
/**
 *  Stream state initializer function:
 *
 *  This function gets a pointer to the stream state machine and the proxy message,
 *  it then initializes the stream state machine and proxy message accordingly.
 *  return value is LE_OK if there was no error and LE_FAULT otherwise.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*streamStateInitialize_fpt)(StreamState_t*, void*);
static const streamStateInitialize_fpt StreamStateInitializersTable[RPC_PROXY_NUM_MESSAGE_TYPES] =
{
    [RPC_PROXY_CONNECT_SERVICE_REQUEST]  = ConnectServiceStreamInitializer,
    [RPC_PROXY_CONNECT_SERVICE_RESPONSE] = ConnectServiceStreamInitializer,
    [RPC_PROXY_DISCONNECT_SERVICE]       = ConnectServiceStreamInitializer,
    [RPC_PROXY_CLIENT_REQUEST]           = ClientRequestStreamInitializer,
    [RPC_PROXY_SERVER_RESPONSE]          = ServerResponseStreamInitializer,
    [RPC_PROXY_KEEPALIVE_REQUEST]        = KeepAliveStreamInitializer,
    [RPC_PROXY_KEEPALIVE_RESPONSE]       = KeepAliveStreamInitializer,
    [RPC_PROXY_SERVER_ASYNC_EVENT]       = EventMessageStreamInitializer,
    [RPC_PROXY_FILESTREAM_MESSAGE]       = FileStreamMessageStreamInitializer,
};


//--------------------------------------------------------------------------------------------------
/**
 *  Dispatch function:
 *
 *  This function takes pointer to stream state structure, proxy message being received and ipc
 *  message buffer as input. It returns LE_OK if handling of an item was successful and an error
 *  value determining the nature of error if handing was unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*dispatchFunc_fpt)(StreamState_t*, void *, void**);

#define LE_RPC_NO_TAG_DISPATCH_IDX          (0)
#define LE_RPC_OUTPUT_SIZE_TAG_DISPATCH_IDX (1)
#define LE_RPC_FILESTREAM_TAG_DISPATCH_IDX  (2)
#define LE_RPC_REFERENCE_TAG_DISPATCH_IDX   (3)
#define LE_RPC_NUM_DISPATCH_IDXS            (4)

//--------------------------------------------------------------------------------------------------
/**
 * Only the following tags are allowed when parsing a stream, any thing else will cause an error:
 */
//--------------------------------------------------------------------------------------------------
static const le_pack_SemanticTag_t TagsExpectedInRecvStream[] =
{
    LE_PACK_REFERENCE,
    LE_PACK_OUT_STRING_SIZE,
    LE_PACK_OUT_BYTE_STR_SIZE,
    LE_PACK_FILESTREAM_ID,
    LE_PACK_FILESTREAM_FLAG,
    LE_PACK_FILESTREAM_REQUEST_SIZE,
    LE_PACK_CONTEXT_PTR_REFERENCE,
    LE_PACK_ASYNC_HANDLER_REFERENCE,
    LE_PACK_OUT_STRING_RESPONSE,
    LE_PACK_OUT_BYTE_STR_RESPONSE
};

//--------------------------------------------------------------------------------------------------
/**
 *  Converts a tag ID to an index in the dispatch table
 */
//--------------------------------------------------------------------------------------------------
static inline unsigned int TagIdToDispatchIdx
(
    le_pack_SemanticTag_t tagId
)
{
    unsigned int ret = 0;
    switch(tagId)
    {
        case (0):
        case (LE_PACK_OUT_STRING_RESPONSE):
        case (LE_PACK_OUT_BYTE_STR_RESPONSE):
            // No tag, or no action needed
            ret = LE_RPC_NO_TAG_DISPATCH_IDX; break;
        case (LE_PACK_OUT_STRING_SIZE):
        case (LE_PACK_OUT_BYTE_STR_SIZE):
            ret = LE_RPC_OUTPUT_SIZE_TAG_DISPATCH_IDX; break;
        case (LE_PACK_FILESTREAM_ID):
        case (LE_PACK_FILESTREAM_FLAG):
        case (LE_PACK_FILESTREAM_REQUEST_SIZE):
            ret = LE_RPC_FILESTREAM_TAG_DISPATCH_IDX; break;
        case (LE_PACK_REFERENCE):
        case (LE_PACK_CONTEXT_PTR_REFERENCE):
        case (LE_PACK_ASYNC_HANDLER_REFERENCE):
            ret = LE_RPC_REFERENCE_TAG_DISPATCH_IDX; break;
        default:
            // tags that are unexpected/unsupported in a receive stream will be caught in the
            // "HandleSemanticTag" function and cause the stream to be dropped so this case is never
            // supposed to happen.
            LE_FATAL("Unexpected tag in stream state, cannot proceed");
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Dispatch table for handling cbor items in receiving streams:
 */
//--------------------------------------------------------------------------------------------------
static const dispatchFunc_fpt DispatchTable[LE_RPC_NUM_DISPATCH_IDXS][LE_PACK_TYPE_INVALID_TYPE+1] =
{
    [LE_RPC_NO_TAG_DISPATCH_IDX] =          {
                                             [LE_PACK_TYPE_POS_INTEGER]  = HandleWithDirectCopy,
                                             [LE_PACK_TYPE_NEG_INTEGER]  = HandleWithDirectCopy,
                                             [LE_PACK_TYPE_BYTE_STRING]  = HandleStringHeader,
                                             [LE_PACK_TYPE_TEXT_STRING]  = HandleStringHeader,
                                             [LE_PACK_TYPE_ITEM_ARRAY]   = HandleArrayHeader,
                                             [LE_PACK_TYPE_SEMANTIC_TAG] = HandleSemanticTag,
                                             [LE_PACK_TYPE_BOOLEAN]      = HandleWithDirectCopy,
                                             [LE_PACK_TYPE_DOUBLE]       = HandleWithDirectCopy,
                                             [LE_PACK_TYPE_INDEF_END]    = HandleIndefEnd,
                                             [LE_PACK_TYPE_INVALID_TYPE] = HandleAsError},
    [LE_RPC_OUTPUT_SIZE_TAG_DISPATCH_IDX] = {
                                             [LE_PACK_TYPE_POS_INTEGER]  = HandleOutputSize,
                                             [LE_PACK_TYPE_NEG_INTEGER]  = HandleAsError,
                                             [LE_PACK_TYPE_BYTE_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_TEXT_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_ITEM_ARRAY]   = HandleAsError,
                                             [LE_PACK_TYPE_SEMANTIC_TAG] = HandleAsError,
                                             [LE_PACK_TYPE_BOOLEAN]      = HandleAsError,
                                             [LE_PACK_TYPE_DOUBLE]       = HandleAsError,
                                             [LE_PACK_TYPE_INDEF_END]    = HandleAsError,
                                             [LE_PACK_TYPE_INVALID_TYPE] = HandleAsError},
    [LE_RPC_FILESTREAM_TAG_DISPATCH_IDX] =  {
                                             [LE_PACK_TYPE_POS_INTEGER]  = HandleFileStreamMetadata,
                                             [LE_PACK_TYPE_NEG_INTEGER]  = HandleAsError,
                                             [LE_PACK_TYPE_BYTE_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_TEXT_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_ITEM_ARRAY]   = HandleAsError,
                                             [LE_PACK_TYPE_SEMANTIC_TAG] = HandleAsError,
                                             [LE_PACK_TYPE_BOOLEAN]      = HandleAsError,
                                             [LE_PACK_TYPE_DOUBLE]       = HandleAsError,
                                             [LE_PACK_TYPE_INDEF_END]    = HandleAsError,
                                             [LE_PACK_TYPE_INVALID_TYPE] = HandleAsError},
    [LE_RPC_REFERENCE_TAG_DISPATCH_IDX] =   {
                                             [LE_PACK_TYPE_POS_INTEGER]  = HandleReference,
                                             [LE_PACK_TYPE_NEG_INTEGER]  = HandleAsError,
                                             [LE_PACK_TYPE_BYTE_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_TEXT_STRING]  = HandleAsError,
                                             [LE_PACK_TYPE_ITEM_ARRAY]   = HandleAsError,
                                             [LE_PACK_TYPE_SEMANTIC_TAG] = HandleAsError,
                                             [LE_PACK_TYPE_BOOLEAN]      = HandleAsError,
                                             [LE_PACK_TYPE_DOUBLE]       = HandleAsError,
                                             [LE_PACK_TYPE_INDEF_END]    = HandleAsError,
                                             [LE_PACK_TYPE_INVALID_TYPE] = HandleAsError}
};

//--------------------------------------------------------------------------------------------------
/**
 *  Go to CBOR Header state
 */
//--------------------------------------------------------------------------------------------------
static void GoToCborHeaderState
(
    StreamState_t* streamStatePtr  ///< [IN] Pointer to the Stream State-Machine data
)
{
    streamStatePtr->state = STREAM_CBOR_HEADER;
    streamStatePtr->expectedSize = 1;
    streamStatePtr->destBuff = (void*) streamStatePtr->workBuff;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to Integer Item state
 */
//--------------------------------------------------------------------------------------------------
static void GoToIntegerItemState
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    size_t expectedBytes           ///< [IN] Bytes expected for this integer item.
)
{
    streamStatePtr->state = STREAM_INTEGER_ITEM;
    streamStatePtr->expectedSize = expectedBytes;
    streamStatePtr->destBuff = (void*)(streamStatePtr->workBuff + 1);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to CBOR Item Body state
 */
//--------------------------------------------------------------------------------------------------
static void GoToCborItemBodyState
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    size_t expectedBytes,          ///< [IN] Bytes expected for this integer item.
    void* destBuff                 ///< [IN] Pointer to destination buffer
)
{
    streamStatePtr->state = STREAM_CBOR_ITEM_BODY;
    streamStatePtr->expectedSize = expectedBytes;
    streamStatePtr->destBuff = destBuff;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to Constant Length Message state
 */
//--------------------------------------------------------------------------------------------------
static void GoToConstantLengthMessageState
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    size_t expectedBytes,          ///< [IN] Bytes expected for this integer item.
    void* destBuff                 ///< [IN] Pointer to destination buffer
)
{
    streamStatePtr->state = STREAM_CONSTANT_LENGTH_MSG;
    streamStatePtr->expectedSize = expectedBytes;
    streamStatePtr->destBuff = destBuff;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to IPC Message ID state
 */
//--------------------------------------------------------------------------------------------------
static void GoToIpcMessageIdState
(
    StreamState_t* streamStatePtr  ///< [IN] Pointer to the Stream State-Machine data
)
{
    streamStatePtr->destBuff = streamStatePtr->workBuff;
    streamStatePtr->expectedSize = IPC_MSG_ID_SIZE;
    streamStatePtr->state = STREAM_MSG_ID;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to Aync Event Init state
 */
//--------------------------------------------------------------------------------------------------
static void GoToAsyncEventInitState
(
    StreamState_t* streamStatePtr  ///< [IN] Pointer to the Stream State-Machine data
)
{
    streamStatePtr->destBuff = streamStatePtr->workBuff;
    streamStatePtr->expectedSize = ASYNC_MSG_INITIAL_EXPECTED_SIZE;
    streamStatePtr->state = STREAM_ASYNC_EVENT_INIT;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Go to Done state
 */
//--------------------------------------------------------------------------------------------------
static void GoToDoneState
(
    StreamState_t* streamStatePtr  ///< [IN] Pointer to the Stream State-Machine data
)
{
    streamStatePtr->state = STREAM_DONE;
    streamStatePtr->expectedSize = 0;
    streamStatePtr->destBuff = NULL;
}

#ifdef RPC_PROXY_LOCAL_SERVICE
//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve a response memory buffer.
 * Helper function for facilitating rolling-up un-optimized data that is received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackRetrieveResponsePointer
(
    /// [IN] Pointer to the Proxy Message
    rpcProxy_Message_t *proxyMessagePtr,
    /// [IN] Pointer to the slot-index in the response buffer array
    uint8_t* slotIndexPtr,
    /// [IN] Pointer to the response buffer pointer
    uint8_t** responsePtr
)
{
    // Retrieve existing array pointer, if it exists
    ResponseParameterArray_t* arrayPtr =
        le_hashmap_Get(ResponseParameterArrayByProxyId,
                       (void*)(uintptr_t) proxyMessagePtr->commonHeader.id);

    if (arrayPtr == NULL)
    {
        // Unable to find response parameter array - Not expected
        LE_ERROR("Pointer to response array is NULL, service-id [%" PRIu32 "], "
                 "proxy id [%" PRIu32 "]; Dropping packet",
                 proxyMessagePtr->commonHeader.serviceId,
                 proxyMessagePtr->commonHeader.id);

        return LE_BAD_PARAMETER;
    }

    if (*slotIndexPtr == RPC_PROXY_MSG_OUT_PARAMETER_MAX_NUM)
    {
        LE_ERROR("Response array overflow error - out of array elements");
        return LE_OVERFLOW;
    }

    *responsePtr = (uint8_t*) arrayPtr->pointer[*slotIndexPtr];
    if (*responsePtr == NULL)
    {
        // Unable to find response parameter array - Not expected
        LE_ERROR("Response Pointer is NULL, service-id [%" PRIu32 "], "
                 "proxy id [%" PRIu32 "]; slot id [%" PRIu8 "] Dropping packet",
                 proxyMessagePtr->commonHeader.serviceId,
                 proxyMessagePtr->commonHeader.id,
                 *slotIndexPtr);

        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Retrieving response pointer, proxy id [%" PRIu32 "], "
             "slot id [%" PRIu8 "], pointer [%" PRIuS "]",
             proxyMessagePtr->commonHeader.id,
             *slotIndexPtr,
             (size_t) arrayPtr->pointer[*slotIndexPtr]);

    // Increment the slotIndex
    *slotIndexPtr = *slotIndexPtr + 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for allocating a response memory buffer.
 * Helper function for facilitating rolling-up un-optimized data that is received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static void RepackAllocateResponseMemory
(
    StreamState_t* streamStatePtr,       ///< [IN] Pointer to the stream State-Machine data
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    rpcProxy_Direction_t dir,            ///< [IN] Direction this parameter is traveling
    size_t size, ///< [IN] Size for the response buffer
    uint8_t** responsePtr ///< [IN] Pointer to the response pointer
)
{
    LE_DEBUG("Storing LocalMessage item, proxyMessageId:%"PRIu32", size: %"PRIuS"",
             proxyMessagePtr->commonHeader.id, size);
    if (size == 0)
    {
        *responsePtr = NULL;
        return;
    }

    // Allocate a local message memory tracker record
    rpcProxy_LocalBuffer_t* localBufferPtr = le_mem_TryVarAlloc(LocalBufferPoolRef,
                                                                sizeof(rpcProxy_LocalBuffer_t) +
                                                                size);
    if (localBufferPtr == NULL)
    {
        LE_FATAL("Failed to allocate memory tracker record for out parameter, size %"PRIuS"",
                 sizeof(rpcProxy_LocalBuffer_t) + size);
        return;
    }

    localBufferPtr->link = LE_DLS_LINK_INIT;
    localBufferPtr->dataSz = size;
    localBufferPtr->dir = dir;
    memset(localBufferPtr->bufferData, 0, size);

    // Enqueue this in the buffer list
    le_dls_Queue(&streamStatePtr->localBuffers, &localBufferPtr->link);

    *responsePtr = localBufferPtr->bufferData;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function for Rolling-up un-optimized data.  It copies the data from the Message Buffer into
 * the response memory after being received over the wire.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackUnOptimizedData
(
    StreamState_t* streamStatePtr,       ///< [IN] Pointer to the stream State-Machine data
    rpcProxy_Message_t *proxyMessagePtr, ///< [IN] Pointer to the original Proxy Message
    void** bufferPtr,                    ///< pointer to ipc message buffer
    uint64_t length,                     ///< length of unoptimized data
    le_pack_Type_t itemType              ///< type of item that is being unoptimized
)
{
    le_result_t result = LE_OK;
    uint8_t* slotIndexPtr = &(streamStatePtr->slotIndex);
    uint8_t* responsePtr;
    bool dataIsNonEmptyString = (streamStatePtr->lastTag == LE_PACK_OUT_STRING_SIZE && length>0) ||
                                itemType == LE_PACK_TYPE_TEXT_STRING;
    if (proxyMessagePtr->commonHeader.type == RPC_PROXY_SERVER_RESPONSE)
    {
        // Retrieve the response pointer
        result = RepackRetrieveResponsePointer(
                    proxyMessagePtr,
                    slotIndexPtr,
                    &responsePtr);

        if (result != LE_OK)
        {
            return result;
        }

        // For byte strings, push the actual size of the returned buffer into the stream
        if (itemType == LE_PACK_TYPE_BYTE_STRING)
        {
            le_pack_PackSize((uint8_t **)bufferPtr, length);
        }
        else if (dataIsNonEmptyString)
        {
            // need to put the null terminator ourselves because the string on the wire wouldn't
            // contain it:
            responsePtr[length] = '\0';
        }
    }
    else
    {
        // Allocate the memory
        size_t bufferSize = (size_t) length;
        if (dataIsNonEmptyString)
        {
            bufferSize += 1; // to account for null terminator.
        }
        RepackAllocateResponseMemory(streamStatePtr, proxyMessagePtr,
                                     GetParamDirection(streamStatePtr->lastTag),
                                     bufferSize, &responsePtr);
        if (responsePtr != NULL && dataIsNonEmptyString)
        {
            responsePtr[length] = '\0';
        }

        // Set pointer to data in new message buffer
        LE_ASSERT(le_pack_PackTaggedSizePointerTuple(
                      (uint8_t**)bufferPtr,
                      length,
                      (void*) responsePtr,
                      (streamStatePtr->lastTag == LE_PACK_OUT_STRING_SIZE)? LE_PACK_OUT_STRING_POINTER:
                      (streamStatePtr->lastTag == LE_PACK_OUT_BYTE_STR_SIZE)? LE_PACK_OUT_BYTE_STR_POINTER:
                      (itemType == LE_PACK_TYPE_TEXT_STRING)? LE_PACK_IN_STRING_POINTER:
                      (itemType == LE_PACK_TYPE_BYTE_STRING)? LE_PACK_IN_BYTE_STR_POINTER: 0));

        LE_DEBUG("Rolling-up data, dataSize [%" PRIu64 "], "
                 "proxy id [%" PRIu32 "], pointer [%p]",
                 length,
                 proxyMessagePtr->commonHeader.id,
                 responsePtr);
        streamStatePtr->lastTag = 0;
        streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(streamStatePtr->lastTag);
    }
    if (itemType == LE_PACK_TYPE_TEXT_STRING || itemType == LE_PACK_TYPE_BYTE_STRING)
    {
        if (length > 0)
        {
            GoToCborItemBodyState(streamStatePtr, (size_t)length, (void*)responsePtr);
        }
        else
        {
            GoToCborHeaderState(streamStatePtr);
        }
    }

    return result;
}
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Gets pointer to the ipc message buffer
 * @return
 *      - pointer to ipc message buffer if stream state has a message reference.
 *      - NULL otherwise.
 */
//--------------------------------------------------------------------------------------------------
static void* GetIpcMsgBufPtr
(
    StreamState_t* streamStatePtr ///< [IN] Pointer to the Stream State-Machine data
)
{
    if (streamStatePtr->msgRef)
    {
        return le_msg_GetPayloadPtr(streamStatePtr->msgRef) + streamStatePtr->ipcMsgPayloadOffset;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Check whether a text or byte string needs to be optimized.
 *
 * depending on platform and context, we may just copy the string to ipc msg
 * buffer OR have to buffer it a pointer!.
 * on Linux(non local service) we always pass the string.
 * on Rtos(local service) we always optimize the string to a pointer unless it's
 * inside an sync response message or structure.
 */
//--------------------------------------------------------------------------------------------------
static bool DoIOptimize
(
    StreamState_t* streamStatePtr ///< [IN] Pointer to the Stream State-Machine data
)
{
    bool doNotOptimize;
#ifndef RPC_PROXY_LOCAL_SERVICE
    doNotOptimize = true; // Linux
#else
    if (streamStatePtr->collectionsLayer > 1 || streamStatePtr->isAsyncMsg)
    {
        doNotOptimize = true;
    }
    else
    {
        doNotOptimize = false;
    }
#endif
    return !doNotOptimize;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle a break item seen in the stream
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleIndefEnd
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    LE_DEBUG("Handling Break: layer: %d", streamStatePtr->collectionsLayer);
    le_result_t ret = LE_OK;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    LE_UNUSED(proxyMessagePtr);

    rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) proxyMessagePtr;
    bool packIndefEnd = (commonHeaderPtr->type != RPC_PROXY_FILESTREAM_MESSAGE);
    if (packIndefEnd && streamStatePtr->msgBuffSizeLeft < LE_PACK_INDEF_END_MAX_SIZE)
    {
        ret = LE_NO_MEMORY;
    }
    else if (!le_pack_UnpackEndOfIndefArray(&workBuff))
    {
        ret = LE_FORMAT_ERROR;
    }
    else
    {
        if(packIndefEnd && !le_pack_PackEndOfIndefArray((uint8_t**) bufferPtr))
        {
            ret = LE_FAULT;
        }
    }

    if (streamStatePtr->collectionsLayer <= 0)
    {
        ret = LE_FORMAT_ERROR;
        LE_ERROR("Found and Indef End item when no indef array open is seen");
    }
    // if packing was successful update size left:
    if (ret == LE_OK)
    {
        streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
        streamStatePtr->collectionsLayer--;
        if (streamStatePtr->collectionsLayer == 0)
        {
            GoToDoneState(streamStatePtr);
        }
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle a semantic tag seen in the stream
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleSemanticTag
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    le_result_t ret = LE_OK;
    le_pack_SemanticTag_t tagId;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    LE_UNUSED(proxyMessagePtr);

    if (!le_pack_UnpackSemanticTag(&workBuff, &tagId))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    // first need to make sure if this is a tag we expect to receive:
    bool expectedTag = false;
    for (unsigned int i = 0 ; i < NUM_ARRAY_MEMBERS(TagsExpectedInRecvStream); i++)
    {
        if (TagsExpectedInRecvStream[i] == tagId)
        {
            expectedTag = true;
            break;
        }
    }
    if (!expectedTag)
    {
        ret = LE_FORMAT_ERROR;
        LE_ERROR("Found unexpected tag %d in stream", tagId);
        goto error;
    }
    // store it in the state strucutre to be used by following item.
    streamStatePtr->lastTag = tagId;
    streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(tagId);

    if (IsTagLocalStrResponse(streamStatePtr->lastTag) ||
        IsTagLocalByteStrResponse(streamStatePtr->lastTag))
    {
        // Tag is passed straight through -- handling at another layer
        le_pack_PackSemanticTag((uint8_t**) bufferPtr, tagId);
    }
    else if ((!DoIOptimize(streamStatePtr)) && IsTagOutParamSize(streamStatePtr->lastTag))
    {
        // we can pack this tag now:
        if (streamStatePtr->msgBuffSizeLeft < LE_PACK_SEMANTIC_TAG_MAX_SIZE)
        {
            ret = LE_NO_MEMORY;
            goto error;
        }
        else if (!le_pack_PackSemanticTag((uint8_t**) bufferPtr, tagId))
        {
            ret = LE_FAULT;
            goto error;
        }
    }
    // If handling was successful update size left:
    streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
    GoToCborHeaderState(streamStatePtr);
    return ret;
error:
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle a cbor string header seen in the stream.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleStringHeader
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    // first need to unpack the value:
    le_result_t ret = LE_OK;
    size_t length;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;

    ssize_t additionalBytes;
    le_pack_Type_t itemType = le_pack_GetType(workBuff, &additionalBytes);

    if (itemType == LE_PACK_TYPE_TEXT_STRING)
    {
        if (!le_pack_UnpackStringHeader(&workBuff, &length))
        {
            ret = LE_FORMAT_ERROR;
            goto error;
        }
    }
    else if(itemType == LE_PACK_TYPE_BYTE_STRING)
    {
        if (!le_pack_UnpackByteStringHeader(&workBuff, &length))
        {
            ret = LE_FORMAT_ERROR;
            goto error;
        }
    }
    else
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }

    rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) proxyMessagePtr;
    if (itemType == LE_PACK_TYPE_BYTE_STRING &&
            commonHeaderPtr->type == RPC_PROXY_FILESTREAM_MESSAGE)
    {
        LE_DEBUG("Handling filestream data of length %"PRIuS"", length);
        if (length <= RPC_PROXY_MAX_FILESTREAM_PAYLOAD_SIZE)
        {
            rpcProxy_FileStreamMessage_t* fileStreamMsgPtr =
                (rpcProxy_FileStreamMessage_t*)proxyMessagePtr;
            fileStreamMsgPtr->payloadSize = (uint16_t) length;
            GoToCborItemBodyState(streamStatePtr, (size_t) length,
                                  (void*) fileStreamMsgPtr->payload);
        }
        else
        {
            ret = LE_NO_MEMORY;
            goto error;
        }
    }
#ifdef RPC_PROXY_LOCAL_SERVICE
    else if (DoIOptimize(streamStatePtr))
    {
        LE_DEBUG("Will optimize a string of size: %"PRIuS"", length);
        if (streamStatePtr->msgBuffSizeLeft <
                LE_PACK_SIZE_POINTER_TUPLE_MAX_SIZE + LE_PACK_SEMANTIC_TAG_MAX_SIZE)
        {
            ret = LE_NO_MEMORY;
            goto error;
        }
        else
        {
            ret = RepackUnOptimizedData(streamStatePtr, proxyMessagePtr, bufferPtr, length, itemType);
            if (ret != LE_OK)
            {
                goto error;
            }
        }
    }
#endif
    else
    {
        LE_DEBUG("Will not optimize a string of size: %"PRIuS"", length);
        if (streamStatePtr->msgBuffSizeLeft < (LE_PACK_STR_HEADER_MAX_SIZE + length))
        {
            ret = LE_NO_MEMORY;
            goto error;
        }
        else if (itemType == LE_PACK_TYPE_TEXT_STRING)
        {
            if (!le_pack_PackStringHeader((uint8_t**)bufferPtr, length))
            {
                ret = LE_FAULT;
                goto error;
            }
        }
        else if (itemType == LE_PACK_TYPE_BYTE_STRING)
        {
            if (!le_pack_PackByteStringHeader((uint8_t**)bufferPtr, length))
            {
                ret = LE_FAULT;
                goto error;
            }
        }

        // if header was packed successfully need to move to next state to receive string body
        GoToCborItemBodyState(streamStatePtr, length, *bufferPtr);
    }

    // done handling the value, update remaining size:
    streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);

    return ret;

error:
    LE_ERROR("Error in handling string header");
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle a cbor array header (not indefinite) seen in the stream.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleArrayHeader
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    // first need to unpack the value:
    le_result_t ret = LE_OK;
    size_t itemCount;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;

    ssize_t additionalBytes;
    le_pack_Type_t itemType = le_pack_GetType(workBuff, &additionalBytes);

    if (itemType == LE_PACK_TYPE_ITEM_ARRAY && additionalBytes >=0)
    {
        if (streamStatePtr->msgBuffSizeLeft < LE_PACK_ARRAY_HEADER_MAX_SIZE)
        {
            ret = LE_NO_MEMORY;
        }
        /* TODO: is there a better way to use this function when there's no array pointer? */
        else if (!le_pack_UnpackArrayHeader(&workBuff,(void*)1,0, &itemCount, UINT32_MAX))
        {
            ret = LE_FORMAT_ERROR;
        }
        else if (!le_pack_PackArrayHeader((uint8_t**)bufferPtr, NULL, 0, (size_t) itemCount, UINT32_MAX))
        {
            ret = LE_FAULT;
        }
    }
    else if (itemType == LE_PACK_TYPE_ITEM_ARRAY && additionalBytes < 0)
    {
        rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) proxyMessagePtr;
        bool packIndefHeader = (commonHeaderPtr->type != RPC_PROXY_FILESTREAM_MESSAGE);
        if (!le_pack_UnpackIndefArrayHeader(&workBuff))
        {
            ret = LE_FORMAT_ERROR;
        }
        else if (packIndefHeader &&
                 streamStatePtr->msgBuffSizeLeft < LE_PACK_INDEF_ARRAY_HEADER_MAX_SIZE)
        {
            ret = LE_NO_MEMORY;
        }
        else if (packIndefHeader && !le_pack_PackIndefArrayHeader((uint8_t**)bufferPtr))
        {
            ret = LE_FAULT;
        }
        else
        {
            streamStatePtr->collectionsLayer++;
        }
    }
    else
    {
        ret = LE_FORMAT_ERROR;
    }

    if (ret == LE_OK)
    {
        streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
        GoToCborHeaderState(streamStatePtr);
    }
    else
    {
        LE_ERROR("Error in handling an array header");
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Set file stream meta data seen in the stream.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleFileStreamMetadata
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void* proxyMessagePtr, ///< [IN] Pointer to the Proxy Message
    void** bufferPtr              ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    le_result_t ret = LE_OK;
    uint16_t value;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    LE_UNUSED(bufferPtr);
    rpcProxy_Message_t* rpcMessagePtr = (rpcProxy_Message_t*) proxyMessagePtr;
    rpcProxy_FileStreamMessage_t* fileStreamMsgPtr = (rpcProxy_FileStreamMessage_t*) proxyMessagePtr;
    rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) proxyMessagePtr;
    rpcProxy_MessageMetadata_t* metaDataPtr;

    if (commonHeaderPtr->type == RPC_PROXY_FILESTREAM_MESSAGE)
    {
        metaDataPtr = &(fileStreamMsgPtr->metaData);
    }
    else if (commonHeaderPtr->type == RPC_PROXY_CLIENT_REQUEST ||
            commonHeaderPtr->type == RPC_PROXY_SERVER_RESPONSE)
    {
        metaDataPtr = &(rpcMessagePtr->metaData);
    }
    else
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    LE_ASSERT(IsTagFileStream(streamStatePtr->lastTag));

    if (!le_pack_UnpackUint16(&workBuff, &value))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    if (streamStatePtr->lastTag == LE_PACK_FILESTREAM_ID)
    {
        metaDataPtr->fileStreamId = value;
    }
    else if (streamStatePtr->lastTag == LE_PACK_FILESTREAM_FLAG)
    {
        metaDataPtr->fileStreamFlags = value;
        metaDataPtr->isFileStreamValid = true;
    }
    else if (streamStatePtr->lastTag == LE_PACK_FILESTREAM_REQUEST_SIZE)
    {
        fileStreamMsgPtr->requestedSize = value;
    }

    streamStatePtr->lastTag = 0;
    streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(streamStatePtr->lastTag);

    GoToCborHeaderState(streamStatePtr);
    return ret;
error:
    LE_ERROR("Error in handling file stream metadata");
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle an "out" parameter size seen in the stream.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleOutputSize
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    le_result_t ret = LE_OK;
    uint32_t value;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    LE_ASSERT(IsTagOutParamSize(streamStatePtr->lastTag));

    if (!le_pack_UnpackUint32(&workBuff, &value))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }

    if (DoIOptimize(streamStatePtr))
    {
        if (streamStatePtr->msgBuffSizeLeft <
                LE_PACK_SIZE_POINTER_TUPLE_MAX_SIZE + LE_PACK_SEMANTIC_TAG_MAX_SIZE)
        {
            ret = LE_NO_MEMORY;
            goto error;
        }
            // allocate memory for size and pack that memory pointer instead:
        ret = RepackUnOptimizedData(streamStatePtr, (rpcProxy_Message_t*)proxyMessagePtr, bufferPtr, value,
                                        LE_PACK_TYPE_POS_INTEGER);
    }
    else
    {
        if (!le_pack_PackUint32((uint8_t**)bufferPtr, value))
        {
            ret = LE_FAULT;
            goto error;
        }
    }
    if (ret == LE_OK)
    {
        streamStatePtr->lastTag = 0;
        streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(streamStatePtr->lastTag);
        streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
        GoToCborHeaderState(streamStatePtr);
    }
    return ret;

error:
    LE_ERROR("Error in handling output size");
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Handle a reference value seen in the stream
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleReference
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    le_result_t ret = LE_OK;
    LE_DEBUG("Handling a reference");
    uint32_t value;
    void* newRef = NULL;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;

    if (!le_pack_UnpackUint32(&workBuff, &value))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }

    if (IsTagEventHandler(streamStatePtr->lastTag))
    {
        // event handler tag to handle.
        LE_DEBUG("Handling an event reference value:%"PRIu32", Tag: %"PRIu16"", value,
                streamStatePtr->lastTag);
        ret = rpcEventHandler_RepackIncomingContext(streamStatePtr->lastTag, (void*)value, &newRef,
                                                    proxyMessagePtr);
        if (ret != LE_OK)
        {
            goto error;
        }
        if (streamStatePtr->isAsyncMsg)
        {
            // if this is an async message, it means we didn't have a message reference so far!
            // so need to catch up before packing the actual reference!
            // must pack all the early stuff first:
            rpcProxy_Message_t* eventMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
            streamStatePtr->msgRef = eventMsgPtr->msgRef;
            if (streamStatePtr->msgRef == NULL)
            {
                LE_ERROR("Failed to receive a msgRef for async message");
                ret = LE_FAULT;
                goto error;
            }
            uint8_t* msgBufPtr = GetIpcMsgBufPtr(streamStatePtr);
            memcpy(msgBufPtr, (uint8_t*)(&(streamStatePtr->asyncMsgId)), IPC_MSG_ID_SIZE);
            msgBufPtr += IPC_MSG_ID_SIZE;
            le_pack_PackIndefArrayHeader(&msgBufPtr);
            *bufferPtr = msgBufPtr;
            streamStatePtr->msgBuffSizeLeft = le_msg_GetMaxPayloadSize(streamStatePtr->msgRef);
            streamStatePtr->msgBuffSizeLeft -= (LE_PACK_SEMANTIC_TAG_MAX_SIZE +
                    LE_PACK_INDEF_ARRAY_HEADER_MAX_SIZE + IPC_MSG_ID_SIZE);
            buffStart = *bufferPtr;
        }
    }
    else
    {
        // this is a regular reference, so newRef is the value
        newRef = (void *)(size_t)value;
    }
    if (streamStatePtr->msgBuffSizeLeft < (LE_PACK_SEMANTIC_TAG_MAX_SIZE +  LE_PACK_UINT32_MAX_SIZE))
    {
        ret = LE_NO_MEMORY;
        goto error;
    }
    if (!le_pack_PackTaggedReference((uint8_t**) bufferPtr, newRef, streamStatePtr->lastTag))
    {
        ret = LE_FAULT;
        goto error;
    }
    streamStatePtr->lastTag = 0;
    streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(streamStatePtr->lastTag);
    streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
    GoToCborHeaderState(streamStatePtr);
    return ret;

error:
    LE_ERROR("Error in handling reference");
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Handle a cbor item as error
 *  This is used to raise an error when the item type is unexpected
 *
 *  @return:
 *      - Always return LE_FORMAT_ERROR
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleAsError
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    LE_UNUSED(streamStatePtr);
    LE_UNUSED(proxyMessagePtr);
    LE_UNUSED(bufferPtr);
    LE_ERROR("Error in handling an item, unexpected item");
    return LE_FORMAT_ERROR;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle a cbor item by directly copying it to ipc buffer
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleWithDirectCopy
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    LE_DEBUG("Handle an item with Direct Copy");
    le_result_t ret = LE_OK;
    void* buffStart = *bufferPtr;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    ssize_t additionalBytes;
    le_pack_Type_t itemType = le_pack_GetType(workBuff, &additionalBytes);
    LE_UNUSED(itemType);
    LE_UNUSED(proxyMessagePtr);

    if (streamStatePtr->msgBuffSizeLeft < (size_t)(additionalBytes + 1))
    {
        ret = LE_NO_MEMORY;
    }
    else
    {
        memcpy(*bufferPtr, workBuff, additionalBytes + 1);
        *bufferPtr += additionalBytes + 1;
    }
    // if packing was successful update size left:
    if (ret == LE_OK)
    {
        streamStatePtr->msgBuffSizeLeft -= (*bufferPtr - buffStart);
        GoToCborHeaderState(streamStatePtr);
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Finish The stream
 * Sets the state of the network message state machine to idle
 */
//--------------------------------------------------------------------------------------------------
static void FinishStream
(
    StreamState_t* streamStatePtr ///< [IN] Pointer to the Stream State-Machine data
)
{
    NetworkMessageState_t* netState =  CONTAINER_OF(streamStatePtr, NetworkMessageState_t,
                                                   streamState);
    netState->recvState = NETWORK_MSG_DONE;
    streamStatePtr->slotIndex = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Repack a cbor item
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RepackItem
(
    StreamState_t* streamStatePtr, ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr,         ///< [IN] Pointer to the Proxy Message
    void** bufferPtr               ///< [OUT] Pointer to buffer for writing the cbor item.
)
{
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;

    ssize_t additionalBytes;
    le_pack_Type_t itemType = le_pack_GetType(workBuff, &additionalBytes);


    LE_DEBUG("RPC RCV:RepackItem:lastTag %d, DispatchIdx:%d", streamStatePtr->lastTag,
            streamStatePtr->nextItemDispatchIdx);
    return DispatchTable[streamStatePtr->nextItemDispatchIdx][itemType](streamStatePtr,
            proxyMessagePtr, bufferPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 *  Process first few bytes of an async event message.
 *  About the STREAM_ASYNC_EVENT_INIT state:
 *
 *  Unlike client-request and server-response messages, we cannot create an ipc message reference
 *  for an async event message solely based on information in its header.
 *  This means we don't have any place to write the incoming message body upon streamstate
 *  initialization.
 *
 *  Async event messages follow a certain format that we use to overcome this challenge:
 *  [IPC msg ID(4bytes)][Indef Array start(1byte)][A semantic tag(1byte)][tag value(2bytes)]
 *  [Reference(1 to 5 bytes)]
 *
 *  The reference value together with rpc message header is enough to create an ipc message
 *  reference.
 *
 *  Expected number of bytes for STREAM_ASYNC_EVENT_INIT is 7, meaning we will receive until right
 *  before the reference. We simply cache the ipc message id and tag value into our stream structure
 *  and wait for the reference value. Once the reference is received, the handleReference routine
 *  will pack ipc msg id, indef array start,... before packing the reference value if the message
 *  is an async event.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleAsyncMessageStart
(
    StreamState_t* streamStatePtr  ///< [IN] Pointer to the Stream State-Machine data
)
{
    le_result_t ret = LE_OK;
    uint32_t id = 0;
    uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
    memcpy((uint8_t*) &id, workBuff, IPC_MSG_ID_SIZE);
    id = be32toh(id);
    // we're not ready to write the id yet, need to receive the reference first
    streamStatePtr->asyncMsgId = id;
    workBuff += IPC_MSG_ID_SIZE;
    le_pack_SemanticTag_t tag = 0;
    if (!le_pack_UnpackIndefArrayHeader(&workBuff))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    streamStatePtr->collectionsLayer = 1;
    if (!le_pack_UnpackSemanticTag(&workBuff, &tag))
    {
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    if (!IsTagEventHandler(tag))
    {
        LE_ERROR("Async message does not start with an event handler tag");
        ret = LE_FORMAT_ERROR;
        goto error;
    }
    streamStatePtr->lastTag = tag;
    streamStatePtr->nextItemDispatchIdx = TagIdToDispatchIdx(tag);
    GoToCborHeaderState(streamStatePtr);

error:
    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Receive an rpc stream
 * @return:
 *      - LE_OK when streaming is finished.
 *      - LE_IN_PROGRESS when streaming is still ongoing.
 *      - LE_FAULT if an error has happened
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_RecvStream
(
    void* handle,                       ///< [IN] Opaque handle to the le_comm communication channel
    StreamState_t* streamStatePtr,      ///< [IN] Pointer to the Stream State-Machine data
    void *proxyMessagePtr               ///< [IN] Pointer to the Proxy Message
)
{
    le_result_t ret = LE_OK;
    void* msgBufPtr = GetIpcMsgBufPtr(streamStatePtr);
    while(streamStatePtr->state != STREAM_DONE && ret == LE_OK)
    {
        LE_DEBUG("RecvStream State: %d", streamStatePtr->state);

        size_t remainingData = streamStatePtr->expectedSize - streamStatePtr->recvSize;
        size_t receivedSize = remainingData;
        le_result_t result = le_comm_Receive(handle, streamStatePtr->destBuff, &receivedSize);
#if RPC_PROXY_HEX_DUMP
        if (result == LE_OK)
        {
            uint8_t* buff = streamStatePtr->destBuff;
            LE_INFO("Requested:%"PRIuS" bytes, Received:%"PRIuS"", remainingData, receivedSize);
            LE_LOG_DUMP(LE_LOG_INFO, buff, receivedSize);
        }
#endif
        if (result != LE_OK)
        {
            ret = result;
            break;
        }
        else if (receivedSize > remainingData)
        {
            ret = LE_OVERFLOW;
            break;
        }
        else if (receivedSize < remainingData)
        {
            // partial data received:
            streamStatePtr->recvSize += receivedSize;
            // Were we just writing directly to ipc message buffer? if yes, move that forward.
            if (streamStatePtr->destBuff == msgBufPtr)
            {
                msgBufPtr += receivedSize;
            }
            // Move destBuff forward by the amount received
            streamStatePtr->destBuff += receivedSize;
            if (msgBufPtr)
            {
                //update ipc message offset to we know where to pickup.
                streamStatePtr->ipcMsgPayloadOffset = msgBufPtr - le_msg_GetPayloadPtr(streamStatePtr->msgRef);
            }
            ret = LE_IN_PROGRESS;
            break;
        }

        uint8_t* workBuff = (uint8_t*)streamStatePtr->workBuff;
        if (streamStatePtr->state == STREAM_CONSTANT_LENGTH_MSG)
        {
            GoToDoneState(streamStatePtr);
        }
        else if (streamStatePtr->state == STREAM_MSG_ID)
        {
            if (msgBufPtr == NULL)
            {
                ret = LE_FAULT;
            }
            else
            {
                uint32_t id = 0;
                memcpy((uint8_t*) &id, workBuff, IPC_MSG_ID_SIZE);
                id = be32toh(id);
                memcpy(msgBufPtr, (uint8_t*) &id, IPC_MSG_ID_SIZE);
                msgBufPtr += IPC_MSG_ID_SIZE;
                streamStatePtr->msgBuffSizeLeft = le_msg_GetMaxPayloadSize(streamStatePtr->msgRef);
                streamStatePtr->msgBuffSizeLeft -= IPC_MSG_ID_SIZE;
                GoToCborHeaderState(streamStatePtr);
                PrintIpcMessageInfo(proxyMessagePtr, id);
            }
        }
        else if (streamStatePtr->state == STREAM_ASYNC_EVENT_INIT)
        {
            ret = HandleAsyncMessageStart(streamStatePtr);
        }
        else if (streamStatePtr->state == STREAM_CBOR_ITEM_BODY)
        {
            if (msgBufPtr == streamStatePtr->destBuff)
            {
                // Were we just writing directly to ipc message buffer? if yes, move that forward.
                msgBufPtr += streamStatePtr->expectedSize;
                streamStatePtr->msgBuffSizeLeft -= streamStatePtr->expectedSize;
            }
            GoToCborHeaderState(streamStatePtr);
        }
        else if (streamStatePtr->state == STREAM_INTEGER_ITEM)
        {
            // At this point, we've received whatever we needed to receive to unpack
            // this item:
            ret = RepackItem(streamStatePtr, proxyMessagePtr, &msgBufPtr);
        }
        else if (streamStatePtr->state == STREAM_CBOR_HEADER)
        {
            ssize_t additionalBytes;
            le_pack_Type_t itemType = le_pack_GetType(workBuff, &additionalBytes);
            LE_UNUSED(itemType);
            if (additionalBytes <= 0)
            {
                // we can parse this item now:
                ret = RepackItem(streamStatePtr, proxyMessagePtr, &msgBufPtr);
            }
            else
            {
                GoToIntegerItemState(streamStatePtr, additionalBytes);
            }
        }
        streamStatePtr->recvSize = 0;
#if RPC_PROXY_HEX_DUMP
        //print current state of ipc message buffer:
        if (streamStatePtr->msgRef)
        {
            size_t numWritten = msgBufPtr - le_msg_GetPayloadPtr(streamStatePtr->msgRef);
            uint8_t* ipcMsgBuf = le_msg_GetPayloadPtr(streamStatePtr->msgRef);
            LE_INFO("IPC MSG buffer content so far:");
            LE_LOG_DUMP(LE_LOG_INFO, ipcMsgBuf, numWritten);
        }
#endif

    } // End-of-while
    if (ret != LE_IN_PROGRESS)
    {
        FinishStream(streamStatePtr);
    }
    if (ret != LE_OK && ret != LE_IN_PROGRESS)
    {
        // An error has happened, need to tear down connection
        rpcProxyNetwork_DeleteNetworkCommunicationChannelByHandle(handle);
    }

    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a ConnectServiceRequest Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ConnectServiceStreamInitializer
(
    StreamState_t* streamStatePtr,     ///< [IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    GoToConstantLengthMessageState(streamStatePtr, RPC_PROXY_CONNECT_SERVICE_MSG_SIZE,
                                   (uint8_t*)proxyMessagePtr + RPC_PROXY_COMMON_HEADER_SIZE);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a KeepAliveRequest Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t KeepAliveStreamInitializer
(
    StreamState_t* streamStatePtr,     ///< [IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    GoToConstantLengthMessageState(streamStatePtr, RPC_PROXY_KEEPALIVE_MSG_SIZE,
                                   (uint8_t*)proxyMessagePtr + RPC_PROXY_COMMON_HEADER_SIZE);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a ClientRequest Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ClientRequestStreamInitializer
(
    StreamState_t* streamStatePtr,     ///<[IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    rpcProxy_Message_t* clientRequestMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    clientRequestMsgPtr->metaData.isFileStreamValid = false;

    le_msg_SessionRef_t sessionRef;

    // Create a new message object and get the message buffer

    // Retrieve the Session reference for the specified Service-ID
    sessionRef = rpcProxy_GetSessionRefById(clientRequestMsgPtr->commonHeader.serviceId);

    if (sessionRef == NULL)
    {
        LE_ERROR("Unable to find matching Session Reference "
                 "in hashmap, service-id [%" PRIu32 "]",
                 clientRequestMsgPtr->commonHeader.serviceId);

        return LE_UNAVAILABLE;
    }

    LE_DEBUG("Successfully retrieved Session Reference, "
             "session safe reference [%" PRIuPTR "]",
             (uintptr_t) sessionRef);

    // Create Client Message
    streamStatePtr->msgRef = le_msg_CreateMsg(sessionRef);
    clientRequestMsgPtr->msgRef = streamStatePtr->msgRef;

    // initialize the state machine
    GoToIpcMessageIdState(streamStatePtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a ServerResponse Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ServerResponseStreamInitializer
(
    StreamState_t* streamStatePtr,     ///<[IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    rpcProxy_Message_t* serverResponseMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    serverResponseMsgPtr->metaData.isFileStreamValid = false;

    // Put this section into initializer for server response messages:
    // Retrieve Message Reference from hash map, using the Proxy Message Id
    le_msg_MessageRef_t msgRef = rpcProxy_GetMsgRefById(serverResponseMsgPtr->commonHeader.id);

    if (msgRef == NULL)
    {
        LE_ERROR("Error retrieving Message Reference, proxy id [%" PRIu32 "]",
                serverResponseMsgPtr->commonHeader.id);
        return LE_FAULT;
    }
    else
    {
        streamStatePtr->msgRef = msgRef;
        serverResponseMsgPtr->msgRef = msgRef;
    }

    // Retrieve the Session reference, using the Service-ID
    le_msg_ServiceRef_t serviceRef = rpcProxy_GetServiceRefById(
        serverResponseMsgPtr->commonHeader.serviceId);
    if (serviceRef == NULL)
    {
        LE_ERROR("Error retrieving Service Reference, service id [%" PRIu32 "]",
                serverResponseMsgPtr->commonHeader.serviceId);
        return LE_FAULT;
    }

    LE_DEBUG("Successfully retrieved Message Reference, proxy id [%" PRIu32 "]",
             serverResponseMsgPtr->commonHeader.id);

    // initialize the state machine
    GoToIpcMessageIdState(streamStatePtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a EventMessage Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EventMessageStreamInitializer
(
    StreamState_t* streamStatePtr,     ///< [IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    rpcProxy_Message_t* eventMsgPtr = (rpcProxy_Message_t*) proxyMessagePtr;
    eventMsgPtr->metaData.isFileStreamValid = false;

    // Retrieve the Service reference, using the Service-ID
    le_msg_ServiceRef_t serviceRef = rpcProxy_GetServiceRefById(eventMsgPtr->commonHeader.serviceId);

    if (serviceRef == NULL)
    {
        LE_ERROR("Error retrieving Service Reference, service id [%" PRIu32 "]",
                eventMsgPtr->commonHeader.serviceId);
        return LE_FAULT;
    }

    GoToAsyncEventInitState(streamStatePtr);
    streamStatePtr->isAsyncMsg = true;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Initializes the stream state machine to receive a FileStream Message
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileStreamMessageStreamInitializer
(
    StreamState_t* streamStatePtr,     ///< [IN] pointer to recv stream state structure
    void* proxyMessagePtr              ///< [IN] Pointer to proxy message structure
)
{
    rpcProxy_FileStreamMessage_t* fileStreamMsgPtr = (rpcProxy_FileStreamMessage_t*) proxyMessagePtr;
    fileStreamMsgPtr->metaData.isFileStreamValid = false;
    GoToCborHeaderState(streamStatePtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the stream state to receive a certain rpc message type
 * @return
 *      - LE_OK: successfully initialized the stream state machine.
 *      - otherwise and error has happened.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcProxy_InitializeStreamState
(
    StreamState_t* streamStatePtr,           ///< [IN] Pointer to stream state variable.
    void* proxyMessagePtr                    ///< [IN] Pointer to proxy message structure
)
{
    //Start by initializing everything to zero:
    memset(streamStatePtr, 0 , sizeof(StreamState_t));
    streamStatePtr->localBuffers = LE_DLS_LIST_INIT;

        rpcProxy_CommonHeader_t* commonHeaderPtr = (rpcProxy_CommonHeader_t*) proxyMessagePtr;

    return StreamStateInitializersTable[commonHeaderPtr->type](streamStatePtr, proxyMessagePtr);
}
