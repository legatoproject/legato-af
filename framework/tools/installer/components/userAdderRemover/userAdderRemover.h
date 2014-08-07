//--------------------------------------------------------------------------------------------------
/**
 * User Add/Remove API provided by the appConfig component.
 *
 * Copyright 2014, Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef USER_ADDER_REMOVER_H_INCLUDE_GUARD
#define USER_ADDER_REMOVER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Add an application's user to the system.
 **/
//--------------------------------------------------------------------------------------------------
void userAddRemove_Add
(
    const char* appName
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes an application's user from the system.
 **/
//--------------------------------------------------------------------------------------------------
void userAddRemove_Remove
(
    const char* appName
);



#endif // USER_ADDER_REMOVER_H_INCLUDE_GUARD
