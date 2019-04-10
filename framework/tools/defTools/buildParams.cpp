//--------------------------------------------------------------------------------------------------
/**
 * @file buildParams.cpp  Implementation of Build Params object methods.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace mk
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
BuildParams_t::BuildParams_t
(
)
//--------------------------------------------------------------------------------------------------
:   beVerbose(false),
    jobCount(0),
    target("localhost"),
    osType("linux"),
    signPkg(false),
    codeGenOnly(false),
    isStandAloneComp(false),
    binPack(false),
    noPie(false),
    argc(0),
    argv(NULL),
    readOnly(false)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Finish setting build params; add anything which may be dependent on other build parameters.
 */
void BuildParams_t::FinishConfig
(
    void
)
{
    std::string frameworkRootPath = envVars::Get("LEGATO_ROOT");

    interfaceDirs.push_front(path::Combine(frameworkRootPath,
                                          "build/" + target + "/framework/include"));
    interfaceDirs.push_front(path::Combine(frameworkRootPath, "framework/include"));

    interfaceDirs.push_front(path::Combine(frameworkRootPath, "interfaces"));
}


} // namespace mk
