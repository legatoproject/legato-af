/**
 * @file lwm2m.h
 *
 * Interface for LWM2M handler sub-component
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#ifndef LEGATO_LWM2M_INCLUDE_GUARD
#define LEGATO_LWM2M_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Interface functions
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Sends a registration update to the server.
 */
//--------------------------------------------------------------------------------------------------
void lwm2m_RegistrationUpdate
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Sends a registration update if observe is not enabled. A registration update would also be sent
 * if the instanceRef is not valid.
 */
//--------------------------------------------------------------------------------------------------
void lwm2m_RegUpdateIfNotObserved
(
    assetData_InstanceDataRef_t instanceRef    ///< The instance of object 9.
);



//--------------------------------------------------------------------------------------------------
/**
 * Init this sub-component
 */
//--------------------------------------------------------------------------------------------------
le_result_t lwm2m_Init
(
    void
);

#endif // LEGATO_LWM2M_INCLUDE_GUARD
