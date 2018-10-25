//--------------------------------------------------------------------------------------------------
/**
 * @file rtosAppBuildScript.h
 *
 * Functions provided by the Application Build Script Generator for Rtos
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_RTOS_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_RTOS_APP_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "appBuildScript.h"
#include "rtosExeBuildScript.h"

namespace ninja
{

class RtosAppBuildScriptGenerator_t : public AppBuildScriptGenerator_t
{
    public:
        void GenerateAppBuildRules(void) override;
        void GenerateAppBundleBuildStatement(model::App_t* appPtr,
                                             const std::string& outputDir) override;

        explicit RtosAppBuildScriptGenerator_t(std::shared_ptr<BuildScriptGenerator_t>
                                                baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr),
          AppBuildScriptGenerator_t(
              std::make_shared<RtosExeBuildScriptGenerator_t>(baseGeneratorPtr)) {}

        RtosAppBuildScriptGenerator_t(const std::string scriptPath,
                                       const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<RtosBuildScriptGenerator_t>(scriptPath, buildParams)),
          AppBuildScriptGenerator_t(
              std::make_shared<RtosExeBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_RTOS_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
