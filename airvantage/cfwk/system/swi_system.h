/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/**
* @file
* @brief This API provides operating system level facilities.
*
* <HR>
*/

#ifndef SWI_SYSTEM_INCLUDE_GUARD
#define SWI_SYSTEM_INCLUDE_GUARD

#include "returncodes.h"

/**
* Initializes the module.
* A call to init is mandatory to enable System library API.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sys_Init();

/**
* Destroys the System library.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sys_Destroy();

/**
* Requests a reboot of the system, with an optional reason passed as a string, which will be logged.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sys_Reboot
(
    const char* reasonPtr ///< [IN] the (logged) reason why the reboot is requested
);


#endif /* SWI_SYSTEM_INCLUDE_GUARD */
