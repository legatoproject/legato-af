 /*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/

/**
* @file
* @brief Common return codes.
*
* This header provides an homogeneous return code namespace for all framework APIs.
*  @ingroup common
*
*/

#ifndef RETURNCODES_INCLUDE_GUARD
#define RETURNCODES_INCLUDE_GUARD

/**
* @enum rc_ReturnCode_t
* \brief Return code constants.
* All error codes are negative.
*/
typedef enum
{
  // RCGEN - DO NOT REMOVE THIS LINE! Code will be generated here.
} rc_ReturnCode_t;


/**
* \brief Convert a rc_ReturnCode_t into string representing the ReturnCode
* Returns a const string or NULL if the code does not exist.
*/
const char *rc_ReturnCodeToString( rc_ReturnCode_t n);


/**
* \brief Convert a ReturnCode string into a rc_ReturnCode_t value
* Returns the code value (negative or null value) or 1 is the string does not
* represent a known error name.
*/
rc_ReturnCode_t rc_StringToReturnCode( const char *name);


#endif // RETURNCODES_INCLUDE_GUARD
