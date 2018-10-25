//--------------------------------------------------------------------------------------------------
/**
 * @file linuxAppBuildScript.h
 *
 * Functions provided by the Application Build Script Generator for Linux
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_LINUX_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_LINUX_APP_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "appBuildScript.h"
#include "linuxExeBuildScript.h"

namespace ninja
{

class LinuxAppBuildScriptGenerator_t : public AppBuildScriptGenerator_t
{
    public:
        void GenerateStagingBundleBuildStatements(model::App_t* appPtr) override;

        explicit LinuxAppBuildScriptGenerator_t(std::shared_ptr<BuildScriptGenerator_t>
                                                baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr),
          AppBuildScriptGenerator_t(
              std::make_shared<LinuxExeBuildScriptGenerator_t>(baseGeneratorPtr)) {}

        LinuxAppBuildScriptGenerator_t(const std::string scriptPath,
                                       const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<LinuxBuildScriptGenerator_t>(scriptPath, buildParams)),
          AppBuildScriptGenerator_t(
              std::make_shared<LinuxExeBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_LINUX_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
