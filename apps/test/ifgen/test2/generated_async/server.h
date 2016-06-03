/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef INTERFACE_H_INCLUDE_GUARD
#define INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Command reference for async server-side function support.  The interface function receives the
 * reference, and must pass it to the corresponding respond function.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ServerCmd* ServerCmdRef_t;

// Interface specific includes
#include "common_server.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void AdvertiseService
(
    void
);


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define TEN 10


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define TWENTY 20


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define SOME_STRING "some string"


//--------------------------------------------------------------------------------------------------
/**
 * BITMASK example
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    A = 0x1,
        ///< first

    B = 0x8,
        ///< second

    C = 0x10
        ///< third
}
BitMaskExample_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'TestA'
 */
//--------------------------------------------------------------------------------------------------
typedef struct TestAHandler* TestAHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'BugTest'
 */
//--------------------------------------------------------------------------------------------------
typedef struct BugTestHandler* BugTestHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler definition
 *
 * @param x
 *        First parameter for the handler
 *             Second comment line is indented 5 extra spaces
 *        Third comment line is missing initial space
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TestAHandlerFunc_t)
(
    int32_t x,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Handler definition for testing bugs
 *
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*BugTestHandlerFunc_t)
(
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Define handler for callback testing
 *
 * @param data
 * @param name
 * @param dataFile
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*CallbackTestHandlerFunc_t)
(
    uint32_t data,
    const char* name,
    int dataFile,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'TestA'
 *
 * This event is used for testing EVENTS and Add/Remove handler functions
 */
//--------------------------------------------------------------------------------------------------
TestAHandlerRef_t AddTestAHandler
(
    TestAHandlerFunc_t myHandlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'TestA'
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestAHandler
(
    TestAHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for allParameters
 */
//--------------------------------------------------------------------------------------------------
void allParametersRespond
(
    ServerCmdRef_t _cmdRef,
    uint32_t b,
    size_t outputNumElements,
    uint32_t* outputPtr,
    char* response,
    char* more
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void allParameters
(
    ServerCmdRef_t _cmdRef,
    common_EnumExample_t a,
    const uint32_t* dataPtr,
    size_t dataNumElements,
    size_t outputNumElements,
    const char* label,
    size_t responseNumElements,
    size_t moreNumElements
);

//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for FileTest
 */
//--------------------------------------------------------------------------------------------------
void FileTestRespond
(
    ServerCmdRef_t _cmdRef,
    int dataOut
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void FileTest
(
    ServerCmdRef_t _cmdRef,
    int dataFile
);

//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for TriggerTestA
 */
//--------------------------------------------------------------------------------------------------
void TriggerTestARespond
(
    ServerCmdRef_t _cmdRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void TriggerTestA
(
    ServerCmdRef_t _cmdRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'BugTest'
 *
 * This event
 * is used for
 *     testing
 * a specific bug, as well as event comment strings.
 *
 * Uses old-style handler, for backwards compatibility testing
 */
//--------------------------------------------------------------------------------------------------
BugTestHandlerRef_t AddBugTestHandler
(
    const char* newPathPtr,
        ///< [IN]

    BugTestHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'BugTest'
 */
//--------------------------------------------------------------------------------------------------
void RemoveBugTestHandler
(
    BugTestHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Test function callback parameters
 */
//--------------------------------------------------------------------------------------------------
int32_t TestCallback
(
    uint32_t someParm,
        ///< [IN]

    const uint8_t* dataArrayPtr,
        ///< [IN]

    size_t dataArrayNumElements,
        ///< [IN]

    CallbackTestHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Server-side respond function for TriggerCallbackTest
 */
//--------------------------------------------------------------------------------------------------
void TriggerCallbackTestRespond
(
    ServerCmdRef_t _cmdRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for server-side async interface function
 */
//--------------------------------------------------------------------------------------------------
void TriggerCallbackTest
(
    ServerCmdRef_t _cmdRef,
    uint32_t data
);


#endif // INTERFACE_H_INCLUDE_GUARD

