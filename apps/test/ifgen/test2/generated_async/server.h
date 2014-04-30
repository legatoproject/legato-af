/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef SERVER_H_INCLUDE_GUARD
#define SERVER_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Command reference for async server-side function support.  The interface function receives the
 * reference, and must pass it to the corresponding respond function.
 */
//--------------------------------------------------------------------------------------------------
typedef struct ServerCmd* ServerCmdRef_t;

// Interface specific includes
#include "common_interface.h"


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
 * Initialize and start the server.
 */
//--------------------------------------------------------------------------------------------------
void StartServer
(
    const char* serviceInstanceName
        ///< [IN]
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
 * Reference type for TestA handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct TestA* TestARef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for BugTest handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct BugTest* BugTestRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler definition
 *
 * @param x
 *        First parameter for the handler
 *        Second comment line
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*TestAFunc_t)
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
typedef void (*BugTestFunc_t)
(
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * TestA handler ADD function
 */
//--------------------------------------------------------------------------------------------------
TestARef_t AddTestA
(
    TestAFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * TestA handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestA
(
    TestARef_t addHandlerRef
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
 * BugTest handler ADD function
 */
//--------------------------------------------------------------------------------------------------
BugTestRef_t AddBugTest
(
    const char* newPathPtr,
        ///< [IN]

    BugTestFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * BugTest handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void RemoveBugTest
(
    BugTestRef_t addHandlerRef
        ///< [IN]
);


#endif // SERVER_H_INCLUDE_GUARD

