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
  RC_OK                  =    0,  ///< Successful.
  RC_NOT_FOUND           =   -1,  ///< The referenced item does not exist or could not be found.
  RC_OUT_OF_RANGE        =   -2,  ///< An index or other value is out of range.
  RC_NO_MEMORY           =   -3,  ///< Insufficient memory is available.
  RC_NOT_PERMITTED       =   -4,  ///< Current user does not have permission to perform requested action.
  RC_UNSPECIFIED_ERROR   =   -5,  ///< An unspecified error happened.
  RC_COMMUNICATION_ERROR =   -6,  ///< Communications error.
  RC_TIMEOUT             =   -7,  ///< A time-out occurred.
  RC_WOULD_BLOCK         =   -8,  ///< Would have blocked if non-blocking behavior was not requested.
  RC_DEADLOCK            =   -9,  ///< Would have caused a deadlock.
  RC_BAD_FORMAT          =  -10,  ///< Inputs or data are not formated correctly.
  RC_DUPLICATE           =  -11,  ///< Duplicate entry found or operation already performed.
  RC_BAD_PARAMETER       =  -12,  ///< Parameter is not valid.
  RC_CLOSED              =  -13,  ///< The file, stream or object was closed.
  RC_IO_ERROR            =  -14,  ///< An IO error occurred.
  RC_NOT_IMPLEMENTED     =  -15,  ///< This feature is not implemented.
  RC_BUSY                =  -16,  ///< The component or service is busy.
  RC_NOT_INITIALIZED     =  -17,  ///< The service or object is not initialized
  RC_END                 =  -18,  ///< The file, stream or buffer reached the end.
  RC_NOT_AVAILABLE       =  -19,  ///< The service is not available.
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
