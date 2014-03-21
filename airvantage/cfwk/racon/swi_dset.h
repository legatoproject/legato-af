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
 * @brief Data Set API provides ways to manipulate incoming data.
 * @ingroup common
 *
 * <HR>
 */

#ifndef SWI_DSET_INCLUDE_GUARD
#define SWI_DSET_INCLUDE_GUARD

#include <stdint.h>
#include <stdbool.h>
#include "returncodes.h"

/**
 * Enum to define data types supported in Data Set object
 */
typedef enum swi_dset_Type
{
  SWI_DSET_UNSUPPORTED, ///< For data type that will not be supported in the C API.
  SWI_DSET_NIL,         ///< Used to indicate a deletion or an absence of variable.
  SWI_DSET_INTEGER,     ///< Integer number, usually put into "int64_t" C type.
  SWI_DSET_FLOAT,       ///< Float number, usually put into "double" C type.
  SWI_DSET_STRING,       ///< String, usually put into "const char*" C type.
  SWI_DSET_BOOL          ///< Bool number, usually put into "bool" C type.
} swi_dset_Type_t;


/**
* struct to deal with incoming data.
*
* swi_dset_Iterator_t has one current element and each element has
*  - a name
*  - a type (#swi_dset_Type_t)
*  - a value
*
* 2 different kind of APIs are provided:
*  - one to iterate over the data, especially useful when the user has no expectation of the types/names of the received data.
*  - the other one to query the received value using the name of the variables.
*
* Please note that:
*  - it is strongly advised not to mix up the use of the 2 kinds of APIs on the same data iterator.
*  - the order of the elements is not guaranteed, so the caller is strongly advised to rely on elements' name rather than on the order.
*
* Memory allocation will be done internally by the functions returning the data iterator.
* Data availability (i.e. memory deallocation) will be specified in the documentation of each function providing this kind of #swi_dset_Iterator_t.
*
*/
typedef struct swi_dset_Iterator swi_dset_Iterator_t;



/**
* Destroys explicitly a data iterator.
* Refer to each API documentation, but most of the time the API returning this object will allocate it and release it,
* so API documentation will explicitly mention that you have to call this API to collect the resources attached to
* the data iterator.
*
* @return RC_OK on success
* @return RC_BAD_FORMAT if data parameter is invalid
*/
rc_ReturnCode_t swi_dset_Destroy
(
    swi_dset_Iterator_t* data ///< [IN] the data iterator to destroy.
);


//Iteration API

/**
*
* General purpose function to iterate over received data.
* Specific functions to retrieve value for most common types are defined below.
* Please note that the order of the elements is not guaranteed, caller is strongly
* advised to rely on elements' name rather than on the order.
*
* @return RC_OK on success
* @return RC_BAD_FORMAT if data parameter is invalid
* @return RC_NOT_FOUND when no more data are available, i.e previous call returned last received value.
*/
rc_ReturnCode_t swi_dset_Next
(
    swi_dset_Iterator_t* data ///< [IN] iterator object to iterate over.
                              ///<      The iterator will be given as parameter on data reception function(s).
);

/**
* Retrieves the name of current element in the iterator.
*
* @return current element name as a NULL terminated string, The returned string will be automatically released when the data
*         iterator will be released. NULL is returned in case of error.
*/
const char* swi_dset_GetName
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which info will be retrieved.
);


/**
* Retrieves the type of current element in the iterator.
* It is strongly advised to call this function to get current element type
* prior calling one of the  swi_dataE_To* function, so that the user can choose the appropriate
* function to get element value.
*
* @return the type of the current element as an #swi_dset_Type_t.
* @return SWI_DSET_UNSUPPORTED in case of error
*/
swi_dset_Type_t swi_dset_GetType
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which info will be retrieved.
);


/**
* Returns integer value of data iterator current element.
* Returned value is undefined if the current element type is not #SWI_DSET_INTEGER,
* so it is strongly advised to call #swi_dset_GetType before using this function.
*
* @return integer value of current element.
*/
int64_t swi_dset_ToInteger
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which integer value will be returned.
);

/**
* Returns float value of data iterator current element.
* Returned value is undefined if the current element type is not #SWI_DSET_FLOAT,
* so it is strongly advised to call #swi_dset_GetType before using this function.
*
* @return float value of current element.
*/
double swi_dset_ToFloat
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which float value will be returned.
);

/**
* Returns boolean value of data iterator current element.
* Returned value is undefined if the current element type is not #SWI_DSET_BOOL,
* so it is strongly advised to call #swi_dset_GetType before using this function.
*
* @return boolean value of current element.
*/
bool swi_dset_ToBool
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which float value will be returned.
);

/**
* Returns string value of data iterator current element.
* Returned value is undefined if the current element type is not #SWI_DSET_STRING,
* so it is strongly advised to call #swi_dset_GetType before using this function.
*
* @return string value of current element.
*/
const char* swi_dset_ToString
(
    swi_dset_Iterator_t* data ///< [IN] iterator object, that will provide the element which string value will be returned.
);



//MAP API

/**
* Retrieves an integer value of an element in the data iterator (not necessarily the current element),
* using element name to find/select it.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER when the element was found according requested name, but the element had
*                             different type so value can not be returned.
* @return RC_NOT_FOUND when requested name was not found in the data iterator.
*/
rc_ReturnCode_t swi_dset_GetIntegerByName
(
    swi_dset_Iterator_t* data, ///< [IN] iterator object to search in
    const char* namePtr,       ///< [IN] name of the element to find
    int64_t* valuePtr          ///< [OUT] integer pointer to store element value.
);

/**
* Retrieves a float value of an element in the data iterator (not necessarily the current element),
* using element name to find/select it.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER when the element was found according requested name, but the element had
*                             different type so value can not be returned.
* @return RC_NOT_FOUND when requested name was not found in the data iterator.
*/
rc_ReturnCode_t swi_dset_GetFloatByName
(
    swi_dset_Iterator_t* data, ///< [IN] iterator object to search in
    const char* namePtr,       ///< [IN] name of the element to find
    double* valuePtr           ///< [OUT] float pointer to store element value.
);

/**
* Retrieves a string value of an element in the data iterator (not necessarily the current element),
* using element name to find/select it.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER when the element was found according requested name, but the element had
*                             different type so value can not be returned.
* @return RC_NOT_FOUND when requested name was not found in the data iterator.
*/
rc_ReturnCode_t swi_dset_GetStringByName
(
    swi_dset_Iterator_t* data, ///< [IN] iterator object to search in
    const char* namePtr,       ///< [IN] name of the element to find
    const char** valuePtr      ///< [OUT] string pointer to store element value.
);


/**
* Retrieves the type of an element given its name.
* Element value is then ready to be retrieved using the appropriate swi_dataE_To* function.
*
* This API is useful when user knows the name of the element that the application will receive,
* but not the type of this element.
*
* @return RC_OK on success
* @return RC_NOT_FOUND when requested name was not found in the data iterator.
*/
rc_ReturnCode_t swi_dset_GetTypeByName
(
    swi_dset_Iterator_t* data, ///< [IN] iterator object to search in
    const char* namePtr,       ///< [IN] the name of the element to select
    swi_dset_Type_t* typePtr   ///< [OUT] the type of the selected element
);


#endif /* SWI_DSET_INCLUDE_GUARD */
