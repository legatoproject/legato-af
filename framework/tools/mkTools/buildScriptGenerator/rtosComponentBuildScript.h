//--------------------------------------------------------------------------------------------------
/**
 * @file rtosComponentBuildScript.h
 *
 * Build components for an RTOS (e.g. FreeRTOS)
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_RTOS_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_RTOS_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "buildScriptRtos.h"
#include "componentBuildScript.h"

namespace ninja
{

class RtosComponentBuildScriptGenerator_t : public ComponentBuildScriptGenerator_t
{
    protected:
        void GenerateCommonCAndCxxFlags(model::Component_t* componentPtr) override;
        void GenerateComponentLinkStatement(model::Component_t* componentPtr) override;

    public:
        explicit RtosComponentBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : ComponentBuildScriptGenerator_t(baseGeneratorPtr) {}

        RtosComponentBuildScriptGenerator_t(const std::string scriptPath,
                                             const mk::BuildParams_t& buildParams)
        : ComponentBuildScriptGenerator_t(
            std::make_shared<RtosBuildScriptGenerator_t>(scriptPath, buildParams)) {}

        virtual ~RtosComponentBuildScriptGenerator_t() {}
};

} // namespace ninja

#endif // LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
