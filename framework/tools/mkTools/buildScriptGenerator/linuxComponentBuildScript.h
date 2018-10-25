//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.h
 *
 * Functions shared with other build script generator files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_LINUX_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_LINUX_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "buildScriptLinux.h"
#include "componentBuildScript.h"

namespace ninja
{

class LinuxComponentBuildScriptGenerator_t : public ComponentBuildScriptGenerator_t
{
    protected:
        void GetImplicitDependencies(model::Component_t* componentPtr) override;

        void GenerateCommonCAndCxxFlags(model::Component_t* componentPtr) override;
        virtual void GenerateLdFlagsDef(model::Component_t* componentPtr);
        virtual void GetDependentLibLdFlags(model::Component_t* componentPtr);

        void GenerateComponentLinkStatement(model::Component_t* componentPtr) override;

    public:
        explicit LinuxComponentBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : ComponentBuildScriptGenerator_t(baseGeneratorPtr) {}

        LinuxComponentBuildScriptGenerator_t(const std::string scriptPath,
                                             const mk::BuildParams_t& buildParams)
        : ComponentBuildScriptGenerator_t(
            std::make_shared<LinuxBuildScriptGenerator_t>(scriptPath, buildParams)) {}

        virtual ~LinuxComponentBuildScriptGenerator_t() {}
};

} // namespace ninja

#endif // LEGATO_NINJA_LINUX_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
