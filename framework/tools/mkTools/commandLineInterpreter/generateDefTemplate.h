//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_CLI_GENERATEDEFTEMPLATE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CLI_GENERATEDEFTEMPLATE_H_INCLUDE_GUARD

namespace defs
{
    void GenerateSystemTemplate(ArgHandler_t& handler);
    void GenerateApplicationTemplate(ArgHandler_t& handler);
    void GenerateComponentTemplate(ArgHandler_t& handler);
    void GenerateModuleTemplate(ArgHandler_t& handler);
}

#endif
