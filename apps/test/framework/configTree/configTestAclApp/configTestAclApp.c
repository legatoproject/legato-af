//--------------------------------------------------------------------------------------------------
/**
 * Test application for configuration tree access control lists.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"


COMPONENT_INIT
{
    // Try to get the name of the tree we're tring to read from.  This should have been supplied on
    // the command line from the first parameter.
    LE_INFO("===== Config ACL test started.");

    const char* treeName = le_arg_GetArg(0);

    LE_FATAL_IF(treeName == NULL, "Required parameter (tree name) missing.");

    // Now, attempt to create an iterator on that tree.
    char nodePath[512] = "";
    sprintf(nodePath, "%s:/cfgAclTest", treeName);

    LE_INFO("===== Reading reading from tree path: '%s'.", nodePath);

    // Read our test value from that tree, then clean up the iterator.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);
    bool limitVal = le_cfg_GetBool(iterRef, "toTheLimit", false);

    LE_INFO("===== Read limit value: %s.", limitVal ? "true" : "false");

    le_cfg_CancelTxn(iterRef);

    LE_INFO("=====  Read ACL Test on tree: %s, successful.  =====", treeName);
}
