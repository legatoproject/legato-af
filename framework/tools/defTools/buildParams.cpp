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
    isRelaxedStrictness(false),
    jobCount(0),
    target("localhost"),
    osType("linux"),
    signPkg(false),
    codeGenOnly(false),
    isStandAloneComp(false),
    binPack(false),
    noPie(false),
    isDryRun(false),
    argc(0),
    argv(NULL),
    readOnly(false),
    haveFrameworkConfig(false)
//--------------------------------------------------------------------------------------------------
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Load Legato KConfig
 *
 * Do this after processing command-line parameters but before processing any .def files
 * to get the environment for .def file processing.
 *
 * This will be done automatically by FinishConfig if not called before then.
 */
void BuildParams_t::LoadFrameworkConfig
(
    void
)
{
    std::string frameworkRootPath = envVars::Get("LEGATO_ROOT");
    std::string envFilePath = path::Combine(frameworkRootPath, "build/" + target + "/config.sh");

    // Load the KConfig-generated environment from the Legato directory.
    if (!file::FileExists(envFilePath))
    {
        throw mk::Exception_t(mk::format(LE_I18N("Bad configuration environment file path '%s'."),
            envFilePath));
    }
    envVars::Load(envFilePath, *this);

    haveFrameworkConfig = true;
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

    if (!haveFrameworkConfig)
    {
        LoadFrameworkConfig();
    }

    interfaceDirs.push_front(path::Combine(frameworkRootPath,
                                          "build/" + target + "/framework/include"));
    interfaceDirs.push_front(path::Combine(frameworkRootPath, "framework/include"));

    interfaceDirs.push_front(path::Combine(frameworkRootPath, "interfaces"));

    // Add platformLimits to the back so it can be overridden by files in an .sdef's
    // interfaceSearch
    interfaceDirs.push_back(path::Combine(frameworkRootPath, "interfaces/platformLimits"));
}


} // namespace mk
