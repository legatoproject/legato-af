/**
 * @file le_ecall.c
 *
 * This file contains the source code of the eCall API.
 *
 * builtMsdSize eCall object's member is used to check whether the MSD has to be encoded or not:
 * - if builtMsdSize > 1, the MSD has been already encoded or imported;
 * - if builtMsdSize <= 1, the MSD is not yet encoded nor imported.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "mdmCfgEntries.h"
#include "asn1Msd.h"
#include "pa_ecall.h"
#include "le_mcc_local.h"
#include "le_ms_local.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum number of eCall sessions.
 */
//--------------------------------------------------------------------------------------------------
#define ECALL_MAX  1

//--------------------------------------------------------------------------------------------------
/**
 * System standard type string length.
 */
//--------------------------------------------------------------------------------------------------
#define SYS_STD_MAX_LEN      (12)

//--------------------------------------------------------------------------------------------------
/**
 * System standard type string length. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define SYS_STD_MAX_BYTES      (SYS_STD_MAX_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Propulsion type string length.
 */
//--------------------------------------------------------------------------------------------------
#define PROPULSION_MAX_LEN      (16)

//--------------------------------------------------------------------------------------------------
/**
 * Propulsion type string length. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define PROPULSION_MAX_BYTES      (PROPULSION_MAX_LEN + 1)


//--------------------------------------------------------------------------------------------------
/**
 * Vehicle type string length.
 */
//--------------------------------------------------------------------------------------------------
#define VEHICLE_TYPE_MAX_LEN      (16)

//--------------------------------------------------------------------------------------------------
/**
 * Vehicle type string length. One extra byte is added for the null character.
 */
//--------------------------------------------------------------------------------------------------
#define VEHICLE_TYPE_MAX_BYTES      (VEHICLE_TYPE_MAX_LEN + 1)

//--------------------------------------------------------------------------------------------------
/**
 * Default MSD version.
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_MSD_VERSION     1

//--------------------------------------------------------------------------------------------------
/**
 * Macro: Get bit mask value.
 */
//--------------------------------------------------------------------------------------------------
#define GET_BIT_MASK_VALUE(_value_, _bitmask_) ((_value_ & _bitmask_) ? true : false)

//--------------------------------------------------------------------------------------------------
/**
 * Macro: Set bit mask value.
 */
//--------------------------------------------------------------------------------------------------
#define SET_BIT_MASK_VALUE(_value_, _bitmask_) ((_value_ == true) ? _bitmask_ : 0)

//--------------------------------------------------------------------------------------------------
/**
 * eCall handler and  MCC notification synchronization timeout
 */
//--------------------------------------------------------------------------------------------------
#define LE_ECALL_SEM_TIMEOUT_SEC         3
#define LE_ECALL_SEM_TIMEOUT_USEC        0

//--------------------------------------------------------------------------------------------------
/**
 * Unlimited dial attempts for eCall session (used for PAN-European system)
 */
//--------------------------------------------------------------------------------------------------
#define UNLIMITED_DIAL_ATTEMPTS      UINT32_MAX

//--------------------------------------------------------------------------------------------------
/**
 * Minimum value of ERA GLONASS Call Cleardown Fallback Timer (CCFT) expressed in minutes
 */
//--------------------------------------------------------------------------------------------------
#define ERA_GLONASS_CCFT_MIN              1

//--------------------------------------------------------------------------------------------------
/**
 * Maximum value of ERA GLONASS Call Cleardown Fallback Timer (CCFT) expressed in minutes
 */
//--------------------------------------------------------------------------------------------------
#define ERA_GLONASS_CCFT_MAX             720

//--------------------------------------------------------------------------------------------------
/**
 * Maximum value of ERA GLONASS eCall auto answer timer expressed in minutes
 */
//--------------------------------------------------------------------------------------------------
#define ERA_GLONASS_AUTOANSTIME_MAX      720

//--------------------------------------------------------------------------------------------------
/**
 * Size of eCall events memory pool
 */
//--------------------------------------------------------------------------------------------------
#define ECALL_EVENTS_POOL_SIZE      3

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for the eCall session state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_SESSION_INIT = 0,
    ECALL_SESSION_REQUEST,
    ECALL_SESSION_NOT_CONNECTED,
    ECALL_SESSION_CONNECTED,
    ECALL_SESSION_COMPLETED,
    ECALL_SESSION_STOPPED,
}
ECallSessionState_t;

//--------------------------------------------------------------------------------------------------
/**
 * ERA-GLONASS specific context.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint16_t        manualDialAttempts;        ///< Manual dial attempts
    uint16_t        autoDialAttempts;          ///< Automatic dial attempts
    uint16_t        dialDuration;              ///< Dial duration value
    uint16_t        nadDeregistrationTime;     ///< NAD deregistration time
    uint16_t        postTestRegistrationTime;  ///< Post test registration time
    bool            pullModeSwitch;            ///< AL ack received positive has been reported or T7
                                               ///< timer has expired, Pull mode must be selected on
                                               ///< next redials
}
EraGlonassContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for redial state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_REDIAL_INIT = 0,
    ECALL_REDIAL_IDLE,
    ECALL_REDIAL_ACTIVE,
    ECALL_REDIAL_STOPPED
}
RedialState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for the cause stopping the redial
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ECALL_REDIAL_STOP_COMPLETE = 0,          ///< eCall session is completed: redial stopped.
    ECALL_REDIAL_STOP_LLACK_RECEIVED,        ///< LLACK is received: redial stopped.
    ECALL_REDIAL_STOP_ALACK_RECEIVED,        ///< ALACK is received: redial stopped.
    ECALL_REDIAL_STOP_FROM_PSAP,             ///< eCall session and redial are stopped from PSAP.
    ECALL_REDIAL_STOP_FROM_USER,             ///< eCall session and redial are stopped from user.
    ECALL_REDIAL_STOP_NO_REDIAL_CONDITION,   ///< Redial is stopped according to the eCall
                                             ///< standard (FprEN 16062:2014) / eCall clear-down.
    ECALL_REDIAL_DURATION_EXPIRED,           ///< Redial duration timer expired
    ECALL_REDIAL_STOP_MAX_DIAL_ATTEMPT       ///< Maximum attempts performed: redial stopped.
}
RedialStopCause_t;

//--------------------------------------------------------------------------------------------------
/**
 * Ecall and Era-Glonass redial context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    RedialState_t   state;                      ///< Redial state machine
    le_timer_Ref_t  intervalTimer;              ///< Interval timer
    le_timer_Ref_t  dialDurationTimer;          ///< Dial duration timer
                                                ///< For panEuropean:
                                                ///< 120 seconds when the call has been connected
                                                ///< once
                                                ///< For EraGlonass:
                                                ///< 5 minutes for dial duration from the first
                                                ///< attempt
    int8_t          dialAttemptsCount;          ///< Counter of dial attempts
    uint32_t        maxDialAttempts;            ///< Maximum redial attempts
    le_clk_Time_t   startTentativeTime;         ///< Relative time of a dial tentative
    uint16_t        intervalBetweenAttempts;    ///< Interval value between dial attempts (in sec)
}
RedialCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * eCall object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ecall_CallRef_t      ref;                                        ///< eCall reference
    char                    psapNumber[LE_MDMDEFS_PHONE_NUM_MAX_BYTES]; ///< PSAP telephone number
    ECallSessionState_t     sessionState;                               ///< eCall session state
    le_ecall_State_t        state;                                      ///< eCall state,
                                                                        ///< as reported to the app
    bool                    isPushed;                                   ///< True if the MSD is
                                                                        /// pushed by the IVS,
                                                                        /// false if it  is sent
                                                                        /// when requested by the
                                                                        /// PSAP (pull)
    msd_t                   msd;                                        ///< MSD
    uint8_t                 builtMsd[LE_ECALL_MSD_MAX_LEN];             ///< built MSD
    size_t                  builtMsdSize;                               ///< Size of the built MSD
    bool                    isMsdImported;                              ///< True if the MSD is
                                                                        ///  imported, false when it
                                                                        ///  is built thanks to
                                                                        ///  SetMsdXxx() functions
    pa_ecall_StartType_t    startType;                                  ///< eCall start type
    EraGlonassContext_t     eraGlonass;                                 ///< ERA-GLONASS context
    RedialCtx_t             redial;                                     ///< Redial context
    uint32_t                callId;                                     ///< call identifier of the
                                                                        ///< call
    le_mcc_TerminationReason_t termination;                             ///< Call termination reason
    int32_t                 specificTerm;                               ///< specific call
                                                                        ///< termination reason
    bool                    ackOrT7Received;                            ///< LL-ACK, HL-ACK,
                                                                        ///< T7 timeout received
    bool                    t5Received;                                 ///< T5 timeout received
    bool                    sendMsdSignalReceived;                      ///< Send MSD signal
                                                                        ///< received
    le_mcc_TerminationReason_t lastTermination;                         ///< Last call termination
                                                                        ///  reason
    int32_t                 lastSpecificTerm;                           ///< Last call specific
                                                                        ///  termination.
    bool                    isMtCall;                                   ///< Record call type(MT/MO)
    int32_t                 mtCallId;                                   ///< Record MT call ID
}
ECall_t;

//--------------------------------------------------------------------------------------------------
/**
 * Report state structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ecall_CallRef_t      ref;                                        ///< eCall reference
    le_ecall_State_t        state;                                      ///< eCall state
}
ReportState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Data associated with an eCall event passed to the main thread
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_ecall_State_t state;                     ///< New eCall state
    bool             terminationReceived;       ///< End of call termination reason received
}
ECallEventData_t;

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declaration of Dial Duration Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DialDurationTimerHandler(le_timer_Ref_t timerRef);

//--------------------------------------------------------------------------------------------------
/**
 * Choosen system standard (PAN-EUROPEAN or ERA-GLONASS).
 *
 */
//--------------------------------------------------------------------------------------------------
static pa_ecall_SysStd_t SystemStandard;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for eCall State notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ECallEventStateId;

//--------------------------------------------------------------------------------------------------
/**
 * eCall object. Only one eCall object is created.
 *
 */
//--------------------------------------------------------------------------------------------------
static ECall_t ECallObj;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for eCall objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ECallRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * ERA-GLONASS Data object.
 *
 */
//--------------------------------------------------------------------------------------------------
static msd_EraGlonassData_t EraGlonassDataObj;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to synchronize eCall handler with MCC notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t SemaphoreRef;

//--------------------------------------------------------------------------------------------------
/**
 * Store the main thread, needed to queue a function treating eCall events to this thread.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t MainThread = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Pool used to pass eCall events to the main thread
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ECallEventsPool;

//--------------------------------------------------------------------------------------------------
/**
 * Convert Decimal Degrees in Degrees/Minutes/Seconds and return the corresponding value in
 * milliarcseconds.
 *
 * ex: 34,53 degrees = 34 degrees + 0,55 degree
 * 0,53 degree = 0,53*60 = 31,8 minutes = 31 minutes + 0,8 minute
 * 0,8 minute = 48 secondes
 *
 * 34.530000 = 34 degrees 31' 48" = (34*60*60.000+31*60.000+48)*1000 milliarcseconds
 *
 * @return value in milliarcseconds.
 */
//--------------------------------------------------------------------------------------------------
static int32_t ConvertDdToDms
(
    int32_t    ddVal
)
{
    int32_t  deg;
    float    degMod;
    float    minAbs;
    float    secDec;
    float    sec;

    // compute degrees
    deg = ddVal/1000000;
    LE_DEBUG("ddVal.%d, degrees.%d", ddVal, deg);

    // compute minutes
    degMod = ddVal%1000000;
    minAbs = degMod*60;
    int32_t min = minAbs/1000000;
    LE_DEBUG("minute.%d", min);

    // compute seconds
    secDec = minAbs - min*1000000;
    sec = secDec*60 / 1000000;
    LE_DEBUG("secondes.%e", sec);

    LE_DEBUG("final result: %d", (int32_t)((deg*60*60.000 + min*60.000 + sec)*1000));

    return ((int32_t)((deg*60*60.000 + min*60.000 + sec)*1000));
}

//--------------------------------------------------------------------------------------------------
/**
 * Report an eCall State to all eCall references.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReportState
(
    le_ecall_State_t state
)
{
    ReportState_t reportState;

    // Update eCall state for le_ecall_GetState function
    ECallObj.state = state;
    // Report state to application
    reportState.ref = ECallObj.ref;
    reportState.state = state;
    le_event_Report(ECallEventStateId, &(reportState), sizeof(reportState));
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop all the timers.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StopTimers
(
    void
)
{
    if(ECallObj.redial.intervalTimer)
    {
        LE_DEBUG("Stop the Interval timer, reference is %p.",
                  ECallObj.redial.intervalTimer);
        if (le_timer_IsRunning(ECallObj.redial.intervalTimer))
        {
            le_timer_Stop(ECallObj.redial.intervalTimer);
        }
    }

    if(ECallObj.redial.dialDurationTimer)
    {
        LE_DEBUG("Stop the dial duration timer, reference is %p.",
                  ECallObj.redial.dialDurationTimer);
        if (le_timer_IsRunning(ECallObj.redial.dialDurationTimer))
        {
            le_timer_Stop(ECallObj.redial.dialDurationTimer);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Init the redial mechanism
 *
 */
//--------------------------------------------------------------------------------------------------
static void RedialInit
(
    void
)
{
    LE_DEBUG("Redial init, state %d", ECallObj.redial.state);
    // Initialize redial state machine
    ECallObj.redial.state = ECALL_REDIAL_INIT;

    if (SystemStandard == PA_ECALL_ERA_GLONASS)
    {
        ECallObj.eraGlonass.pullModeSwitch = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start redial period.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RedialStart
(
    void
)
{
    LE_DEBUG("Start redial: state %d, isPush %d, SystemStandard %d",
             ECallObj.redial.state, ECallObj.isPushed, SystemStandard);

    // Check redial state
    switch(ECallObj.redial.state)
    {
        case ECALL_REDIAL_INIT:
        case ECALL_REDIAL_IDLE:
            break;
        case ECALL_REDIAL_STOPPED:
            LE_DEBUG("Redial stopped");
            return;
        case ECALL_REDIAL_ACTIVE:
            LE_WARN("Redial already started");
            return;
    }

    // Update redial state machine
    ECallObj.redial.state = ECALL_REDIAL_ACTIVE;

    if (SystemStandard == PA_ECALL_PAN_EUROPEAN)
    {
        if (!ECallObj.isPushed)
        {
            if (le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK)
            {
                LE_WARN("Unable to set the Push mode!");
            }
            else
            {
                ECallObj.isPushed = true;
            }
        }
    }

    // Starts dial duration timer
    le_clk_Time_t interval;
    if (SystemStandard == PA_ECALL_PAN_EUROPEAN)
    {
        interval.sec = 120;
        interval.usec = 0;
    }
    else // PA_ECALL_ERA_GLONASS
    {
        interval.sec = ECallObj.eraGlonass.dialDuration;
        interval.usec = 0;
    }

    if (!le_timer_IsRunning(ECallObj.redial.dialDurationTimer))
    {
        LE_DEBUG("Start Dial Duration timer, reference is %p, duration is %d seconds!",
                  ECallObj.redial.dialDurationTimer, (int)interval.sec);
        LE_ERROR_IF( ((le_timer_SetInterval(
                    ECallObj.redial.dialDurationTimer, interval)
                    != LE_OK) ||
                    (le_timer_SetHandler(ECallObj.redial.dialDurationTimer,
                    DialDurationTimerHandler) != LE_OK) ||
                    (le_timer_Start(ECallObj.redial.dialDurationTimer)
                    != LE_OK)),
                    "Cannot start the dialDurationTimer !");
    }
    else
    {
        LE_WARN("Dial Duration is already started!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear the redial mechanism when the connection is established
 *
 */
//--------------------------------------------------------------------------------------------------
static void RedialClear
(
    void
)
{
    // Check redial state
    switch(ECallObj.redial.state)
    {
        case ECALL_REDIAL_INIT:
        case ECALL_REDIAL_ACTIVE:
            break;
        case ECALL_REDIAL_STOPPED:
            LE_DEBUG("Redial stopped");
            return;
        case ECALL_REDIAL_IDLE:
            LE_WARN("Redial already in idle state");
            return;
    }

    LE_DEBUG("Clear redial: state %d", ECallObj.redial.state);

    // Update redial state machine
    ECallObj.redial.state = ECALL_REDIAL_IDLE;

    // Stop redial timers
    StopTimers();

    // Initialize dial attempt counter
    ECallObj.redial.dialAttemptsCount = 0;

    // Initialize max dial attempt
    if (SystemStandard == PA_ECALL_PAN_EUROPEAN)
    {
        ECallObj.redial.maxDialAttempts = UNLIMITED_DIAL_ATTEMPTS;
    }
    else
    {
        switch(ECallObj.startType)
        {
            case PA_ECALL_START_AUTO:
                ECallObj.redial.maxDialAttempts = ECallObj.eraGlonass.autoDialAttempts;
                break;
            case PA_ECALL_START_MANUAL:
            case PA_ECALL_START_TEST:
                ECallObj.redial.maxDialAttempts = ECallObj.eraGlonass.manualDialAttempts;
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop the redial mecanism.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RedialStop
(
    RedialStopCause_t stopCause
)
{
    LE_DEBUG("Stop redial: cause %d, state %d", stopCause, ECallObj.redial.state);

    // Check redial state
    switch(ECallObj.redial.state)
    {
        case ECALL_REDIAL_INIT:
        case ECALL_REDIAL_IDLE:
        case ECALL_REDIAL_ACTIVE:
            break;
        case ECALL_REDIAL_STOPPED:
            LE_DEBUG("Redial already stopped [%d]", ECallObj.redial.state);
            return;
    }

    // Update redial state machine
    ECallObj.redial.state = ECALL_REDIAL_STOPPED;

    // Manage redial stop cause
    switch(stopCause)
    {
        case ECALL_REDIAL_STOP_LLACK_RECEIVED:
        case ECALL_REDIAL_STOP_ALACK_RECEIVED:
        case ECALL_REDIAL_STOP_COMPLETE:
        {
            // Redial state machine should be ECALL_REDIAL_STOPPED. Nothing else to do.
            break;
        }
        case ECALL_REDIAL_STOP_MAX_DIAL_ATTEMPT:
        case ECALL_REDIAL_DURATION_EXPIRED:
        {
            StopTimers();
            // Update eCall session state
            ECallObj.sessionState = ECALL_SESSION_STOPPED;
            // Send End of redial event
            ReportState(LE_ECALL_STATE_END_OF_REDIAL_PERIOD);
            break;
        }
        case ECALL_REDIAL_STOP_NO_REDIAL_CONDITION:
        case ECALL_REDIAL_STOP_FROM_PSAP:
        case ECALL_REDIAL_STOP_FROM_USER:
        {
            StopTimers();
            break;
        }
        default:
        {
            LE_ERROR("Unknown stop cause %d", stopCause);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Attemp a redial if all conditions are valid (redial period, number of redial attempts, ...)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DialAttempt
(
    void
)
{
    le_result_t result;

    // Check redial state
    switch(ECallObj.redial.state)
    {
        case ECALL_REDIAL_INIT:
        case ECALL_REDIAL_ACTIVE:
            LE_DEBUG("Dial attempt requested");
            break;
        case ECALL_REDIAL_IDLE:
        case ECALL_REDIAL_STOPPED:
            LE_DEBUG("Dial attempt rejected [%d]", ECallObj.redial.state);
            return LE_FAULT;
    }

    LE_DEBUG("isPush %d, SystemStandard %d", ECallObj.isPushed, SystemStandard);

    // Management of PUSH/PULL mode
    if (SystemStandard == PA_ECALL_ERA_GLONASS)
    {
        // Cf. ERA-GLONASS GOST R 54620-2011, 7.5.1.2
        if ((ECallObj.eraGlonass.pullModeSwitch) && (ECallObj.isPushed))
        {
            if (LE_OK != le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PULL))
            {
                LE_WARN("Unable to set the Pull mode!");
            }
            else
            {
                ECallObj.isPushed = false;
            }
        }
    }
    else // PA_ECALL_PAN_EUROPEAN
    {
        if (!ECallObj.isPushed)
        {
            if (LE_OK != le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH))
            {
                LE_WARN("Unable to set the Push mode!");
            }
            else
            {
                ECallObj.isPushed = true;
            }
        }
    }

    // Check dial attempts counter
    if(ECallObj.redial.maxDialAttempts == UNLIMITED_DIAL_ATTEMPTS)
    {
        LE_INFO("Start dial attempt #%d",
                ECallObj.redial.dialAttemptsCount+1);
        result = pa_ecall_Start(ECallObj.startType);
        if (LE_OK == result)
        {
            ECallObj.redial.dialAttemptsCount++;
        }
        else
        {
            LE_ERROR("Dial attempt failed!");
        }
    }
    else
    {
        if(ECallObj.redial.dialAttemptsCount < ECallObj.redial.maxDialAttempts)
        {
            LE_INFO("Start dial attempt %d of %d",
                    ECallObj.redial.dialAttemptsCount+1,
                    ECallObj.redial.maxDialAttempts);
            result = pa_ecall_Start(ECallObj.startType);
            if (LE_OK == result)
            {
                ECallObj.redial.dialAttemptsCount++;
            }
            else
            {
                LE_ERROR("Dial attempt failed!");
            }
        }
        else
        {
            LE_INFO("Max dial attempts done");
            // Stop redial
            RedialStop(ECALL_REDIAL_STOP_MAX_DIAL_ATTEMPT);
            result = LE_OK;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Invalidate the MSD stored in the eCall object
 *
 */
//--------------------------------------------------------------------------------------------------
static void InvalidateMsd
(
    void
)
{
    memset(ECallObj.builtMsd, 0, sizeof(ECallObj.builtMsd));
    ECallObj.builtMsdSize = 0;
    ECallObj.isMsdImported = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Dial Duration Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DialDurationTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    if( timerRef != ECallObj.redial.dialDurationTimer )
    {
        LE_WARN("Dial duration handler received an unexpected timer %p.", timerRef);
    }

    LE_INFO("Dial duration expires! stop dialing...");

    // Invalidate MSD
    InvalidateMsd();

    // Hangup all call and stop eCall redial anyway
    le_mcc_HangUpAll();

    // Update eCall session state
    ECallObj.sessionState = ECALL_SESSION_STOPPED;

    // Stop redial period
    RedialStop(ECALL_REDIAL_DURATION_EXPIRED);
}

//--------------------------------------------------------------------------------------------------
/**
 * Interval Duration Timer handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void IntervalTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    if( timerRef != ECallObj.redial.intervalTimer)
    {
        LE_WARN("Interval timer handler received an unexpected timer %p.", timerRef);
    }

    // Dial attempt
    DialAttempt();
}

//--------------------------------------------------------------------------------------------------
/**
 * Attemp a redial after the interval duration
 *
 */
//--------------------------------------------------------------------------------------------------
static void DialAttemptInterval
(
    void
)
{

    le_clk_Time_t time = le_clk_GetRelativeTime();
    le_clk_Time_t interval;
    interval.usec = 0;

    if ((time.sec-ECallObj.redial.startTentativeTime.sec) >=
         ECallObj.redial.intervalBetweenAttempts)
    {
        interval.sec = 1;
    }
    else
    {
        interval.sec = ECallObj.redial.intervalBetweenAttempts -
                    (time.sec-ECallObj.redial.startTentativeTime.sec);
    }

    LE_INFO("Interval timer reference is %p, redial in %d seconds",
             ECallObj.redial.intervalTimer, (int)interval.sec);

    LE_ERROR_IF( ((le_timer_SetInterval(ECallObj.redial.intervalTimer, interval)
                    != LE_OK) ||
                    (le_timer_SetHandler(ECallObj.redial.intervalTimer,
                    IntervalTimerHandler) != LE_OK) ||
                    (le_timer_Start(ECallObj.redial.intervalTimer) != LE_OK)),
                    "Cannot start the Interval timer!");
}



//--------------------------------------------------------------------------------------------------
/**
 * Map the vehicle type string to enum.
 * the APIs enum to the asn1Msd.h enum.
 */
//--------------------------------------------------------------------------------------------------
static msd_VehicleType_t VehicleTypeEnumToEnumAsn1
(
    le_ecall_MsdVehicleType_t vehType // IN
)
{
    switch( vehType )
    {
        case LE_ECALL_MSD_VEHICLE_PASSENGER_M1:
            return MSD_VEHICLE_PASSENGER_M1;

        case LE_ECALL_MSD_VEHICLE_BUS_M2:
            return MSD_VEHICLE_BUS_M2;

        case LE_ECALL_MSD_VEHICLE_BUS_M3:
            return MSD_VEHICLE_BUS_M3;

        case LE_ECALL_MSD_VEHICLE_COMMERCIAL_N1:
            return MSD_VEHICLE_COMMERCIAL_N1;

        case LE_ECALL_MSD_VEHICLE_HEAVY_N2:
            return MSD_VEHICLE_HEAVY_N2;

        case LE_ECALL_MSD_VEHICLE_HEAVY_N3:
            return MSD_VEHICLE_HEAVY_N3;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L1E:
            return MSD_VEHICLE_MOTORCYCLE_L1E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L2E:
            return MSD_VEHICLE_MOTORCYCLE_L2E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L3E:
            return MSD_VEHICLE_MOTORCYCLE_L3E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L4E:
            return MSD_VEHICLE_MOTORCYCLE_L4E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L5E:
            return MSD_VEHICLE_MOTORCYCLE_L5E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L6E:
            return MSD_VEHICLE_MOTORCYCLE_L6E;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L7E:
            return MSD_VEHICLE_MOTORCYCLE_L7E;
        default:
            LE_ERROR( "vehType outside enum range");
            return MSD_VEHICLE_PASSENGER_M1;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Map the vehicle type string to enum.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no vehicle type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VehicleTypeStringToEnum
(
    char* vehStr,  // IN
    le_ecall_MsdVehicleType_t* vehType // OUT
)
{
    le_result_t result;
    LE_FATAL_IF((vehStr == NULL), "vehStr is NULL !");

    if (!strcmp(vehStr, "Passenger-M1"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_PASSENGER_M1;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Bus-M2"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_BUS_M2;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Bus-M3"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_BUS_M3;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Commercial-N1"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_COMMERCIAL_N1;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Heavy-N2"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_HEAVY_N2;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Heavy-N3"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_HEAVY_N3;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L1e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L1E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L2e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L2E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L3e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L3E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L4e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L4E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L5e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L5E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L6e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L6E;
        result = LE_OK;
    }
    else if (!strcmp(vehStr, "Motorcycle-L7e"))
    {
        *vehType = LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L7E;
        result = LE_OK;
    }
    else
    {
        LE_ERROR("Unknown vehicle type string '%s'", vehStr);
        result = LE_FAULT;
    }

    LE_DEBUG("VehicleTypeStringToEnum '%s' -> %d", vehStr,  *vehType);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Map the vehicle type enum to string.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no vehicle type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t VehicleTypeEnumToString
(
    le_ecall_MsdVehicleType_t vehType, // IN
    char* vehStr  // OUT
)
{
    le_result_t result = LE_OK;
    LE_FATAL_IF((vehStr == NULL), "vehStr is NULL !");
    switch( vehType )
    {
        case LE_ECALL_MSD_VEHICLE_PASSENGER_M1:
        {
            strncpy(vehStr, "Passenger-M1", VEHICLE_TYPE_MAX_BYTES);
        }
        break;

        case LE_ECALL_MSD_VEHICLE_BUS_M2:
        {
            strncpy(vehStr, "Bus-M2", VEHICLE_TYPE_MAX_BYTES);
        }
        break;

        case LE_ECALL_MSD_VEHICLE_BUS_M3:
        {
            strncpy(vehStr, "Bus-M3", VEHICLE_TYPE_MAX_BYTES);
        }
        break;

        case LE_ECALL_MSD_VEHICLE_COMMERCIAL_N1:
        {
            strncpy(vehStr, "Commercial-N1", VEHICLE_TYPE_MAX_BYTES);
        }
        break;

        case LE_ECALL_MSD_VEHICLE_HEAVY_N2:
        {
            strncpy(vehStr, "Heavy-N2", VEHICLE_TYPE_MAX_BYTES);
        }
        break;

        case LE_ECALL_MSD_VEHICLE_HEAVY_N3:
        {
            strncpy(vehStr, "Heavy-N3", VEHICLE_TYPE_MAX_BYTES);
        }
        break;


        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L1E:
        {
            strncpy(vehStr, "Motorcycle-L1e", 1+sizeof("Motorcycle-L1e"));
        }
        break;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L2E:
        {
            strncpy(vehStr, "Motorcycle-L2e", 1+sizeof("Motorcycle-L2e"));
        }
        break;


        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L3E:
        {
            strncpy(vehStr, "Motorcycle-L3e", 1+sizeof("Motorcycle-L3e"));
        }
        break;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L4E:
        {
            strncpy(vehStr, "Motorcycle-L4e", 1+sizeof("Motorcycle-L4e"));
        }
        break;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L5E:
        {
            strncpy(vehStr, "Motorcycle-L5e", 1+sizeof("Motorcycle-L5e"));
        }
        break;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L6E:
        {
            strncpy(vehStr, "Motorcycle-L6e", 1+sizeof("Motorcycle-L6e"));
        }
        break;

        case LE_ECALL_MSD_VEHICLE_MOTORCYCLE_L7E:
        {
            strncpy(vehStr, "Motorcycle-L7e", 1+sizeof("Motorcycle-L7e"));
        }
        break;

        default:
        {
            LE_ERROR("ERROR vehType enum not recognized %d", vehType);
            result = LE_FAULT;
        }
    }

    LE_DEBUG("VehicleTypeEnumToString  %d -> '%s'", vehType, vehStr );
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse the vehicle type from the config DB entry and update the corresponding MSD element.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no vehicle type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetVehicleType
(
    char* vehStr
)
{
    LE_FATAL_IF((vehStr == NULL), "vehStr is NULL !");

    le_ecall_MsdVehicleType_t vehType;
    le_result_t result = VehicleTypeStringToEnum( vehStr, &vehType);
    if (LE_OK == result)
    {
        ECallObj.msd.msdMsg.msdStruct.control.vehType = VehicleTypeEnumToEnumAsn1(vehType);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the propulsion type from the config DB entry and update the corresponding MSD element.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT no propulsion type found.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseAndSetPropulsionType
(
    char* propStr,
    msd_VehiclePropulsionStorageType_t* vehiclePropulsionStorageTypePtr
)
{
    LE_FATAL_IF((propStr == NULL), "propStr is NULL !");

    if (!strcmp(propStr, "Gasoline"))
    {
        vehiclePropulsionStorageTypePtr->gasolineTankPresent = true;
    }
    else if (!strcmp(propStr, "Diesel"))
    {
        vehiclePropulsionStorageTypePtr->dieselTankPresent = true;
    }
    else if (!strcmp(propStr, "NaturalGas"))
    {
        vehiclePropulsionStorageTypePtr->compressedNaturalGas = true;
    }
    else if (!strcmp(propStr, "Propane"))
    {
        vehiclePropulsionStorageTypePtr->liquidPropaneGas = true;
    }
    else if (!strcmp(propStr, "Electric"))
    {
        vehiclePropulsionStorageTypePtr->electricEnergyStorage = true;
    }
    else if (!strcmp(propStr, "Hydrogen"))
    {
        vehiclePropulsionStorageTypePtr->hydrogenStorage = true;
    }
    else if (!strcmp(propStr, "Other"))
    {
        vehiclePropulsionStorageTypePtr->otherStorage = true;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the propulsions type
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT no valid propulsion type found
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetPropulsionType
(
    void
)
{
    uint8_t i=0;
    char cfgNodeLoc[8] = {0};
    char configPath[LE_CFG_STR_LEN_BYTES];
    char propStr[PROPULSION_MAX_BYTES] = {0};
    le_result_t res = LE_OK;
    msd_VehiclePropulsionStorageType_t  vehPropulsionStorageType;

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_ECALL_PATH, CFG_NODE_PROP);
    le_cfg_IteratorRef_t propCfg = le_cfg_CreateReadTxn(configPath);

    snprintf(cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);

    // Init propulsion type bitmask
    memset(&vehPropulsionStorageType, 0, sizeof(vehPropulsionStorageType));

    while (!le_cfg_IsEmpty(propCfg, cfgNodeLoc))
    {
        if (le_cfg_GetString(propCfg, cfgNodeLoc, propStr, sizeof(propStr), "") != LE_OK)
        {
            LE_ERROR("No node value set for '%s'", CFG_NODE_PROP);
            res = LE_FAULT;
            break;
        }
        if (strlen(propStr) == 0)
        {
            LE_ERROR("No node value set for '%s'", CFG_NODE_PROP);
            res = LE_FAULT;
            break;
        }
        LE_DEBUG("eCall settings, Propulsion is %s", propStr);
        if (ParseAndSetPropulsionType(propStr, &vehPropulsionStorageType) != LE_OK)
        {
            LE_ERROR("Bad propulsion type!");
            res = LE_FAULT;
            break;
        }
        else
        {
            // Store propulsion type to MSD message
            memcpy(&ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType
                    , &vehPropulsionStorageType
                    , sizeof(vehPropulsionStorageType));
            res = LE_OK;
        }

        i++;
        snprintf(cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
    }
    le_cfg_CancelTxn(propCfg);

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Map the coordinate system type APIs enum to the asn1Msd.h enum
 */
//--------------------------------------------------------------------------------------------------
static msd_CoordinateSystemType_t CoordinateSystemTypeEnumToEnumAsn1
(
    le_ecall_MsdCoordinateType_t coordinateType ///< [IN] Coordinate system type
)
{
    switch (coordinateType)
    {
        case LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_ABSENT:
            return MSD_COORDINATE_SYSTEM_TYPE_ABSENT;

        case LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_WGS84:
            return MSD_COORDINATE_SYSTEM_TYPE_WGS84;

        case LE_ECALL_MSD_COORDINATE_SYSTEM_TYPE_PZ90:
            return MSD_COORDINATE_SYSTEM_TYPE_PZ90;

        default:
            LE_ERROR( "coordinateType outside enum range");
            return MSD_COORDINATE_SYSTEM_TYPE_ABSENT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the eCall settings
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT there are missing settings
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadECallSettings
(
    ECall_t* eCallPtr
)
{
    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s", CFG_MODEMSERVICE_ECALL_PATH);

    LE_DEBUG("Start reading eCall information in ConfigDB");

    le_cfg_IteratorRef_t eCallCfg = le_cfg_CreateReadTxn(configPath);

    {
        // Get VIN
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_VIN))
        {
            char vinStr[LE_ECALL_VIN_MAX_BYTES] = {0};
            if (le_cfg_GetString(eCallCfg, CFG_NODE_VIN, vinStr, sizeof(vinStr), "") != LE_OK)
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_VIN);
            }
            else if (strlen(vinStr) > 0)
            {
                memcpy((void *)&(eCallPtr->msd.msdMsg.msdStruct.vehIdentificationNumber),
                       (const void *)vinStr,
                       strlen(vinStr));
            }
            LE_DEBUG("eCall settings, VIN is %s", vinStr);
        }
        else
        {
            LE_WARN("No value set for '%s' !", CFG_NODE_VIN);
        }

        // Get vehicle type
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_VEH))
        {
            char  vehStr[VEHICLE_TYPE_MAX_BYTES] = {0};
            if (le_cfg_GetString(eCallCfg, CFG_NODE_VEH, vehStr, sizeof(vehStr), "") != LE_OK)
            {
                LE_WARN("No node value set for '%s'", CFG_NODE_VEH);
            }
            else if (strlen(vehStr) > 0)
            {
                LE_DEBUG("eCall settings, vehicle is %s", vehStr);
                if (LE_OK != ParseAndSetVehicleType(vehStr))
                {
                    LE_WARN("Bad vehicle type!");
                }
            }
        }
        else
        {
            LE_WARN("No value set for '%s' !", CFG_NODE_VEH);
        }

        // Get MSD version
        if (le_cfg_NodeExists(eCallCfg, CFG_NODE_MSDVERSION))
        {
            eCallPtr->msd.version = le_cfg_GetInt(eCallCfg, CFG_NODE_MSDVERSION, 0);
            LE_DEBUG("eCall settings, MSD version is %d", eCallPtr->msd.version);
            if (eCallPtr->msd.version == 0)
            {
                LE_WARN("No correct value set for '%s' ! Use the default one (%d)",
                        CFG_NODE_MSDVERSION,
                        DEFAULT_MSD_VERSION);
                eCallPtr->msd.version = DEFAULT_MSD_VERSION;
            }
        }
        else
        {
            LE_WARN("No value set for '%s' ! Use the default one (%d)",
                    CFG_NODE_MSDVERSION,
                    DEFAULT_MSD_VERSION);
            eCallPtr->msd.version = DEFAULT_MSD_VERSION;
        }

        // Get system standard
        {
            char  sysStr[SYS_STD_MAX_BYTES] = {0};
            bool isEraGlonass = false;
            if (le_cfg_NodeExists(eCallCfg, CFG_NODE_SYSTEM_STD))
            {
                if (le_cfg_GetString(eCallCfg,
                                      CFG_NODE_SYSTEM_STD,
                                      sysStr,
                                      sizeof(sysStr),
                                      "PAN-EUROPEAN") != LE_OK)
                {
                    LE_WARN("No node value set for '%s' ! Use the default one (%s)",
                            CFG_NODE_SYSTEM_STD,
                            sysStr);
                }
                else if ((strncmp(sysStr, "PAN-EUROPEAN", strlen("PAN-EUROPEAN")) != 0) &&
                         (strncmp(sysStr, "ERA-GLONASS", strlen("ERA-GLONASS")) != 0))
                {
                    LE_WARN("Bad value set for '%s' ! Use the default one (%s)",
                            CFG_NODE_SYSTEM_STD,
                            sysStr);
                }
                LE_DEBUG("eCall settings, system standard is %s", sysStr);
                if (strncmp(sysStr, "ERA-GLONASS", strlen("ERA-GLONASS")) == 0)
                {
                    isEraGlonass = true;
                }
            }
            else
            {
                LE_WARN("No node value set for '%s' ! Use the default one (%s)",
                        CFG_NODE_SYSTEM_STD,
                        sysStr);
            }
            if (isEraGlonass)
            {
                SystemStandard = PA_ECALL_ERA_GLONASS;
            }
            else
            {
                SystemStandard = PA_ECALL_PAN_EUROPEAN;
            }
            LE_INFO("Selected standard is %s (%d)", sysStr, SystemStandard);

            if ( LE_OK != pa_ecall_UpdateSystemStandard(SystemStandard))
            {
               LE_INFO("Update PA system standard (%d) failed!", SystemStandard);
            }
        }
    }

    le_cfg_CancelTxn(eCallCfg);

    return GetPropulsionType();
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function when the eCall settings are modified in the configuration tree.
 */
//--------------------------------------------------------------------------------------------------
static void SettingsUpdate
(
    void* contextPtr
)
{
    LE_INFO("eCall settings have changed!");
    LoadECallSettings(&ECallObj);
}

//--------------------------------------------------------------------------------------------------
/**
 * Encode an MSD from the eCall data object.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t EncodeMsd
(
    ECall_t*   eCallPtr
)
{
    uint8_t outOptionalDataForEraGlonass[160]={0}; ///< 160 Bytes is guaranteed enough
                                                   ///< for the data part
    uint8_t oid[] = {1,4,1}; ///< OID version for MSD version 1
    uint8_t oidV2[] = {1,4,2}; ///< OID version for MSD version 2

    LE_FATAL_IF((eCallPtr == NULL), "eCallPtr is NULL !");

    if (!eCallPtr->isMsdImported)
    {
        LE_DEBUG("eCall MSD: VIN.%17s, version.%d, veh.%d",
                (char*)&eCallPtr->msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0],
                eCallPtr->msd.version,
                (int)eCallPtr->msd.msdMsg.msdStruct.control.vehType);

        LE_DEBUG("eCall MSD: gasoline.%d, diesel.%d, gas.%d, propane.%d, electric.%d, hydrogen.%d",
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent,
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent,
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas,
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas,
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage,
                eCallPtr->msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage);

        LE_DEBUG("eCall MSD: isPosTrusted.%d, lat.%d, long.%d, dir.%d",
                eCallPtr->msd.msdMsg.msdStruct.control.positionCanBeTrusted,
                eCallPtr->msd.msdMsg.msdStruct.vehLocation.latitude,
                eCallPtr->msd.msdMsg.msdStruct.vehLocation.longitude,
                eCallPtr->msd.msdMsg.msdStruct.vehDirection);

        LE_DEBUG("eCall MSD: isPax.%d, paxCount.%d",
                eCallPtr->msd.msdMsg.msdStruct.numberOfPassengersPres,
                eCallPtr->msd.msdMsg.msdStruct.numberOfPassengers);

        LE_DEBUG("isPush %d, SystemStandard %d", ECallObj.isPushed, SystemStandard);

        // Encode optional Data for ERA GLONASS if any
        if(SystemStandard == PA_ECALL_ERA_GLONASS)
        {
            EraGlonassDataObj.msdVersion = eCallPtr->msd.version;

            if (EraGlonassDataObj.presentCrashInfo ||
                EraGlonassDataObj.presentCrashSeverity ||
                EraGlonassDataObj.presentDiagnosticResult ||
                EraGlonassDataObj.presentCoordinateSystemTypeInfo)
            {
                if ((eCallPtr->msd.msdMsg.optionalData.dataLen =
                    msd_EncodeOptionalDataForEraGlonass(
                        &EraGlonassDataObj, outOptionalDataForEraGlonass))
                    <= 0)
                {
                    LE_ERROR("Unable to encode optional Data for ERA GLONASS!");
                    return LE_FAULT;
                }

                if( 0 < eCallPtr->msd.msdMsg.optionalData.dataLen )
                {
                    if (eCallPtr->msd.version == 1)
                    {
                        eCallPtr->msd.msdMsg.optionalData.oid = oid;
                        eCallPtr->msd.msdMsg.optionalData.oidlen = sizeof(oid);
                    }
                    else if (eCallPtr->msd.version == 2)
                    {
                        eCallPtr->msd.msdMsg.optionalData.oid = oidV2;
                        eCallPtr->msd.msdMsg.optionalData.oidlen = sizeof(oidV2);
                    }

                    eCallPtr->msd.msdMsg.optionalDataPres = true;
                    eCallPtr->msd.msdMsg.optionalData.data = outOptionalDataForEraGlonass;
                }
            }
            else
            {
                eCallPtr->msd.msdMsg.optionalDataPres = false;
                eCallPtr->msd.msdMsg.optionalData.oid = NULL;
                eCallPtr->msd.msdMsg.optionalData.oidlen = 0;
                eCallPtr->msd.msdMsg.optionalData.dataLen = 0;
            }

            LE_DEBUG("eCall optional Data: Length %d",
                     eCallPtr->msd.msdMsg.optionalData.dataLen);
        }

        // Encode MSD message
        if ((eCallPtr->builtMsdSize = msd_EncodeMsdMessage(&eCallPtr->msd, eCallPtr->builtMsd))
            == LE_FAULT)
        {
            LE_ERROR("Unable to encode the MSD! Please verify your settings in the config tree.");
            return LE_FAULT;
        }
    }
    else
    {
        LE_WARN("an MSD has been imported, no need to encode it.");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer eCall State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerECallStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ReportState_t *reportStatePtr = reportPtr;
    le_ecall_StateChangeHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    LE_DEBUG("First Layer Handler Function called with state %d for eCall ref.%p",
             reportStatePtr->state,
             reportStatePtr->ref);

    clientHandlerFunc(reportStatePtr->ref, reportStatePtr->state, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Restore state machine as initialized.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RestoreEcallStatus
(
   le_ecall_CallRef_t    ecallRef    ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }


    if (!eCallPtr->isMsdImported)
    {
        eCallPtr->msd.msdMsg.msdStruct.messageIdentifier = 0;
        eCallPtr->msd.msdMsg.msdStruct.timestamp = (uint32_t)time(NULL);

        if (PA_ECALL_START_AUTO == ECallObj.startType)
        {
            eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = true;
            eCallPtr->msd.msdMsg.msdStruct.control.testCall = false;
        }
        else if (PA_ECALL_START_MANUAL == ECallObj.startType)
        {
            eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
            eCallPtr->msd.msdMsg.msdStruct.control.testCall = false;
        }
        else
        {
            eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
            eCallPtr->msd.msdMsg.msdStruct.control.testCall = true;
        }

        // Update MSD with msg ID, timestamp and control flags
        if (EncodeMsd(eCallPtr) != LE_OK)
        {
            LE_ERROR("Encode MSD failure (msg ID, timestamp and control flags)");
            return LE_FAULT;
        }
    }

    // Initialize redial state machine
    RedialInit();

    if (!ECallObj.isPushed)
    {
        if (LE_OK != le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH))
        {
            LE_WARN("Unable to set the Push mode!");
        }
        else
        {
            ECallObj.isPushed = true;
        }
    }

    // Update eCall context
    ECallObj.redial.startTentativeTime = le_clk_GetRelativeTime();
    ECallObj.termination = LE_MCC_TERM_UNDEFINED;
    ECallObj.specificTerm = 0;
    ECallObj.ackOrT7Received = false;
    ECallObj.t5Received = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Processing of new eCall state.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProcessECallState
(
    void* param1Ptr,
    void* param2Ptr
)
{
    bool endOfRedialPeriod = false;
    ECallEventData_t* eCallEventDataPtr = (ECallEventData_t*) param1Ptr;

    LE_DEBUG("Process new eCall state %d (sessionState %d)",
             eCallEventDataPtr->state, ECallObj.sessionState);

    switch (eCallEventDataPtr->state)
    {
        case LE_ECALL_STATE_STARTED: /* eCall session started */
        {
            LE_DEBUG("Start dialing...");
            // Update eCall context
            ECallObj.sessionState = ECALL_SESSION_NOT_CONNECTED;
            ECallObj.redial.startTentativeTime = le_clk_GetRelativeTime();
            ECallObj.termination = LE_MCC_TERM_UNDEFINED;
            ECallObj.specificTerm = 0;
            ECallObj.ackOrT7Received = false;
            ECallObj.t5Received = false;
            ECallObj.sendMsdSignalReceived = false;
            break;
        }

        case LE_ECALL_STATE_DISCONNECTED: /* Emergency call is disconnected */
        {
            LE_DEBUG("Termination reason: %d, ackOrT7Received: %d, "
                     "t5Received: %d, sendMsdSignalReceived: %d, "
                     "terminationReceived: %d",
                     ECallObj.termination,
                     ECallObj.ackOrT7Received,
                     ECallObj.t5Received,
                     ECallObj.sendMsdSignalReceived,
                     eCallEventDataPtr->terminationReceived);

            LE_DEBUG("isPush %d, SystemStandard %d, pullMode %d",
                  ECallObj.isPushed, SystemStandard, ECallObj.eraGlonass.pullModeSwitch);

            // Update eCall session state
            switch(ECallObj.sessionState)
            {
                case ECALL_SESSION_CONNECTED:
                {
                    // End Of Redial Period if incoming call is not an eCall
                    if (!ECallObj.sendMsdSignalReceived && ECallObj.isMtCall)
                    {
                        LE_INFO("Not an eCall, stop redial mechanism");
                        endOfRedialPeriod = true;
                    }
                    // Check redial condition (cf N16062:2014 7.9)
                    else if (   (true == eCallEventDataPtr->terminationReceived)
                        && ((ECallObj.ackOrT7Received) || (ECallObj.t5Received))
                        && (LE_MCC_TERM_REMOTE_ENDED == ECallObj.termination)
                       )
                    {
                        // After the IVS has received an LL-ACK or AL-ACK
                        // or T7-IVS MSD maximum transmission time ends or T5-IVS waits
                        // for SEND MSD period ends, the IVS shall recognize a normal
                        // hang-up from the network. The IVS shall not attempt an
                        // automatic redial following a call clear-down.

                        // End Of Redial Period
                        endOfRedialPeriod = true;
                    }
                    // End of Redial if call drop due to Normal Clear in Pull mode
                    else if (    (false == ECallObj.sendMsdSignalReceived)
                              && (false == ECallObj.isPushed)
                              && (true == eCallEventDataPtr->terminationReceived)
                              && (LE_MCC_TERM_REMOTE_ENDED == ECallObj.termination)
                            )
                    {
                        // End Of Redial Period
                        endOfRedialPeriod = true;
                    }
                    else
                    {
                        // Start redial with abnormal hang up(cf N16062:2014 7.12.13)
                        RedialStart();

                        // eCall session attempt if redial mechanism is not stopped
                        DialAttempt();
                    }

                    // Update eCall session state
                    ECallObj.sessionState = ECALL_SESSION_NOT_CONNECTED;
                    break;
                }

                case ECALL_SESSION_NOT_CONNECTED:
                {
                    LE_WARN("Failed to connect with PSAP");
                    // eCall session was not connected. Redial in attempt interval
                    DialAttemptInterval();

                    // No need to update eCall session state, already not connected
                    break;
                }

                case ECALL_SESSION_COMPLETED:
                {
                    // Check redial condition (cf N16062:2014 7.9)
                    if (   (true == eCallEventDataPtr->terminationReceived)
                        && (LE_MCC_TERM_REMOTE_ENDED == ECallObj.termination)
                       )
                    {
                        // End Of Redial Period
                        endOfRedialPeriod = true;
                    }
                    else
                    {
                        // Start redial with abnormal hang up(cf N16062:2014 7.12.13)
                        RedialStart();

                        // eCall session attempt if redial mechanism is not stopped
                        DialAttempt();
                    }

                    // Update eCall session state
                    ECallObj.sessionState = ECALL_SESSION_NOT_CONNECTED;
                    break;
                }

                case ECALL_SESSION_STOPPED:
                    // Session stopped: no redial
                    // No need to update eCall session state, already stopped
                    endOfRedialPeriod = true;
                    break;

                case ECALL_SESSION_INIT:
                case ECALL_SESSION_REQUEST:
                default:
                    LE_ERROR("Unexpected session state %d", ECallObj.sessionState);
                    // Update eCall session state
                    ECallObj.sessionState = ECALL_SESSION_NOT_CONNECTED;
                    break;
            }
            break;
        }

        case LE_ECALL_STATE_CONNECTED: /* Emergency call is established */
        {
            // Update eCall session state
            ECallObj.sessionState = ECALL_SESSION_CONNECTED;
            // Clear redial during connection
            RedialClear();
            break;
        }

        case LE_ECALL_STATE_COMPLETED: /* eCall session completed */
        {
            // Update eCall session state
            ECallObj.sessionState = ECALL_SESSION_COMPLETED;
            break;
        }

        case LE_ECALL_STATE_ALACK_RECEIVED_POSITIVE: /* eCall session completed */
        case LE_ECALL_STATE_TIMEOUT_T7: /* Timeout for T7 */
        {
            // Redial condition (cf N16062:2014 7.9)
            ECallObj.ackOrT7Received = true;

            if (PA_ECALL_ERA_GLONASS == SystemStandard)
            {
                // Cf. ERA-GLONASS GOST R 54620-2011, 7.5.1.2:
                // After receiving AL-ACK, IVS should redial
                // in pull mode if the connection is lost.
                ECallObj.eraGlonass.pullModeSwitch = true;
            }
            break;
        }

        case LE_ECALL_STATE_ALACK_RECEIVED_CLEAR_DOWN: /* AL-ACK clear-down received */
        {
            // According to the eCall standard (FprEN 16062:2014) / eCall clear-down
            // After the PSAP has sent the LL-ACK or T4 – PSAP wait for INITIATION signal period
            // or T8 - PSAP MSD maximum reception time ends
            // and the IVS receives a AL-ACK with status = “clear- down”,
            // it shall clear-down the call.
            // The IVS shall not attempt an automatic redial following a call clear-down.

            // End Of Redial Period
            endOfRedialPeriod = true;
            break;
        }

        case LE_ECALL_STATE_STOPPED: /* eCall session has been stopped by PSAP
                                        or IVS le_ecall_End() */
        {
            // The eCall is now correctly closed, the MSD can be invalidated.
            InvalidateMsd();

            // Update eCall session state
            ECallObj.sessionState = ECALL_SESSION_STOPPED;
            // Stop redial period
            RedialStop(ECALL_REDIAL_STOP_FROM_PSAP);
            break;
        }

        case LE_ECALL_STATE_MSD_TX_STARTED: /* MSD transmission is started */
        case LE_ECALL_STATE_WAITING_PSAP_START_IND: /* Waiting for PSAP start indication */
        case LE_ECALL_STATE_LLNACK_RECEIVED: /* LL-NACK received */
        case LE_ECALL_STATE_MSD_TX_COMPLETED: /* MSD transmission is complete */
        case LE_ECALL_STATE_RESET: /* eCall session has lost synchronization and starts over */
        case LE_ECALL_STATE_MSD_TX_FAILED: /* MSD transmission has failed */
        case LE_ECALL_STATE_FAILED: /* Unsuccessful eCall session */
        case LE_ECALL_STATE_TIMEOUT_T3: /* Timeout for T3 */
        case LE_ECALL_STATE_TIMEOUT_T6: /* Timeout for T6 */
        case LE_ECALL_STATE_TIMEOUT_T9: /* Timeout for T9 */
        case LE_ECALL_STATE_TIMEOUT_T10: /* Timeout for T10 */
        {
            // Nothing to do, just report the event
             break;
        }

        case LE_ECALL_STATE_TIMEOUT_T5: /* Timeout for T5 */
        {
            // To check redial condition (cf N16062:2014 7.9)
            LE_DEBUG("STATE_TIMEOUT_T5");
            ECallObj.t5Received = true;
            break;
        }

        case LE_ECALL_STATE_TIMEOUT_T2: /* Timeout for T2 */
        {
            // End Of Redial Period
            endOfRedialPeriod = true;
            break;
        }

        case LE_ECALL_STATE_LLACK_RECEIVED: /* LL-ACK received */
        {
            // To check redial condition (cf N16062:2014 7.9)
            ECallObj.ackOrT7Received = true;
            break;
        }

        case LE_ECALL_STATE_PSAP_START_IND_RECEIVED: /* PSAP start indication received */
        {
            LE_DEBUG("STATE_PSAP_START_IND_RECEIVED");
            ECallObj.sendMsdSignalReceived = true;

            // To check incoming eCall
            if (ECallObj.isMtCall)
            {
                LE_DEBUG("Incoming call request MSD");
                ECallObj.callId = ECallObj.mtCallId;
                if (LE_OK != RestoreEcallStatus(ECallObj.ref))
                {
                    LE_INFO("Restore eCall status failed!");
                }
            }
            break;
        }

        case LE_ECALL_STATE_UNKNOWN: /* Unknown state */
        default:
        {
            LE_ERROR("Unknown eCall indication %d", eCallEventDataPtr->state);
            break;
        }
    }

    // Report the eCall state
    ReportState(eCallEventDataPtr->state);

    // Report the End of Redial Period event if needed
    if (endOfRedialPeriod)
    {
        // Stop redial period
        RedialStop(ECALL_REDIAL_STOP_NO_REDIAL_CONDITION);

        // Update eCall session state
        ECallObj.sessionState = ECALL_SESSION_STOPPED;
    }

    // Release memory allocated for eCall event
    le_mem_Release(eCallEventDataPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal eCall State handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ECallStateHandler
(
    le_ecall_State_t* statePtr
)
{
    // Allocate and fill eCall event data
    ECallEventData_t* eCallEventDataPtr = le_mem_ForceAlloc(ECallEventsPool);
    eCallEventDataPtr->state = *statePtr;
    eCallEventDataPtr->terminationReceived = false;

    LE_DEBUG("Received new eCall state %d", eCallEventDataPtr->state);

    // Disconnection of eCall notified, wait for the call termination reason notified by MCC.
    if (LE_ECALL_STATE_DISCONNECTED == *statePtr)
    {
        le_clk_Time_t timer = { .sec=LE_ECALL_SEM_TIMEOUT_SEC,
                                .usec=LE_ECALL_SEM_TIMEOUT_USEC };
        le_result_t semTerminationResult;

        // Get call termination result
        semTerminationResult = le_sem_WaitWithTimeOut(SemaphoreRef, timer);
        if (LE_OK != semTerminationResult)
        {
            LE_ERROR("MCC notification timeout happen");
        }
        else
        {
            eCallEventDataPtr->terminationReceived = true;
        }
        LE_DEBUG("Termination reason: %d, semaphore: %d",
                 ECallObj.termination, semTerminationResult);
    }

    if (NULL != MainThread)
    {
        // Timers are linked to the thread originating their start or stop:
        // the new eCall state processing should therefore be done in the same thread as
        // the eCall initialization to be able to correctly start/stop the timers.
        le_event_QueueFunctionToThread(MainThread, ProcessECallState, eCallEventDataPtr, NULL);
    }
    else
    {
        LE_ERROR("Main thread not stored!");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Internal call event handler function.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CallEventHandler
(
    le_mcc_CallRef_t  callRef,
    le_mcc_Event_t   event,
    void* contextPtr
)
{
    int32_t callId;
    ECall_t* eCallPtr = (ECall_t*) contextPtr;

    if (!eCallPtr)
    {
        LE_ERROR("NULL eCallPtr !!!");
        return;
    }

    le_result_t res = le_mcc_GetCallIdentifier(callRef, &callId);
    if (res != LE_OK)
    {
        LE_ERROR("Error in GetCallIdentifier %d", res);
        return;
    }

    if (LE_MCC_EVENT_INCOMING == event)
    {
        LE_DEBUG("Incoming call, record callId");
        ECallObj.sendMsdSignalReceived= false;
        ECallObj.isMtCall = true;
        ECallObj.mtCallId = callId;
    }
    else if (LE_MCC_EVENT_ORIGINATING == event)
    {
        LE_DEBUG("Outgoing call");
        ECallObj.isMtCall = false;
    }

    LE_DEBUG("session state %d, event %d", ECallObj.sessionState, event);

    // Check if an eCall session is active
    if ((ECALL_SESSION_INIT == eCallPtr->sessionState) ||
        (ECALL_SESSION_STOPPED == eCallPtr->sessionState))
    {
        // The call is not an eCall, no treatment necessary
        LE_DEBUG("No active eCall session, MCC event ignored");
        return;
    }

    LE_DEBUG("callId %d eCallPtr->callId %d", callId, eCallPtr->callId);

    switch (event)
    {
        case LE_MCC_EVENT_INCOMING:
        case LE_MCC_EVENT_ORIGINATING:
            LE_DEBUG("Updating callId: eCallPtr->callId %d = callId %d. sessionState %d",
                     eCallPtr->callId, callId, ECallObj.sessionState);
            // An incoming or outgoing call is being established while an eCall session is active:
            // it is an eCall, update the eCall id with this call identifier
            eCallPtr->callId = callId;
            break;

        case LE_MCC_EVENT_TERMINATED:
            if (callId == eCallPtr->callId)
            {
                // Get the termination reason of the current call. These values are used internally
                // and may be cleared in case of a redial.
                eCallPtr->termination = le_mcc_GetTerminationReason(callRef);
                eCallPtr->specificTerm = le_mcc_GetPlatformSpecificTerminationCode(callRef);

                // Save a copy of the termination reason in order to keep track of the disconnection
                // reason of the last call
                eCallPtr->lastTermination = eCallPtr->termination;
                eCallPtr->lastSpecificTerm = eCallPtr->specificTerm;
                eCallPtr->callId = -1;

                LE_DEBUG("Call termination status available: termination %d, specificTerm %d",
                        eCallPtr->termination, eCallPtr->specificTerm);
                // Synchronize eCall handler with MCC notification
                le_sem_Post(SemaphoreRef);
            }
            break;

        default:
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * ECall event handler function thread
 */
//--------------------------------------------------------------------------------------------------
static void* ECallThread
(
    void* contextPtr
)
{
    le_sem_Ref_t initSemaphore = (le_sem_Ref_t)contextPtr;

    LE_INFO("ECall event thread started");

    // Register a handler function for eCall state indications
    if (pa_ecall_AddEventHandler(ECallStateHandler) == NULL)
    {
        LE_ERROR("Add pa_ecall_AddEventHandler failed");
        return NULL;
    }

    le_sem_Post(initSemaphore);

#if LE_CONFIG_ENABLE_ECALL
    // Watchdog eCall event loop.
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_ECALL_LOOP, watchdogInterval);
#else
    LE_FATAL("ECall started but was disabled at compile time.");
#endif

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the eCall service
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_Init
(
    void
)
{
    le_ecall_MsdTxMode_t msdTxMode;

    LE_INFO("le_ecall_Init called.");

    // Create an event Id for eCall State notification
    ECallEventStateId = le_event_CreateId("ECallState", sizeof(ReportState_t));

    // Create the Safe Reference Map to use for eCall object Safe References.
    ECallRefMap = le_ref_CreateMap("ECallMap", ECALL_MAX);

    // Initialize ECallObj
    memset(&ECallObj, 0, sizeof(ECallObj));

    // Initialize MSD structure
    ECallObj.psapNumber[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovds[0] = '\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovisModelyear[0] ='\0';
    ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isovisSeqPlant[0] = '\0';
    ECallObj.msd.version = 1;
    ECallObj.isPushed = true;
    ECallObj.redial.maxDialAttempts = UNLIMITED_DIAL_ATTEMPTS;
    ECallObj.msd.msdMsg.msdStruct.control.vehType = MSD_VEHICLE_PASSENGER_M1;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage = false;
    ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage = false;

    // Retrieve the eCall settings from the configuration tree, including the static values of MSD
    if (LoadECallSettings(&ECallObj) != LE_OK)
    {
        LE_ERROR("There are missing eCall settings, can't perform eCall!");
    }

    if (pa_ecall_Init(SystemStandard) != LE_OK)
    {
        LE_ERROR("Cannot initialize Platform Adaptor for eCall, can't perform eCall!");
        return LE_FAULT;
    }

    // Initialize redial state machine
    RedialInit();

    // Ecall Context initialization
    ECallObj.sessionState = ECALL_SESSION_INIT;
    ECallObj.startType = PA_ECALL_START_MANUAL;
    ECallObj.redial.intervalTimer = le_timer_Create("Interval");
    ECallObj.redial.intervalBetweenAttempts = 30; // 30 seconds
    ECallObj.redial.dialDurationTimer = le_timer_Create("dialDurationTimer");
    ECallObj.ackOrT7Received = false;
    ECallObj.t5Received = false;
    ECallObj.sendMsdSignalReceived = false;
    ECallObj.isMtCall = false;
    ECallObj.mtCallId = 0;

    ECallObj.eraGlonass.manualDialAttempts = 10;
    ECallObj.eraGlonass.autoDialAttempts = 10;
    ECallObj.eraGlonass.dialDuration = 300;
    ECallObj.eraGlonass.pullModeSwitch = false;

    // Add a config tree handler for eCall settings update.
    le_cfg_AddChangeHandler(CFG_MODEMSERVICE_ECALL_PATH, SettingsUpdate, NULL);

    LE_DEBUG("eCall settings: gasoline.%d, diesel.%d, gas.%d, propane.%d, electric.%d, hydrogen.%d",
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage,
             ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage);

    // Initialize the other members of the eCall object
    ECallObj.msd.msdMsg.msdStruct.messageIdentifier = 0;
    ECallObj.msd.msdMsg.msdStruct.timestamp = 0;
    ECallObj.msd.msdMsg.msdStruct.control.automaticActivation = true;
    ECallObj.msd.msdMsg.msdStruct.control.testCall = false;
    ECallObj.msd.msdMsg.msdStruct.control.positionCanBeTrusted = false;
    ECallObj.msd.msdMsg.msdStruct.recentVehLocationN1Pres = false;
    ECallObj.msd.msdMsg.msdStruct.recentVehLocationN2Pres = false;
    ECallObj.msd.msdMsg.msdStruct.numberOfPassengersPres = false;
    ECallObj.msd.msdMsg.msdStruct.numberOfPassengers = 0;
    ECallObj.state = LE_ECALL_STATE_STOPPED;

    // Initialize MSD
    InvalidateMsd();

    // Initialize the eCall ERA-GLONASS Data object
    memset(&EraGlonassDataObj, 0, sizeof(EraGlonassDataObj));
    EraGlonassDataObj.presentCrashSeverity = false;
    EraGlonassDataObj.presentDiagnosticResult = false;
    EraGlonassDataObj.presentCrashInfo = false;
    EraGlonassDataObj.presentCoordinateSystemTypeInfo = false;

    if (le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK)
    {
        LE_WARN("Unable to set the Push mode!");
    }

    if (le_ecall_GetMsdTxMode(&msdTxMode) != LE_OK)
    {
        LE_WARN("Unable to retrieve the configured Push/Pull mode!");
    }
    if (msdTxMode == LE_ECALL_TX_MODE_PULL)
    {
        ECallObj.isPushed = false;
    }
    else
    {
        ECallObj.isPushed = true;
    }

    LE_DEBUG("eCall settings: VIN.%17s, version.%d, isPushed.%d, maxDialAttempts.%d, veh.%d",
             (char*)&ECallObj.msd.msdMsg.msdStruct.vehIdentificationNumber.isowmi[0],
             ECallObj.msd.version,
             ECallObj.isPushed,
             ECallObj.redial.maxDialAttempts,
             (int)ECallObj.msd.msdMsg.msdStruct.control.vehType);

    // Initialize call identifier
    ECallObj.callId = -1;

    // Initialize call termination
    ECallObj.termination = LE_MCC_TERM_UNDEFINED;
    ECallObj.lastTermination = LE_MCC_TERM_UNDEFINED;

    // Register handler on call events
    le_mcc_AddCallEventHandler(CallEventHandler, &ECallObj);

    // Register object, which also means it was initialized properly
    ECallObj.ref = le_ref_CreateRef(ECallRefMap, &ECallObj);

    // Timers are linked to the thread originating their start or stop:
    // as the current thread will be used to start the timers, store it to be able
    // to use this thread to treat the eCall events and stop the timers later.
    MainThread = le_thread_GetCurrent();

    // Create pool to report eCall events to the handler
    ECallEventsPool = le_mem_CreatePool("ECallEventsPool", sizeof(ECallEventData_t));
    // Expand the pool to be able to store eCall events received almost at the same time
    le_mem_ExpandPool(ECallEventsPool, ECALL_EVENTS_POOL_SIZE);

    // Start ECall thread
    le_sem_Ref_t initSemaphore = le_sem_Create("InitSem",0);
    le_thread_Start(le_thread_Create("ECallThread", ECallThread, (void*)initSemaphore));
    le_sem_Wait(initSemaphore);
    le_sem_Delete(initSemaphore);

    // Init semaphore to synchronize eCall handler with MCC notification
    SemaphoreRef = le_sem_Create("MccNotificationSem",0);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the eCall operation mode to eCall only, only emergency number can be
 * used to start an eCall session. The modem doesn't try to register on the Cellular network.
 * This function forces the modem to behave as eCall only mode whatever U/SIM operation mode.
 * The change doesn't persist over power cycles.
 * This function can be called before making an eCall.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ForceOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_ONLY_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * Same as le_ecall_ForceOnlyMode(), but the change persists over power cycles.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ForcePersistentOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_FORCED_PERSISTENT_ONLY_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function exits from eCall Only mode. It configures the eCall operation mode to Normal mode,
 * the modem uses the default operation mode at power up (or after U/SIM hotswap). The modem behaves
 * following the U/SIM eCall operation mode; for example the U/SIM can be configured only for eCall,
 * or a combination of eCall and commercial service provision.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ExitOnlyMode
(
    void
)
{
    return (pa_ecall_SetOperationMode(LE_ECALL_NORMAL_MODE));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the configured Operation mode.
 *
 * @return
 *      - LE_OK     on success
 *      - LE_FAULT  for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetConfiguredOperationMode
(
    le_ecall_OpMode_t *opModePtr    ///< [OUT] Operation mode
)
{
    if (NULL == opModePtr)
    {
        LE_KILL_CLIENT("opModePtr is NULL!");
        return LE_FAULT;
    }

    return (pa_ecall_GetOperationMode(opModePtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new eCall object
 *
 * The eCall is not actually established at this point. It's still up to the caller to call
 * le_ecall_Start() when ready.
 *
 * @return A reference to the new Call object.
 *
 * @note On failure, the process exits; you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_ecall_CallRef_t le_ecall_Create
(
    void
)
{
    LE_FATAL_IF(ECallObj.ref == NULL, "eCall object was not initialized properly.");
    return ECallObj.ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Call to free up a call reference.
 *
 * @note This will free the reference, but not necessarily stop an active eCall. If there are
 *       other holders of this reference the eCall will remain active.
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_Delete
(
    le_ecall_CallRef_t ecallRef     ///< [IN] eCall reference
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the position transmitted by the MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad input parameter
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPosition
(
    le_ecall_CallRef_t  ecallRef,   ///< [IN] eCall reference
    bool                isTrusted,  ///< [IN] True if the position can be trusted, false otherwise
    int32_t             latitude,   ///< [IN] The latitude in degrees with 6 decimal places,
                                    ///       positive North. Maximum value is +90 degrees
                                    ///       (+90000000), minimum value is -90 degrees (-90000000).
    int32_t             longitude,  ///< [IN] The longitude in degrees with 6 decimal places,
                                    ///       positive East. Maximum value is +180 degrees
                                    ///       (+180000000), minimum value is -180 degrees
                                    ///       (-180000000).
    int32_t             direction   ///< [IN] The direction of the vehicle from magnetic north (0
                                    ///       to 358, clockwise) in 2-degrees unit. Valid range is
                                    ///       0 to 179. If direction of travel is invalid or
                                    ///       unknown, the value 0xFF shall be used.
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    if ((latitude > 90000000) || (latitude < -90000000))
    {
        LE_ERROR("Maximum latitude value is +90000000, minimum latitude value is -90000000!");
        return LE_BAD_PARAMETER;
    }
    if ((longitude > 180000000) || (longitude < -180000000))
    {
        LE_ERROR("Maximum longitude value is +180000000, minimum longitude value is -180000000!");
        return LE_BAD_PARAMETER;
    }
    if ((direction > 179) && (direction != 0xFF))
    {
        LE_ERROR("The direction of the vehicle must be in 2°-degrees steps, maximum value is 178!");
        return LE_BAD_PARAMETER;
    }

    eCallPtr->msd.msdMsg.msdStruct.control.positionCanBeTrusted = isTrusted;
    eCallPtr->msd.msdMsg.msdStruct.vehLocation.latitude = ConvertDdToDms(latitude);
    eCallPtr->msd.msdMsg.msdStruct.vehLocation.longitude = ConvertDdToDms(longitude);
    eCallPtr->msd.msdMsg.msdStruct.vehDirection = direction;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the position Delta N-1 from position set in le_ecall_SetMsdPosition() transmitted by the MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad input parameter
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPositionN1
(
    le_ecall_CallRef_t  ecallRef,   ///< eCall reference
    int32_t     latitudeDeltaN1,    ///< latitude delta from position set in SetMsdPosition
                                    ///< 1 Unit = 100 miliarcseconds, which is approximately 3m
                                    ///< maximum value: 511 = 0 0'51.100'' (±1580m)
                                    ///< minimum value: -512 = -0 0'51.200'' (± -1583m)
    int32_t     longitudeDeltaN1    ///< longitude delta from position set in SetMsdPosition
                                    ///< 1 Unit = 100 miliarcseconds, which is approximately 3m
                                    ///< maximum value: 511 = 0 0'51.100'' (±1580m)
                                    ///< minimum value: -512 = -0 0'51.200'' (± -1583m)
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    if ((latitudeDeltaN1 > ASN1_LATITUDE_DELTA_MAX) ||
        (latitudeDeltaN1 < ASN1_LATITUDE_DELTA_MIN))
    {
        LE_ERROR("Maximum latitude delta value is %d, minimum latitude value is %d!",
            ASN1_LATITUDE_DELTA_MAX, ASN1_LATITUDE_DELTA_MIN);
        return LE_BAD_PARAMETER;
    }
    if ((longitudeDeltaN1 > ASN1_LONGITUDE_DELTA_MAX) ||
        (longitudeDeltaN1 < ASN1_LONGITUDE_DELTA_MIN))
    {
        LE_ERROR("Maximum longitude delta value is %d, minimum longitude value is %d!",
            ASN1_LONGITUDE_DELTA_MAX, ASN1_LONGITUDE_DELTA_MIN );
        return LE_BAD_PARAMETER;
    }

    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN1Pres = true;
    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN1.latitudeDelta = latitudeDeltaN1;
    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN1.longitudeDelta = longitudeDeltaN1;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the position Delta N-2 from position set in le_ecall_SetMsdPosition() transmitted by the MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad input parameter
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPositionN2
(
    le_ecall_CallRef_t  ecallRef,   ///< eCall reference
    int32_t     latitudeDeltaN2,    ///< latitude delta from position set in SetMsdPositionN1
                                    ///< 1 Unit = 100 miliarcseconds, which is approximately 3m
                                    ///< maximum value: 511 = 0 0'51.100'' (±1580m)
                                    ///< minimum value: -512 = -0 0'51.200'' (± -1583m)
    int32_t     longitudeDeltaN2    ///< longitude delta from position set in SetMsdPositionN1
                                    ///< 1 Unit = 100 miliarcseconds, which is approximately 3m
                                    ///< maximum value: 511 = 0 0'51.100'' (±1580m)
                                    ///< minimum value: -512 = -0 0'51.200'' (± -1583m)
)

{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    if ((latitudeDeltaN2 > ASN1_LATITUDE_DELTA_MAX) ||
        (latitudeDeltaN2 < ASN1_LATITUDE_DELTA_MIN))
    {
        LE_ERROR("Maximum latitude delta value is %d, minimum latitude value is %d!",
            ASN1_LATITUDE_DELTA_MAX, ASN1_LATITUDE_DELTA_MIN);
        return LE_BAD_PARAMETER;
    }
    if ((longitudeDeltaN2 > ASN1_LONGITUDE_DELTA_MAX) ||
        (longitudeDeltaN2 < ASN1_LONGITUDE_DELTA_MIN))
    {
        LE_ERROR("Maximum longitude delta value is %d, minimum longitude value is %d!",
            ASN1_LONGITUDE_DELTA_MAX, ASN1_LONGITUDE_DELTA_MIN );
        return LE_BAD_PARAMETER;
    }

    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN2Pres = true;
    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN2.latitudeDelta = latitudeDeltaN2;
    eCallPtr->msd.msdMsg.msdStruct.recentVehLocationN2.longitudeDelta = longitudeDeltaN2;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the number of passengers transmitted by the MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdPassengersCount
(
    le_ecall_CallRef_t  ecallRef,   ///< [IN] eCall reference
    uint32_t            paxCount    ///< [IN] number of passengers
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    eCallPtr->msd.msdMsg.msdStruct.numberOfPassengersPres = true;
    eCallPtr->msd.msdMsg.msdStruct.numberOfPassengers = paxCount;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Import an already prepared MSD.
 *
 * MSD is transmitted only after an emergency call has been established.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW The imported MSD length exceeds the MSD_MAX_LEN maximum length.
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note On failure, the process exits; you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ImportMsd
(
    le_ecall_CallRef_t  ecallRef,      ///< [IN] eCall reference
    const uint8_t*      msdPtr,        ///< [IN] Prepared MSD
    size_t              msdNumElements ///< [IN] Prepared MSD size in bytes
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (msdNumElements > sizeof(eCallPtr->builtMsd))
    {
        LE_ERROR("Imported MSD is too long (%zd > %zd)",
                 msdNumElements,
                 sizeof(eCallPtr->builtMsd));
        return LE_OVERFLOW;
    }

    memcpy(eCallPtr->builtMsd, msdPtr, msdNumElements);
    eCallPtr->builtMsdSize = msdNumElements;
    eCallPtr->isMsdImported = true;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send the MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 *
 * @note On failure, the process exits, so you don't have to worry about checking the returned
 *       reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SendMsd
(
    le_ecall_CallRef_t  ecallRef      ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    eCallPtr->msd.msdMsg.msdStruct.messageIdentifier++;

    // Update MSD with msg ID
    if (EncodeMsd(eCallPtr) != LE_OK)
    {
        LE_ERROR("Encode MSD failure (msg ID)");
        return LE_FAULT;
    }

    return pa_ecall_SendMsd(eCallPtr->builtMsd, eCallPtr->builtMsdSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Export the encoded MSD.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW  The encoded MSD length exceeds the user's buffer length.
 *      - LE_NOT_FOUND  No encoded MSD is available.
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *
 * @note If the caller is passing a bad pointer into this function, it's a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ExportMsd
(
    le_ecall_CallRef_t  ecallRef,           ///< [IN] eCall reference
    uint8_t*            msdPtr,             ///< [OUT] Encoded MSD message.
    size_t*             msdNumElementsPtr   ///< [IN,OUT] Ecoded MSD size in bytes
)
{
    if (msdPtr == NULL)
    {
        LE_KILL_CLIENT("msdPtr is NULL.");
        return LE_FAULT;
    }

    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (sizeof(eCallPtr->builtMsd) > *msdNumElementsPtr)
    {
        LE_ERROR("Encoded MSD is too long for your buffer (%zd > %zd)!",
                 sizeof(eCallPtr->builtMsd),
                 *msdNumElementsPtr);
        return LE_OVERFLOW;
    }

    if ((!eCallPtr->isMsdImported) &&
        (eCallPtr->builtMsdSize = msd_EncodeMsdMessage(&eCallPtr->msd,
                                                        eCallPtr->builtMsd)) == LE_FAULT)
    {
        LE_ERROR("Unable to encode the MSD!");
        return LE_NOT_FOUND;
    }

    memcpy(msdPtr, eCallPtr->builtMsd , *msdNumElementsPtr);
    *msdNumElementsPtr = eCallPtr->builtMsdSize;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start an automatic eCall session
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY an eCall session is already in progress
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartAutomatic
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (   (ECallObj.sessionState != ECALL_SESSION_INIT)
        && (ECallObj.sessionState != ECALL_SESSION_STOPPED))
    {
        LE_ERROR("An eCall session is already in progress, sessionState %d", ECallObj.sessionState);
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    // Update eCall session state
    ECallObj.sessionState = ECALL_SESSION_REQUEST;

    eCallPtr->msd.msdMsg.msdStruct.messageIdentifier = 0;
    eCallPtr->msd.msdMsg.msdStruct.timestamp = (uint32_t)time(NULL);
    eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = true;
    eCallPtr->msd.msdMsg.msdStruct.control.testCall = false;

    // Update MSD with msg ID, timestamp and control flags
    if (EncodeMsd(eCallPtr) != LE_OK)
    {
        LE_ERROR("Encode MSD failure (msg ID, timestamp and control flags)");
        return LE_FAULT;
    }

    // Update eCall start type
    ECallObj.startType = PA_ECALL_START_AUTO;

    // Initialize redial state machine
    RedialInit();
    // Start redial period for ERA GLONASS system standard
    if (SystemStandard == PA_ECALL_ERA_GLONASS)
    {
        // Clear redial
        RedialClear();
        // Start redial
        RedialStart();
    }

    if (!ECallObj.isPushed)
    {
        if (le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK)
        {
            LE_WARN("Unable to set the Push mode!");
        }
        else
        {
            ECallObj.isPushed = true;
        }
    }

    return DialAttempt();
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a manual eCall session
 *
 * @return
 *      - LE_OK on success
 *      - LE_BUSY an eCall session is already in progress
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartManual
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (   (ECallObj.sessionState != ECALL_SESSION_INIT)
        && (ECallObj.sessionState != ECALL_SESSION_STOPPED))
    {
        LE_ERROR("An eCall session is already in progress, sessionState %d", ECallObj.sessionState);
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    // Update eCall session state
    ECallObj.sessionState = ECALL_SESSION_REQUEST;

    eCallPtr->msd.msdMsg.msdStruct.messageIdentifier = 0;
    eCallPtr->msd.msdMsg.msdStruct.timestamp = (uint32_t)time(NULL);
    eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
    eCallPtr->msd.msdMsg.msdStruct.control.testCall = false;

    // Update MSD with msg ID, timestamp and control flags
    if (EncodeMsd(eCallPtr) != LE_OK)
    {
        LE_ERROR("Encode MSD failure (msg ID, timestamp and control flags)");
        return LE_FAULT;
    }

    // Update eCall start type
    ECallObj.startType = PA_ECALL_START_MANUAL;

    // Initialize redial state machine
    RedialInit();
    // Start redial period for ERA GLONASS system standard
    if (SystemStandard == PA_ECALL_ERA_GLONASS)
    {
        // Clear redial
        RedialClear();
        // Start redial
        RedialStart();
    }

    if (!ECallObj.isPushed)
    {
        if (le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK)
        {
            LE_WARN("Unable to set the Push mode!");
        }
        else
        {
            ECallObj.isPushed = true;
        }
    }

    return DialAttempt();
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a test eCall session
 *
 * @return
 *       - LE_OK on success
 *       - LE_BUSY an eCall session is already in progress
 *       - LE_BAD_PARAMETER bad eCall reference
 *       - LE_FAULT for other failures
 *       - LE_UNSUPPORTED Not supported on this platform
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_StartTest
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (   (ECallObj.sessionState != ECALL_SESSION_INIT)
        && (ECallObj.sessionState != ECALL_SESSION_STOPPED))
    {
        LE_ERROR("An eCall session is already in progress, sessionState %d", ECallObj.sessionState);
        return LE_BUSY;
    }

    // Hang up all the ongoing calls using the communication channel required for eCall
    if (le_mcc_HangUpAll() != LE_OK)
    {
        LE_ERROR("Hang up ongoing call(s) failed");
        return LE_FAULT;
    }

    // Update eCall session state
    ECallObj.sessionState = ECALL_SESSION_REQUEST;

    eCallPtr->msd.msdMsg.msdStruct.messageIdentifier = 0;
    eCallPtr->msd.msdMsg.msdStruct.timestamp = (uint32_t)time(NULL);
    eCallPtr->msd.msdMsg.msdStruct.control.automaticActivation = false;
    eCallPtr->msd.msdMsg.msdStruct.control.testCall = true;

    // Update MSD with msg ID, timestamp and control flags
    if (EncodeMsd(eCallPtr) != LE_OK)
    {
        LE_ERROR("Encode MSD failure (msg ID, timestamp and control flags)");
        return LE_FAULT;
    }

    // Update eCall start type
    ECallObj.startType = PA_ECALL_START_TEST;

    // Initialize redial state machine
    RedialInit();
    // Start redial period for ERA GLONASS system standard
    if (SystemStandard == PA_ECALL_ERA_GLONASS)
    {
        // Clear redial
        RedialClear();
        // Start redial
        RedialStart();
    }

    if (!ECallObj.isPushed)
    {
        if (le_ecall_SetMsdTxMode(LE_ECALL_TX_MODE_PUSH) != LE_OK)
        {
            LE_WARN("Unable to set the Push mode!");
        }
        else
        {
            ECallObj.isPushed = true;
        }
    }

    return DialAttempt();
}

//--------------------------------------------------------------------------------------------------
/**
 * End the current eCall session
 *
 * @note
 * - During an automatic eCall session, it is not possible to end the ongoing session until
 *   LE_ECALL_STATE_STOPPED event is notified.
 * - For manual session, it is possible to end the ongoing session if the eCall is not connected
 *   (LE_ECALL_STATE_CONNECTED not yet received) and after LE_ECALL_STATE_STOPPED event is notified.
 * - Using this API doesn't affect the NAD deregistration mechanism. However, it disables the call
 *   auto answer function if already running.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT for other failures
 *      - LE_UNSUPPORTED Not supported on this platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_End
(
    le_ecall_CallRef_t    ecallRef   ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    le_result_t result;

    if (NULL == eCallPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    // Incoming auto/manual ecall in progress
    if (ECallObj.isMtCall)
    {
        if (ECallObj.sendMsdSignalReceived &&
            (PA_ECALL_START_TEST != eCallPtr->startType) &&
            ((ECALL_SESSION_CONNECTED == eCallPtr->sessionState) ||
            (ECALL_SESSION_COMPLETED == eCallPtr->sessionState)))
        {
            LE_ERROR("Incoming Ecall cannot be terminated, Ecall in progress");
            return LE_FAULT;
        }
    }
    else
    {
        // Automatic eCall session:
        // don't end current eCall session unless ECALL_SESSION_STOPPED state is reached.
        if ((PA_ECALL_START_AUTO == eCallPtr->startType) &&
            (ECALL_SESSION_STOPPED != eCallPtr->sessionState))
        {
            LE_ERROR("An automatic Ecall cannot be terminated");
            return LE_FAULT;
        }

        // Manual eCall session:
        // don't end current eCall session if eCall session state is ECALL_SESSION_CONNECTED or
        // ECALL_SESSION_COMPLETED.
        if ((PA_ECALL_START_MANUAL == eCallPtr->startType) &&
            ((ECALL_SESSION_CONNECTED == eCallPtr->sessionState) ||
             (ECALL_SESSION_COMPLETED == eCallPtr->sessionState)))
        {
            LE_ERROR("Ecall transaction cannot be terminated, Ecall in progress");
            return LE_FAULT;
        }
    }

    // Invalidate MSD
    InvalidateMsd();

    result = pa_ecall_End();

    // Stop redial
    if (LE_OK == result)
    {
        // Update eCall session state
        ECallObj.sessionState = ECALL_SESSION_STOPPED;
        // Stop redial
        RedialStop(ECALL_REDIAL_STOP_FROM_USER);
    }

    return (result);
}

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
    le_ecall_CallRef_t      ecallRef
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_ECALL_STATE_UNKNOWN;
    }

    return (eCallPtr->state);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler ADD function
 */
//--------------------------------------------------------------------------------------------------
le_ecall_StateChangeHandlerRef_t le_ecall_AddStateChangeHandler
(
    le_ecall_StateChangeHandlerFunc_t handlerPtr, ///< [IN] Handler function
    void*                             contextPtr      ///< [IN] Context pointer
)
{
    le_event_HandlerRef_t        handlerRef;

    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("ECallStateChangeHandler",
                                            ECallEventStateId,
                                            FirstLayerECallStateChangeHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_ecall_StateChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * le_ecall_StateChangeHandler handler REMOVE function
 */
//--------------------------------------------------------------------------------------------------
void le_ecall_RemoveStateChangeHandler
(
    le_ecall_StateChangeHandlerRef_t addHandlerRef ///< [IN] The handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Public Safely Answering Point telephone number.
 *
 * @note That PSAP number is not applied to Manually or Automatically initiated eCall. For those
 *       modes, an emergency call is launched.
 *
 * @warning This function doesn't modify the U/SIM content.
 *
 * @return
 *  - LE_OK           On success
 *  - LE_FAULT        For other failures
 *  - LE_UNSUPPORTED  Not supported on this platform
 *
 * @note If PSAP number is empty or too long (max LE_MDMDEFS_PHONE_NUM_MAX_LEN digits), it is a
 *       fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetPsapNumber
(
    const char* psapPtr    ///< [IN] Public Safely Answering Point number
)
{
    size_t length;

    if (psapPtr == NULL)
    {
        LE_KILL_CLIENT("Null argument provided");
        return LE_FAULT;
    }

    length = strlen(psapPtr);

    if ((length == 0) || (length > LE_MDMDEFS_PHONE_NUM_MAX_LEN))
    {
        LE_KILL_CLIENT( "PSAP number length (%zu) should not be empty or greater than %d",
                         length,
                         LE_MDMDEFS_PHONE_NUM_MAX_LEN);
        return LE_FAULT;
    }

    if(le_utf8_Copy(ECallObj.psapNumber, psapPtr, sizeof(ECallObj.psapNumber), NULL) != LE_OK)
    {
        return LE_FAULT;
    }

    return (pa_ecall_SetPsapNumber(ECallObj.psapNumber));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Public Safely Answering Point telephone number set with le_ecall_SetPsapNumber()
 * function.
 *
 * @note That PSAP number is not applied to Manually or Automatically initiated eCall. For those
 *       modes, an emergency call is launched.
 *
 * @warning This function doesn't read the U/SIM content.
 *
 * @return
 *  - LE_OK           On success
 *  - LE_FAULT        On failures or if le_ecall_SetPsapNumber() has never been called before
  * - LE_OVERFLOW     Retrieved PSAP number is too long for the out parameter
 *  - LE_UNSUPPORTED  Not supported on this platform
 *
 * @note If the passed PSAP pointer is NULL, a fatal error is raised and the function will not
 *       return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetPsapNumber
(
    char   *psapPtr,           ///< [OUT] Public Safely Answering Point number
    size_t  len                ///< [IN] maximum length of PSAP number.
)
{
    if (psapPtr == NULL)
    {
        LE_KILL_CLIENT("psapPtr is NULL !");
        return LE_FAULT;
    }

    return (pa_ecall_GetPsapNumber(psapPtr, len));
}


//--------------------------------------------------------------------------------------------------
/**
 * When modem is in ECALL_FORCED_PERSISTENT_ONLY_MODE or ECALL_ONLY_MODE, this function
 * can be called to request the modem to read the number to dial from the FDN/SDN of the U/SIM.
 *
 * @note If FDN directory is updated with new dial numbers, be sure that the SIM card is refreshed.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_UseUSimNumbers
(
    void
)
{
    return (pa_ecall_UseUSimNumbers());
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the NAD (network access device) deregistration time value. After termination of an emergency
 * call the in-vehicle system remains registered on the network for the period of time, defined by
 * the installation parameter NAD (network access device) deregistration time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *
 * @note The formula to calculate NAD deregistration time for PAN_EUROPEAN is as below:
 *       ECallConfiguration.nad_deregistration_time = (deregTime+59)/60;
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetNadDeregistrationTime
(
    uint16_t deregTime ///< [IN] NAD (network access device) deregistration time value (in minutes).
)
{
    ECallObj.eraGlonass.nadDeregistrationTime = deregTime;
    return pa_ecall_SetNadDeregistrationTime(deregTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the NAD (network access device) deregistration time value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetNadDeregistrationTime
(
    uint16_t* deregTimePtr ///< [OUT] NAD (network access device) deregistration time value
                           /// (in minutes).
)
{
    le_result_t result;

    result = pa_ecall_GetNadDeregistrationTime(deregTimePtr);
    if(result == LE_OK)
    {
        // Update eCall Context value
        ECallObj.eraGlonass.nadDeregistrationTime = *deregTimePtr;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the push/pull transmission mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdTxMode
(
    le_ecall_MsdTxMode_t mode   ///< [IN] Transmission mode
)
{
    LE_DEBUG("set MsdTx %d", mode);
    return (pa_ecall_SetMsdTxMode(mode));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the push/pull transmission mode.
 *
 * @return
 *      - LE_OK     on success
 *      - LE_FAULT  for other failures
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetMsdTxMode
(
    le_ecall_MsdTxMode_t* modePtr   ///< [OUT] Transmission mode
)
{
    if (NULL == modePtr)
    {
        LE_KILL_CLIENT("modePtr is NULL!");
        return LE_FAULT;
    }

    return (pa_ecall_GetMsdTxMode(modePtr));
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the minimum interval value between dial attempts. Available for both manual and test modes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetIntervalBetweenDialAttempts
(
    uint16_t    pause   ///< [IN] the minimum interval value in seconds
)
{
    ECallObj.redial.intervalBetweenAttempts = pause;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the minimum interval value between dial attempts.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetIntervalBetweenDialAttempts
(
     uint16_t*    pausePtr   ///< [OUT] minimum interval value in seconds
)
{
    if (pausePtr == NULL)
    {
        LE_ERROR("pausePtr is NULL !");
        return LE_FAULT;
    }

    *pausePtr = ECallObj.redial.intervalBetweenAttempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_MANUAL_DIAL_ATTEMPTS value. If a dial attempt under manual emergency call
 * initiation failed, it should be repeated maximally ECALL_MANUAL_DIAL_ATTEMPTS-1 times within
 * the maximal time limit of ECALL_DIAL_DURATION. The default value is 10.
 * Redial attempts stop once the call has been cleared down correctly, or if counter/timer reached
 * their limits. Available for both manual and test modes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassManualDialAttempts
(
    uint16_t    attempts  ///< [IN] ECALL_MANUAL_DIAL_ATTEMPTS value
)
{
    ECallObj.eraGlonass.manualDialAttempts = attempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set tthe ECALL_AUTO_DIAL_ATTEMPTS value. If a dial attempt under automatic emergency call
 * initiation failed, it should be repeated maximally ECALL_AUTO_DIAL_ATTEMPTS-1 times within
 * the maximal time limit of ECALL_DIAL_DURATION. The default value is 10.
 * Redial attempts stop once the call has been cleared down correctly, or if counter/timer reached
 * their limits.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassAutoDialAttempts
(
    uint16_t    attempts  ///< [IN] ECALL_AUTO_DIAL_ATTEMPTS value
)
{
    ECallObj.eraGlonass.autoDialAttempts = attempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_DIAL_DURATION time. It's the maximum time the IVS have to connect the emergency
 * call to the PSAP, including all redial attempts.
 * If the call is not connected within this time (or ManualDialAttempts/AutoDialAttempts dial
 * attempts), it will stop.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassDialDuration
(
    uint16_t    duration   ///< [IN] ECALL_DIAL_DURATION time value (in seconds)
)
{
    le_clk_Time_t interval;
    interval.sec = duration;
    interval.usec = 0;

    ECallObj.eraGlonass.dialDuration = duration;
    LE_DEBUG("Set ERA-GLONASS dial duration timer %p to %d seconds",
              ECallObj.redial.dialDurationTimer, (int)duration);

    if (le_timer_SetInterval(ECallObj.redial.dialDurationTimer, interval) != LE_OK)
    {
        LE_ERROR("Cannot start the DialDuration timer!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_CCFT time which is the maximum delay before initiating an automatic call
 * termination. When the delay is reached and IVS NAD didn't receive a call clear-down indication
 * is immediatly terminated.
 *
 * @note Allowed range of values is 1 to 720 minutes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassFallbackTime
(
    uint16_t    duration   ///< [IN] ECALL_CCFT time value (in minutes)
)
{
    if ((duration < ERA_GLONASS_CCFT_MIN) || (duration > ERA_GLONASS_CCFT_MAX))
    {
        LE_ERROR("Allowed range of values for CCFT timer is %d to %d minutes. %d given",
                 ERA_GLONASS_CCFT_MIN, ERA_GLONASS_CCFT_MAX, duration);
        return LE_FAULT;
    }

    return pa_ecall_SetEraGlonassFallbackTime(duration);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_AUTO_ANSWER_TIME time. It is a time interval wherein IVDS responds to incoming
 * calls automatically after emergency call completion.
 *
 * @note Default value of auto answer time is 20 minutes. Maximum value is 720 minutes.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassAutoAnswerTime
(
    uint16_t autoAnswerTime ///< [IN] ECALL_AUTO_ANSWER_TIME time value (in minutes)
)
{
    if (autoAnswerTime > ERA_GLONASS_AUTOANSTIME_MAX)
    {
        LE_ERROR("Maximum value for auto answer time is %d minutes", ERA_GLONASS_AUTOANSTIME_MAX);
        return LE_FAULT;
    }

    return pa_ecall_SetEraGlonassAutoAnswerTime(autoAnswerTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ECALL_MSD_MAX_TRANSMISSION_TIME time. It is a time period for MSD transmission.
 *
 * @note Default value of MSD transmission time is 20 seconds.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassMSDMaxTransmissionTime
(
    uint16_t msdMaxTransTime ///< [IN] ECALL_MSD_MAX_TRANSMISSION_TIME time value (in seconds)
)
{
    return pa_ecall_SetEraGlonassMSDMaxTransmissionTime(msdMaxTransTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS ECALL_POST_TEST_REGISTRATION_TIME time.
 *
 * After completion of transmission of test diagnostics results in a test eCall session, the
 * in-vehicle system remains registered on the network for the period of time defined by the
 * ECALL_POST_TEST_REGISTRATION_TIME value.
 *
 * @note The ECALL_POST_TEST_REGISTRATION_TIME setting takes effect immediately and is persistent to
 * reset.
 *
 * @note An ECALL_POST_TEST_REGISTRATION_TIME value of zero means the IVS doesn't remain registered
 * after completion of transmission of test (diagnostics) results.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetEraGlonassPostTestRegistrationTime
(
    uint16_t postTestRegTime ///< [IN] ECALL_POST_TEST_REGISTRATION_TIME time value (in seconds).
)
{
    ECallObj.eraGlonass.postTestRegistrationTime = postTestRegTime;
    return pa_ecall_SetEraGlonassPostTestRegistrationTime(postTestRegTime);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_MANUAL_DIAL_ATTEMPTS value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassManualDialAttempts
(
    uint16_t*    attemptsPtr  ///< [OUT] ECALL_MANUAL_DIAL_ATTEMPTS value
)
{
    if (attemptsPtr == NULL)
    {
        LE_KILL_CLIENT("attemptsPtr is NULL !");
        return LE_FAULT;
    }

    LE_FATAL_IF(ECallObj.ref == NULL, "eCall object was not initialized properly.");

    *attemptsPtr = ECallObj.eraGlonass.manualDialAttempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_AUTO_DIAL_ATTEMPTS value.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassAutoDialAttempts
(
    uint16_t*    attemptsPtr  ///< [OUT] ECALL_AUTO_DIAL_ATTEMPTS value
)
{
    if (attemptsPtr == NULL)
    {
        LE_KILL_CLIENT("attemptsPtr is NULL !");
        return LE_FAULT;
    }

    LE_FATAL_IF(ECallObj.ref == NULL, "eCall object was not initialized properly.");

    *attemptsPtr = ECallObj.eraGlonass.autoDialAttempts;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_DIAL_DURATION time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassDialDuration
(
    uint16_t*    durationPtr  ///< [OUT] ECALL_DIAL_DURATION time value (in seconds)
)
{
    if (durationPtr == NULL)
    {
        LE_KILL_CLIENT("durationPtr is NULL !");
        return LE_FAULT;
    }

    LE_FATAL_IF(ECallObj.ref == NULL, "eCall object was not initialized properly.");

    *durationPtr = ECallObj.eraGlonass.dialDuration;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_CCFT time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on execution failure
 *
 * @warning Introducing a NULL pointer as argument of this function leads to a client kill.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassFallbackTime
(
    uint16_t*    durationPtr  ///< [OUT] ECALL_CCFT time value (in minutes)
)
{
    if (durationPtr == NULL)
    {
        LE_KILL_CLIENT("durationPtr is NULL !");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassFallbackTime(durationPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_AUTO_ANSWER_TIME time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on execution failure
 *
 * @warning Introducing a NULL pointer as argument of this function leads to a client kill.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassAutoAnswerTime
(
    uint16_t* autoAnswerTimePtr ///< [OUT] ECALL_AUTO_ANSWER_TIME time value (in minutes)
)
{
    if (NULL == autoAnswerTimePtr)
    {
        LE_KILL_CLIENT("autoAnswerTimePtr is NULL!");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassAutoAnswerTime(autoAnswerTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ECALL_MSD_MAX_TRANSMISSION_TIME time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassMSDMaxTransmissionTime
(
    uint16_t* msdMaxTransTimePtr ///< [OUT] ECALL_MSD_MAX_TRANSMISSION_TIME time value (in seconds)
)
{
    if (NULL == msdMaxTransTimePtr)
    {
        LE_KILL_CLIENT("msdMaxTransTimePtr is NULL!");
        return LE_FAULT;
    }

    return pa_ecall_GetEraGlonassMSDMaxTransmissionTime(msdMaxTransTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the ERA-GLONASS ECALL_POST_TEST_REGISTRATION_TIME time.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED if the function is not supported by the target
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetEraGlonassPostTestRegistrationTime
(
    uint16_t* postTestRegTimePtr  ///< [OUT] ECALL_POST_TEST_REGISTRATION_TIME time value (in
                                  ///< seconds).
)
{
    if (NULL == postTestRegTimePtr)
    {
        LE_ERROR("postTestRegTimePtr is NULL!");
        return LE_FAULT;
    }

    le_result_t result = pa_ecall_GetEraGlonassPostTestRegistrationTime(postTestRegTimePtr);
    if (LE_OK == result)
    {
        // Update eCall Context value
        ECallObj.eraGlonass.postTestRegistrationTime = *postTestRegTimePtr;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS crash severity parameter
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdEraGlonassCrashSeverity
(
    le_ecall_CallRef_t  ecallRef,       ///< [IN] eCall reference
    uint32_t            crashSeverity   ///< [IN] ERA-GLONASS crash severity parameter
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCrashSeverity = true;
    EraGlonassDataObj.crashSeverity = crashSeverity;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS crash severity parameter. Optional parameter is not included
 * in the MSD message.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassCrashSeverity
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference

)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCrashSeverity = false;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS diagnostic result using a bit mask.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------

le_result_t le_ecall_SetMsdEraGlonassDiagnosticResult
(
    le_ecall_CallRef_t                 ecallRef,             ///< [IN] eCall reference
    le_ecall_DiagnosticResultBitMask_t diagnosticResultMask  ///< ERA-GLONASS diagnostic
                                                             ///< result bit mask.
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("DiagnosticResult mask 0x%016"PRIX64, (uint64_t)diagnosticResultMask);
    EraGlonassDataObj.presentDiagnosticResult = true;

    EraGlonassDataObj.diagnosticResult.presentMicConnectionFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_MIC_CONNECTION_FAILURE);
    EraGlonassDataObj.diagnosticResult.micConnectionFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_MIC_CONNECTION_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentMicFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_MIC_FAILURE);
    EraGlonassDataObj.diagnosticResult.micFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_MIC_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentRightSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_RIGHT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.rightSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_RIGHT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentLeftSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_LEFT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.leftSpeakerFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_LEFT_SPEAKER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentSpeakersFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_SPEAKERS_FAILURE);
    EraGlonassDataObj.diagnosticResult.speakersFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_SPEAKERS_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentIgnitionLineFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_IGNITION_LINE_FAILURE);
    EraGlonassDataObj.diagnosticResult.ignitionLineFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_IGNITION_LINE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentUimFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_UIM_FAILURE);
    EraGlonassDataObj.diagnosticResult.uimFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_UIM_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentStatusIndicatorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_STATUS_INDICATOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.statusIndicatorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_STATUS_INDICATOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentBatteryFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_BATTERY_FAILURE);
    EraGlonassDataObj.diagnosticResult.batteryFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_BATTERY_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentBatteryVoltageLow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_BATTERY_VOLTAGE_LOW);
    EraGlonassDataObj.diagnosticResult.batteryVoltageLow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_BATTERY_VOLTAGE_LOW);
    EraGlonassDataObj.diagnosticResult.presentCrashSensorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_CRASH_SENSOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.crashSensorFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_CRASH_SENSOR_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentFirmwareImageCorruption =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_FIRMWARE_IMAGE_CORRUPTION);
    EraGlonassDataObj.diagnosticResult.firmwareImageCorruption =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_FIRMWARE_IMAGE_CORRUPTION);
    EraGlonassDataObj.diagnosticResult.presentCommModuleInterfaceFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_COMM_MODULE_INTERFACE_FAILURE);
    EraGlonassDataObj.diagnosticResult.commModuleInterfaceFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_COMM_MODULE_INTERFACE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentGnssReceiverFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_GNSS_RECEIVER_FAILURE);
    EraGlonassDataObj.diagnosticResult.gnssReceiverFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_GNSS_RECEIVER_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentRaimProblem =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_RAIM_PROBLEM);
    EraGlonassDataObj.diagnosticResult.raimProblem =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_RAIM_PROBLEM);
    EraGlonassDataObj.diagnosticResult.presentGnssAntennaFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_GNSS_ANTENNA_FAILURE);
    EraGlonassDataObj.diagnosticResult.gnssAntennaFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_GNSS_ANTENNA_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentCommModuleFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_COMM_MODULE_FAILURE);
    EraGlonassDataObj.diagnosticResult.commModuleFailure =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_COMM_MODULE_FAILURE);
    EraGlonassDataObj.diagnosticResult.presentEventsMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_EVENTS_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.eventsMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_EVENTS_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.presentCrashProfileMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_CRASH_PROFILE_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.crashProfileMemoryOverflow =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_CRASH_PROFILE_MEMORY_OVERFLOW);
    EraGlonassDataObj.diagnosticResult.presentOtherCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_OTHER_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.otherCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_OTHER_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.presentOtherNotCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_PRESENT_OTHER_NOT_CRITICAL_FAILURES);
    EraGlonassDataObj.diagnosticResult.otherNotCriticalFailures =
        GET_BIT_MASK_VALUE(diagnosticResultMask
                           ,LE_ECALL_DIAG_RESULT_OTHER_NOT_CRITICAL_FAILURES);

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS diagnostic result bit mask. Therefore that optional parameter is
 * not included in the MSD message.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassDiagnosticResult
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("DiagnosticResult mask reseted");
    EraGlonassDataObj.presentDiagnosticResult = false;

    return EncodeMsd(eCallPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS crash type bit mask
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdEraGlonassCrashInfo
(
    le_ecall_CallRef_t          ecallRef,       ///< [IN] eCall reference
    le_ecall_CrashInfoBitMask_t crashInfoMask   ///< ERA-GLONASS crash type bit mask.
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("CrashInfo mask %08X", (uint16_t)crashInfoMask);

    EraGlonassDataObj.presentCrashInfo = true;

    EraGlonassDataObj.crashType.presentCrashFront =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT);
    EraGlonassDataObj.crashType.crashFront =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_FRONT);
    EraGlonassDataObj.crashType.presentCrashLeft =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_LEFT);
    EraGlonassDataObj.crashType.crashLeft =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_LEFT);
    EraGlonassDataObj.crashType.presentCrashRight =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_RIGHT);
    EraGlonassDataObj.crashType.crashRight =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_RIGHT);
    EraGlonassDataObj.crashType.presentCrashRear =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_REAR);
    EraGlonassDataObj.crashType.crashRear =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_REAR);
    EraGlonassDataObj.crashType.presentCrashRollover =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_ROLLOVER);
    EraGlonassDataObj.crashType.crashRollover =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_ROLLOVER);
    EraGlonassDataObj.crashType.presentCrashSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_SIDE);
    EraGlonassDataObj.crashType.crashSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_SIDE);
    EraGlonassDataObj.crashType.presentCrashFrontOrSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_FRONT_OR_SIDE);
    EraGlonassDataObj.crashType.crashFrontOrSide =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_FRONT_OR_SIDE);
    EraGlonassDataObj.crashType.presentCrashAnotherType =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_PRESENT_CRASH_ANOTHER_TYPE);
    EraGlonassDataObj.crashType.crashAnotherType =
        GET_BIT_MASK_VALUE(crashInfoMask, LE_ECALL_CRASH_INFO_CRASH_ANOTHER_TYPE);

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS crash type bit mask. Optional parameter is not included
 * in the MSD message.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassCrashInfo
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);
    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    LE_DEBUG("CrashInfo mask reseted");

    EraGlonassDataObj.presentCrashInfo = false;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the ERA-GLONASS coordinate system type
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdEraGlonassCoordinateSystemType
(
    le_ecall_CallRef_t           ecallRef,        ///< [IN] eCall reference
    le_ecall_MsdCoordinateType_t coordinateType   ///< [IN] ERA-GLONASS coordinate system type
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (NULL == eCallPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCoordinateSystemTypeInfo = true;
    EraGlonassDataObj.coordinateSystemType = CoordinateSystemTypeEnumToEnumAsn1(coordinateType);

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the ERA-GLONASS coordinate system type. Optional parameter is not included
 * in the MSD message.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE an MSD has been already imported
 *      - LE_BAD_PARAMETER bad eCall reference
 *      - LE_FAULT on other failures
 *
 * @note The process exits, if an invalid eCall reference is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_ResetMsdEraGlonassCoordinateSystemType
(
    le_ecall_CallRef_t   ecallRef       ///< [IN] eCall reference
)
{
    ECall_t*   eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (NULL == eCallPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_BAD_PARAMETER;
    }

    if (eCallPtr->isMsdImported)
    {
        LE_ERROR("An MSD has been already imported!");
        return LE_DUPLICATE;
    }

    EraGlonassDataObj.presentCoordinateSystemTypeInfo = false;
    EraGlonassDataObj.coordinateSystemType = MSD_COORDINATE_SYSTEM_TYPE_ABSENT;

    return EncodeMsd(eCallPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the termination reason.
 *
 * @return The termination reason.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_mcc_TerminationReason_t le_ecall_GetTerminationReason
(
    le_ecall_CallRef_t ecallRef
        ///< [IN]
        ///< eCall reference.
)
{
    ECall_t* eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_MCC_TERM_UNDEFINED;
    }

    return eCallPtr->lastTermination;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific termination code.
 *
 * @return The platform specific termination code.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_ecall_GetPlatformSpecificTerminationCode
(
    le_ecall_CallRef_t ecallRef
        ///< [IN]
        ///< eCall reference.
)
{
    ECall_t* eCallPtr = le_ref_Lookup(ECallRefMap, ecallRef);

    if (eCallPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", ecallRef);
        return LE_MCC_TERM_UNDEFINED;
    }

    return eCallPtr->lastSpecificTerm;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the system standard.
 * Default is PAN EUROPEAN
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetSystemStandard
(
    le_ecall_SystemStandard_t systemStandard
        ///< [IN]
        ///< System mode
)
{
    le_cfg_IteratorRef_t iteratorRef;
    const char *standard = "\0";
    pa_ecall_SysStd_t system;

    if ( (LE_ECALL_PAN_EUROPEAN != systemStandard) && (LE_ECALL_ERA_GLONASS != systemStandard) )
    {
        LE_ERROR("parameter %d has invalid value", systemStandard);
        return LE_FAULT;
    }

    switch (systemStandard)
    {
        case LE_ECALL_PAN_EUROPEAN:
            standard = "PAN-EUROPEAN";
            system = PA_ECALL_PAN_EUROPEAN;
            break;
        case LE_ECALL_ERA_GLONASS:
            standard = "ERA-GLONASS";
            system = PA_ECALL_ERA_GLONASS;
            break;
        default: break;
    }

    if ( LE_OK != pa_ecall_UpdateSystemStandard(system))
    {
       LE_INFO("Update PA system standard (%d) failed!", system);
    }

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_ECALL_PATH);
    le_cfg_SetString(iteratorRef, CFG_NODE_SYSTEM_STD, standard);
    le_cfg_CommitTxn(iteratorRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the system standard.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetSystemStandard
(
    le_ecall_SystemStandard_t* systemStandardPtr
        ///< [OUT] System mode
)
{
    le_cfg_IteratorRef_t iteratorRef = NULL;
    char systemModeString[20];

    if (NULL == systemStandardPtr)
    {
        LE_KILL_CLIENT("systemStandardPtr is NULL.");
        return LE_FAULT;
    }

    iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (!le_cfg_NodeExists(iteratorRef, CFG_NODE_SYSTEM_STD))
    {
        LE_WARN("Node 'systemStandard' does not exist. Default to PAN EU");
        *systemStandardPtr = LE_ECALL_PAN_EUROPEAN;
        le_cfg_CancelTxn( iteratorRef );
        return LE_OK;
    }

    if (LE_OVERFLOW == le_cfg_GetString(iteratorRef, CFG_NODE_SYSTEM_STD,
        systemModeString, sizeof(systemModeString), ""))
    {
        LE_WARN("Node '"CFG_NODE_SYSTEM_STD"' exist, but is too long. Default to PAN EU. :%s",
            systemModeString);

        *systemStandardPtr = LE_ECALL_PAN_EUROPEAN;
        le_cfg_CancelTxn( iteratorRef );
        return LE_OK;
    }

    if (0 == strncmp( "PAN-EUROPEAN", systemModeString, 12))
    {
        *systemStandardPtr = LE_ECALL_PAN_EUROPEAN;
    }
    else if (0 == strncmp( "ERA-GLONASS", systemModeString, 11))
    {
        *systemStandardPtr = LE_ECALL_ERA_GLONASS;
    }
    else
    {
        LE_WARN("Node '"CFG_NODE_SYSTEM_STD"' exist, but does not match:%s. Default to PAN EU.",
                systemModeString);
        *systemStandardPtr = LE_ECALL_PAN_EUROPEAN;
    }

    le_cfg_CancelTxn( iteratorRef );
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the MSDs version and store it in the config tree
 * Default value is 1
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetMsdVersion
(
    uint32_t msdVersion
        ///< [IN] Msd version
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( CFG_MODEMSERVICE_ECALL_PATH );

    le_cfg_SetInt(iteratorRef, CFG_NODE_MSDVERSION, msdVersion);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Set MsdVersion to %d", msdVersion);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the MSD version stored in the config tree.
 * @note that if the value is not correct in the config tree, it will default to 1.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER if inparameter is NULL
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetMsdVersion
(
    uint32_t* msdVersionPtr
        ///< [OUT] Msd version
)
{
    le_cfg_IteratorRef_t iteratorRef = NULL;

    if (NULL == msdVersionPtr)
    {
        LE_KILL_CLIENT("msdVersionPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_MSDVERSION))
    {
        *msdVersionPtr = le_cfg_GetInt(iteratorRef, CFG_NODE_MSDVERSION, 0);
        LE_DEBUG("MSD version read as %d", *msdVersionPtr);
        if (*msdVersionPtr == 0)
        {
            LE_WARN("No correct value set for '%s' ! Use the default one (%d)",
                    CFG_NODE_MSDVERSION,
                    DEFAULT_MSD_VERSION);

            *msdVersionPtr = DEFAULT_MSD_VERSION;
        }
    }
    else
    {
        LE_WARN("No config tree value set for '%s' ! Use the default one (%d)",
                CFG_NODE_MSDVERSION,
                DEFAULT_MSD_VERSION);
        *msdVersionPtr = DEFAULT_MSD_VERSION;
    }

    le_cfg_CancelTxn( iteratorRef );

    return LE_OK;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the Vehicled Type and store it in the config tree
 * Default value is 0
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetVehicleType
(
    le_ecall_MsdVehicleType_t vehicleType
        ///< [IN] Vehicle type
)
{
    le_result_t result;
    le_cfg_IteratorRef_t iteratorRef;
    char vehStr[VEHICLE_TYPE_MAX_BYTES];

    memset(vehStr, 0, VEHICLE_TYPE_MAX_BYTES);

    result = VehicleTypeEnumToString(vehicleType, vehStr);
    if (LE_FAULT == result)
    {
        LE_WARN("No value set for '%s' !", CFG_NODE_VEH);
        return LE_FAULT;
    }

    LE_DEBUG("VehicleTypeEnumToString %d -> '%s' !", vehicleType, vehStr);
    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_ECALL_PATH);
    le_cfg_SetString(iteratorRef, CFG_NODE_VEH, vehStr);
    le_cfg_CommitTxn(iteratorRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Vehicled Type stored in the config tree.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER parameter is NULL
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetVehicleType
(
    le_ecall_MsdVehicleType_t* vehicleTypePtr
        ///< [OUT] Vehicle type
)
{
    le_result_t result = LE_FAULT;
    le_cfg_IteratorRef_t iteratorRef = NULL;
    char  vehStr[VEHICLE_TYPE_MAX_BYTES] = {0};

    if (NULL == vehicleTypePtr)
    {
        LE_KILL_CLIENT("vehicleTypePtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    iteratorRef = le_cfg_CreateReadTxn( CFG_MODEMSERVICE_ECALL_PATH );

    // Get vehicle type
    if (le_cfg_NodeExists(iteratorRef, CFG_NODE_VEH))
    {
        if (le_cfg_GetString(iteratorRef, CFG_NODE_VEH, vehStr, sizeof(vehStr), "") != LE_OK)
        {
            LE_WARN("No node value set for '%s'", CFG_NODE_VEH);
        }
        else if (strlen(vehStr) > 0)
        {
            le_ecall_MsdVehicleType_t vehicleType;
            LE_DEBUG("vehicle type is %s", vehStr);
            if (VehicleTypeStringToEnum(vehStr, &vehicleType) == LE_OK)
            {
                *vehicleTypePtr = vehicleType;
                result = LE_OK;
            }
            else
            {
                LE_WARN("Bad vehicle type. No match found in vehicletype");
                result = LE_FAULT;
            }
        }
    }
    else
    {
        LE_WARN("No value set for '%s' !", CFG_NODE_VEH);
        result = LE_FAULT;

    }

    le_cfg_CancelTxn( iteratorRef );

    LE_DEBUG("vehicle type is %s", vehStr);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * The Vehicle Identification Number is defined by iso 3833 as a 17 character
 * alphanumeric code, which does not include the letters i, I, o, O, q, Q. Also
 * the letters u, U, z, Z and the digit 0 are not allowed in the model year code
 *
 */
//--------------------------------------------------------------------------------------------------
static int VerifyVIN
(
    char *vin
)
{
    int ret = 0;
    char c;

    c = (char)tolower(vin[9]);

    if ( ('0' == c) || ('u' == c) || ('z' == c) )
    {
        LE_WARN("Year digit cannot be %c", vin[9]);
        ret = -1;
    }

    while ( (*vin) && (!ret) )
    {
        c= (char)tolower(*vin);
        if ( ('i' == c) || ('o' == c) || ('q' == c) )
        {
            LE_WARN("%c not allowed in VIN", *vin);
            ret = -1;
        }

        vin++;
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the VIN (Vehicle Identification Number)
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER parameter is NULL.
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetVIN
(
    const char* vin ///< [IN] VIN (Vehicle Identification Number)
)
{
    le_cfg_IteratorRef_t iterator;

    if (strlen(vin) != LE_ECALL_VIN_MAX_LEN)
    {
        LE_WARN("VIN has to be %d bytes long: %s -> %zd",
            LE_ECALL_VIN_MAX_LEN, vin, strlen(vin));
        return LE_FAULT;
    }

    if (VerifyVIN((char *)vin))
    {
        return LE_FAULT;
    }

    iterator = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_ECALL_PATH);

    le_cfg_SetString(iterator, CFG_NODE_VIN, vin);

    le_cfg_CommitTxn(iterator);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the VIN (Vehicle Identification Number)
 *
 * @return
 *  - LE_OK on success
 *  - LE_NOT_FOUND if the value is not set.
 *  - LE_BAD_PARAMETER parameter is NULL or too small
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetVIN
(
    char* vin,              ///< [OUT] VIN size is 18 chars
    size_t vinNumElements   ///< [IN]
)
{
    le_cfg_IteratorRef_t iterator;

    if (!vin)
    {
        LE_KILL_CLIENT("vin is NULL !");
        return LE_BAD_PARAMETER;
    }

    if (vinNumElements != LE_ECALL_VIN_MAX_BYTES)
    {
        LE_WARN("VIN has to be at least %d bytes long", LE_ECALL_VIN_MAX_BYTES);
        return LE_BAD_PARAMETER;
    }

    iterator = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_ECALL_PATH);

    // Get VIN
    if (!le_cfg_NodeExists(iterator, CFG_NODE_VIN))
    {
        LE_WARN("No node value set for '%s'", CFG_NODE_VIN);

        le_cfg_CancelTxn(iterator);

        return LE_NOT_FOUND;
    }

    memset(vin, 0, LE_ECALL_VIN_MAX_BYTES);

    le_cfg_GetString(iterator, CFG_NODE_VIN, vin, LE_ECALL_VIN_MAX_BYTES, "");
    if (strlen(vin) != LE_ECALL_VIN_MAX_LEN)
    {
        LE_DEBUG("eCall settings, VIN is set to %s", vin);

        memset(vin, 0, LE_ECALL_VIN_MAX_BYTES);

        LE_WARN("No node value set for '%s'", CFG_NODE_VIN);

        le_cfg_CancelTxn(iterator);

        return LE_NOT_FOUND;
    }

    le_cfg_CancelTxn(iterator);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the propulsion type in the config tree.
 * Note that a vehicle may have more than one propulsion type.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_SetPropulsionType
(
    le_ecall_PropulsionTypeBitMask_t propulsionTypeBitMask
        ///< [IN] bitmask
)
{
    uint8_t i=0;
    char cfgNodeLoc[8] = {0};
    char configPath[LE_CFG_STR_LEN_BYTES];
    le_result_t res = LE_OK;

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_MODEMSERVICE_ECALL_PATH, CFG_NODE_PROP);

    // First remove old node data.
    le_cfg_QuickDeleteNode( configPath );

    // recreate the node /propulsionType/
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn( configPath );

    if (LE_ECALL_PROPULSION_TYPE_GASOLINE & propulsionTypeBitMask)
    {
        snprintf(cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);

        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Gasoline");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_DIESEL & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Diesel");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_NATURALGAS & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "NaturalGas");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_PROPANE & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Propane");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_ELECTRIC & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Electric");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_HYDROGEN & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Hydrogen");
        i++;
    }

    if (LE_ECALL_PROPULSION_TYPE_OTHER & propulsionTypeBitMask)
    {
        snprintf (cfgNodeLoc, sizeof(cfgNodeLoc), "%d", i);
        le_cfg_SetString ( iteratorRef, cfgNodeLoc, "Other");
        i++;
    }

    // if i is 0 at this point, there was no valid bit set in the bitmask
    if (i == 0)
    {
        LE_WARN("Bitmask had no valid bits set 0x%x", propulsionTypeBitMask);
        res = LE_FAULT;
    }
    if (LE_OK == res)
    {
        le_cfg_CommitTxn(iteratorRef);
    }
    else
    {
        le_cfg_CancelTxn( iteratorRef );
    }
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the propulsion stored in the config tree.
 * Note that a vehicle may have more than one propulsion type.
 *
 * @return
 *  - LE_OK on success
 *  - LE_NOT_FOUND if the value is not set
 *  - LE_FAULT for other failures
 *  - LE_BAD_PARAMETER parameter is NULL
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_ecall_GetPropulsionType
(
    le_ecall_PropulsionTypeBitMask_t* propulsionTypePtr
        ///< [OUT] bitmask
)
{
    le_result_t result;
    le_ecall_PropulsionTypeBitMask_t resultBitMask= 0;

    if (NULL == propulsionTypePtr)
    {
        LE_KILL_CLIENT("propulsionTypePtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    // GetPropulsionType reads from config tree
    // and writes a boolean values in the MSD.
    result = GetPropulsionType();

    if (LE_OK == result)
    {
        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.gasolineTankPresent)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_GASOLINE;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.dieselTankPresent)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_DIESEL;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.compressedNaturalGas)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_NATURALGAS;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.liquidPropaneGas)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_PROPANE;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.electricEnergyStorage)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_ELECTRIC;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.hydrogenStorage)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_HYDROGEN;
        }

        if (ECallObj.msd.msdMsg.msdStruct.vehPropulsionStorageType.otherStorage)
        {
            resultBitMask |=  LE_ECALL_PROPULSION_TYPE_OTHER;
        }

        *propulsionTypePtr = resultBitMask;
    }

    return result;
}
