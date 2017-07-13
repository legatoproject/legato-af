//--------------------------------------------------------------------------------------------------
/**
 * @file exportedAdefGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace adefGen
{


namespace
{


// Type mapping std collections to our types to make referring to them easier.
typedef std::set<model::FileSystemObject_t*> FsObjectSet_t;
typedef std::list<model::FileSystemObject_t*> FsObjectList_t;

typedef std::list<model::Process_t*> ProcessList_t;

typedef std::map<std::string, model::ApiServerInterfaceInstance_t*> ApiServerIfInstMap_t;
typedef std::map<std::string, model::ApiClientInterfaceInstance_t*> ApiClientIfInstMap_t;

typedef std::map<std::string, model::Permissions_t> ConfigPermissionMap_t;


//--------------------------------------------------------------------------------------------------
/**
 * Structure for describing the FS objects that the application needs to import from the device.
 **/
//--------------------------------------------------------------------------------------------------
struct RequiredFsObject_t
{
    FsObjectSet_t files;
    FsObjectSet_t dirs;
    FsObjectSet_t devices;
};


//--------------------------------------------------------------------------------------------------
/**
 * Structure for recording the FS objects that have been bundled in with the application.
 .
 **/
//--------------------------------------------------------------------------------------------------
struct BundledFsObject_t
{
    FsObjectSet_t files;
    FsObjectSet_t dirs;
};


//--------------------------------------------------------------------------------------------------
/**
 * Used to flag whether to write out permission flags or not.
 **/
//--------------------------------------------------------------------------------------------------
enum class WritePerm_t
{
    Yes,
    No
};


//--------------------------------------------------------------------------------------------------
/**
 * Used to flag whether to remap a FS object into the app bundle space or just leave as-is.
 **/
//--------------------------------------------------------------------------------------------------
enum class RemapSrc_t
{
    Yes,
    No
};


}  // namespace


//--------------------------------------------------------------------------------------------------
/**
 * Write out an adef value.  If the value hasn't been set and left as a default, then do not write
 * it out.  Otherwise write out it's value now.
 *
 * @return true if the value was written, false if not.
 **/
//--------------------------------------------------------------------------------------------------
template <typename ValueType_t>
static bool GenerateValue
(
    std::ostream& defStream,
    const std::string& name,
    const ValueType_t& value,
    const std::string& indent = ""
)
//--------------------------------------------------------------------------------------------------
{
    if (value.IsSet())
    {
        defStream << indent << name << ": " << value.Get() << "\n";

        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Explicit specialization for values of std::string.  The value isn't written if the string is
 * empty.
 *
 * @return true if the value was written, false if not.
 **/
//--------------------------------------------------------------------------------------------------
template <>
bool GenerateValue
(
    std::ostream& defStream,
    const std::string& name,
    const std::string& value,
    const std::string& indent
)
//--------------------------------------------------------------------------------------------------
{
    if (value != "")
    {
        defStream << indent  << name << ": " << value << "\n";

        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Explicit specialization for values of type bool.
 *
 * @return Always returns true, because the value is always written.
 **/
//--------------------------------------------------------------------------------------------------
template <>
bool GenerateValue
(
    std::ostream& defStream,
    const std::string& name,
    const bool& value,
    const std::string& indent
)
//--------------------------------------------------------------------------------------------------
{
    defStream << indent  << name << ": " << (value ? "true" : "false") << "\n";

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write out the field definitions for all of the basic, top level fields for the application.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBasicInfo
(
    std::ostream& defStream,
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "\n";

    GenerateValue(defStream, "version", appPtr->version);

    defStream << "start: " << (appPtr->startTrigger == model::App_t::AUTO ? "auto"
                                                                          : "manual") << "\n";

    GenerateValue(defStream, "sandboxed", appPtr->isSandboxed);
    GenerateValue(defStream, "watchdogAction", appPtr->watchdogAction);
    GenerateValue(defStream, "watchdogTimeout", appPtr->watchdogTimeout);
    GenerateValue(defStream, "maxWatchdogTimeout", appPtr->maxWatchdogTimeout);
    GenerateValue(defStream, "cpuShare", appPtr->cpuShare);
    GenerateValue(defStream, "maxFileSystemBytes", appPtr->maxFileSystemBytes);
    GenerateValue(defStream, "maxMemoryBytes", appPtr->maxMemoryBytes);
    GenerateValue(defStream, "maxMQueueBytes", appPtr->maxMQueueBytes);
    GenerateValue(defStream, "maxQueuedSignals", appPtr->maxQueuedSignals);
    GenerateValue(defStream, "maxThreads", appPtr->maxThreads);
    GenerateValue(defStream, "maxSecureStorageBytes", appPtr->maxSecureStorageBytes);

    if (!appPtr->groups.empty())
    {
        defStream << "\n"
                     "groups:\n"
                     "{\n";

        for (auto group : appPtr->groups)
        {
            defStream << "    " << group << "\n";
        }

        defStream << "}\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a list of config trees and the permissions on those trees required by the application.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateConfigPermissions
(
    std::ostream& defStream,
    const ConfigPermissionMap_t& configTrees
)
//--------------------------------------------------------------------------------------------------
{
    if (!configTrees.empty())
    {
        defStream << "    configTree:\n"
                     "    {\n";

        for (auto item : configTrees)
        {
            defStream << "        ";

            if (item.second.IsWriteable())
            {
                defStream << "[w] ";
            }

            defStream << item.first << "\n";
        }

        defStream << "    }\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a filesystem object line item.  Only write the permissions for the item if we are
 * requested to.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFsObjectItem
(
    std::ostream& defStream,
    const model::FileSystemObject_t& item,
    WritePerm_t writePermissions
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "        ";

    if (writePermissions == WritePerm_t::Yes)
    {
        defStream << "["
                  << (item.permissions.IsReadable() ? "r" : "")
                  << (item.permissions.IsWriteable() ? "w" : "")
                  << (item.permissions.IsExecutable() ? "x" : "")
                  << "]   ";
    }

    defStream << item.srcPath << "   " << item.destPath << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a FS object section.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFsObjectItems
(
    std::ostream& defStream,
    const std::string& sectionName,
    const FsObjectSet_t& items,
    WritePerm_t writePermissions
)
//--------------------------------------------------------------------------------------------------
{
    if (items.empty())
    {
        return;
    }

    defStream << "\n"
                 "    " << sectionName << ":\n"
                 "    {\n";

    for (auto item : items)
    {
        GenerateFsObjectItem(defStream, *item, writePermissions);
    }

    defStream << "    }\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the ADEF requires section and it's subsections.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateRequiresSection
(
    std::ostream& defStream,
    model::App_t* appPtr,
    RequiredFsObject_t& required
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "\n"
                 "requires:\n"
                 "{\n";

    GenerateConfigPermissions(defStream, appPtr->configTrees);
    GenerateFsObjectItems(defStream, "file", required.files, WritePerm_t::No);
    GenerateFsObjectItems(defStream, "dir", required.dirs, WritePerm_t::No);
    GenerateFsObjectItems(defStream, "device", required.devices, WritePerm_t::Yes);

    defStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the ADEF bundles section and it's subsections.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBundlesSection
(
    std::ostream& defStream,
    model::App_t* appPtr,
    BundledFsObject_t bundled
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "\n"
                 "bundles:\n"
                 "{";

    GenerateFsObjectItems(defStream, "file", bundled.files, WritePerm_t::Yes);
    GenerateFsObjectItems(defStream, "dir", bundled.dirs, WritePerm_t::No);

    defStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the list of environment variables for a process section.
 **/
//--------------------------------------------------------------------------------------------------
static bool GenerateEnvVars
(
    std::ostream& defStream,
    const std::map<std::string, std::string>& envVars
)
//--------------------------------------------------------------------------------------------------
{
    if (envVars.empty())
    {
        return false;
    }


    defStream << "    envVars:\n"
                 "    {\n";

    for (auto envVar : envVars)
    {
        defStream << "        " << envVar.first << " = \"" << envVar.second << "\"\n";
    }

    defStream << "    }\n";

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the actual application run commands for the process section.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateRunSection
(
    std::ostream& defStream,
    const ProcessList_t& processes
)
//--------------------------------------------------------------------------------------------------
{
    if (processes.empty())
    {
        return;
    }

    defStream << "    run:\n"
                 "    {\n";

    for (auto proc : processes)
    {
        const std::string& name = proc->GetName();

        defStream << "        ";

        // Only generate a proc name if it's different than the executable.
        if (   (name != "")
            && (name != path::GetLastNode(proc->exePath)))
        {
            defStream << name << " = ";
        }

        // Now generate the actual command line to be executed.
        defStream << "( " << proc->exePath;

        for (auto arg : proc->commandLineArgs)
        {
            size_t spaceFound = arg.find(' ');

            if (spaceFound != std::string::npos)
            {
                defStream << " \"" << arg << "\"";
            }
            else
            {
                defStream << " " << arg;
            }
        }

        defStream << " )\n";
    }

    defStream << "    }\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the list of processes and their environments.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessesSection
(
    std::ostream& defStream,
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->processEnvs.empty())
    {
        return;
    }

    for (auto procEnv : appPtr->processEnvs)
    {
        defStream << "\n"
                     "processes:\n"
                     "{\n";

        const std::string ind = "    ";

        bool w = false;  // Has anything been written to the output?

        w |= GenerateValue(defStream, "faultAction", procEnv->faultAction, ind);
        w |= GenerateValue(defStream, "maxFileBytes", procEnv->maxFileBytes, ind);
        w |= GenerateValue(defStream, "maxCoreDumpFileBytes", procEnv->maxCoreDumpFileBytes, ind);
        w |= GenerateValue(defStream, "maxLockedMemoryBytes", procEnv->maxLockedMemoryBytes, ind);
        w |= GenerateValue(defStream, "maxFileDescriptors", procEnv->maxFileDescriptors, ind);
        w |= GenerateValue(defStream, "watchdogAction", procEnv->watchdogAction, ind);
        w |= GenerateValue(defStream, "watchdogTimeout", procEnv->watchdogTimeout, ind);
        w |= GenerateValue(defStream, "maxWatchdogTimeout", procEnv->maxWatchdogTimeout, ind);
        w |= GenerateValue(defStream, "priority", procEnv->GetStartPriority(), ind);
        w |= GenerateValue(defStream, "maxPriority", procEnv->GetMaxPriority(), ind);

        if (w)
        {
            defStream << "\n";
        }

        if (   (GenerateEnvVars(defStream, procEnv->envVars))
            && (procEnv->processes.empty() == false))
        {
            defStream << "\n";
        }

        GenerateRunSection(defStream, procEnv->processes);

        defStream << "}\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the API line in the extern requires or a provides section.
 **/
//--------------------------------------------------------------------------------------------------
template <typename ApiObject_t>
static void GenerateApiUsage
(
    std::ostream& defStream,
    const std::string& apiAlias,
    ApiObject_t apiObject,
    bool isOptional = false
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "        ";

    // Only include the alias if it differs from the default one.
    if (apiAlias != apiObject->ifPtr->apiFilePtr->defaultPrefix)
    {
        defStream << apiAlias << " = ";
    }

    // Map the file path into the binary application directory so that the api files can be bundled
    // in with the application.
    defStream << "$CURDIR/interfaces/"
              << path::GetLastNode(apiObject->ifPtr->apiFilePtr->path);

    if (isOptional)
    {
        defStream << " [optional]";
    }

    defStream << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an API requirement for a component interface that has been bound to an external source.
 **/
//--------------------------------------------------------------------------------------------------
static bool GenerateExternComponentApiUsage
(
    std::ostream& defStream,
    const model::ComponentInstance_t* componentInstPtr
)
//--------------------------------------------------------------------------------------------------
{
    bool generatedCode = false;

    for (auto clientApiInstPtr : componentInstPtr->clientApis)
    {
        auto bindingPtr = clientApiInstPtr->bindingPtr;

        if (bindingPtr->serverType != model::Binding_t::INTERNAL)
        {
            generatedCode = true;
            GenerateApiUsage(defStream,
                             bindingPtr->clientIfName,
                             clientApiInstPtr,
                             clientApiInstPtr->ifPtr->optional);
        }
    }

    return generatedCode;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an extern requires section, which is exclusively required APIs.
 *
 * @return A true if the section was actually written.  False if there are no required APIs, thus
 *         not requiring a section to be written after all.
 **/
//--------------------------------------------------------------------------------------------------
static bool GenerateExternRequiresSection
(
    std::ostream& defStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    bool generatedCode = false;
    std::stringstream substream;

    for (auto mapItem : appPtr->executables)
    {
        for (auto componentInstPtr : mapItem.second->componentInstances)
        {
            generatedCode |= GenerateExternComponentApiUsage(substream, componentInstPtr);
        }
    }

    for (auto mapItem : appPtr->externClientInterfaces)
    {
        GenerateApiUsage(substream, mapItem.first, mapItem.second, mapItem.second->ifPtr->optional);
        generatedCode = true;
    }

    if (generatedCode)
    {
        defStream << "    requires:\n"
                     "    {\n"
                  << substream.str()
                  << "    }\n";
    }

    return generatedCode;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an extern provides section, which is exclusively provided APIs.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateExternProvidesSection
(
    std::ostream& defStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->externServerInterfaces.empty())
    {
        return;
    }

    defStream << "    provides:\n"
                 "    {\n";

    for (auto mapItem : appPtr->externServerInterfaces)
    {
        GenerateApiUsage(defStream, mapItem.first, mapItem.second);
    }

    defStream << "    }\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the extern section so that the application can define the external services that it
 * requires and provides to the system it will be running on.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateExternSection
(
    std::ostream& defStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "\n"
                 "extern:\n"
                 "{\n";

    if (GenerateExternRequiresSection(defStream, appPtr))
    {
        defStream << "\n";
    }

    GenerateExternProvidesSection(defStream, appPtr);

    defStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write out a binding for the app's internal interface.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBinding
(
    std::ostream& defStream,
    const model::Binding_t* bindingPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure that this is a full component name.
    if (bindingPtr->clientIfName.find('.') != std::string::npos)
    {
        defStream << "    " << bindingPtr->clientIfName << " -> ";

        if (bindingPtr->serverType == model::Binding_t::EXTERNAL_USER)
        {
            defStream << "<" << bindingPtr->serverAgentName << ">";
        }
        else
        {
            defStream << bindingPtr->serverAgentName;
        }

        defStream << "." << bindingPtr->serverIfName << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a list of bindings for the apps internal interfaces.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBindings
(
    std::ostream& defStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    defStream << "bindings:\n"
                  "{\n";

    for (auto mapItem : appPtr->executables)
    {
        for (auto componentInstPtr : mapItem.second->componentInstances)
        {
            for (auto clientApiInstPtr : componentInstPtr->clientApis)
            {
                GenerateBinding(defStream, clientApiInstPtr->bindingPtr);
            }
        }
    }

    defStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Gather up the required/bundled FS objects for a given application and it's subcomponents.
 **/
//--------------------------------------------------------------------------------------------------
static void GatherFsObjects
(
    const model::App_t* appPtr,
    RequiredFsObject_t& required,
    BundledFsObject_t& bundled
)
//--------------------------------------------------------------------------------------------------
{
    path::Path_t stagingPath = "$builddir";
    stagingPath += appPtr->workingDir;
    stagingPath += "staging";

    auto stagingPrefix = stagingPath.str + "/";

    auto& allBundledFiles = appPtr->getTargetInfo<target::FileSystemAppInfo_t>()->allBundledFiles;

    for (auto fileObjIter = allBundledFiles.begin();
         fileObjIter != allBundledFiles.end();
         ++fileObjIter)
    {
        if (fileObjIter->destPath.compare(0, stagingPrefix.length(), stagingPrefix) != 0)
        {
            // Internal error
            throw mk::Exception_t(
                mk::format(LE_I18N("INTERNAL ERROR: Bundled file '%s' (source '%s') is outside the"
                                   " staging directory."),
                           fileObjIter->destPath, fileObjIter->srcPath)
            );
        }

        auto basePath = fileObjIter->destPath.substr(stagingPrefix.length() - 1);
        auto newPath = std::string(".") + basePath;

        bundled.files.insert(new model::FileSystemObject_t(newPath,
                                                           basePath,
                                                           fileObjIter->permissions));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Called to generate an adef suitable for exporting in a binary application redistributable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateExportedAdef
(
    model::App_t* appPtr,       ///< Generate an exported adef for this application.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string dirPath = path::Combine(buildParams.workingDir, appPtr->name);
    std::string filePath = path::Combine(dirPath, appPtr->name + ".adef");

    file::MakeDir(dirPath);

    std::ofstream defStream(filePath, std::ofstream::trunc);

    defStream << "\n"
                 "//\n"
                 "// Application definition created for the application " << appPtr->name << ".\n"
                 "// This is an auto generated definition for a binary-only application.\n"
                 "//\n"
                 "// Do not edit, doing so may cause the application to fail.\n"
                 "//\n";

    RequiredFsObject_t required;
    BundledFsObject_t bundled;

    GatherFsObjects(appPtr, required, bundled);

    GenerateBasicInfo(defStream, appPtr);
    GenerateRequiresSection(defStream, appPtr, required);
    GenerateBundlesSection(defStream, appPtr, bundled);
    GenerateProcessesSection(defStream, appPtr);
    GenerateExternSection(defStream, appPtr);
    GenerateBindings(defStream, appPtr);

}


} // namespace adefGen
