/** @file messaging.h
 *
 * Low-Level Messaging API implementation's main module's inter-module interface definitions.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef MESSAGING_H_INCLUDE_GUARD
#define MESSAGING_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the messaging module.  This must be called only once at start-up, before
 * any other functions in that module are called.
 */
//--------------------------------------------------------------------------------------------------
void msg_Init
(
    void
);


#endif // MESSAGING_H_INCLUDE_GUARD
