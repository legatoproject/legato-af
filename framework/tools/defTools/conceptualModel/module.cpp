//--------------------------------------------------------------------------------------------------
/**
 * @file module.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{

 std::map<std::string, Module_t*> Module_t::ModuleMap;

//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
Module_t::Module_t(parseTree::MdefFile_t *filePtr)
//--------------------------------------------------------------------------------------------------
: defFilePtr(filePtr),
  dir(path::GetContainingDir(filePtr->path)),
  moduleBuildType(Invalid),
  loadTrigger(AUTO)
//--------------------------------------------------------------------------------------------------
{
    std::string canonicalPath = path::MakeCanonical(filePtr->path);
    std::string moduleName;

    moduleName = path::RemoveSuffix(path::GetLastNode(canonicalPath), ".mdef");
    ModuleMap[moduleName] = this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a pre-existing module object for the given module name found.
 *
 * @return Pointer to the object or NULL if not found.
 **/
//--------------------------------------------------------------------------------------------------

Module_t* Module_t::GetModule
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    auto i = ModuleMap.find(name);

    if (i == ModuleMap.end())
    {
        return NULL;
    }
    else
    {
        return i->second;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set build environment and artifacts related to this module.
 **/
//--------------------------------------------------------------------------------------------------
void Module_t::SetBuildEnvironment(ModuleBuildType_t type, std::string path)
{
    if (type == Prebuilt)
    {
        name = path::RemoveSuffix(path::GetLastNode(path), ".ko");
    }
    else
    {
        name = path::RemoveSuffix(path::GetLastNode(path), ".mdef");
    }

    // Now setup build environment:
    workingDir = "modules/" + name;
    auto koFilePath = workingDir + "/" + name + ".ko";
    auto koFileObj = new model::ObjectFile_t(koFilePath, path);
    koFiles.insert(std::make_pair(path, koFileObj));
}


//--------------------------------------------------------------------------------------------------
/**
 * Set build environment and artifacts related to this sub module.
 **/
//--------------------------------------------------------------------------------------------------
void Module_t::SetBuildEnvironmentSubModule(std::string path)
{
    name = path::RemoveSuffix(path::GetLastNode(path), ".mdef");

    // Setup build environment:
    workingDir = "modules/" + name;
    auto koFilePath = workingDir + "/" + subModuleName + ".ko";
    auto koFileObj = new model::ObjectFile_t(koFilePath, path);
    koFiles.insert(std::make_pair(path, koFileObj));
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a parameter name/value pair to be passed to module's insmod command in the format <name>=<value>.
 **/
//--------------------------------------------------------------------------------------------------
void Module_t::AddParam(std::string name, std::string value)
{
    params[name] = value;
}

} // namespace modeller
