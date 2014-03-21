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
 * @brief This API provides functionalities to send and receive SMS.
 *
 * <HR>
 */


#ifndef SWI_SMS_INCLUDE_GUARD
#define SWI_SMS_INCLUDE_GUARD


#include "returncodes.h"



/**
* Initializes the module.
* A call to init is mandatory to enable SMS library APIs.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sms_Init();


/**
* Destroys the Sms library.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sms_Destroy();


/**
 * The encoding format to send the SMS.
 * Supported formats depend on network operator.
 * As a SMS payload has a limited size(140 bytes) and each format encode a character on various number of bits,
 * the choice of encoding format impacts the number of characters sent into one SMS.
 */
typedef enum swi_sms_Format{
  SWI_SMS_7BITS, ///< 7 bits format, regular encoding format to send text using GSM 7bit alphabet.
                 ///< 160 characters maximum per message.
  SWI_SMS_8BITS, ///< 8 bits format, especially useful to send binary payload, not widely supported.
                 ///< 140 characters maximum per message.
  SWI_SMS_UCS2   ///< UCS-2 format (i.e. Universal Character Set on 16 bits), useful to send characters
                 ///< of specific alphabet which don't fit on one byte(e.g. Japanese).
                 ///< 70 characters maximum per message.
} swi_sms_Format_t;


/**
* Sends an SMS.
*
* @return RC_OK on success
* @return RC_SERVICE_UNAVAILABLE when network status made the SMS sending fail
* @return RC_BAD_FORMAT when selected SMS format is not supported.
*/
rc_ReturnCode_t swi_sms_Send
(
    const char *recipientPtr, ///< [IN] phone number to send the SMS to.
    const char* messagePtr,   ///< [IN] string containing the message
    swi_sms_Format_t format   ///< [IN] the format to use to send the message, see SMS_Format for accepted values.
                              ///<      Supported formats may differ depending on the hardware and network capabilities.
);



/**
* SMS reception callback.
*
* String parameters will be released after the callback have returned.
*
* @param senderPtr [IN] phone number of the SMS sender
* @param messagePtr [IN] message content
*
*/
typedef rc_ReturnCode_t (*swi_sms_ReceptionCB_t)
(
    const char* senderPtr, ///<
    const char* messagePtr ///<
);

/**
 * Registration identifier.
 * Used to identify an registration so that it can be unregistered afterwards.
 */
typedef void * swi_sms_regId_t;

/**
* Registers a callback on SMS reception.
*
* The callback will be called in a new pthread.
*
* New SMS will be notified if the content (sender or message) of the SMS matches the given patterns.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sms_Register
(
    swi_sms_ReceptionCB_t callback, ///< [IN] function to be called on sms reception matching both patterns.
    const char* senderPatternPtr,   ///< [IN] string, regex pattern that matches the sender address, NULL means "no filtering".
    const char* messagePatternPtr,  ///< [IN] string, regex pattern that matches the message, NULL means "no filtering".
    swi_sms_regId_t *regIdPtr       ///< [OUT] identifier of the registration, to be used to cancel it afterward using SMS_Unregister function.
);


/**
* Cancels a callback registration on SMS reception.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_sms_Unregister
(
    swi_sms_regId_t regId ///< [IN] identifier of the registration to cancel, the id returned by previous SMS_Register call.
);


#endif /* SWI_SMS_INCLUDE_GUARD */
