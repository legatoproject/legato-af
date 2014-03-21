
// -------------------------------------------------------------------------------------------------
/**
 *  Utility to work with a config tree from the command line.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_cfg_interface.h"
#include "le_cfgAdmin_interface.h"




#define STRING_MAX 513  // Size of the string buffer used to communicate with the config tree.
#define COMMAND_MAX 8   // Size of the command string used by the import/export command.




// -------------------------------------------------------------------------------------------------
/**
 *  Simply write the useage text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void DumpHelpText
(
    void
)
{
    printf("Usage:\n\n"
           "To read a value:\n"
           "\tconfig <tree path>\n\n"
           "To write a value:\n"
           "\tconfig <tree path> <new value>\n\n"
           "To delete a node:\n"
           "\tconfig delete <tree path>\n\n"
           "To import config data:\n"
           "\tconfig import <tree path> <file path>\n\n"
           "To export config data:\n"
           "\tconfig export <tree path> <file path>\n\n"
           "Where:\n"
           "\t<tree path>: Is a path to the tree and node to operate on.\n"
           "\t<file path>: Path to the file to import from or export to.\n"
           "\t<new value>: Is a string value to write to the config tree.\n"
           "\n"
           "\tA tree path is specified similarly to a *nix path.\n"
           "\n"
           "\tPaths can be either absolute:\n"
           "\n"
           "\t    /a/rooted/path/to/somewhere\n"
           "\n"
           "\tOr relative:\n"
           "\n"
           "\t    a/relative/path\n"
           "\t    ./another/relative/path\n"
           "\t    ../one/more/relative/path\n"
           "\n"
           "\tThe configTree supports multiple trees, a default tree is given per user.\n"
           "\tIf the config tool is run as root, then alternative trees can be specified\n"
           "\tin the path by giving a tree name, then a colon and the value path as\n"
           "\tnormal.\n"
           "\n"
           "\tAs an example, here's of the previous paths, but selecting the tree named\n"
           "\t'foo' instead of the default tree:\n"
           "\n"
           "\t    foo:/a/rooted/path/to/somewhere\n"
           "\t    foo:a/relative/path\n"
           "\t    foo:./another/relative/path\n"
           "\t    foo:../one/more/relative/path\n");
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to retreive a command line argument.  If anything goes wrong with the retreival then
 *  error messages are generated.
 *
 *  @return LE_OK if everything goes to plan.  LE_OVERFLOW if the supplied string buffer isn't large
 *          enough.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t GetArg
(
    char* bufferPtr,         ///< The buffer to copy into.
    size_t bufferSize,       ///< Size of the supplied buffer.
    size_t index,            ///< Index of the argument to retreive.
    const char* bufferName   ///< If the get fails, the argument name to report to the user.
)
{
    le_result_t result = le_arg_GetArg(index, bufferPtr, bufferSize);

    if (result == LE_OVERFLOW)
    {
        printf("%s: parameter too large.\n", bufferName);
    }
    else if (result != LE_OK)
    {
        printf("%s: Unexpected error occured.  %d: %s\n",
               bufferName,
               result,
               LE_RESULT_TXT(result));
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Given an iterator, this function will return a string that describes the type of node that the
 *  iterator is pointed at.
 */
// -------------------------------------------------------------------------------------------------
static const char* NodeTypeStr
(
    le_cfg_nodeType_t nodeType  ///< The iterator object to read.
)
{
    switch (nodeType)
    {
        case LE_CFG_TYPE_STRING:
            return "string";

        case LE_CFG_TYPE_EMPTY:
            return "empty";

        case LE_CFG_TYPE_BOOL:
            return "bool";

        case LE_CFG_TYPE_INT:
            return "int";

        case LE_CFG_TYPE_FLOAT:
            return "float";

        case LE_CFG_TYPE_STEM:
            return "stem";

        case LE_CFG_TYPE_DENIED:
            return "**DENIED**";
    }

    return "unknown";
}




// -------------------------------------------------------------------------------------------------
/**
 *  Given an iterator object, walk the tree from that location and write out the tree structure to
 *  standard out.
 */
// -------------------------------------------------------------------------------------------------
static void DumpTree
(
    le_cfg_iteratorRef_t iterRef,  ///< Write out the tree pointed to by this iterator.
    size_t indent                  ///< The amount of indentation to use for this item.
)
{
    // Note that becuse this is a recursive function, the buffer here is static in order to save on
    // stack space.  The implication here is that we then have to be careful how it is later
    // accessed.  Also, this makes the function not thread safe.  But this trade off was made as
    // this was not intended to be a multi-threaded program.
    static char strBuffer[STRING_MAX] = { 0 };

    do
    {
        // Quick and dirty way to indent the tree item.
        size_t i;

        for (i = 0; i < indent; i++)
        {
            printf(" ");
        }

        // Simply grab the name and the type of the current node.
        le_cfg_GetNodeName(iterRef, strBuffer, STRING_MAX);
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef);

        switch (type)
        {
            // It's a stem object, so mark this item as being a stem and recurse into the stem's
            // sub-items.
            case LE_CFG_TYPE_STEM:
                printf("%s/\n", strBuffer);

                le_cfg_GoToFirstChild(iterRef);
                DumpTree(iterRef, indent + 2);
                le_cfg_GoToParent(iterRef);

                // If we got back up to where we started then don't iterate the "root" node's
                // siblings.
                if (indent == 0)
                {
                    return;
                }
                break;

            // The node is empty, so simply mark it as such.
            case LE_CFG_TYPE_EMPTY:
                printf("%s<empty>\n", strBuffer);
                break;

            // The node has a different type.  So write out the name and the type.  Then print the
            // value.
            default:
                printf("%s<%s> == ", strBuffer, NodeTypeStr(le_cfg_GetNodeType(iterRef)));
                le_cfg_GetString(iterRef, "", strBuffer, STRING_MAX);
                printf("%s\n", strBuffer);
                break;
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt read a value from the tree, and write it to standard out.  If the
 *  specifed node is a stem, then the tree structure will be printed, starting at the specifed node.
 *
 *  The node to work with is retreived from command argument 0.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int ReadNodeValue
(
    void
)
{
    char nodePath[STRING_MAX] = { 0 };
    char nodeValue[STRING_MAX] = { 0 };

    // Get the node path from our command line arguments.
    if (GetArg(nodePath, STRING_MAX, 0, "Node Path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Start a read transaction at the specified node path.  Then dump the value, (if any.)
    le_cfg_iteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);

    switch (le_cfg_GetNodeType(iterRef))
    {
        case LE_CFG_TYPE_EMPTY:
            // Nothing to do here.
            break;

        case LE_CFG_TYPE_STEM:
            DumpTree(iterRef, 0);
            break;

        default:
            le_cfg_GetString(iterRef, "", nodeValue, STRING_MAX - 1);
            printf("%s\n", nodeValue);
            break;
    }

    le_cfg_DeleteIterator(iterRef);

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to write a string value to a given configTree node.
 *
 *  The path to the node to write to is given in command argument 0.
 *  The new value to write to this node is given in command argument 1.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int WriteNodeValue
(
    void
)
{
    char nodePath[STRING_MAX] = { 0 };
    char nodeValue[STRING_MAX] = { 0 };

    // Get our arguments...
    if (GetArg(nodePath, STRING_MAX, 0, "Node Path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    if (GetArg(nodeValue, STRING_MAX, 1, "New Value") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Make sure that this write isn't a delete node command...
    if (strcmp("delete", nodePath) == 0)
    {
        le_result_t result = le_cfg_QuickDeleteNode(nodeValue);

        if (result != LE_OK)
        {
            printf("Could not delete node, '%s'.  Reason, %s.\n",
                   nodeValue,
                   LE_RESULT_TXT(result));

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    // Looks like we're trying to write a value to a node.  Get the node's current type and then
    // write the requested value to that node.
    le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);
    le_cfg_nodeType_t originalType = le_cfg_GetNodeType(iterRef);

    le_cfg_SetString(iterRef, "", nodeValue);
    le_cfg_nodeType_t newType = le_cfg_GetNodeType(iterRef);

    // Now check to see if there was any conversion done.  If the original value was empty or didn't
    // exist, don't bother reporting a change.  Otherwise, if the types have changed, report this to
    // the user.
    if (   ((originalType != LE_CFG_TYPE_EMPTY) && (originalType != LE_CFG_TYPE_DENIED))
        && (originalType != newType))
    {
        printf("Converting node '%s' from %s to %s.\n",
               nodePath,
               NodeTypeStr(originalType),
               NodeTypeStr(newType));
    }

    // Finally, commit the value update.  If that fails, report that to the user and to the calling
    // process.  Otherwise report success.
    le_result_t result = le_cfg_CommitWrite(iterRef);
    if (result != LE_OK)
    {
        printf("Could not write to node, '%s'.  Reason, %s.\n",
               nodePath,
               LE_RESULT_TXT(result));

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function checks the supplied result value and generates an error message for the user, (if
 *  required.)
 */
// -------------------------------------------------------------------------------------------------
static void ImportExportFailMessage
(
    le_result_t result,        ///< The result value we're checking.
    const char* operationPtr,  ///< Name of the attempted operation.
    const char* filePathPtr,   ///< The path the file involved in the import/export.
    const char* nodePathPtr    ///< The path to the node in the config tree.
)
{
    if (result == LE_OK)
    {
        return;
    }

    printf("%s failure, %d: %s.\nFile Path: %s\nNode Path: %s",
           operationPtr,
           result,
           LE_RESULT_TXT(result),
           filePathPtr,
           nodePathPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Function called to handling importing and exporting tree data from/to a file in the file system.
 *
 *  Command line parameters used:
 *     0: The command, can be either "import" or "export".
 *     1: The path to the node in the configTree.
 *     2: Path to the file in the filesystem.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int ImportExportNode
(
    void
)
{
    char command[COMMAND_MAX] = { 0 };
    char filePath[STRING_MAX] = { 0 };
    char nodePath[STRING_MAX] = { 0 };

    // Extract our command line parameters, bail on the first sign of trouble.
    if (GetArg(command, COMMAND_MAX, 0, "Command") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    if (GetArg(nodePath, STRING_MAX, 1, "Node Path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    if (GetArg(filePath, STRING_MAX, 2, "File Path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Ditch any relative paths and convert the path to an absolute one.
    char absPath[PATH_MAX] = { 0 };
    realpath(filePath, absPath);

    int exitValue = EXIT_SUCCESS;

    // Ok, are we importing or exporting?
    if (strcmp("import", command) == 0)
    {
        // Importing, co create a write transaction, and attempt the import.  If that fails for any
        // reason, report it to the user.  Otherwise stay silent and report success or failure in
        // our result code.
        le_cfg_iteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);
        le_result_t result = le_cfgAdmin_ImportTree(iterRef, absPath, nodePath);

        if (result == LE_OK)
        {
            // Looks like the read itseful was successful, attempt to commit the data to the tree.
            result = le_cfg_CommitWrite(iterRef);
            ImportExportFailMessage(result, "Import", filePath, nodePath);

            exitValue = result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        else
        {
            ImportExportFailMessage(result, "Import", filePath, nodePath);
            le_cfg_DeleteIterator(iterRef);

            exitValue = EXIT_FAILURE;
        }
    }
    else if (strcmp("export", command) == 0)
    {
        // We're exporting data from the tree.  Create a read transaction and dump the data.  If
        // anything went wrong, dump it to stdandard out.
        le_cfg_iteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);
        le_result_t result = le_cfgAdmin_ExportTree(iterRef, absPath, nodePath);

        ImportExportFailMessage(result, "Export", filePath, nodePath);

        le_cfg_DeleteIterator(iterRef);

        exitValue = result == LE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        // No idea what the user is trying to do.  Report the error, and dump the usage text.
        printf("Error, unknown command, %s.\n\n", command);
        DumpHelpText();
        exitValue = EXIT_FAILURE;
    }

    return exitValue;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the component.  This initializer will extract the number of commandline arguments
 *  given to the executable and determine what operation to perform.  Once that is done, we exit
 *  and report success or failure to the process that started the executable.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Make sure that our connection to the config tree is initialized.
    le_cfg_Initialize();
    le_cfgAdmin_Initialize();

    // Figure out what the user is trying to do.
    switch (le_arg_NumArgs())
    {
        case 1:
            exit(ReadNodeValue());

        case 2:
            exit(WriteNodeValue());

        case 3:
            exit(ImportExportNode());

        // At this point we know that the wrong number of params have been supplied, so report
        // failure and dump the usage text to standard out.
        default:
            DumpHelpText();
            exit(EXIT_FAILURE);
    }
}
