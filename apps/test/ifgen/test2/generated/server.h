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
 * Function takes all the possible kinds of parameters, but returns nothing
 */
//--------------------------------------------------------------------------------------------------
void allParameters
(
    common_EnumExample_t a,
        ///< [IN] first one-line comment
        ///<      second one-line comment

    uint32_t* bPtr,
        ///< [OUT]

    const uint32_t* dataPtr,
        ///< [IN]

    size_t dataNumElements,
        ///< [IN]

    uint32_t* outputPtr,
        ///< [OUT] some more comments here
        ///<       and some comments here as well

    size_t* outputNumElementsPtr,
        ///< [INOUT]

    const char* label,
        ///< [IN]

    char* response,
        ///< [OUT] comments on final parameter, first line
        ///<       and more comments

    size_t responseNumElements,
        ///< [IN]

    char* more,
        ///< [OUT] This parameter tests a bug fix

    size_t moreNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Test file descriptors as IN and OUT parameters
 */
//--------------------------------------------------------------------------------------------------
void FileTest
(
    int dataFile,
        ///< [IN] file descriptor as IN parameter

    int* dataOutPtr
        ///< [OUT] file descriptor as OUT parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * This function fakes an event, so that the handler will be called.
 * Only needed for testing.  Would never exist on a real system.
 */
//--------------------------------------------------------------------------------------------------
void TriggerTestA
(
    void
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
 * Function to fake an event for the callback function testing
 */
//--------------------------------------------------------------------------------------------------
void TriggerCallbackTest
(
    uint32_t data
        ///< [IN]
);


#endif // INTERFACE_H_INCLUDE_GUARD

