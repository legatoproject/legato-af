//--------------------------------------------------------------------------------------------------
/**
 * @file module.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 **/
//--------------------------------------------------------------------------------------------------
Module_t::Module_t(parseTree::MdefFile_t *filePtr)
//--------------------------------------------------------------------------------------------------
: defFilePtr(filePtr),
  dir(path::GetContainingDir(filePtr->path)),
  moduleBuildType(Invalid)
//--------------------------------------------------------------------------------------------------
{
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

    workingDir = "modules/" + name;
    auto koFilePath = workingDir + "/" + name + ".ko";
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
