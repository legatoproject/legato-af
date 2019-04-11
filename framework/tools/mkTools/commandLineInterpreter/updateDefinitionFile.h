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
    void UpdateSdefApp(ArgHandler_t& handler);
    void UpdateSdefModule(ArgHandler_t& handler);
    void UpdateSdefRenameApp(ArgHandler_t handler);
    void UpdateSdefRenameModule(ArgHandler_t& handler);
    void UpdateSdefRemoveApp(ArgHandler_t handler);
    void UpdateSdefRemoveModule(ArgHandler_t& handler);

    void UpdateAdefComponent(ArgHandler_t& handler);
    void UpdateAdefRenameComponent(ArgHandler_t& handler, model::System_t* systemPtr);
    void UpdateAdefRemoveComponent(ArgHandler_t& handler);

    void ParseSdefReadSearchPath(ArgHandler_t& handler);
}

#endif
