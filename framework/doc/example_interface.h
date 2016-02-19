/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * Example API file
 */

#ifndef EXAMPLE_H_INCLUDE_GUARD
#define EXAMPLE_H_INCLUDE_GUARD


#include "legato.h"

// Interface specific includes
#include "defn_interface.h"
#include "common_interface.h"


//--------------------------------------------------------------------------------------------------
/**
 * Connect the client to the service
 */
//--------------------------------------------------------------------------------------------------
void example_ConnectService
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect the client from the service
 */
//--------------------------------------------------------------------------------------------------
void example_DisconnectService
(
    void
);


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define EXAMPLE_TEN 10


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define EXAMPLE_TWENTY 20


//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
#define EXAMPLE_SOME_STRING "some string"


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'example_TestA'
 */
//--------------------------------------------------------------------------------------------------
typedef struct example_TestAHandler* example_TestAHandlerRef_t;


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
typedef void (*example_TestAHandlerFunc_t)
(
    int32_t x,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'example_TestA'
 *
 * This event provides an example of an EVENT definition
 */
//--------------------------------------------------------------------------------------------------
example_TestAHandlerRef_t example_AddTestAHandler
(
    uint32_t data,
        ///< [IN]
        ///< Used when registering the handler i.e. it is
        ///< passed into the generated ADD_HANDLER function.

    example_TestAHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'example_TestA'
 */
//--------------------------------------------------------------------------------------------------
void example_RemoveTestAHandler
(
    example_TestAHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Function takes all the possible kinds of parameters, but returns nothing
 */
//--------------------------------------------------------------------------------------------------
void example_AllParameters
(
    common_EnumExample_t a,
        ///< [IN]
        ///< first one-line comment
        ///< second one-line comment

    uint32_t* bPtr,
        ///< [OUT]

    const uint32_t* dataPtr,
        ///< [IN]

    size_t dataNumElements,
        ///< [IN]

    uint32_t* outputPtr,
        ///< [OUT]
        ///< some more comments here
        ///< and some comments here as well

    size_t* outputNumElementsPtr,
        ///< [INOUT]

    const char* label,
        ///< [IN]

    char* response,
        ///< [OUT]
        ///< comments on final parameter, first line
        ///< and more comments

    size_t responseNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Test file descriptors as IN and OUT parameters
 */
//--------------------------------------------------------------------------------------------------
void example_FileTest
(
    int dataFile,
        ///< [IN]
        ///< file descriptor as IN parameter

    int* dataOutPtr
        ///< [OUT]
        ///< file descriptor as OUT parameter
);

//--------------------------------------------------------------------------------------------------
/**
 * Function that takes a handler parameter
 */
//--------------------------------------------------------------------------------------------------
int32_t example_UseCallback
(
    uint32_t someParm,
        ///< [IN]

    example_TestAHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);


#endif // EXAMPLE_H_INCLUDE_GUARD

