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
: defFilePtr(filePtr)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Set path to module binary and build artifacts related to this module.
 **/
//--------------------------------------------------------------------------------------------------
void Module_t::SetPath(std::string modulePath)
{
    path = modulePath;
    name = path::RemoveSuffix(path::GetLastNode(path), ".ko");
    workingDir = "modules/" + name;
    auto objFilePath = workingDir + "/" + name + ".ko";
    objFilePtr = new model::ObjectFile_t(objFilePath, path);
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
