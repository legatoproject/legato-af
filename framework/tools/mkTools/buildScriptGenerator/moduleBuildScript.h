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


//--------------------------------------------------------------------------------------------------
/**
 * Null module builder which throws an error on all module builds.
 *
 * Kernel modules are an optional feature not supported by all Legato systems.  For systems
 * which don't support them, use this null build script generator to throw errors if attempting
 * to create a module.
 */
//--------------------------------------------------------------------------------------------------
class NullModuleBuildScriptGenerator_t : public ModuleBuildScriptGenerator_t
{
    public:
        NullModuleBuildScriptGenerator_t(std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : ModuleBuildScriptGenerator_t(baseGeneratorPtr) {}

        void GenerateBuildStatements(model::Module_t* modulePtr)
        {
            throw mk::Exception_t(
                LE_I18N("INTERNAL ERROR: Kernel modules not supported on this system type."));
        }

        void Generate(model::Module_t* modulePtr)
        {
            throw mk::Exception_t(
                LE_I18N("INTERNAL ERROR: Kernel modules not supported on this system type."));
        }

        virtual ~NullModuleBuildScriptGenerator_t() {}
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
