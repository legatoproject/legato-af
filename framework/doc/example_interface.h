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
 * Reference type for example_TestA handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct example_TestA* example_TestARef_t;


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
typedef void (*example_TestAFunc_t)
(
    int32_t x,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * example_TestA handler ADD function
 */
//--------------------------------------------------------------------------------------------------
example_TestARef_t example_AddTestA
(
    example_TestAFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * example_TestA handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void example_RemoveTestA
(
    example_TestARef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Function takes all the possible kinds of parameters, but returns nothing
 */
//--------------------------------------------------------------------------------------------------
void example_allParameters
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


#endif // EXAMPLE_H_INCLUDE_GUARD

