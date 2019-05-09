//--------------------------------------------------------------------------------------------------
/**
 * @file sysStatus.h
 *
 * Functions relating to system status (whether a system is good, bad or
 * is being tried under probation)
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_SYSTEM_STATUS_H_INCLUDE_GUARD
#define LEGATO_SYSTEM_STATUS_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of possible system statuses.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    SYS_GOOD,      ///< "good"
    SYS_BAD,       ///< "bad"
    SYS_PROBATION  ///< "tried N" or untried.
}
sysStatus_Status_t;


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return The system status (SYS_BAD on error).
 */
//--------------------------------------------------------------------------------------------------
sysStatus_Status_t sysStatus_GetStatus
(
    const char* systemNamePtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 */
//--------------------------------------------------------------------------------------------------
sysStatus_Status_t sysStatus_Status
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove the status file thus setting the try status to untried
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_SetUntried
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * If the current system status is in Probation, then this return the number of times the system has
 * been tried while in probation.
 *
 * Do not call this if you are not in probation!
 *
 * @return A count of the number of times the system has been "tried."
 */
//--------------------------------------------------------------------------------------------------
int sysStatus_TryCount
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Increment the try count.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_IncrementTryCount
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrement the try count.
 * If the system is still under probation the try count will be decremented, else
 * there is no effect. Will FATAL if the system has already been marked bad.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_DecrementTryCount
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of boot count
 *
 * @return The number of consecutive reboots by the current system."
 */
//--------------------------------------------------------------------------------------------------
int sysStatus_BootCount
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Decrement the boot count.
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_DecrementBootCount
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "bad".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkBad
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "tried 1".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkTried
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Mark the system "good".
 */
//--------------------------------------------------------------------------------------------------
void sysStatus_MarkGood
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the current system.
 *
 * @return true if the system is marked "good", false otherwise (e.g., if "tried 2").
 */
//--------------------------------------------------------------------------------------------------
bool sysStatus_IsGood
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether legato system is Read-Only or not.
 *
 * @return
 *     true if the system is Read-Only
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool sysStatus_IsReadOnly
(
    void
);

#endif // LEGATO_SYSTEM_STATUS_H_INCLUDE_GUARD

