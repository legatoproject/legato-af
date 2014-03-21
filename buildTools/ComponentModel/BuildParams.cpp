//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Build Params object methods.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
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
BuildParams_t::BuildParams_t
(
)
//--------------------------------------------------------------------------------------------------
:   m_IsVerbose(false),
    m_Target("localhost"),
    m_ExeOutputDir("."),
    m_LibOutputDir("."),
    m_ObjOutputDir(".")

//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor
 **/
//--------------------------------------------------------------------------------------------------
BuildParams_t::BuildParams_t
(
    const BuildParams_t& original
)
//--------------------------------------------------------------------------------------------------
:   m_IsVerbose(original.m_IsVerbose),
    m_Target(original.m_Target),
    m_ExeOutputDir(original.m_ExeOutputDir),
    m_LibOutputDir(original.m_LibOutputDir),
    m_ObjOutputDir(original.m_ObjOutputDir)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Move constructor
 **/
//--------------------------------------------------------------------------------------------------
BuildParams_t::BuildParams_t
(
    BuildParams_t&& original
)
//--------------------------------------------------------------------------------------------------
:   m_IsVerbose(std::move(original.m_IsVerbose)),
    m_Target(std::move(original.m_Target)),
    m_ExeOutputDir(std::move(original.m_ExeOutputDir)),
    m_LibOutputDir(std::move(original.m_LibOutputDir)),
    m_ObjOutputDir(std::move(original.m_ObjOutputDir))
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator (=)
 **/
//--------------------------------------------------------------------------------------------------
BuildParams_t& BuildParams_t::operator =
(
    const BuildParams_t& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_IsVerbose = original.m_IsVerbose;
        m_Target = original.m_Target;
        m_ExeOutputDir = original.m_ExeOutputDir;
        m_LibOutputDir = original.m_LibOutputDir;
        m_ObjOutputDir = original.m_ObjOutputDir;
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator (=)
 **/
//--------------------------------------------------------------------------------------------------
BuildParams_t& BuildParams_t::operator =
(
    BuildParams_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (&original != this)
    {
        m_IsVerbose = std::move(original.m_IsVerbose);
        m_Target = std::move(original.m_Target);
        m_ExeOutputDir = std::move(original.m_ExeOutputDir);
        m_LibOutputDir = std::move(original.m_LibOutputDir);
        m_ObjOutputDir = std::move(original.m_ObjOutputDir);
    }

    return *this;
}



}
