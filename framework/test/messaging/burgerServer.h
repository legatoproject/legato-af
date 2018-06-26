//--------------------------------------------------------------------------------------------------
/**
 * Automated unit test for the Low-Level Messaging APIs.
 *
 * Burger Protocol Server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef BURGER_SERVER_H_INCLUDE_GUARD
#define BURGER_SERVER_H_INCLUDE_GUARD

#if defined(TEST_LOCAL)
//--------------------------------------------------------------------------------------------------
/**
 * On RTOS, use a local service.
 */
//--------------------------------------------------------------------------------------------------
extern le_msg_LocalService_t BurgerService;
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Initializes burger server.  On non-Linux systems must be run before any client can even try
 * to connect, but doesn't need to be run on server thread.
 */
//--------------------------------------------------------------------------------------------------
void burgerServer_Init
(
    const char* serviceInstanceName
);


//--------------------------------------------------------------------------------------------------
/**
 * Starts an instance of the Burger Protocol server in the calling thread.
 **/
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t burgerServer_Start
(
    const char* serviceInstanceName,
    size_t maxRequests
);

#endif // BURGER_SERVER_H_INCLUDE_GUARD
