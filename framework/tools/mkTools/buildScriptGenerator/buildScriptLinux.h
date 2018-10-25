//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptLinux.h
 *
 * Declarations of Linux-specific build rules.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_BUILD_SCRIPT_LINUX_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_BUILD_SCRIPT_LINUX_H_INCLUDE_GUARD

#include "buildScriptCommon.h"

namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Linux build script generator.
 */
//--------------------------------------------------------------------------------------------------
class LinuxBuildScriptGenerator_t : public BuildScriptGenerator_t
{
    public:
        virtual void GenerateCFlags(void) override;
        virtual void GenerateBuildRules(void) override;

    public:
        LinuxBuildScriptGenerator_t(const std::string scriptPath,
                                    const mk::BuildParams_t& buildParams)
        : BuildScriptGenerator_t(scriptPath, buildParams) {}

        virtual ~LinuxBuildScriptGenerator_t() {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_BUILD_SCRIPT_LINUX_H_INCLUDE_GUARD
