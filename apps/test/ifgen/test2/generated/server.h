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


typedef void (*TestAFunc_t)
(
    int32_t x,
    void* contextPtr
);


typedef void (*BugTestFunc_t)
(
    void* contextPtr
);

typedef struct TestA* TestARef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
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
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void RemoveTestA
(
    TestARef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void allParameters
(
    int32_t a,
        ///< [IN]

    uint32_t* bPtr,
        ///< [OUT]

    const uint32_t* dataPtr,
        ///< [IN]

    size_t dataNumElements,
        ///< [IN]

    uint32_t* outputPtr,
        ///< [OUT]

    size_t* outputNumElementsPtr,
        ///< [INOUT]

    const char* label,
        ///< [IN]

    char* response,
        ///< [OUT]

    size_t responseNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
void TriggerTestA
(
    void
);

typedef struct BugTest* BugTestRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
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
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void RemoveBugTest
(
    BugTestRef_t addHandlerRef
        ///< [IN]
);


#endif // SERVER_H_INCLUDE_GUARD

