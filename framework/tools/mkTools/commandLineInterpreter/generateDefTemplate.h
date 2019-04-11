//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_CLI_GENERATEDEFTEMPLATE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CLI_GENERATEDEFTEMPLATE_H_INCLUDE_GUARD

namespace defs
{
    void GenerateSdefTemplate(ArgHandler_t handler);
    void GenerateAdefTemplate(ArgHandler_t handler);
    void GenerateCdefTemplate(ArgHandler_t& handler);
    void GenerateMdefTemplate(ArgHandler_t handler);
}

#endif
