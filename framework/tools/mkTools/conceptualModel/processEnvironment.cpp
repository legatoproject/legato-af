//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Process Environment class methods.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
ProcessEnv_t::ProcessEnv_t
(
    void
)
//--------------------------------------------------------------------------------------------------
:   maxFileBytes(100 * 1024), // 100 K
    maxCoreDumpFileBytes(maxFileBytes.Get()),
    maxLockedMemoryBytes(8 * 1024),   // 8 KB
    maxFileDescriptors(256)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum priority level for all threads running in this process environment.
 */
//--------------------------------------------------------------------------------------------------
void ProcessEnv_t::SetMaxPriority
(
    const std::string& priority
)
//--------------------------------------------------------------------------------------------------
{
    maxPriority = priority;

    // Make sure that no processes are started at a priority higher than the maximum allowed.
    if (startPriority.IsSet() && (startPriority > maxPriority))
    {
        std::cerr << "Warning: clamping start priority level '" << startPriority.Get()
                  << "' to maximum priority level '" << priority << "'." << std::endl;
        startPriority = maxPriority;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the starting priority level for processes running in this process environment.
 */
//--------------------------------------------------------------------------------------------------
void ProcessEnv_t::SetStartPriority
(
    const std::string& priority
)
//--------------------------------------------------------------------------------------------------
{
    startPriority = priority;

    // Make sure that no processes are started at a priority higher than the maximum allowed.
    if (maxPriority.IsSet() && (startPriority > maxPriority))
    {
        std::cerr << "Warning: clamping start priority level '" << priority
                  << "' to maximum priority level '" << maxPriority.Get() << "'." << std::endl;
        startPriority = maxPriority;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * @return  true if this process environment allows any threads to run at real-time priority levels.
 **/
//--------------------------------------------------------------------------------------------------
bool ProcessEnv_t::AreRealTimeThreadsPermitted
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return (maxPriority.IsRealTime() || startPriority.IsRealTime());
}


} // namespace model
