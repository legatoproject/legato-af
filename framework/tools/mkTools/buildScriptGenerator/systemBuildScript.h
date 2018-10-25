//--------------------------------------------------------------------------------------------------
/**
 * @file systemBuildScript.h
 *
 * Functions shared with other build script generator files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "buildScriptCommon.h"
#include "appBuildScript.h"
#include "moduleBuildScript.h"

namespace ninja
{

class SystemBuildScriptGenerator_t : protected RequireAppGenerator_t,
                                     protected RequireModuleGenerator_t
{
    protected:
        virtual void GenerateCommentHeader(model::System_t* systemPtr);
        virtual void GenerateBuildRules(model::System_t* systemPtr);
        virtual void GenerateSystemBuildRules(model::System_t* systemPtr) = 0;
        virtual void GenerateSystemPackBuildStatement(model::System_t* systemPtr) = 0;
        virtual void GenerateNinjaScriptBuildStatement(model::System_t* systemPtr);

    protected:
        SystemBuildScriptGenerator_t(
            std::shared_ptr<AppBuildScriptGenerator_t> appGeneratorPtr,
            std::shared_ptr<ModuleBuildScriptGenerator_t> moduleGeneratorPtr)
        : RequireBaseGenerator_t(appGeneratorPtr.get()),
          RequireAppGenerator_t(appGeneratorPtr),
          RequireModuleGenerator_t(moduleGeneratorPtr) {}

    public:
        virtual void Generate(model::System_t* systemPtr);

        virtual ~SystemBuildScriptGenerator_t() {}
};


} // namespace ninja

#endif // LEGATO_NINJA_SYSTEM_BUILD_SCRIPT_H_INCLUDE_GUARD
