/**
 * @file rpcDaemonTest.c
 *
 * This file contains the source code for the RPC Proxy Unit Test.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "le_rpcProxy.h"

//--------------------------------------------------------------------------------------------------
/**
 * Extern Function Declarations
 */
//--------------------------------------------------------------------------------------------------
extern le_result_t le_rpcProxy_Initialize(void);
extern void ServerMsgRecvHandler(le_msg_MessageRef_t, void*);


//--------------------------------------------------------------------------------------------------
/**
 * Local Service for sending test session messages to the RPC Test Daemon.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_LocalService_t TestSession;

//--------------------------------------------------------------------------------------------------
/**
 * Test Service reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t TestServiceRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Test Case Variables and Definitions
 */
//--------------------------------------------------------------------------------------------------
#define _MAX_MSG_SIZE  512
#define SERVICE_INSTANCE_NAME "RPC-Proxy-Unit-Test"

// Define the message type for communicating between client and server
typedef struct __attribute__((packed))
{
    uint32_t id;
    uint8_t buffer[_MAX_MSG_SIZE];
}
Message_t;


//--------------------------------------------------------------------------------------------------
/**
 * Test Case Data
 */
//--------------------------------------------------------------------------------------------------

// Maximum number of Foo objects we expect to have at one time.
#define MAX_FOO_OBJECTS  27

// Actual Foo objects.
typedef struct
{
    uint32_t  foo;
}
Foo_t;

static le_mem_PoolRef_t FooPool;

// Safe Reference Map for Foo objects.
static le_ref_MapRef_t FooRefMap;
static Foo_t* fooPtr = NULL;

// Uint8
#define TEST_CASE_1_VALUE     0xa2
// Uint16
#define TEST_CASE_2_VALUE     0x12ff
// Uint32
#define TEST_CASE_3_VALUE     0x7734adee
// Uint64
#define TEST_CASE_4_VALUE     0x1122334455667788
// Int8
#define TEST_CASE_5_VALUE     -55
// Int16
#define TEST_CASE_6_VALUE     -12345
// Int32
#define TEST_CASE_7_VALUE     -2147483648
// Int64
#define TEST_CASE_8_VALUE     -922337206854775808
// Size
#define TEST_CASE_9_VALUE     0x91234591
// Bool
#define TEST_CASE_10_VALUE    true
// Char
#define TEST_CASE_11_VALUE    't'
// Double
#define TEST_CASE_12_VALUE    0xff124433ac123465
// Result
#define TEST_CASE_13_VALUE    LE_COMM_ERROR
// OnOff
#define TEST_CASE_14_VALUE    LE_ON

// Reference
static void* testSafeRef;
#define TEST_CASE_15_VALUE    testSafeRef

// String
#define TEST_CASE_16_VALUE    "Hello World!"

// Array
static uint8_t bufBuffer[10] = {1, 2, 3, 166, 5, 32, 7, 8, 9, 10};
#define TEST_CASE_17_VALUE    bufBuffer

// PointerTuple
#define TEST_CASE_18_VALUE    "This is a test string of some unknown length\n"
#define TEST_CASE_MAX         18


//--------------------------------------------------------------------------------------------------
/**
 * Pool for Test Session messages.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(TestSessionMessage,
                          TEST_CASE_MAX,
                          LE_MSG_LOCAL_HEADER_SIZE +
                          sizeof(Message_t));


//--------------------------------------------------------------------------------------------------
/**
 * Test Stub Function for evaluating Client-Request messages.
 */
//--------------------------------------------------------------------------------------------------
le_result_t rpcDaemonTest_ProcessClientRequest
(
    rpcProxy_Message_t* proxyMessagePtr
)
{
    uint8_t* msgBufPtr;
    uint32_t id;

    // Initialize Message Buffer Pointers
    msgBufPtr = &proxyMessagePtr->message[0];

    // Extract the the Msg ID (uint32_t) from the message payload
    memcpy((uint8_t*) &id, msgBufPtr, LE_PACK_SIZEOF_UINT32);

    msgBufPtr = msgBufPtr + LE_PACK_SIZEOF_UINT32;

    // Extract the TadID from the message payload
    TagID_t tagId = *msgBufPtr;

    // Switch on the Msg ID, which has been re-purposed
    // to identify the Test Case number
    switch (id)
    {
        case LE_PACK_UINT8:  // Test Case #1
        {
            uint8_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 1 - Tag ID Sanity Check");

            // Unpack the Uint8 value
            LE_ASSERT(le_pack_UnpackUint8(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_1_VALUE, "TEST CASE 1");
            break;
        }

        case LE_PACK_UINT16:  // Test Case #2
        {
            uint16_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 2 - Tag ID Sanity Check");

            // Unpack the Uint16 value
            LE_ASSERT(le_pack_UnpackUint16(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_2_VALUE, "TEST CASE 2");
            break;
        }

        case LE_PACK_UINT32:  // Test Case #3
        {
            uint32_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 3 - Tag ID Sanity Check");

            // Unpack the Uint32 value
            LE_ASSERT(le_pack_UnpackUint32(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_3_VALUE, "TEST CASE 3");
            break;
        }

        case LE_PACK_UINT64:  // Test Case #4
        {
            uint64_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 4 - Tag ID Sanity Check");

            // Unpack the Uint64 value
            LE_ASSERT(le_pack_UnpackUint64(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_4_VALUE, "TEST CASE 4");
            break;
        }

        case LE_PACK_INT8:  // Test Case #5
        {
            int8_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 5 - Tag ID Sanity Check");

            // Unpack the Int8 value
            LE_ASSERT(le_pack_UnpackInt8(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_5_VALUE, "TEST CASE 5");
            break;
        }

        case LE_PACK_INT16:  // Test Case #6
        {
            int16_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 6 - Tag ID Sanity Check");

            // Unpack the Int16 value
            LE_ASSERT(le_pack_UnpackInt16(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_6_VALUE, "TEST CASE 6");
            break;
        }

        case LE_PACK_INT32:  // Test Case #7
        {
            int32_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 7 - Tag ID Sanity Check");

            // Unpack the Int32 value
            LE_ASSERT(le_pack_UnpackInt32(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_7_VALUE, "TEST CASE 7");
            break;
        }

        case LE_PACK_INT64:  // Test Case #8
        {
            int64_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 8 - Tag ID Sanity Check");

            // Unpack the Int64 value
            LE_ASSERT(le_pack_UnpackInt64(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_8_VALUE, "TEST CASE 8");
            break;
        }

        case LE_PACK_SIZE:  // Test Case #9
        {
            size_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 9 - Tag ID Sanity Check");

            // Unpack the Size value
            LE_ASSERT(le_pack_UnpackSize(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_9_VALUE, "TEST CASE 9");
            break;
        }

        case LE_PACK_BOOL:  // Test Case #10
        {
            bool value;

            LE_TEST_OK(tagId == id, "TEST CASE 10 - Tag ID Sanity Check");

            // Unpack the Bool value
            LE_ASSERT(le_pack_UnpackBool(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_10_VALUE, "TEST CASE 10");
            break;
        }

        case LE_PACK_CHAR:  // Test Case #11
        {
            char value;

            LE_TEST_OK(tagId == id, "TEST CASE 11 - Tag ID Sanity Check");

            // Unpack the Char value
            LE_ASSERT(le_pack_UnpackChar(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_11_VALUE, "TEST CASE 11");
            break;
        }

        case LE_PACK_DOUBLE:  // Test Case #12
        {
            double value;

            LE_TEST_OK(tagId == id, "TEST CASE 12 - Tag ID Sanity Check");

            // Unpack the Double value
            LE_ASSERT(le_pack_UnpackDouble(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_12_VALUE, "TEST CASE 12");
            break;
        }

        case LE_PACK_RESULT:  // Test Case #13
        {
            le_result_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 13 - Tag ID Sanity Check");

            // Unpack the Result value
            LE_ASSERT(le_pack_UnpackResult(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_13_VALUE, "TEST CASE 13, value [%d], expected [%d]", value, TEST_CASE_13_VALUE);
            break;
        }

        case LE_PACK_ONOFF:  // Test Case #14
        {
            le_onoff_t value;

            LE_TEST_OK(tagId == id, "TEST CASE 14 - Tag ID Sanity Check");

            // Unpack the OnOff value
            LE_ASSERT(le_pack_UnpackOnOff(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_14_VALUE, "TEST CASE 14");
            break;
        }

        case LE_PACK_REFERENCE:  // Test Case #15
        {
            void* value = NULL;

            LE_TEST_OK(tagId == id, "TEST CASE 15 - Tag ID Sanity Check");

            // Unpack the Reference value
            LE_ASSERT(le_pack_UnpackReference(&msgBufPtr, &value));
            LE_TEST_OK(value == TEST_CASE_15_VALUE, "TEST CASE 15");
            break;
        }

        case LE_PACK_STRING:  // Test Case #16
        {
            char value[257] = {0};

            LE_TEST_OK(tagId == id, "TEST CASE 16 - Tag ID Sanity Check");

            // Unpack the String value
            LE_ASSERT(le_pack_UnpackString(&msgBufPtr, value, sizeof(value), 256));
            LE_TEST_OK(strcmp(value, TEST_CASE_16_VALUE) == 0, "TEST CASE 16");
            break;
        }

        case LE_PACK_ARRAYHEADER:  // Test Case #17
        {
            size_t bufSize = 0;
            uint8_t bufPtr[10] = { 0 };
            bool bufResult;

            LE_TEST_OK(tagId == id, "TEST CASE 17 - Tag ID Sanity Check");

            // Unpack the Array value
            LE_PACK_UNPACKARRAY( &msgBufPtr, bufPtr, &bufSize, 10, le_pack_UnpackUint8, &bufResult );
            LE_ASSERT(bufResult);

            LE_TEST_OK(memcmp(bufPtr, TEST_CASE_17_VALUE, sizeof(bufPtr)) == 0, "TEST CASE 17");
            break;
        }

        case LE_PACK_POINTERTUPLE:  // Test Case #18
        {
            char value[257];
            size_t bufSize;

            LE_TEST_OK(tagId == id, "TEST CASE 18 - Tag ID Sanity Check");

            LE_ASSERT(le_pack_UnpackSize( &msgBufPtr, &bufSize ));
            LE_TEST_OK(bufSize == sizeof(TEST_CASE_18_VALUE), "TEST CASE 18 - Size Sanity Check");

#if UINTPTR_MAX == UINT32_MAX
            uint32_t pointer;
            LE_ASSERT(le_pack_UnpackUint32( &msgBufPtr, (uint32_t*)&pointer ));
#elif UINTPTR_MAX == UINT64_MAX
            uint64_t pointer;
            LE_ASSERT(le_pack_UnpackUint64( &msgBufPtr, (uint64_t*)&pointer ));
#endif
            // Copy the data into new message buffer
            memcpy(value, (uint8_t*) pointer, bufSize);
            LE_TEST_OK(memcmp(value, TEST_CASE_18_VALUE, bufSize) == 0, "TEST CASE 18");
            break;
        }

        default:
            LE_WARN("Unsupport Test Case #[%" PRIu32 "]", id);
            break;
    }

    if (id == TEST_CASE_MAX)
    {
        // End the test sequence.
        LE_TEST_EXIT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initializes Test Session
 */
//--------------------------------------------------------------------------------------------------
void testSession_Init
(
    const char* serviceInstanceName
)
{
    TestServiceRef = le_msg_InitLocalService(&TestSession,
                                             serviceInstanceName,
                                             le_mem_InitStaticPool(TestSessionMessage,
                                                                   TEST_CASE_MAX,
                                                                   LE_MSG_LOCAL_HEADER_SIZE +
                                                                   sizeof(Message_t)));

    // Create the Foo object pool.
    FooPool = le_mem_CreatePool("FooPool", sizeof(Foo_t));
    le_mem_ExpandPool(FooPool, MAX_FOO_OBJECTS);

    // Create the Safe Reference Map to use for Foo object Safe References.
    FooRefMap = le_ref_CreateMap("FooMap", MAX_FOO_OBJECTS);

    fooPtr = le_mem_ForceAlloc(FooPool);

    // Create and return a Safe Reference for this Foo object.
    testSafeRef = le_ref_CreateRef(FooRefMap, fooPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t result;
    le_msg_SessionRef_t sessionRef;  // Test Session Reference
    le_msg_MessageRef_t msgRef;      // Test Message
    Message_t* msgPtr;
    __attribute__((unused)) uint8_t* msgBufPtr;
    le_msg_LocalMessage_t* localMsgPtr;

    LE_INFO("======== RPC Proxy Unit-Test ========");

    // Initialize TestSession
    testSession_Init(SERVICE_INSTANCE_NAME);

    // Initialize RPC Proxy component
    result = le_rpcProxy_Initialize();
    if (result != LE_OK)
    {
        LE_FATAL("Error initializing RPC Proxy Test daemon, result [%d]", result);
    }


    LE_INFO("======== Preparing Unit-Test of RPC Proxy ========");

    // Create a Test Session
    sessionRef = le_msg_CreateLocalSession(&TestSession);

    LE_INFO("======== Starting Unit-Test of RPC Proxy ========");

    LE_TEST_PLAN((TEST_CASE_MAX * 2) + 1);

    //
    // Create Test-Case #1 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_UINT8; // 1
    LE_ASSERT(le_pack_PackUint8( &msgBufPtr, TEST_CASE_1_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);


    //
    // Create Test-Case #2 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_UINT16; // 2
    LE_ASSERT(le_pack_PackUint16( &msgBufPtr, TEST_CASE_2_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #3 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_UINT32; // 3
    LE_ASSERT(le_pack_PackUint32( &msgBufPtr, TEST_CASE_3_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #4 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_UINT64; // 4
    LE_ASSERT(le_pack_PackUint64( &msgBufPtr, TEST_CASE_4_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #5 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_INT8; // 5
    LE_ASSERT(le_pack_PackInt8( &msgBufPtr, TEST_CASE_5_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #6 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_INT16; // 6
    LE_ASSERT(le_pack_PackInt16( &msgBufPtr, TEST_CASE_6_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #7 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_INT32; // 7
    LE_ASSERT(le_pack_PackInt32( &msgBufPtr, TEST_CASE_7_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #8 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_INT64; // 8
    LE_ASSERT(le_pack_PackInt64( &msgBufPtr, TEST_CASE_8_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #9 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_SIZE; // 9
    LE_ASSERT(le_pack_PackSize( &msgBufPtr, TEST_CASE_9_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #10 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_BOOL; // 10
    LE_ASSERT(le_pack_PackBool( &msgBufPtr, TEST_CASE_10_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #11 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_CHAR; // 11
    LE_ASSERT(le_pack_PackChar( &msgBufPtr, TEST_CASE_11_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #12 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_DOUBLE; // 12
    LE_ASSERT(le_pack_PackDouble( &msgBufPtr, TEST_CASE_12_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #13 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_RESULT; // 13
    LE_ASSERT(le_pack_PackResult( &msgBufPtr, TEST_CASE_13_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #14 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_ONOFF; // 14
    LE_ASSERT(le_pack_PackOnOff( &msgBufPtr, TEST_CASE_14_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #15 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_REFERENCE; // 15
    LE_ASSERT(le_pack_PackReference( &msgBufPtr, TEST_CASE_15_VALUE ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #16 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_STRING; // 16
    LE_ASSERT(le_pack_PackString( &msgBufPtr, TEST_CASE_16_VALUE, sizeof(TEST_CASE_16_VALUE) ));
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #17 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_ARRAYHEADER; // 17

    bool bufResult;
    uint8_t *bufPtr = TEST_CASE_17_VALUE;
    size_t bufSize = sizeof(bufBuffer);
    size_t *bufSizePtr = &bufSize;

    LE_PACK_PACKARRAY( &msgBufPtr, bufPtr, (*bufSizePtr), 10, le_pack_PackUint8, &bufResult );
    LE_ASSERT(bufResult);
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    //
    // Create Test-Case #18 Legato Message
    //
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgBufPtr = msgPtr->buffer;

    localMsgPtr = CONTAINER_OF(msgRef, le_msg_LocalMessage_t, message);
    localMsgPtr->needsResponse = true;

    msgPtr->id = LE_PACK_POINTERTUPLE; // 18

    char value[257];
    memcpy(value, TEST_CASE_18_VALUE, sizeof(TEST_CASE_18_VALUE));

    LE_ASSERT(le_pack_PackTaggedSize( &msgBufPtr, sizeof(TEST_CASE_18_VALUE), LE_PACK_POINTERTUPLE ));
#if UINTPTR_MAX == UINT32_MAX
    LE_ASSERT(le_pack_PackUint32( &msgBufPtr, (uint32_t)value ));
#elif UINTPTR_MAX == UINT64_MAX
    LE_ASSERT(le_pack_PackUint64( &msgBufPtr, (uint64_t)value ));
#endif
    // Call the RPC Proxy Message Receive Handler
    ServerMsgRecvHandler(msgRef, SERVICE_INSTANCE_NAME);
    le_msg_ReleaseMsg(msgRef);

    LE_INFO("======== Finished RPC Proxy Unit-Test ========");
}