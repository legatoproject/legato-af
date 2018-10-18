//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the code implementation of the support for networking
 *  APIs and functionalities.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSNET_H_INCLUDE_GUARD
#define LEGATO_DCSNET_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Function for retrieving the network interface state of the given network interface in the 1st
 * argument
 *
 * @return
 *     - The function returns the retrieved channel state in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_net_GetNetIntfState
(
    const char *connIntf,
    bool *state
);


#endif
