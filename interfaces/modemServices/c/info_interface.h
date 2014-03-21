/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */


#ifndef INFO_INTERFACE_H_INCLUDE_GUARD
#define INFO_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_info_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_info_StopClient
(
    void
);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetImei
(
    char* imei,
        ///< [OUT]

    size_t imeiNumElements
        ///< [IN]
);


#endif // INFO_INTERFACE_H_INCLUDE_GUARD

