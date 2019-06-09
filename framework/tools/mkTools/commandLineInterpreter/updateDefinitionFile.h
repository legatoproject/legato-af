//--------------------------------------------------------------------------------------------------
/**
 * @file updateDefinitionFile.h
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_CLI_UPDATEDEFINITIONFILE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CLI_UPDATEDEFINITIONFILE_H_INCLUDE_GUARD

namespace updateDefs
{
    void ParseSdefReadSearchPath(ArgHandler_t& handler);
    void ParseSdefUpdateItem (ArgHandler_t& handler);
    void UpdateDefinitionFile(std::string sdefFilePath, std::string backupDefFilePath,
                              std::vector<ArgHandler_t::LinePosition_t>& writePos);
    void EvaluateAdefGetEditLinePosition (ArgHandler_t& handler, model::System_t* systemPtr);
}

#endif
