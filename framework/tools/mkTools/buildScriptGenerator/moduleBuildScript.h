//--------------------------------------------------------------------------------------------------
/**
 * @file moduleBuildScript.h
 *
 * Functions provided by the Modules Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD

namespace ninja
{

class ModuleBuildScriptGenerator_t : protected RequireBaseGenerator_t
{
    friend struct RequireBaseGenerator_t;
    friend struct RequireModuleGenerator_t;

    protected:
        virtual void GenerateCommentHeader(model::Module_t* modulePtr);
        virtual void GenerateMakefile(model::Module_t* modulePtr);

        virtual void GenerateNinjaScriptBuildStatement(model::Module_t* modulePtr);

    public:
        ModuleBuildScriptGenerator_t(std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr) {}

        ModuleBuildScriptGenerator_t(const std::string scriptPath,
                                     const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(std::make_shared<BuildScriptGenerator_t>(scriptPath, buildParams))
        {}

        virtual void GenerateBuildStatements(model::Module_t* modulePtr);
        virtual void Generate(model::Module_t* modulePtr);
        virtual void GenerateModuleBundleBuildStatement(model::Module_t* modulePtr,
                                                        const std::string& outputDir);
        virtual void GenerateStagingBundleBuildStatements(model::Module_t* modulePtr);
        virtual void GenerateFileBundleBuildStatement(model::FileSystemObjectSet_t& bundledFiles,
                                                model::Module_t* appPtr,
                                                const model::FileSystemObject_t* fileSystemObjPtr);
        virtual void GenerateDirBundleBuildStatements(model::FileSystemObjectSet_t& bundledFiles,
                                                model::Module_t* modulePtr,
                                                const model::FileSystemObject_t* fileSystemObjPtr);

        virtual ~ModuleBuildScriptGenerator_t() {}
};


/**
 * Derive from this class for generators which need access to a module generator
 */
struct RequireModuleGenerator_t : public virtual RequireBaseGenerator_t
{
    std::shared_ptr<ModuleBuildScriptGenerator_t> moduleGeneratorPtr;

    explicit RequireModuleGenerator_t
    (
        std::shared_ptr<ModuleBuildScriptGenerator_t> moduleGeneratorPtr
    )
    : RequireBaseGenerator_t(moduleGeneratorPtr->baseGeneratorPtr),
      moduleGeneratorPtr(moduleGeneratorPtr) {}
};


} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD
