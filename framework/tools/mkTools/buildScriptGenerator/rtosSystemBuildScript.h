//--------------------------------------------------------------------------------------------------
/**
 * @file rtosSystemBuildScript.h
 *
 * Functions for RTOS system build scripts.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_RTOS_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_RTOS_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "systemBuildScript.h"
#include "buildScriptRtos.h"
#include "moduleBuildScript.h"
#include "rtosAppBuildScript.h"

namespace ninja
{

class RtosSystemBuildScriptGenerator_t : public SystemBuildScriptGenerator_t
{
    protected:
        void GenerateSystemBuildRules(model::System_t* systemPtr) override;
        void GenerateSystemPackBuildStatement(model::System_t* systemPtr) override;
        virtual void GenerateLdFlags(void);
    public:
        RtosSystemBuildScriptGenerator_t(const std::string scriptPath,
                                          const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<RtosBuildScriptGenerator_t>(scriptPath, buildParams)),
          SystemBuildScriptGenerator_t(
              std::make_shared<RtosAppBuildScriptGenerator_t>(baseGeneratorPtr),
              std::make_shared<NullModuleBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_NINJA_RTOS_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
