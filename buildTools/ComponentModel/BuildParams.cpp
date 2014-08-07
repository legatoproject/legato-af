//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Build Params object methods.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
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
    m_ObjOutputDir("."),
    m_StagingDir(".")

//--------------------------------------------------------------------------------------------------
{
    std::string frameworkRootPath = DoEnvVarSubstitution("$LEGATO_ROOT");

    m_InterfaceDirs.push_back(CombinePath(frameworkRootPath, "interfaces"));

    m_InterfaceDirs.push_back(CombinePath(frameworkRootPath, "framework/c/inc"));
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
    m_InterfaceDirs(original.m_InterfaceDirs),
    m_ComponentDirs(original.m_ComponentDirs),
    m_ExeOutputDir(original.m_ExeOutputDir),
    m_LibOutputDir(original.m_LibOutputDir),
    m_ObjOutputDir(original.m_ObjOutputDir),
    m_StagingDir(original.m_StagingDir),
    m_CCompilerFlags(original.m_CCompilerFlags),
    m_LinkerFlags(original.m_LinkerFlags)

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
    m_InterfaceDirs(std::move(original.m_InterfaceDirs)),
    m_ComponentDirs(std::move(original.m_ComponentDirs)),
    m_ExeOutputDir(std::move(original.m_ExeOutputDir)),
    m_LibOutputDir(std::move(original.m_LibOutputDir)),
    m_ObjOutputDir(std::move(original.m_ObjOutputDir)),
    m_StagingDir(std::move(original.m_StagingDir)),
    m_CCompilerFlags(std::move(original.m_CCompilerFlags)),
    m_LinkerFlags(std::move(original.m_LinkerFlags))
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
        m_InterfaceDirs = original.m_InterfaceDirs;
        m_ComponentDirs = original.m_ComponentDirs;
        m_ExeOutputDir = original.m_ExeOutputDir;
        m_LibOutputDir = original.m_LibOutputDir;
        m_ObjOutputDir = original.m_ObjOutputDir;
        m_StagingDir = original.m_StagingDir;
        m_CCompilerFlags = original.m_CCompilerFlags;
        m_LinkerFlags = original.m_LinkerFlags;
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
        m_InterfaceDirs = std::move(original.m_InterfaceDirs);
        m_ComponentDirs = std::move(original.m_ComponentDirs);
        m_ExeOutputDir = std::move(original.m_ExeOutputDir);
        m_LibOutputDir = std::move(original.m_LibOutputDir);
        m_ObjOutputDir = std::move(original.m_ObjOutputDir);
        m_StagingDir = std::move(original.m_StagingDir);
        m_CCompilerFlags = std::move(original.m_CCompilerFlags);
        m_LinkerFlags = std::move(original.m_LinkerFlags);
    }

    return *this;
}



}
