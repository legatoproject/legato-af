/**
 * @file linux.h
 *
 * Linux-specific information for principal model nodes (systems, apps, components, etc.)
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MKTOOLS_LINUX_TARGET_INFO_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_LINUX_TARGET_INFO_H_INCLUDE_GUARD

namespace target
{

/**
 * Target-specific info for nodes building on systems with a filesystem.
 */
class FileSystemInfo_t : public model::TargetInfo_t
{
    public:
        model::FileSystemObjectSet_t allBundledFiles;

        FileSystemInfo_t() {}
};

/**
 * Target-specific info for components building on Linux
 */

class LinuxComponentInfo_t : public model::TargetInfo_t
{
    public:
        std::string lib;

        LinuxComponentInfo_t(const model::Component_t* componentPtr,
                             const mk::BuildParams_t& buildParams)
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
                lib = path::Combine(baseComponentPath,
                                    "libComponent_" + componentPtr->name + ".so");
            }
            else if (componentPtr->HasJavaCode())
            {
                lib = path::Combine(baseComponentPath,
                                    "libComponent_" + componentPtr->name + ".jar");
            }
        }
};

}

#endif /* LEGATO_MKTOOLS_LINUX_TARGET_INFO_H_INCLUDE_GUARD */
