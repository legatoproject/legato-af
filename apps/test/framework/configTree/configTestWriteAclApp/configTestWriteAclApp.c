//--------------------------------------------------------------------------------------------------
/**
 * Test application for configuration tree write access control lists.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"



COMPONENT_INIT
{
    // Try to get the name of the tree we're tring to write to.  This should have been supplied on
    // the command line from the first parameter.
    LE_INFO("---- Config ACL write test started.");

    const char* treeName = le_arg_GetArg(0);

    LE_FATAL_IF(treeName == NULL, "Missing required parameter (tree name).");

    // Now, attempt to create an iterator on that tree.
    char nodePath[512] = "";
    sprintf(nodePath, "%s:/cfgAclTest", treeName);

    LE_INFO("---- Writing to tree path: '%s'.", nodePath);

    // Write our value and attempt to read it back.  Make sure it is what was written.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);

    le_cfg_SetBool(iterRef, "toTheLimit", true);
    le_cfg_CommitTxn(iterRef);

    LE_INFO("=====  Write ACL Test on tree: %s, successful.  =====", treeName);
}
