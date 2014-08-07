/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_ecall eCall API
 *
 * @ref ecall_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * @section le_ecall_toc Table of Contents
 *
 *  - @ref le_ecall_intro <br>
 *  - @ref le_ecall_session <br>
 *  - @ref le_ecall_msd <br>
 *  - @ref le_ecall_concurrency <br>
 *  - @ref le_ecall_samples <br>
 *  - @ref le_ecall_configdb <br>
 *
 * @section le_ecall_intro Introduction
 *
 * When a serious vehicle accident occurs, in-vehicle sensors will automatically trigger an eCall.
 * When activated, the in-vehicle system (IVS) establishes a 112-voice connection.
 *
 * The Mobile Network Operator handles the eCall like any other 112 call and routes the call to the
 * most appropriate emergency response centre - Public Safety Answering Point (PSAP).
 *
 * At the same time, a digital "minimum set of data" (MSD) message is sent over the voice call using
 * in-band modem signals. The MSD includes accident information, such as time, location, driving
 * direction, and vehicle description.
 *
 * The eCall can also be activated manually. The mobile network operator identifies that the 112
 * call is an eCall from the “eCall flag” inserted by the vehicle’s communication module.
 *
 *
 * @section le_ecall_session eCall Session
 *
 * In order to start an eCall session, an eCall object must be created by calling
 * @c le_ecall_Create(). An eCall session can be stopped using @c le_ecall_End().
 *
 * The type of eCall and the kind of activation are specified using different functions to start an
 * eCall session:
 * - @c le_ecall_StartManual(): initiate a manual eCall session
 * - @c le_ecall_StartAutomatic(): initiate an automatic eCall session
 * - @c le_ecall_StartTest(): initiate a test eCall session
 *
 * When the eCall object is no longer needed, call @c le_ecall_Delete() to free all allocated
 * resources associated with the object.
 *
 * The current state of an eCall session can be queried using @c le_ecall_GetState().
 * Alternatively, an application can register a handler to be notified when the session state
 * changes. The handler can be managed using @c le_ecall_AddStateChangeHandler() and
 * @c le_ecall_RemoveStateChangeHandler().
 *
 *
 * @section le_ecall_msd Minimum Set of Data (MSD)
 *
 * The dynamic values of the MSD can be set with:
 * - @c le_ecall_SetMsdPosition() sets the position of the vehicle.
 * - @c le_ecall_SetMsdPassengersCount() sets the number of passengers.
 *
 * The static values are retrieved from the configuration tree.
 *
 * Moreover it is possible to import a prepared MSD using the @c le_ecall_ImportMsd() function.
 * The prepared MSD must answer the requirements described in the "EN 15722:2013" publication (this
 * publication has been prepared by Technical Committee CEN/TC 278 “Intelligent Transport Systems).
 *
 * The MSD transmission will be performed automatically when the emergency call is established with
 * the PSAP.
 *
 * @section le_ecall_concurrency Concurrency
 *
 * If another application try to use the eCall service while a session is already in progress, the
 * @c le_ecall_StartManual(), @c le_ecall_StartAutomatic(), @c le_ecall_StartTest() and
 * @c le_ecall_End() functions will return a LE_DUPLICATE error. But, the eCall session in progress
 * won't be interrupted or disturbed in any manners. However, the appplication can follow the
 * progress of the session with the 'state' functions like @c le_ecall_GetState() and
 * @c le_ecall_AddStateChangeHandler().
 * Note that a manual eCall won't interrupt an automatic eCall, and vice versa an automatic eCall
 * won't interrupt a manual eCall.
 *
 * @section le_ecall_samples Code sample
 *
 * The following code sample show how an eCall session is used.
 *
 * @code
 *
 * le_result_t TriggerAutomaticEcall
 * (
 *     uint32_t    paxCount,
 *     int32_t     latitude,
 *     int32_t     longitude,
 *     int32_t     direction
 * )
 * {
 *     // Create the eCall Session
 *     le_ecall_ObjRef_t eCallRef = le_ecall_Create();
 *
 *     // Set the dynamic MSD values, the static values are retrieved from the config tree
 *     SetMsdPosition(eCallRef,
 *                    true,
 *                    latitude,
 *                    longitude,
 *                    direction);
 *     SetMsdPassengersCount(eCallRef, paxCount);
 *
 *     // Start the eCall session
 *     le_ecall_StartAutomatic(eCallRef);
 * }
 *
 * @endcode
 *
 *
 * @section le_ecall_configdb Configuration tree
 * @copydoc le_ecall_configdbPage_Hide
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/**
 * @interface le_ecall_configdbPage_Hide
 *
 * The configuration database path for the eCall is:
 * @verbatim
   /
       modemServices/
           eCall/
               psap<string> = <PSAP number>
               pushPull<string> = <push-pull mode>
               msdVersion<int> = <MSD value>
               maxRedialAttempts<int> = <maximum redial attempts value>
               vehicleType<string> = <vehicle type>
               vin<string> = <VIN>
               propulsionType/
                   0<string> = <propulsion type>
                   1<string> = <propulsion type>
                   ...
   @endverbatim
 *
 * The 'psap' field is the PSAP number.
 *
 * The 'pushPull' field can be set with the following two choices (string type):
 * - "Push": the MSD is pushed by the IVS
 * - "Pull": the MSD is sent when requested by the PSAP
 *
 * The 'msdVersion' field is the MSD format version.
 *
 * The 'maxRedialAttempts' field is the time of that the IVS shall attempt to redial the call if the
 * initial eCall attempt fails to connect, or the call is dropped for any reason other than by the
 * PSAP operator clearing the call down or T2 (IVS  Call Clear-down Fallback Timer) ends.
 *
 * The 'vehicleType' field can be set with the following choices (string type):
 * - "Passenger-M1" (Passenger vehicle, Class M1)
 * - "Bus-M2" (Buses and coaches, Class M2)
 * - "Bus-M3" (Buses and coaches, Class M2)
 * - "Commercial-N1" (Light commercial vehicles, Class N1)
 * - "Heavy-N2" (Heavy duty vehicles, Class N2)
 * - "Heavy-N3" (Heavy duty vehicles, Class N2)
 * - "Motorcycle-L1e" (Motorcycles, Class L1e)
 * - "Motorcycle-L2e" (Motorcycles, Class L1e)
 * - "Motorcycle-L3e" (Motorcycles, Class L1e)
 * - "Motorcycle-L4e" (Motorcycles, Class L1e)
 * - "Motorcycle-L5e" (Motorcycles, Class L1e)
 * - "Motorcycle-L6e" (Motorcycles, Class L1e)
 * - "Motorcycle-L7e" (Motorcycles, Class L1e)
 *
 * The 'vin' is the vehicle identification number (string type).
 *
 * The 'propulsionType' field can be set with the following choices (string type):
 * - "Gasoline" (Gasoline propulsion)
 * - "Diesel" (Diesel propulsion)
 * - "NaturalGas" (Compressed natural gas propulsion)
 * - "Propane" (Liquid propane gas propulsion)
 * - "Electric" (Electric propulsion)
 * - "Hydrogen" (Hydrogen propulsion)
 * - "Other" (Other type of propulsion)
 */
/**
 * @file ecall_interface.h
 *
 * Legato @ref c_ecall include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef ECALL_INTERFACE_H_INCLUDE_GUARD
#define ECALL_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Reference returned by Create function and used by other functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ecall_Obj* le_ecall_ObjRef_t;


//--------------------------------------------------------------------------------------------------
/**
 *  eCall session states.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_ECALL_STATE_UNKNOWN,
        ///< Unknown state.

    LE_ECALL_STATE_CONNECTED,
        ///< Emergency call is established.

    LE_ECALL_STATE_MSD_TX_COMPLETED,
        ///< MSD transmission is complete.

    LE_ECALL_STATE_MSD_TX_FAILED,
        ///< MSD transmission has failed.

    LE_ECALL_STATE_STOPPED,
        ///< eCall session has been stopped by the PSAP.

    LE_ECALL_STATE_RESET,
        ///< eCall session has lost synchronization and starts over.

    LE_ECALL_STATE_COMPLETED,
        ///< Successful eCall session.

    LE_ECALL_STATE_FAILED
        ///< Unsuccessful eCall session.
}
le_ecall_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Reference type for le_ecall_StateChangeHandler handler ADD/REMOVE functions
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_ecall_StateChangeHandler* le_ecall_StateChangeHandlerRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Handler for eCall state changes.
 *
 *
 * @param state
 *        eCall state
 * @param contextPtr
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_ecall_StateChangeHandlerFunc_t)
(
    le_ecall_State_t state,
    void* contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_ecall_StateChangeHandlerRef_t le_ecall_AddStateChangeHandler
(
    le_ecall_StateChangeHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_RemoveStateChangeHandler
(
    le_ecall_StateChangeHandlerRef_t addHandlerRef
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a new eCall object
 *
 * The eCall is not actually established at this point. It is still up to the caller to call
 * le_ecall_Start() when ready.
 *
 * @return A reference to the new Call object.
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_ecall_ObjRef_t le_ecall_Create
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @note This will free the reference, but not necessarily stop an active eCall. If there are
 *       other holders of this reference then the eCall will remain active.
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_Delete
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the position transmitted by the MSD.
 *
 * The MSD is not actually transferred at this point. It is still up to the caller to call
 * le_ecall_LoadMsd() when the MSD is fully built with the le_ecall_SetMsdXxx() functions.
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPosition
(
    le_ecall_ObjRef_t ecallRef,
        ///< [IN]
        ///< eCall reference

    bool isTrusted,
        ///< [IN]
        ///< true if the position is accurate, false otherwise

    int32_t latitude,
        ///< [IN]
        ///< latitude in degrees with 6 decimal places, positive North

    int32_t longitude,
        ///< [IN]
        ///< longitude in degrees with 6 decimal places, positive East

    int32_t direction
        ///< [IN]
        ///< direction in degrees (where 0 is True North)
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the number of passengers transmitted by the MSD.
 *
 * The MSD is not actually transferred at this point. It is still up to the caller to call
 * le_ecall_LoadMsd() when the MSD is fully built with the le_ecall_SetMsdXxx() functions.
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPassengersCount
(
    le_ecall_ObjRef_t ecallRef,
        ///< [IN]
        ///< eCall reference

    uint32_t paxCount
        ///< [IN]
        ///< number of passengers
);

//--------------------------------------------------------------------------------------------------
/**
 * Import an already prepared MSD.
 *
 * The MSD is not actually transferred at this point, this functions only creates a new MSD object.
 * It is still up to the caller to call le_ecall_LoadMsd().
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an MSD has been already imported
 * @return LE_FAULT for other failures
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ImportMsd
(
    le_ecall_ObjRef_t ecallRef,
        ///< [IN]
        ///< eCall reference

    const uint8_t* msdPtr,
        ///< [IN]
        ///< the prepared MSD

    size_t msdNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Start an automatic eCall session
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartAutomatic
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a manual eCall session
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartManual
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Start a test eCall session
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE an eCall session is already in progress
 * @return LE_FAULT for other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartTest
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);

//--------------------------------------------------------------------------------------------------
/**
 * End the current eCall session
 *
 * @return LE_OK on success
 * @return LE_DUPLICATE the eCall session was started by another application
 * @return LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_End
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the current state for the given eCall
 *
 * @return the current state for the given eCall
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_ecall_State_t le_ecall_GetState
(
    le_ecall_ObjRef_t ecallRef
        ///< [IN]
        ///< eCall reference
);


#endif // ECALL_INTERFACE_H_INCLUDE_GUARD

