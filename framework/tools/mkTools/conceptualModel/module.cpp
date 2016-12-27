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
  koFilePtr(NULL)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Set build environment and artifacts related to this module.
 **/
//--------------------------------------------------------------------------------------------------
void Module_t::SetBuildEnvironment(void)
{
    name = path::RemoveSuffix(path::GetLastNode(defFilePtr->path), ".mdef");
    workingDir = "modules/" + name;
    auto koFilePath = workingDir + "/" + name + ".ko";
    koFilePtr = new model::ObjectFile_t(koFilePath, defFilePtr->path);
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
