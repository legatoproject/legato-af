//--------------------------------------------------------------------------------------------------
/**
 * @file linuxSystemBuildScript.h
 *
 * Functions for Linux system build scripts.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_LINUX_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_LINUX_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "systemBuildScript.h"
#include "buildScriptLinux.h"
#include "moduleBuildScript.h"
#include "linuxAppBuildScript.h"

namespace ninja
{

class LinuxSystemBuildScriptGenerator_t : public SystemBuildScriptGenerator_t
{
    protected:
        template<class T>
        void GenerateRpcCfgReferences(const std::map<std::string, T> &externInterfaces,
                                      std::set<std::string> &rpcCfgRefs);

        void GenerateSystemBuildRules(model::System_t* systemPtr) override;
        void GenerateSystemPackBuildStatement(model::System_t* systemPtr) override;
    public:
        LinuxSystemBuildScriptGenerator_t(const std::string scriptPath,
                                          const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
              std::make_shared<LinuxBuildScriptGenerator_t>(scriptPath, buildParams)),
          SystemBuildScriptGenerator_t(
              std::make_shared<LinuxAppBuildScriptGenerator_t>(baseGeneratorPtr),
              std::make_shared<ModuleBuildScriptGenerator_t>(baseGeneratorPtr)) {}
};

} // namespace ninja

#endif // LEGATO_NINJA_LINUX_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
