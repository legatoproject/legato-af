/**
 * @file rtos.h
 *
 * RTOS-specific information for principal model nodes (systems, apps, components, etc.)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MKTOOLS_RTOS_TARGET_INFO_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_RTOS_TARGET_INFO_H_INCLUDE_GUARD

namespace target
{

/**
 * Target-specific info for components building on RTOS
 */
class RtosComponentInfo_t : public model::TargetInfo_t
{
    public:
        std::string staticlib;
        int globalUsage;
        int componentKey;
        static int nextKey;

        RtosComponentInfo_t(const model::Component_t* componentPtr,
                             const mk::BuildParams_t& buildParams)
        : globalUsage(0), componentKey(nextKey++)
        {
            // If the library output directory has not been specified, then put each component's
            // library file under its own working directory.
            std::string baseComponentPath;

            if (buildParams.libOutputDir.empty())
            {
                baseComponentPath = path::Combine(path::Combine(buildParams.workingDir,
                                                                componentPtr->workingDir),
                                                  "obj");
            }
            // Otherwise, put the component library directly into the library output directory.
            else
            {
                baseComponentPath = buildParams.libOutputDir;
            }

            if (componentPtr->HasCOrCppCode())
            {
                staticlib = path::Combine(baseComponentPath,
                                          "component_" + componentPtr->name + ".o");
            }
            else if (componentPtr->HasJavaCode())
            {
                throw mk::Exception_t(LE_I18N("RTOS targets do not support Java"));
            }
        }
};

/**
 * Target-specific info for components building on RTOS
 */
class RtosComponentInstanceInfo_t : public model::TargetInfo_t
{
    public:
        int instanceNum;

        RtosComponentInstanceInfo_t(int instanceNum)
        : instanceNum(instanceNum)
        {
        }
};

/**
 * Target-specific info for components building on RTOS
 */
class RtosExeInfo_t : public model::TargetInfo_t
{
    public:
        /// Task name
        std::string taskName;

        /// Thread entry point name
        std::string entryPoint;

        /// Initialization function -- called in main thread before *any* executables are started.
        std::string initFunc;
};

}

#endif /* LEGATO_MKTOOLS_RTOS_TARGET_INFO_H_INCLUDE_GUARD */
