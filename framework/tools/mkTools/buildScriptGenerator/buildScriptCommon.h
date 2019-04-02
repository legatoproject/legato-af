//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptCommon.h
 *
 * Declarations of common functions shared by the build script generators.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Create a build script file and open it.
 **/
//--------------------------------------------------------------------------------------------------
void OpenFile
(
    std::ofstream& script,
    const std::string& filePath,
    bool beVerbose
);


//--------------------------------------------------------------------------------------------------
/**
 * Close a build script file and check for errors.
 **/
//--------------------------------------------------------------------------------------------------
void CloseFile
(
    std::ofstream& script
);

//--------------------------------------------------------------------------------------------------
/**
 * Escape special ninja characters in a string
 */
//--------------------------------------------------------------------------------------------------
std::string EscapeString
(
    const std::string &str
);

//--------------------------------------------------------------------------------------------------
/**
 * Generic build script generator.
 */
//--------------------------------------------------------------------------------------------------
class BuildScriptGenerator_t
{
    friend struct RequireBaseGenerator_t;

    protected:
        std::ofstream script;
        const mk::BuildParams_t& buildParams;
        const std::string scriptPath;

    public:
        virtual void GenerateIncludedApis(const model::ApiFile_t* apiFilePtr);

        virtual void GenerateIfgenFlags(void);
        virtual void GenerateCFlags(void);
        virtual void GenerateBuildRules(void);

        virtual void GenerateNinjaScriptBuildStatement(const std::set<std::string>& dependencies);
        virtual void GenerateFileBundleBuildStatement(const model::FileSystemObject_t& fileObject,
                                                      model::FileSystemObjectSet_t& bundledFiles);
        virtual void GenerateDirBundleBuildStatements(const model::FileSystemObject_t& fileObject,
                                                      model::FileSystemObjectSet_t& bundledFiles);

        std::string GetPathEnvVarDecl(void);
        std::string PermissionsToModeFlags(model::Permissions_t permissions);

    public:
        BuildScriptGenerator_t(const std::string scriptPath,
                               const mk::BuildParams_t& buildParams);

        virtual ~BuildScriptGenerator_t();
};

/**
 * Derive from this class for generators which need access to a base generator (i.e. all of them)
 */
struct RequireBaseGenerator_t
{
    std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr;
    std::ostream& script;
    const mk::BuildParams_t buildParams;

    explicit RequireBaseGenerator_t(std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
    : baseGeneratorPtr(baseGeneratorPtr),
      script(baseGeneratorPtr->script),
      buildParams(baseGeneratorPtr->buildParams) {}

    template<class T>
    explicit RequireBaseGenerator_t(T* generatorPtr)
    : baseGeneratorPtr(generatorPtr->baseGeneratorPtr),
      script(baseGeneratorPtr->script),
      buildParams(baseGeneratorPtr->buildParams) {}
};

} // namespace ninja

#endif // LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD
