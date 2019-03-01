

/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef TEMPERATURE_INTERFACE_H_INCLUDE_GUARD
#define TEMPERATURE_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t temperature_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t temperature_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the server and advertise the service.
 */
//--------------------------------------------------------------------------------------------------
void temperature_AdvertiseService
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type used by Add/Remove functions for EVENT 'temperature_UpdatedValue'
 */
//--------------------------------------------------------------------------------------------------
typedef struct temperature_UpdatedValueHandler* temperature_UpdatedValueHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 */
//--------------------------------------------------------------------------------------------------
typedef void (*temperature_UpdatedValueHandlerFunc_t)
(
    int32_t value,
        ///< temperature in F
    void* contextPtr
        ///<
);


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'temperature_UpdatedValue'
 */
//--------------------------------------------------------------------------------------------------
temperature_UpdatedValueHandlerRef_t temperature_AddUpdatedValueHandler
(
    temperature_UpdatedValueHandlerFunc_t handlerPtr,
        ///< [IN]
    void* contextPtr
        ///< [IN]
);



//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'temperature_UpdatedValue'
 */
//--------------------------------------------------------------------------------------------------
void temperature_RemoveUpdatedValueHandler
(
    temperature_UpdatedValueHandlerRef_t handlerRef
        ///< [IN]
);


#endif // TEMPERATURE_INTERFACE_H_INCLUDE_GUARD