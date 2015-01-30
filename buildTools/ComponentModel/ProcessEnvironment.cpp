//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Process Environment class methods.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
ProcessEnvironment::ProcessEnvironment
(
    void
)
//--------------------------------------------------------------------------------------------------
:   m_MaxFileBytes(100 * 1024), // 100 K
    m_MaxCoreDumpFileBytes(m_MaxFileBytes.Get()),
    m_MaxLockedMemoryBytes(8 * 1024),   // 8 KB
    m_MaxFileDescriptors(256)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Move constructor.
 */
//--------------------------------------------------------------------------------------------------
ProcessEnvironment::ProcessEnvironment
(
    ProcessEnvironment&& original
)
//--------------------------------------------------------------------------------------------------
:   m_ProcessList(std::move(original.m_ProcessList)),
    m_EnvVarList(std::move(original.m_EnvVarList)),
    m_StartPriority(std::move(original.m_StartPriority)),
    m_MaxPriority(std::move(original.m_MaxPriority)),
    m_FaultAction(std::move(original.m_FaultAction)),
    m_MaxFileBytes(std::move(original.m_MaxFileBytes)),
    m_MaxCoreDumpFileBytes(std::move(original.m_MaxCoreDumpFileBytes)),
    m_MaxLockedMemoryBytes(std::move(original.m_MaxLockedMemoryBytes)),
    m_MaxFileDescriptors(std::move(original.m_MaxFileDescriptors)),
    m_WatchdogTimeout(std::move(original.m_WatchdogTimeout)),
    m_WatchdogAction(std::move(original.m_WatchdogAction))
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum priority level for all threads running in this process environment.
 */
//--------------------------------------------------------------------------------------------------
void ProcessEnvironment::MaxPriority
(
    const std::string& priority
)
//--------------------------------------------------------------------------------------------------
{
    m_MaxPriority = priority;

    // Make sure that no processes are started at a priority higher than the maximum allowed.
    if (m_StartPriority.IsSet() && (m_StartPriority > m_MaxPriority))
    {
        std::cerr << "Warning: clamping start priority level '" << m_StartPriority.Get()
                  << "' to maximum priority level '" << priority << "'." << std::endl;
        m_StartPriority = m_MaxPriority;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the starting priority level for processes running in this process environment.
 */
//--------------------------------------------------------------------------------------------------
void ProcessEnvironment::StartPriority
(
    const std::string& priority
)
//--------------------------------------------------------------------------------------------------
{
    m_StartPriority = priority;

    // Make sure that no processes are started at a priority higher than the maximum allowed.
    if (m_MaxPriority.IsSet() && (m_StartPriority > m_MaxPriority))
    {
        std::cerr << "Warning: clamping start priority level '" << priority
                  << "' to maximum priority level '" << m_MaxPriority.Get() << "'." << std::endl;
        m_StartPriority = m_MaxPriority;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * @return  true if this process environment allows any threads to run at real-time priority levels.
 **/
//--------------------------------------------------------------------------------------------------
bool ProcessEnvironment::AreRealTimeThreadsPermitted
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return (m_MaxPriority.IsRealTime() || m_StartPriority.IsRealTime());
}


} // namespace legato
