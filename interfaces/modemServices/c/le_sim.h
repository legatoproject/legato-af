/** @file le_sim.h
 *
 * Legato @ref c_sim include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SIM_INCLUDE_GUARD
#define LEGATO_SIM_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Minimum PIN length (4 digits)
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SIM_PIN_MIN_LEN     4


//--------------------------------------------------------------------------------------------------
/**
 * Maximum PIN length (8 digits)
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SIM_PIN_MAX_LEN     8


//--------------------------------------------------------------------------------------------------
/**
 * PUK length (8 digits)
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SIM_PUK_LEN     8

//--------------------------------------------------------------------------------------------------
/**
 * ICCID length
 * According to GSM Phase 1
 */
//--------------------------------------------------------------------------------------------------
#define LE_SIM_ICCID_LEN     (20+1)

//--------------------------------------------------------------------------------------------------
/**
 * IMSI length
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_SIM_IMSI_LEN     (15+1)


//--------------------------------------------------------------------------------------------------
/**
 * SIM states.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_SIM_INSERTED      = 0, ///< SIM card is inserted and locked.
    LE_SIM_ABSENT        = 1, ///< SIM card is absent.
    LE_SIM_READY         = 2, ///< SIM card is inserted and unlocked.
    LE_SIM_BLOCKED       = 3, ///< SIM card is blocked.
    LE_SIM_BUSY          = 4, ///< SIM card is busy.
    LE_SIM_STATE_UNKNOWN = 5, ///< Unknown SIM state.
}
le_sim_States_t;

//--------------------------------------------------------------------------------------------------
// Other new type definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declare a reference type for referring to SIM objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sim_Obj* le_sim_ObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a "New State" event handler that was registered using le_sim_AddNewStateHandler().
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_sim_NewStateHandler* le_sim_NewStateHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to report SIM state notifications.
 *
 * @param simRef      SIM reference.
 * @param contextPtr  Context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_sim_NewStateHandlerFunc_t)
(
    le_sim_ObjRef_t  simRef,
    void*         contextPtr
);


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of SIM card slots.
 *
 * @return Number of the SIM card slots mounted on the device.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_sim_CountSlots
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the current selected card.
 *
 * @return Number of the current selected SIM card.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_sim_GetSelectedCard
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a SIM object.
 *
 * @return Reference to the SIM object.
 *
 * @note
 *      On failure, the process exits, so no need to check the return value for validity
 */
//--------------------------------------------------------------------------------------------------
le_sim_ObjRef_t le_sim_Create
(
    uint32_t cardNum  ///< [IN] The SIM card number (1 or 2, it depends on the platform).
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete a SIM object.
 *
 * This frees all the resources allocated for
 * the SIM object. If several users own the SIM object (e.g., several
 * handler functions registered for new state notifications), the SIM object will only
 * be actually deleted after the last user deletes the SIM object.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_Delete
(
    le_sim_ObjRef_t  simRef    ///< [IN] SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the slot number of the SIM card.
 *
 * @return Slot number of the SIM card.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_sim_GetSlotNumber
(
    le_sim_ObjRef_t simRef   ///< [IN] SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the integrated circuit card identifier (ICCID) of the SIM card (20 digits)
 *
 * @return LE_OK             ICCID was successfully retrieved.
 * @return LE_OVERFLOW       iccidPtr buffer was too small for the ICCID.
 * @return LE_NOT_POSSIBLE  ICCID could not be retrieved.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetICCID
(
    le_sim_ObjRef_t simRef,   ///< [IN]  SIM object.
    char * iccidPtr,       ///< [OUT] Buffer to hold the ICCID.
    size_t iccidLen        ///< [IN] Buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the identification number (IMSI) of the SIM card. (max 15 digits)
 *
 * @return LE_OK            IMSI was successfully retrieved.
 * @return LE_OVERFLOW      imsiPtr buffer was too small for the IMSI.
 * @return LE_NOT_POSSIBLE  IMSI could not be retrieved.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetIMSI
(
    le_sim_ObjRef_t simRef,  ///< [IN] SIM object.
    char * imsiPtr,       ///< [OUT] Buffer to hold the IMSI.
    size_t imsiLen        ///< [IN] Buffer length
);

//--------------------------------------------------------------------------------------------------
/**
 * Verify if the SIM card is present or not.
 *
 * @return true   SIM card is present.
 * @return false  SIM card is absent
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsPresent
(
    le_sim_ObjRef_t simRef    ///< [IN]  SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Verify if the SIM is ready (PIN code correctly inserted or not
 * required).
 *
 * @return true   PIN is correctly inserted or not required.
 * @return false  PIN must be inserted
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_sim_IsReady
(
    le_sim_ObjRef_t simRef    ///< [IN]  SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enter the PIN code.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     PIN code is not long enough (min 4 digits).
 * @return LE_NOT_POSSIBLE  Function failed to enter the PIN code.
 * @return LE_OK            Function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_EnterPIN
(
    le_sim_ObjRef_t simRef,    ///< [IN] SIM object.
    const char*  pinPtr     ///< [IN] PIN code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Change the PIN code.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     PIN code is/are not long enough (min 4 digits).
 * @return LE_NOT_POSSIBLE  Function failed to change the PIN code.
 * @return LE_OK            Function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_ChangePIN
(
    le_sim_ObjRef_t simRef,    ///< [IN]  SIM object.
    const char*  oldpinPtr, ///< [IN]  old PIN code.
    const char*  newpinPtr  ///< [IN]  new PIN code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of remaining PIN insertion tries.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_NOT_POSSIBLE  Function failed to get the number of remaining PIN insertion tries.
 * @return A positive value Function succeeded. The number of remaining PIN insertion tries.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_sim_GetRemainingPINTries
(
    le_sim_ObjRef_t simRef    ///< [IN] SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Unlock the SIM card: it disables the request of the PIN code.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     PIN code is not long enough (min 4 digits).
 * @return LE_NOT_POSSIBLE  Function failed to unlock the SIM card.
 * @return LE_OK            Function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unlock
(
    le_sim_ObjRef_t simRef, ///< [IN]  SIM object.
    const char*  pinPtr  ///< [IN]  PIN code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Lock the SIM card: it enables the request of the PIN code.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     PIN code is not long enough (min 4 digits).
 * @return LE_NOT_POSSIBLE  Function failed to unlock the SIM card.
 * @return LE_OK            Function succeeded.
 *
 * @note If PIN code is too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Lock
(
    le_sim_ObjRef_t simRef, ///< [IN]  SIM object.
    const char*  pinPtr  ///< [IN]  PIN code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Unblock the SIM card.
 *
 * @return LE_NOT_FOUND     Function failed to select the SIM card for this operation.
 * @return LE_UNDERFLOW     PIN code is not long enough (min 4 digits).
 * @return LE_OUT_OF_RANGE  PUK code length is not correct (8 digits).
 * @return LE_NOT_POSSIBLE  Function failed to unblock the SIM card.
 * @return LE_OK            Function succeeded.
 *
 * @note If new PIN or puk code are too long (max 8 digits), it is a fatal error, the
 *       function will not return.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_Unblock
(
    le_sim_ObjRef_t simRef,    ///< [IN]  SIM object.
    const char*  pukPtr,    ///< [IN]  PUK code.
    const char*  newpinPtr  ///< [IN]  New PIN code.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM state.
 *
 * @return Current SIM state.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_sim_States_t le_sim_GetState
(
    le_sim_ObjRef_t simRef    ///< [IN]  SIM object.
);

//--------------------------------------------------------------------------------------------------
/**
 * Register a handler function for New State notification.
 *
 * @return A handler reference, only needed to remove the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_sim_NewStateHandlerRef_t le_sim_AddNewStateHandler
(
    le_sim_NewStateHandlerFunc_t handlerFuncPtr, ///< [IN] Handler function for New State notification.
    void*                        contextPtr      ///< [IN] Handler's context.
);

//--------------------------------------------------------------------------------------------------
/**
 * Unregister a handler function
 *
 * @note Doesn't return on failure, don't need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_sim_RemoveNewStateHandler
(
    le_sim_NewStateHandlerRef_t handlerRef ///< [IN] Handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the SIM Phone Number.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Phone Number can't fit in phoneNumberStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sim_GetSubscriberPhoneNumber
(
    le_sim_ObjRef_t simRef,            ///< [IN]  SIM object.
    char        *phoneNumberStr,    ///< [OUT] The phone Number
    size_t       phoneNumberStrSize ///< [IN]  Size of phoneNumberStr
);

#endif // LEGATO_SIM_INCLUDE_GUARD
