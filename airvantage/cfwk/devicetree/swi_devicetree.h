/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/


/**
 * @file
 * @brief This file gives an set/get/notify API on the device tree.
 *
 * This module provides read/write access to system parameters and settings, as well as
 * the ability to receive notifications when they change.
 * System parameters and settings are addressed based on a predefined tree (the "Device Tree")
 * that organizes them based on functionality.
 * This tree will continue to be developed and expanded as the Application Framework evolves.
 * <HR>
 *
 */

#ifndef SWI_DEVICETREE_INCLUDE_GUARD
#define SWI_DEVICETREE_INCLUDE_GUARD

#include <stdlib.h>
#include <stdbool.h>
#include "returncodes.h"
#include "swi_dset.h"


/**
* Initializes the module.
* A call to init is mandatory to enable DeviceTree library APIs.
*
* @return RC_OK on success.
*/
rc_ReturnCode_t swi_dt_Init();


/**
* Destroys the DeviceTree library.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_dt_Destroy();

/**
* Retrieves a variable's value from the device tree.
*
* There are 2 cases: either the requested path is a node or it is a value.
*
* Leaf case:
* When retrieving a leaf, the result will be put as the single element in a #swi_dset_Iterator_t object, and the `isLeaf` parameter will
* be set to `true`.
*
* Internal Node case:
* The retrieval is not recursive: asking for a path prefix (i.e. a node) will not return every key/value pairs sharing this prefix.
* Instead, the `isLeaf` parameter will be set to `false` and the result will be a list of element put in a #swi_dset_Iterator_t object, the list provides
* the absolute and relatives paths to the prefix's direct children.
* Each iterator element will give the name of one sub-element of the requested node.
*
* For instance, if the branch foo.bar contains { x=1, y={z1=2, z2=3}}, a get("foo.bar") will return { "foo.bar.x"="x", "foo.bar.y"="y" }.
* No children value is returned, and the children of y, which are the grand-children of foo.bar, are not iterated.
*
* @return RC_OK on success
* @return RC_NOT_FOUND when requested path was not found
*
*/
rc_ReturnCode_t swi_dt_Get
(
    const char* pathPtr,            ///< [IN] the path to retrieve, can be a leaf or a node.
    swi_dset_Iterator_t** data,     ///< [OUT] the data iterator containing the results of the Get operation.
                                    ///<       The DeviceTree library will allocate data iterator resources.
                                    ///<       The user is responsible to release the data iterator resources using #swi_dset_Destroy function.
    bool *isLeaf                    ///< [OUT] Optional output boolean parameter to indicate that the request was on a leaf node (`true` is returned)
                                    ///<       or on an internal node (`false` is returned)
);


/* Code sample for DT_Get
 * data structure:
 * system = { gprs = { rssi=33, apn="m2m", ip ="0.0.0.0" }, eth = { ip="192.168.0.42"}}
 *
 * DT_Init();
 * swi_dset_Iterator* data;
 *
 * ...
 *
 * //Interior Get:
 * rc_ReturnCode_t res = RC_OK;
 * bool isLeaf = false;
 * res = swi_dt_Get("system.gprs", &data, &isLeaf)
 * assert(RC_OK == res && !isLeaf);
 * do{
 *  const char* name, value;
 *  swi_dset_Type type  = swi_dset_GetType(data);
 *  name = swi_dset_GetName(data)
 *  assert(NULL !=  name);
 *  printf("name = %s, type = %s \n"); // -> "name = "system.gprs.rssi" then "name = "system.gprs.apn" and "name = "system.gprs.ip" , "type = string" for each element
 *  value = swi_dset_ToString(data);
 *  printf("value = %s"); //-> "value = rssi" then "value = apn" and "value = ip"
 * }
 * while(RC_OK == swi_dset_Next(data));
 * swi_dset_Destroy(data);
 *
 * //Leaf Get:
 *
 * rc_ReturnCode_t res = RC_OK;
 * bool isLeaf = false;
 * res = swi_dt_Get("system.gprs.rssi", &data, &isLeaf)
 * assert(RC_OK == res && isLeaf);
 * int64_t value=0;
 * assert(RC_OK == swi_dset_ToInteger(data, &value));
 * swi_dset_Destroy(data);
 *
 */

/**
* Retrieves several variable values from the device tree.
*
* Only leaf paths will be retrieved, any node path will be silently discarded.
* Each leaf value will be put as an element in the #swi_dset_Iterator_t object.
* Each element's name will be the full path of the variable.
*
* @return RC_OK on success
* @return RC_NOT_FOUND when one of the requested path was not found,
*                                 then the whole request will fail and no value is returned.
*
*/
rc_ReturnCode_t swi_dt_MultipleGet
(
    size_t numVars,             ///< [IN] the number of variables to retrieve.
    const char** pathPtr,       ///< [IN] pointer to an array of strings (with numVars entries), contains the path of each variable to retrieve.
    swi_dset_Iterator_t** data ///< [OUT] the data iterator containing the results of the Get operation.
                                ///<       The DeviceTree library will allocate data iterator resources.
                                ///<       The user is responsible to release the data iterator resources using #swi_dset_Destroy function.
);

/**
* Sets an integer variable value into the variable tree.
*
* @return RC_OK on success
* @return RC_NOT_PERMITTED when requesting non-leaf value
* @return RC_NOT_FOUND when requested path was not found
*/
rc_ReturnCode_t swi_dt_SetInteger
(
    const char* pathPtr, ///< [IN] the path to variable to set in device tree.
    int value            ///< [IN] integer value to set
);

/**
* Sets a float variable value into the variable tree.
*
* @return RC_OK on success
* @return RC_NOT_PERMITTED when requesting non-leaf value
* @return RC_NOT_FOUND when requested path was not found
*/
rc_ReturnCode_t swi_dt_SetFloat
(
    const char* pathPtr, ///< [IN] the path to variable to set in device tree.
    double value         ///< [IN] float value to set
);

/**
* Sets a string variable value into the variable tree.
*
* @return RC_OK on success
* @return RC_NOT_PERMITTED when requesting non-leaf value
* @return RC_NOT_FOUND when requested path was not found
*/
rc_ReturnCode_t swi_dt_SetString
(
    const char* pathPtr, ///< [IN] pathPtr the path to variable to set in device tree.
    const char* valuePtr ///< [IN] string value to set
);


/**
* Sets a NULL variable value into the variable tree.
*
* @return RC_OK on success
* @return RC_NOT_PERMITTED when requesting non-leaf value
* @return RC_NOT_FOUND when requested path was not found
*/
rc_ReturnCode_t swi_dt_SetNull
(
    const char* pathPtr ///< [IN] pathPtr the path to variable to set in device tree.
);


/**
* Sets a boolean variable value into the variable tree.
*
* @return RC_OK on success
* @return RC_NOT_PERMITTED when requesting non-leaf value
* @return RC_NOT_FOUND when requested path was not found
*/
rc_ReturnCode_t swi_dt_SetBool
(
    const char* pathPtr, ///< [IN] pathPtr the path to variable to set in device tree.
    bool value ///< [IN] boolean value to set
);



/**
* Variable change notification callback.
*
* It will be called when a watched variable has changed.
*
* @param data [IN]  data iterator containing the variable values.
*                   The data iterator will be allocated by DeviceTree library,
*                   and automatically released when the callback returns.
*
*/
typedef void (* swi_dt_NotifyCB_t)
(
        swi_dset_Iterator_t* data ///<
);

/**
 * Registration identifier.
 * Used to identify an registration so that it can be unregistered afterwards.
 */
typedef void * swi_dt_regId_t;

/**
* Registers to receive a notification when one or several variables change.
*
* The callback will be called in a new pthread.
*
* The callback is triggered every time one or some of the variables listed in regvars changes.
* It receives as parameter a swi_dset_Iterator_t with variable-name / variable-value pairs;
* these variables are all the variables listed in regvars which have changed,
* plus every variables listed in passivevars, whether their values changed or not.
*
* Please note that the callback can be called with swi_dset_Iterator_t param containing #swi_dset_Type_t #SWI_DSET_NIL type to indicate variable(s) deletion.
* Variables listed in regvars and passivevars can be either fully qualified variable names, or a path which denotes every individual variable below this path.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_dt_Register
(
    size_t numRegVars,           ///< [IN] the number of variables to register to.
    const char** regVarsPtr,     ///< [IN] pointer to an array of strings (with numRegVars entries), the variables which must be monitored for change.
    swi_dt_NotifyCB_t cb,        ///< [IN] the function triggered every time a regvars variable changes.
                                 ///<      The callback is called with a #swi_dset_Iterator_t containing all the changes,
                                 ///<      paths as element names and values as values.
    size_t numPassiveVars,       ///< [IN] the number of passive variables to receive.
    const char** passiveVarsPtr, ///< [IN] pointer to an array of strings (with numPassiveVars entries), optional variables to always pass to callback,
                                 ///<      whether they changed or not.
    swi_dt_regId_t* regIdPtr     ///< [OUT] identifier of the registration, to be used to cancel it afterward using swi_dt_Unregister function.
);


/**
* Cancels registration to receive a notification when a variable changes.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_dt_Unregister(
    swi_dt_regId_t regId ///< [IN] identifier of the registration to cancel, the id returned by previous swi_dt_Register call.
);

#endif /* SWI_DEVICETREE_INCLUDE_GUARD */
