//--------------------------------------------------------------------------------------------------
/**
 * @file appBuildScript.h
 *
 * Functions provided by the Application Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "exeBuildScript.h"

namespace ninja
{

class AppBuildScriptGenerator_t : protected RequireExeGenerator_t
{
    friend struct RequireBaseGenerator_t;
    friend struct RequireAppGenerator_t;

    protected:
        virtual void GenerateCommentHeader(model::App_t* appPtr);
        virtual void GenerateFileBundleBuildStatement(
            model::FileSystemObjectSet_t& bundledFiles,
            model::App_t* appPtr,
            const model::FileSystemObject_t* fileSystemObjPtr);
        virtual void GenerateDirBundleBuildStatements(
            model::FileSystemObjectSet_t& bundledFiles,
            model::App_t* appPtr,
            const model::FileSystemObject_t* fileSystemObjPtr);

    protected:
        virtual void GenerateAppBuildRules(void);
        virtual void GenerateStagingBundleBuildStatements(model::App_t* appPtr);
        virtual void GenerateNinjaScriptBuildStatement(model::App_t* appPtr);

        explicit AppBuildScriptGenerator_t(std::shared_ptr<ExeBuildScriptGenerator_t>
                                           exeGeneratorPtr)
        : RequireBaseGenerator_t(exeGeneratorPtr.get()),
          RequireExeGenerator_t(exeGeneratorPtr) {}

    public:
        virtual void GenerateBuildRules(void);
        virtual void GenerateExeBuildStatements(model::App_t* appPtr);
        virtual void GenerateAppBundleBuildStatement(model::App_t* appPtr,
                                                     const std::string& outputDir);
        virtual void Generate(model::App_t* appPtr);

        virtual ~AppBuildScriptGenerator_t() {}
};


/**
 * Derive from this class for generators which need access to a app generator
 */
struct RequireAppGenerator_t : public RequireExeGenerator_t
{
    std::shared_ptr<AppBuildScriptGenerator_t> appGeneratorPtr;

    explicit RequireAppGenerator_t
    (
        std::shared_ptr<AppBuildScriptGenerator_t> appGeneratorPtr
    )
    : RequireBaseGenerator_t(appGeneratorPtr.get()),
      RequireExeGenerator_t(appGeneratorPtr->exeGeneratorPtr),
      appGeneratorPtr(appGeneratorPtr) {}
};


} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
