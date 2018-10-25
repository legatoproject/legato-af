//--------------------------------------------------------------------------------------------------
/**
 * @file linuxExeBuildScript.h
 *
 * Functions provided by the Executables Build Script Generator for Linux
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_LINUX_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_LINUX_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "exeBuildScript.h"
#include "linuxComponentBuildScript.h"

namespace ninja
{

class LinuxExeBuildScriptGenerator_t : public ExeBuildScriptGenerator_t
{
    protected:
        virtual std::string GetLinkRule(model::Exe_t* exePtr);

        void GenerateCandCxxFlags(model::Exe_t* exePtr) override;
        virtual void GetDependentLibLdFlags(model::Exe_t* exePtr);

        void GenerateBuildStatement(model::Exe_t* exePtr) override;
    public:
        explicit LinuxExeBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr),
          ExeBuildScriptGenerator_t(std::make_shared<LinuxComponentBuildScriptGenerator_t>(
                                        baseGeneratorPtr)) {}

        LinuxExeBuildScriptGenerator_t(const std::string scriptPath,
                                       const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<LinuxBuildScriptGenerator_t>(scriptPath, buildParams)),
          ExeBuildScriptGenerator_t(
              std::make_shared<LinuxComponentBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_LINUX_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
