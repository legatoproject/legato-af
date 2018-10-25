//--------------------------------------------------------------------------------------------------
/**
 * @file rtosExeBuildScript.h
 *
 * Functions provided by the Executables Build Script Generator for RTOSes
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_RTOS_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_RTOS_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "exeBuildScript.h"
#include "rtosComponentBuildScript.h"

namespace ninja
{

class RtosExeBuildScriptGenerator_t : public ExeBuildScriptGenerator_t
{
    protected:
        void GenerateBuildStatement(model::Exe_t* exePtr) override;

    public:
        explicit RtosExeBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr),
          ExeBuildScriptGenerator_t(std::make_shared<RtosComponentBuildScriptGenerator_t>(
                                        baseGeneratorPtr)) {}

        RtosExeBuildScriptGenerator_t(const std::string scriptPath,
                                       const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<RtosBuildScriptGenerator_t>(scriptPath, buildParams)),
          ExeBuildScriptGenerator_t(
              std::make_shared<RtosComponentBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_RTOS_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
