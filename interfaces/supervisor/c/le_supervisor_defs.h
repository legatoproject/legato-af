/** @file le_supervisor_defs.h
 *
 * This file defines types for the Supervisor API.
 *
 * @todo  This should probably not be necessary when the ifgen tool automatically includes
 *        "legato.h" and allows type definitions in the .api file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
#ifndef SUPERVISOR_API_TYPES_H_INCLUDE_GUARD
#define SUPERVISOR_API_TYPES_H_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * The Supervisor API service name.  This should be used by both the client and the server and
 * starting the service.
 */
//--------------------------------------------------------------------------------------------------
#define LE_SUPERVISOR_API               "SupervisorAPI"


#endif //SUPERVISOR_API_TYPES_H_INCLUDE_GUARD
