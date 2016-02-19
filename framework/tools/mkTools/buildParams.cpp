//--------------------------------------------------------------------------------------------------
/**
 * @file buildParams.cpp  Implementation of Build Params object methods.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


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
    target("localhost"),
    libOutputDir(""),
    workingDir(""),
    codeGenOnly(false)
//--------------------------------------------------------------------------------------------------
{
    std::string frameworkRootPath = envVars::Get("LEGATO_ROOT");

    interfaceDirs.push_back(path::Combine(frameworkRootPath, "interfaces"));

    interfaceDirs.push_back(path::Combine(frameworkRootPath, "framework/c/inc"));
}


} // namespace mk
