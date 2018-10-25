/**
 * @file generators.h
 *
 * Common interface definition for all mkTools generators.
 */

#ifndef LEGATO_MKTOOLS_GENERATORS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_GENERATORS_H_INCLUDE_GUARD

namespace generator
{

/**
 * Type of component file generators
 */
typedef void (*ComponentGenerator_t)(model::Component_t* componentPtr,
                                     const mk::BuildParams_t& buildParams);

/**
 * Type of executable file generators
 */
typedef void (*ExeGenerator_t)(model::Exe_t* exePtr,
                               const mk::BuildParams_t& buildParams);

/**
 * Type of app file generators
 */
typedef void (*AppGenerator_t)(model::App_t* appPtr,
                               const mk::BuildParams_t& buildParams);

/**
 * Type of module file generators
 */
typedef void (*ModuleGenerator_t)(model::Module_t* modulePtr,
                                  const mk::BuildParams_t& buildParams);

/**
 * Type of system file generators
 */
typedef void (*SystemGenerator_t)(model::System_t* systemPtr,
                                  const mk::BuildParams_t& buildParams);

/**
 * Run all generators in a collection on a model
 */
template<class Model, class Generator>
void RunAllGenerators(std::map<std::string, Generator> generatorMap,
                      Model* modelPtr,
                      const mk::BuildParams_t& buildParams)
{

    auto generatorIter = generatorMap.find(buildParams.osType);

    if (generatorMap.end() == generatorIter)
    {
        std::string errorMsg =
            mk::format(LE_I18N("Unknown OS type '%s'.  Options are:"), buildParams.osType);
        for (auto &osType : generatorMap)
        {
            errorMsg += "\n    " + osType.first;
        }
        throw mk::Exception_t(errorMsg);
    }

    for (auto generatorFuncIter = generatorIter->second;
         (*generatorFuncIter);
         ++generatorFuncIter)
    {
        (*generatorFuncIter)(modelPtr, buildParams);
    }
}

/**
 * Adaptor to run a component generator on all components in an app.
 */
template<ComponentGenerator_t ComponentGenerator>
void ForAllComponents
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
{
    for (auto componentPtr : appPtr->components)
    {
        ComponentGenerator(componentPtr, buildParams);
    }
}

/**
 * Adaptor to run a component generator on all components in an executable.
 */
template<ComponentGenerator_t ComponentGenerator>
void ForAllComponents
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
{
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        ComponentGenerator(componentInstancePtr->componentPtr, buildParams);
    }
}

/**
 * Adaptor to run an app generator on all apps in a system.
 */
template<AppGenerator_t AppGenerator>
void ForAllApps
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    for (auto appMapEntry : systemPtr->apps)
    {
        AppGenerator(appMapEntry.second, buildParams);
    }
}

}

#endif
