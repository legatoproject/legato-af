/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_mcc Modem Call Control
 *
 * @subpage c_mcc_profile <br>
 * @subpage c_mcc_call <br>
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/**
 * @page c_mcc_profile Modem Call Control Profile
 *
 * @ref mcc_profile_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * The Modem Call Control (mcc) API uses profiles. Calls can be initiated or
 * received through these profiles. Each profile represents a call type or specific configuration of a
 * given call (e.g., a profile can represent a given cellular modem/SIM combination - if the modem
 * in question supports multiple SIM operation).
 *
 * @section le_mcc_profiles Using Profiles
 *
 * A given Legato device can support multiple profiles, but usually, the MCC API will be
 * configured with a single profile name that will initiate or receive calls.
 *
 * Call @c le_mcc_profile_GetByName() to access a specific profile by name.
 *
 * @c le_mcc_profile_GetState() API allows the application to get the current state of the profile.
 *
 *  @c le_mcc_profile_AddStateChangeHandler() API installs a handler function that is notified
 *  when the profile's state changes.
 *
 * @c le_mcc_profile_RemoveStateChangeHandler() API uninstalls the handler function.
 *
 * When the Profile object is no longer needed, call @c le_mcc_profile_Release() must be used to
 * release the profile.
 *
 * Here's a profile code sample:
 *
 * @code
 *
 * void ManageMyProfile(void)
 * {
 *
 *     [...]
 *
 *     // Get the Modem profile working on SIM card #1
 *     le_mcc_profile_ObjRef_t myProfile = le_mcc_profile_GetByName("Modem-Sim1");
 *
 *     // Create and start a call on that profile
 *     le_mcc_call_ObjRef_t myCall = le_mcc_profile_CreateCall(myProfile, "+18008800800");
 *     le_mcc_call_Start(myCall);
 *
 *     [...]
 *     // Set the profile into 'Do Not Disturb' mode: it will disable it's ablity to accept incoming
 *     // calls.
 *     le_mcc_profile_SetDoNotDisturb(myProfile);
 *
 *     [...]
 *     // Clear the 'Do Not Disturb' mode: the profile gets back to be able to accept incoming calls.
 *     le_mcc_profile_ClearDoNotDisturb(myProfile);
 *
 *     [...]
 *     // Set the profile into 'call forward mode': all calls incoming to this profile will
 *     // automatically be forwarded to another telephone number.
 *     le_mcc_profile_SetForwarding(myProfile, "+18008910910");
 *
 *     [...]
 *
 * }
 *
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/**
 * @file mcc_profile_interface.h
 *
 * Legato @ref c_mcc_profile include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD
#define MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_mcc_profile_StateChangeHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_StateChangeHandler* le_mcc_profile_StateChangeHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_mcc_profile_CallEventHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mcc_profile_CallEventHandler* le_mcc_profile_CallEventHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for profile state changes.
 *
 *
 * @param newState
 *        New state profile.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mcc_profile_StateChangeHandlerFunc_t)
(
    le_mcc_profile_State_t newState,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Handler for call state changes.
 *
 *
 * @param callRef
 *        Call associated with the event.
 * @param event
 *        Call event.
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mcc_profile_CallEventHandlerFunc_t)
(
    le_mcc_call_ObjRef_t callRef,
    le_mcc_call_Event_t event,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_profile_StateChangeHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_StateChangeHandlerRef_t le_mcc_profile_AddStateChangeHandler
(
    le_mcc_profile_ObjRef_t profileRef,
        ///< [IN]
        ///< The profile reference.

    le_mcc_profile_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_profile_StateChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveStateChangeHandler
(
    le_mcc_profile_StateChangeHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_profile_CallEventHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_CallEventHandlerRef_t le_mcc_profile_AddCallEventHandler
(
    le_mcc_profile_ObjRef_t profileRef,
        ///< [IN]
        ///< The profile to update.

    le_mcc_profile_CallEventHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_mcc_profile_CallEventHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_RemoveCallEventHandler
(
    le_mcc_profile_CallEventHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 *  Access a particular profile by name.
 *
 *  @return The profileRef or NULL if profileName is not found.
 *
 *  @note If profil name is too long (max 100 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_ObjRef_t le_mcc_profile_GetByName
(
    const char* profileNamePtr
        ///< [IN]
        ///< The name of the profile to search for.
);

//--------------------------------------------------------------------------------------------------
/**
 * Must be called to release a Call Profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mcc_profile_Release
(
    le_mcc_profile_ObjRef_t profileRef
        ///< [IN]
        ///< The Call profile reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * Used to determine the current state of a given profile.
 *
 * @return Current state of the profile.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_profile_State_t le_mcc_profile_GetState
(
    le_mcc_profile_ObjRef_t profileRef
        ///< [IN]
        ///< The profile reference to read.
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a new call object with a destination telephone number.
 *
 * @c le_mcc_call_Start() must still initiate the call when ready.
 *
 * @return A reference to the new Call object.
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 *
 * @note If destination number is too long (max 17 digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_call_ObjRef_t le_mcc_profile_CreateCall
(
    le_mcc_profile_ObjRef_t profileRef,
        ///< [IN]
        ///< The profile to create a new call on.

    const char* destinationPtr
        ///< [IN]
        ///< The target number we are going to call.
);


#endif // MCC_PROFILE_INTERFACE_H_INCLUDE_GUARD

