//--------------------------------------------------------------------------------------------------
/**
 * @file exportedAdefGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"



namespace std
{


//--------------------------------------------------------------------------------------------------
/**
 * Explicit specialization of the template std::less.  This is for use in the collection
 * std::set<FileSystemObject_t*>.
 **/
//--------------------------------------------------------------------------------------------------
template <>
struct less<model::FileSystemObject_t*>
{
    //----------------------------------------------------------------------------------------------
    /**
     * Function call operator that handles the actual comparisons of the object.
     *
     * @return true if a is < b, false otherwise.
     **/
    //----------------------------------------------------------------------------------------------
    bool operator ()
    (
        model::FileSystemObject_t* const& a,
        model::FileSystemObject_t* const& b
    )
    const
    //----------------------------------------------------------------------------------------------
    {
        // Test srcPath, is a < b?

        if (a->srcPath < b->srcPath)
        {
            return true;
        }
        else if (a->srcPath > b->srcPath)
        {
            return false;
        }

        // srcPath is equal, so test destPath.

        if (a->destPath < b->destPath)
        {
            return true;
        }
        else if (a->destPath > b->destPath)
        {
            return false;
        }

        // destPath is equal, so test the permissions.

        return a->permissions < b->permissions;
    }
};


}  // namespace std


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
    RequiredFsObject_t required
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
 * Append one list of FS objects to another.
 **/
//--------------------------------------------------------------------------------------------------
static void FsSetAppend
(
    FsObjectSet_t& dest,
    const FsObjectList_t& src,
    RemapSrc_t remap
)
//--------------------------------------------------------------------------------------------------
{
    for (auto item : src)
    {
        if (remap == RemapSrc_t::Yes)
        {
            std::string dirName;

            if (item->permissions.IsWriteable())
            {
                dirName = "writeable";
            }
            else
            {
                dirName = "read-only";
            }

            dest.insert(new model::FileSystemObject_t
                        {
                            "./" + dirName + item->destPath,
                            item->destPath,
                            item->permissions
                        });
        }
        else
        {
            dest.insert(item);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gather up the required/bundled FS objects for a single component and it's sub-components.
 **/
//--------------------------------------------------------------------------------------------------
static void GatherFsObjects
(
    const model::Component_t* componentPtr,
    RequiredFsObject_t& required,
    BundledFsObject_t& bundled
);


//--------------------------------------------------------------------------------------------------
/**
 * Gather up all the required/bundled FS objects for a component collection.
 **/
//--------------------------------------------------------------------------------------------------
template <typename CollectionType_t>
static void GatherFsObjectsFromComponents
(
    const CollectionType_t& components,
    RequiredFsObject_t& required,
    BundledFsObject_t& bundled
)
//--------------------------------------------------------------------------------------------------
{
    for (auto component : components)
    {
        GatherFsObjects(component, required, bundled);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gather up the required/bundled FS objects for a given application or component object.
 **/
//--------------------------------------------------------------------------------------------------
template <typename ModelObject_t>
static void AppendFromModelObject
(
    const ModelObject_t* modelObjectPtr,
    RequiredFsObject_t& required,
    BundledFsObject_t& bundled
)
//--------------------------------------------------------------------------------------------------
{
    FsSetAppend(required.files, modelObjectPtr->requiredFiles, RemapSrc_t::No);
    FsSetAppend(required.dirs, modelObjectPtr->requiredDirs, RemapSrc_t::No);
    FsSetAppend(required.devices, modelObjectPtr->requiredDevices, RemapSrc_t::No);

    FsSetAppend(bundled.files, modelObjectPtr->bundledFiles, RemapSrc_t::Yes);
    FsSetAppend(bundled.dirs, modelObjectPtr->bundledDirs, RemapSrc_t::Yes);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gather up the required/bundled FS objects for a given component and it's subcomponents.
 **/
//--------------------------------------------------------------------------------------------------
static void GatherFsObjects
(
    const model::Component_t* componentPtr,
    RequiredFsObject_t& required,
    BundledFsObject_t& bundled
)
//--------------------------------------------------------------------------------------------------
{
    AppendFromModelObject(componentPtr, required, bundled);
    GatherFsObjectsFromComponents(componentPtr->subComponents, required, bundled);

    if (!componentPtr->lib.empty())
    {
        size_t pos = componentPtr->lib.find("read-only/lib/");
        std::string newPath = path::Combine(".", componentPtr->lib.substr(pos));

        model::Permissions_t perm;

        perm.SetReadable();
        perm.SetExecutable();

        bundled.files.insert(new model::FileSystemObject_t { newPath, "/lib/", perm });
    }
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
    AppendFromModelObject(appPtr, required, bundled);
    GatherFsObjectsFromComponents(appPtr->components, required, bundled);

    for (auto exeIter : appPtr->executables)
    {
        model::Exe_t* exePtr = exeIter.second;

        size_t pos = exePtr->path.find("read-only/bin/");
        std::string newPath = path::Combine(".", exePtr->path.substr(pos));

        model::Permissions_t perm;

        perm.SetReadable();
        perm.SetExecutable();

        bundled.files.insert(new model::FileSystemObject_t { newPath, "/bin/", perm });
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
