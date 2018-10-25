//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptRtos.h
 *
 * Declarations of RTOS-specific build rules.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_BUILD_SCRIPT_RTOS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_BUILD_SCRIPT_RTOS_H_INCLUDE_GUARD

#include "buildScriptCommon.h"

namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Rtos build script generator.
 */
//--------------------------------------------------------------------------------------------------
class RtosBuildScriptGenerator_t : public BuildScriptGenerator_t
{
    public:
        virtual void GenerateBuildRules(void) override;
        virtual void GenerateCFlags(void) override;
        virtual void GenerateIfgenFlags(void) override;
    public:
        RtosBuildScriptGenerator_t(const std::string scriptPath,
                                    const mk::BuildParams_t& buildParams)
        : BuildScriptGenerator_t(scriptPath, buildParams) {}

        virtual ~RtosBuildScriptGenerator_t() {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_BUILD_SCRIPT_RTOS_H_INCLUDE_GUARD
