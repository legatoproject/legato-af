//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Process Environment class methods.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Default constructor
 */
//--------------------------------------------------------------------------------------------------
ProcessEnvironment::ProcessEnvironment
(
)
//--------------------------------------------------------------------------------------------------
:   m_CoreFileSize(SIZE_MAX),
    m_MaxFileSize(SIZE_MAX),
    m_MemLockSize(SIZE_MAX),
    m_NumFds(SIZE_MAX)
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
    m_Priority(std::move(original.m_Priority)),
    m_FaultAction(std::move(original.m_FaultAction)),
    m_CoreFileSize(std::move(original.m_CoreFileSize)),
    m_MaxFileSize(std::move(original.m_MaxFileSize)),
    m_MemLockSize(std::move(original.m_MemLockSize)),
    m_NumFds(std::move(original.m_NumFds))
//--------------------------------------------------------------------------------------------------
{
}



}
