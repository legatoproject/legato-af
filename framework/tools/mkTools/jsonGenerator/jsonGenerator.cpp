
//--------------------------------------------------------------------------------------------------
/**
 * @file jsonGenerator.cpp
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "value.h"
#include "jsonOut.h"


namespace json
{



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template <typename value_t>
static data::Array_t JsonArray
(
    const std::map<std::string, value_t>& container,
    std::function<data::Value_t(value_t)> converter
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t jsonValues;

    for (const auto& value : container)
    {
        auto newValue = converter(value.second);
        jsonValues.push_back(newValue);
    }

    return jsonValues;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template <typename value_t>
static data::Array_t JsonArray
(
    const std::set<value_t>& container,
    std::function<data::Value_t(value_t)> converter
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t jsonValues;

    for (const auto& value : container)
    {
        auto newValue = converter(value);
        jsonValues.push_back(newValue);
    }

    return jsonValues;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template <typename value_t>
static data::Array_t JsonArray
(
    const std::list<value_t>& container,
    std::function<data::Value_t(const value_t)> converter
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t jsonValues;

    for (const auto& value : container)
    {
        auto newValue = converter(value);
        jsonValues.push_back(newValue);
    }

    return jsonValues;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template <typename container_t>
 data::Array_t JsonArray
(
    const container_t& container
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t jsonValues;

    for (const auto& value : container)
    {
        jsonValues.push_back(value);
    }

    return jsonValues;
}




static data::Value_t ModelComponent(model::Component_t* component);
static data::Value_t ModelApp(model::App_t* application);
static data::Value_t ModelSystem(model::System_t* systemPtr);
static data::Value_t ModelModule(model::Module_t* module);



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static struct Cache_t
{
    std::map<std::string, model::Component_t*> components;
    std::map<std::string, model::App_t*> apps;
    std::map<std::string, model::Module_t*> modules;
    std::map<std::string, model::System_t*> systems;

    std::map<std::string, std::list<parseTree::Token_t*>> tokenMap;

    inline const std::string& Append(model::Component_t* objectPtr)
    {
        const auto& str = CacheObject(components, objectPtr);
        return str;
    }

    inline const std::string& Append(model::App_t* objectPtr)
    {
        const auto& str = CacheObject(apps, objectPtr);
        return str;
    }

    inline const std::string& Append(model::Module_t* objectPtr)
    {
        const auto& str = CacheObject(modules, objectPtr);
        return str;
    }

    inline const std::string& Append(model::System_t* objectPtr)
    {
        const auto& str = CacheObject(systems, objectPtr);
        return str;
    }

    inline void Append(parseTree::Token_t* firstTokenPtr, parseTree::Token_t* lastTokenPtr)
    {
        auto currentPtr = firstTokenPtr;

        if (currentPtr != nullptr)
        {
            do
            {
                tokenMap[currentPtr->filePtr->path].push_back(currentPtr);
                currentPtr = currentPtr->nextPtr;
            }
            while (   (currentPtr != lastTokenPtr)
                   && (currentPtr != nullptr));
        }

        if (currentPtr != nullptr)
        {
            tokenMap[currentPtr->filePtr->path].push_back(currentPtr);
        }
    }

    inline void Append(const parseTree::DefFile_t* defFile)
    {
        for (const auto& section : defFile->sections)
        {
            Append(section->firstTokenPtr, section->lastTokenPtr);
        }

        for (const auto& iter : defFile->includedFiles)
        {
            Append(iter.second->firstTokenPtr, iter.second->lastTokenPtr);
        }
    }

    data::Array_t Components() const
    {
        return JsonArray<model::Component_t*>(components, ModelComponent);
    }

    data::Array_t Apps() const
    {
        return JsonArray<model::App_t*>(apps, ModelApp);
    }

    data::Array_t Modules() const
    {
        return JsonArray<model::Module_t*>(modules, ModelModule);
    }

    data::Array_t Systems() const
    {
        return JsonArray<model::System_t*>(systems, ModelSystem);
    }

    data::Object_t TokenMap() const
    {
        data::Object_t tokenCollection;

        for (const auto& entry : tokenMap)
        {
            tokenCollection[entry.first] = JsonArray<parseTree::Token_t*>(entry.second,
                [](parseTree::Token_t* token)
                {
                    return data::Object_t
                        {
                            { "type", token->TypeName() },
                            { "line", (int)token->line },
                            { "column", (int)token->column },
                            { "text", token->text }
                        };
                });
        }

        return tokenCollection;
    }

    private:
        //------------------------------------------------------------------------------------------
        /**
         * .
         **/
        //------------------------------------------------------------------------------------------
        template <typename object_t>
        const std::string& CacheObject
        (
            std::map<std::string, object_t>& collection,
            const object_t objectPtr
        )
        //------------------------------------------------------------------------------------------
        {
            const std::string& path = objectPtr->defFilePtr->path;

            if (collection.find(path) == collection.end())
            {
                collection.insert({path, objectPtr});
            }

            return objectPtr->name;
        }
}
Cache;



static data::Value_t ModelBuildParams(const mk::BuildParams_t& buildParams);



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template <typename value_t>
static void AppendOptional
(
    data::Object_t& object,
    const std::string& name,
    const value_t& value,
    const std::string& defaultValue
)
//--------------------------------------------------------------------------------------------------
{
    if (value.IsSet())
    {
        object.insert({ name, value.Get() });
    }
    else
    {
        object.insert({ name, defaultValue });
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelBinding
(
    model::Binding_t* bindingPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto EndPointStr = [](model::Binding_t::EndPointType_t type) -> std::string
        {
            switch (type)
            {
                case model::Binding_t::EndPointType_t::INTERNAL:
                    return "internal";

                case model::Binding_t::EndPointType_t::EXTERNAL_APP:
                    return "app";

                case model::Binding_t::EndPointType_t::EXTERNAL_USER:
                    return "user";

                default:
                    break;
            }

            return "";
        };

    auto BindSide = [&EndPointStr](model::Binding_t::EndPointType_t type,
                                  std::string& agentName,
                                  std::string& ifName)
        {
            return data::Object_t
                {
                    { "type", EndPointStr(type) },
                    { "agent", agentName },
                    { "interface", ifName }
                };
        };

    if (bindingPtr == nullptr)
    {
        return data::Object_t {};
    }

    return data::Object_t
        {
            {
                "client",
                BindSide(bindingPtr->clientType,
                         bindingPtr->clientAgentName,
                         bindingPtr->clientIfName)
            },

            {
                "server",
                BindSide(bindingPtr->serverType,
                         bindingPtr->serverAgentName,
                         bindingPtr->serverIfName)
            }
        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelBindings
(
    const std::map<std::string, model::Binding_t*>& bindings
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t jsonBindInfo;

    for (const auto& bindIter : bindings)
    {
        jsonBindInfo.push_back(ModelBinding(bindIter.second));
    }

    return jsonBindInfo;
}



//static data::Value_t ModelApiClientIfInstance
//(
//    model::ApiClientInterface_t* ifPtr
//)
//{
//    return data::Object_t
//        {
//            { "name", ifPtr->internalName },
//            { "path", ifPtr->apiFilePtr->path },
//            { "manualStart", ifPtr->manualStart },
//            { "optional", ifPtr->optional }
//        };
//}
//
//
//
//static data::Value_t ModelApiServerIfInstance
//(
//    model::ApiServerInterface_t* ifPtr
//)
//{
//    return data::Object_t
//        {
//            { "name", ifPtr->internalName },
//            { "path", ifPtr->apiFilePtr->path },
//            { "manualStart", ifPtr->manualStart },
//            { "async", ifPtr->async }
//        };
//}



//static data::Value_t ModelApiClientIfInstances
//(
//    const std::map<std::string, model::ApiClientInterfaceInstance_t*>& clientInterfaces
//)
//{
//    data::Array_t ifArray;
//
//    for (const auto& iter : clientInterfaces)
//    {
//        const auto ifPtr = iter.second;
//        data::Object_t jsonInfo =
//            {
//                { "name", ifPtr->name },
//                { "interface", ModelApiClientIfInstance(ifPtr->ifPtr) },
//                { "binding", ModelBinding(ifPtr->bindingPtr) }
//            };
//
//        ifArray.push_back(jsonInfo);
//    }
//
//    return ifArray;
//}
//
//
//
//static data::Value_t ModelApiServerIfInstances
//(
//    const std::map<std::string, model::ApiServerInterfaceInstance_t*>& serverInterfaces
//)
//{
//    data::Array_t ifArray;
//
//    for (const auto& iter : serverInterfaces)
//    {
//        const auto ifPtr = iter.second;
//        data::Object_t jsonInfo =
//            {
//                { "name", ifPtr->name },
//                { "interface", ModelApiServerIfInstance(ifPtr->ifPtr) }
//            };
//
//        ifArray.push_back(jsonInfo);
//    }
//
//    return ifArray;
//}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelFilePtrSet
(
    const model::FileObjectPtrSet_t& files
)
//--------------------------------------------------------------------------------------------------
{
    data::Array_t fileInfoArray;

    for (const auto& filePtr : files)
    {
        auto fileInfo = data::Object_t
            {
                { "source", filePtr->srcPath },
                { "dest", filePtr->destPath },

                {
                    "permissions",
                    data::Object_t
                    {
                        { "read", filePtr->permissions.IsReadable() },
                        { "write", filePtr->permissions.IsWriteable() },
                        { "execute", filePtr->permissions.IsExecutable() },
                    }
                }
            };

        fileInfoArray.push_back(fileInfo);
    }

    return fileInfoArray;
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelComponent
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    Cache.Append(componentPtr->defFilePtr);

    return data::Object_t
        {
            { "name", componentPtr->name },
            { "path", componentPtr->defFilePtr->path },

            {
                "sources",
                data::Object_t
                {
                    {
                        "c",
                        JsonArray<model::ObjectFile_t*>(componentPtr->cObjectFiles,
                            [](model::ObjectFile_t* objPtr)
                            {
                                return objPtr->sourceFilePath;
                            })
                    },

                    {
                        "cxx",
                        JsonArray<model::ObjectFile_t*>(componentPtr->cxxObjectFiles,
                            [](model::ObjectFile_t* objPtr)
                            {
                                return objPtr->sourceFilePath;
                            })
                    }
                }
            },

            { "staticLibs", JsonArray(componentPtr->staticLibs) },
            { "externalBuild", JsonArray(componentPtr->externalBuildCommands) },

            {
                "components",
                JsonArray<model::Component_t::ComponentProvideHeader_t>(componentPtr->subComponents,
                    [](model::Component_t::ComponentProvideHeader_t subComponentPtr)
                    {
                        return subComponentPtr.componentPtr->name;
                    })
            },

            {
                "modules",
                JsonArray<model::Module_t::ModuleInfoOptional_t>(componentPtr->requiredModules,
                    [](model::Module_t::ModuleInfoOptional_t moduleRef)
                    {
                        return moduleRef.tokenPtr->text;
                    })
            },

            {
                "compiler",
                data::Object_t
                {
                    {
                        "flags",
                        data::Object_t
                        {
                            { "cFlags", JsonArray(componentPtr->cFlags) },
                            { "cxxFlags", JsonArray(componentPtr->cxxFlags) },
                            { "ldFlags", JsonArray(componentPtr->ldFlags) }
                        }
                    },
                }
            },

            {
                "bundled",
                data::Object_t
                {
                    { "files", ModelFilePtrSet(componentPtr->bundledFiles) },
                    { "dirs", ModelFilePtrSet(componentPtr->bundledDirs) }
                }
            },

            {
                "required",
                data::Object_t
                {
                    { "files", ModelFilePtrSet(componentPtr->requiredFiles) },
                    { "dirs", ModelFilePtrSet(componentPtr->requiredDirs) },
                    { "devices", ModelFilePtrSet(componentPtr->requiredDevices) }
                }
            },

            {
                "api",
                data::Object_t
                {
                    {
                        "types",
                        JsonArray<model::ApiTypesOnlyInterface_t*>(componentPtr->typesOnlyApis,
                            [](model::ApiTypesOnlyInterface_t* apiPtr)
                            {
                                return data::Object_t
                                    {
                                        { "name", apiPtr->internalName },
                                        { "path", apiPtr->apiFilePtr->path }
                                    };
                            })
                    },

                    //{ "provides", ModelApiServerIfInstances(componentPtr->serverApis) },
                    //{ "requires", ModelApiClientIfInstances(componentPtr->clientApis) }
                }
            },
        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelApp
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    Cache.Append(appPtr->defFilePtr);

    return data::Object_t
        {
            { "name", appPtr->name },
            { "path", appPtr->defFilePtr->path },
            { "version", appPtr->version },
            { "isSandboxed", appPtr->isSandboxed },

            {
                "startTrigger",
                appPtr->startTrigger == model::App_t::MANUAL ? "manual" : "auto"
            },

            { "isPreBuilt", appPtr->isPreBuilt },
            { "preloadedMd5", appPtr->preloadedMd5 },

            {
                "processEnvs",
                JsonArray<model::ProcessEnv_t*>(appPtr->processEnvs,
                        [](model::ProcessEnv_t* processEnvPtr)
                        {
                            return data::Object_t
                                {
                                };
                        })
            },

            { "groups", JsonArray(appPtr->groups) },

            // configTrees

            {
                "components",
                JsonArray<model::Component_t*>(appPtr->components,
                    [](model::Component_t* componentPtr)
                    {
                        return Cache.Append(componentPtr);
                    })
            },

            {
                "bundled",
                data::Object_t
                {
                    { "files", ModelFilePtrSet(appPtr->bundledFiles) },
                    { "dirs", ModelFilePtrSet(appPtr->bundledDirs) },
                    { "binaries", ModelFilePtrSet(appPtr->bundledBinaries) }
                }
            },

            {
                "required",
                data::Object_t
                {
                    { "files", ModelFilePtrSet(appPtr->requiredFiles) },
                    { "dirs", ModelFilePtrSet(appPtr->requiredDirs) },
                    { "devices", ModelFilePtrSet(appPtr->requiredDevices) }
                }
            },

            {
                "modules",
                JsonArray<model::Module_t::ModuleInfoOptional_t>(appPtr->requiredModules,
                    [](model::Module_t::ModuleInfoOptional_t moduleInfo)
                    {
                        return data::Object_t
                            {
                                { "optional", moduleInfo.isOptional },
                                { "name", moduleInfo.tokenPtr->text }
                            };
                    })
            },

            {
                "limits",
                data::Object_t
                {
                    { "cpuShare", (int)appPtr->cpuShare.Get() },
                    { "maxFileSystemBytes", (int)appPtr->maxFileSystemBytes.Get() },
                    { "maxMemoryBytes", (int)appPtr->maxMemoryBytes.Get() },
                    { "maxMQueueBytes", (int)appPtr->maxMQueueBytes.Get() },
                    { "maxQueuedSignals", (int)appPtr->maxQueuedSignals.Get() },
                    { "maxThreads", (int)appPtr->maxThreads.Get() },
                    { "maxSecureStorageBytes", (int)appPtr->maxSecureStorageBytes.Get() }
                }
            },

            {
                "watchdog",
                [](model::App_t* appPtr)
                {
                    data::Object_t watchdogValues;
                    AppendOptional(watchdogValues, "action", appPtr->watchdogAction, "");
                    AppendOptional(watchdogValues, "timeout", appPtr->watchdogTimeout, "");
                    AppendOptional(watchdogValues, "maxTimeout", appPtr->maxWatchdogTimeout, "");
                    return watchdogValues;
                }
                (appPtr)
            },

            {
                "interfaces",
                data::Object_t
                {
                    {
                        "preBuilt",
                        data::Object_t
                        {
                            //{"clients",
                            //    ModelApiClientIfInstances(appPtr->preBuiltServerInterfaces) },
                            //{ "servers",
                            //    ModelApiServerIfInstances(appPtr->preBuiltClientInterfaces) }
                        }
                    },

                    {
                        "extern",
                        data::Object_t
                        {
                            //{ "clients",
                            //    ModelApiClientIfInstances(appPtr->externServerInterfaces) },
                            //{ "servers",
                            //    ModelApiServerIfInstances(appPtr->externClientInterfaces) }
                        }
                    }
                }
            }
        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelSystem
(
    model::System_t* systemPtr
)
{
    Cache.Append(systemPtr->defFilePtr);

    return data::Object_t
        {
            { "name", systemPtr->name },
            { "path", systemPtr->defFilePtr->path },
            { "watchdogKick", systemPtr->externalWatchdogKick },

            {
                "apps", JsonArray<model::App_t*>(systemPtr->apps,
                    [](model::App_t* appPtr)
                    {
                        return Cache.Append(appPtr);
                    })
            },

            {
                "commands",
                JsonArray<model::Command_t*>(systemPtr->commands,
                    [](model::Command_t* commandPtr)
                    {
                        return data::Object_t
                            {
                                { "name", commandPtr->name },
                                { "exePath", commandPtr->exePath }
                            };
                    })
            },

            {
                "modules",
                JsonArray< model::Module_t::ModuleInfoOptional_t>(systemPtr->modules,
                    []( model::Module_t::ModuleInfoOptional_t moduleInfo)
                    {
                        return data::Object_t
                            {
                                { "optional", moduleInfo.isOptional },
                                { "info", ModelModule(moduleInfo.modPtr) }
                            };
                    })
            },

            {
                "users",
                JsonArray<model::User_t*>(systemPtr->users,
                    [](model::User_t* userPtr)
                    {
                        return data::Object_t
                            {
                                { "name", userPtr->name },
                                { "bindings", ModelBindings(userPtr->bindings) }
                            };
                    })
            }

        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelModule
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    Cache.Append(modulePtr->defFilePtr);

    return data::Object_t { { "name", modulePtr->name } };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelBuildParams
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    return data::Object_t
        {
            { "beVerbose", buildParams.beVerbose },
            { "jobCount", buildParams.jobCount },
            { "target", buildParams.target },
            { "codeGenOnly", buildParams.codeGenOnly },
            { "isStandAloneComp", buildParams.isStandAloneComp },
            { "binPack", buildParams.binPack },
            { "noPie", buildParams.noPie },

            {
                "args",
                [&buildParams]()
                {
                    data::Array_t args;

                    for (int i = 0; i < buildParams.argc; ++i)
                    {
                        args.push_back(buildParams.argv[i]);
                    }

                    return args;
                }
                ()
            },

            {
                "search",
                data::Object_t
                {
                    { "interfaceDirs", JsonArray(buildParams.interfaceDirs) },
                    { "moduleDirs", JsonArray(buildParams.moduleDirs) },
                    { "appDirs", JsonArray(buildParams.appDirs) },
                    { "componentDirs", JsonArray(buildParams.componentDirs) },
                    { "sourceDirs", JsonArray(buildParams.sourceDirs) }
                }
            },

            {
                "compiler",
                data::Object_t
                {
                    {
                        "flags",
                        data::Object_t
                        {
                            { "cFlags", buildParams.cFlags },
                            { "cxxFlags", buildParams.cxxFlags },
                            { "ldFlags", buildParams.ldFlags }
                        }
                    },

                    { "crossToolPaths", JsonArray(buildParams.crossToolPaths) },
                    { "cCompilerPath", buildParams.cCompilerPath },
                    { "cxxCompilerPath", buildParams.cxxCompilerPath },
                    { "toolChainDir", buildParams.toolChainDir },
                    { "toolChainPrefix", buildParams.toolChainPrefix },
                    { "sysrootDir", buildParams.sysrootDir },
                    { "stripPath", buildParams.stripPath },
                    { "objcopyPath", buildParams.objcopyPath },
                    { "readelfPath", buildParams.readelfPath },
                    { "compilerCachePath", buildParams.compilerCachePath },
                    { "linkerPath", buildParams.linkerPath },
                    { "archiverPath", buildParams.archiverPath },
                    { "assemblerPath", buildParams.assemblerPath }
                }
            },

            {
                "directories",
                data::Object_t
                {
                    { "libOutputDir", buildParams.libOutputDir },
                    { "outputDir", buildParams.outputDir },
                    { "workingDir", buildParams.workingDir },
                    { "debugDir", buildParams.debugDir }
                }
            },

            {
                "signing",
                data::Object_t
                {
                    { "privKey", buildParams.privKey },
                    { "pubCert", buildParams.pubCert },
                    { "signPkg", buildParams.signPkg }
                }
            }
        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
static data::Value_t ModelDocument
(
    const Cache_t& modelObjects,
    const mk::BuildParams_t& buildParams,
    const std::list<std::string>& errors
)
//--------------------------------------------------------------------------------------------------
{
    return data::Object_t
        {
            { "version", "1" },

            { "buildParams", ModelBuildParams(buildParams) },
            { "errors", data::Array_t {} },

            {
                "model",
                data::Object_t
                {
                    { "systems", modelObjects.Systems() },
                    { "apps", modelObjects.Apps() },
                    { "components", modelObjects.Components() },
                    { "modules", modelObjects.Modules() },
                }
            },

            { "tokenMap", Cache.TokenMap() }
        };
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
void GenerateModel
(
    std::ostream& out,
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> errors;

    Cache.Append(componentPtr);
    out << ModelDocument(Cache, buildParams, errors);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
void GenerateModel
(
    std::ostream& out,
    model::App_t* applicationPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> errors;

    Cache.Append(applicationPtr);
    out << ModelDocument(Cache, buildParams, errors);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
void GenerateModel
(
    std::ostream& out,
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> errors;

    Cache.Append(systemPtr);
    out << ModelDocument(Cache, buildParams, errors);
}



//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
void GenerateModel
(
    std::ostream& out,
    model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> errors;

    Cache.Append(modulePtr);
    out << ModelDocument(Cache, buildParams, errors);
}



} // namespace json
