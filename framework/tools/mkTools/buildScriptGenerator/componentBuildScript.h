//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.h
 *
 * Functions shared with other build script generator files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD

#include "buildScriptCommon.h"

namespace ninja
{

class ComponentBuildScriptGenerator_t : protected RequireBaseGenerator_t
{
    friend struct RequireBaseGenerator_t;
    friend struct RequireComponentGenerator_t;

    protected:
        std::set<std::string> generatedComponents;
        std::set<std::string> generatedIPC;
    protected:
        virtual void GetImplicitDependencies(model::Component_t* componentPtr);
        virtual void GetExternalDependencies(model::Component_t* componentPtr);
        virtual void GetObjectFiles(model::Component_t* componentPtr);

        virtual void GetCInterfaceHeaders(std::list<std::string>& result,
                                          model::Component_t* componentPtr);
        virtual void GetJavaInterfaceFiles(std::list<std::string>& result,
                                           model::Component_t* componentPtr);

        virtual void GenerateTypesOnlyBuildStatement(const model::ApiTypesOnlyInterface_t* ifPtr);
        virtual void GenerateJavaTypesOnlyBuildStatement(const model::ApiTypesOnlyInterface_t* ifPtr);
        virtual void GenerateClientUsetypesBuildStatement(const model::ApiFile_t* apiFilePtr);
        virtual void GenerateServerUsetypesBuildStatement(const model::ApiFile_t* apiFilePtr);
        virtual void GenerateCommonUsetypesBuildStatement(const model::ApiFile_t* apiFilePtr);
        virtual void GenerateJavaUsetypesBuildStatement(const model::ApiFile_t* apiFilePtr);
        virtual void GenerateCCommonBuildStatement(const model::ApiFile_t* apiFilePtr);
        virtual void GenerateCBuildStatement(const model::ApiClientInterface_t* ifPtr);
        virtual void GenerateCBuildStatement(const model::ApiServerInterface_t* ifPtr);

        virtual void GenerateJavaBuildStatement(const model::InterfaceJavaFiles_t& javaFiles,
                                                const model::Component_t* componentPtr,
                                                const model::ApiFile_t* apiFilePtr,
                                                const std::string& internalName,
                                                bool isClient);
        virtual void GenerateJavaBuildStatement(const model::ApiClientInterface_t* ifPtr);
        virtual void GenerateJavaBuildStatement(const model::ApiServerInterface_t* ifPtr);

        virtual void GeneratePythonBuildStatement(
                                                const model::InterfacePythonFiles_t& pythonFiles,
                                                const model::Component_t* componentPtr,
                                                const model::ApiFile_t* apiFilePtr,
                                                const std::string& internalName,
                                                const std::string& workDir,
                                                bool isClient
                                                );
        virtual void GeneratePythonBuildStatement(const model::ApiClientInterface_t* ifPtr);

        virtual void GenerateCommonCAndCxxFlags(model::Component_t* componentPtr);

        virtual void GenerateComponentLinkStatement(model::Component_t* componentPtr) = 0;

        virtual void GenerateCommentHeader(model::Component_t* componentPtr);

        virtual void GenerateHeaderDir(const std::string& path);

        virtual void GenerateNinjaScriptBuildStatement(model::Component_t* componentPtr);

        virtual void AddNinjaDependencies(model::Component_t* componentPtr,
                                          std::set<std::string>& dependencies);

    public:
        virtual void GenerateCSourceBuildStatement(model::Component_t* componentPtr,
                                                   const model::ObjectFile_t* objFilePtr,
                                                   const std::list<std::string>& apiHeaders);
        virtual void GenerateCxxSourceBuildStatement(model::Component_t* componentPtr,
                                                     const model::ObjectFile_t* objFilePtr,
                                                     const std::list<std::string>& apiHeaders);
        virtual void GenerateJavaBuildCommand(const std::string& outputJar,
                                              const std::string& classDestPath,
                                              const std::list<std::string>& sources,
                                              const std::list<std::string>& jarClassPath);
        virtual void GenerateRunPathLdFlags(void);

    public:
        explicit ComponentBuildScriptGenerator_t(
            std::shared_ptr<BuildScriptGenerator_t> baseGeneratorPtr)
        : RequireBaseGenerator_t(baseGeneratorPtr) {}

        ComponentBuildScriptGenerator_t(const std::string scriptPath,
                                        const mk::BuildParams_t& buildParams)
        : RequireBaseGenerator_t(
            std::make_shared<BuildScriptGenerator_t>(scriptPath, buildParams)) {}

        virtual void Generate(model::Component_t* componentPtr);
        virtual void GenerateBuildRules(void);

        virtual void GetCommonApiFiles(model::Component_t* componentPtr,
                                       std::set<std::string> &commonApiObjects);

        virtual void GenerateBuildStatements(model::Component_t* componentPtr);
        virtual void GenerateBuildStatementsRecursive(model::Component_t* componentPtr);
        virtual void GenerateIpcBuildStatements(model::Component_t* componentPtr);

        virtual ~ComponentBuildScriptGenerator_t() {};
};


/**
 * Derive from this class for generators which need access to a component generator
 */
struct RequireComponentGenerator_t : public virtual RequireBaseGenerator_t
{
    std::shared_ptr<ComponentBuildScriptGenerator_t> componentGeneratorPtr;

    explicit RequireComponentGenerator_t
    (
        std::shared_ptr<ComponentBuildScriptGenerator_t> componentGeneratorPtr
    )
    : RequireBaseGenerator_t(componentGeneratorPtr->baseGeneratorPtr),
      componentGeneratorPtr(componentGeneratorPtr) {}
};

} // namespace ninja

#endif // LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
