//--------------------------------------------------------------------------------------------------
/**
 * Application User Add/Remove API.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef APP_USER_H_INCLUDE_GUARD
#define APP_USER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's user to the system.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t appUser_Add
(
    const char* appName
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's user from the system.
 *
 * @return LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t appUser_Remove
(
    const char* appName
);



#endif // APP_USER_H_INCLUDE_GUARD
