//--------------------------------------------------------------------------------------------------
/**
 * @file exeBuildScript.h
 *
 * Functions provided by the Executables Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "componentBuildScript.h"

namespace ninja
{

class ExeBuildScriptGenerator_t : protected RequireComponentGenerator_t
{
    friend struct RequireBaseGenerator_t;
    friend struct RequireExeGenerator_t;

    protected:
        virtual void GenerateCommentHeader(model::Exe_t* exePtr);
        virtual std::string GetLinkRule(model::Exe_t* exePtr);

        virtual void GetDependentLibLdFlags(model::Exe_t* exePtr);
        virtual void GenerateBuildStatement(model::Exe_t* exePtr);
        virtual void GenerateIpcBuildStatements(model::Exe_t* exePtr);

        virtual void GenerateCandCxxFlags(model::Exe_t* exePtr);

        virtual void GenerateNinjaScriptBuildStatement(model::Exe_t* exePtr);

        explicit ExeBuildScriptGenerator_t(
            std::shared_ptr<ComponentBuildScriptGenerator_t> componentGeneratorPtr)
        : RequireBaseGenerator_t(componentGeneratorPtr.get()),
          RequireComponentGenerator_t(componentGeneratorPtr) {}

    public:
        explicit ExeBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr),
          RequireComponentGenerator_t(std::make_shared<ComponentBuildScriptGenerator_t>(
                                          baseGeneratorPtr)) {}

        ExeBuildScriptGenerator_t(const std::string scriptPath,
                                  const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(std::make_shared<BuildScriptGenerator_t>(scriptPath, buildParams)),
          RequireComponentGenerator_t(std::make_shared<ComponentBuildScriptGenerator_t>(
                                          baseGeneratorPtr)) {}

        virtual void GenerateBuildRules(void);
        virtual void GenerateBuildStatements(model::Exe_t* exePtr);
        virtual void Generate(model::Exe_t* exePtr);

        virtual ~ExeBuildScriptGenerator_t() {}
};


/**
 * Derive from this class for generators which need access to a exe generator
 */
struct RequireExeGenerator_t : public RequireComponentGenerator_t
{
    std::shared_ptr<ExeBuildScriptGenerator_t> exeGeneratorPtr;

    explicit RequireExeGenerator_t
    (
        std::shared_ptr<ExeBuildScriptGenerator_t> exeGeneratorPtr
    )
    : RequireBaseGenerator_t(exeGeneratorPtr->baseGeneratorPtr),
      RequireComponentGenerator_t(exeGeneratorPtr->componentGeneratorPtr),
      exeGeneratorPtr(exeGeneratorPtr) {}
};


} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
