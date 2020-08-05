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
    struct CompPosition_t
    {
        bool isExeMultiComp; // Bool flag to check if executables: section has multiple components.
        int foundPos;        // Position where the component is found.
        int nextPos;         // Position of the next token after the found component.
        int sectionPos;      // Start position of the line where the component is found.
        int sectionNextPos;  // Next token position of the line where the component is found.
    };

    void ParseSdefReadSearchPath(ArgHandler_t& handler);
    void ParseSdefUpdateItem (ArgHandler_t& handler);
    void UpdateDefinitionFile(ArgHandler_t& handler, std::string sdefFilePath);
    void EvaluateAdefGetEditLinePosition (ArgHandler_t& handler, model::System_t* systemPtr);
    void EvaluateCdefGetEditLinePosition (ArgHandler_t& handler, std::string cdefTestFilePath);
}

#endif
