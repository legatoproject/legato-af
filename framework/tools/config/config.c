
// -------------------------------------------------------------------------------------------------
/**
 *  @file config.c
 *
 *  Utility to work with a config tree from the command line.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_cfg_interface.h"
#include "le_cfgAdmin_interface.h"




/// Size of the string buffer used to communicate with the config tree.
#define STRING_MAX 513




/// Size of the command string used by the import/export command.
#define COMMAND_MAX 16




/// Maximum expected size of a config tree name.
#define TREE_NAME_MAX 65




/// Name used to launch this program.
static char ProgramName[STRING_MAX] = { 0 };




// -------------------------------------------------------------------------------------------------
/**
 *  Indicies of the various command line parameters expected by the various sub-commands.
 */
// -------------------------------------------------------------------------------------------------
typedef enum ParamIndices
{
    PARAM_COMMAND_ID        = 0,  ///< Main command index.

    PARAM_GET_NODE_PATH     = 1,  ///< The path the read is occuring on.

    PARAM_SET_NODE_PATH     = 1,  ///< Path the write is occuring on.
    PARAM_SET_VALUE         = 2,  ///< Value being written to the node in question.
    PARAM_SET_TYPE          = 3,  ///< If specified, the type of value being written.

    PARAM_IMP_EXP_NODE_PATH = 1,  ///< Path to the node being imported or exported.
    PARAM_IMP_EXP_FILE_PATH = 2,  ///< The path in the filesystem the import/export is from/to.

    PARAM_DEL_NODE_PATH     = 1   ///< Path to the node being deleted.
}
ParamIndices_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Simply write the useage text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void DumpHelpText
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    printf("Usage:\n\n"
           "To read a value:\n"
           "\t%s get <tree path>\n\n"
           "To write a value:\n"
           "\t%s set <tree path> <new value> [<type>]\n\n"
           "To delete a node:\n"
           "\t%s delete <tree path>\n\n"
           "To import config data:\n"
           "\t%s import <tree path> <file path>\n\n"
           "To export config data:\n"
           "\t%s export <tree path> <file path>\n\n"
           "Where:\n"
           "\t<tree path>: Is a path to the tree and node to operate on.\n"
           "\t<file path>: Path to the file to import from or export to.\n"
           "\t<new value>: Is a string value to write to the config tree.\n"
           "\t<type>:      Is optional and mmust be one of bool, int, or string.\n"
           "\t             If type is bool, then value must be either true or false.\n"
           "\t             If unspecified, the default type will be string.\n"
           "\n"
           "\tA tree path is specified similarly to a *nix path.\n"
           "\n"
           "\tFor example:\n"
           "\n"
           "\t    /a/path/to/somewhere\n"
           "\n"
           "\tThe configTree supports multiple trees, a default tree is given per user.\n"
           "\tIf the config tool is run as root, then alternative trees can be specified\n"
           "\tin the path by giving a tree name, then a colon and the value path as\n"
           "\tnormal.\n"
           "\n"
           "\tAs an example, here's of the previous paths, but selecting the tree named\n"
           "\t'foo' instead of the default tree:\n"
           "\n"
           "\t    foo:/a/path/to/somewhere\n"
           "\n\n",
           ProgramName,
           ProgramName,
           ProgramName,
           ProgramName,
           ProgramName);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to ensuere that one of the program's requried parameters has been set.  If it isn't then
 *  a message is printed to STDERR.
 *
 *  @return LE_OK if the parameter is found and was copied ok.  LE_FAULT if the param was not
 *          supplied, or if the internal buffer was too small for the param string.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t GetRequiredParameter
(
    size_t argIndex,        ///< Index of the arg in question.
    char* argBufferPtr,     ///< The buffer to write the string to.
    size_t argBufferSize,   ///< The size of the buffer in question.
    const char* argNamePtr  ///< The name of the param we're looking for.
)
// -------------------------------------------------------------------------------------------------
{
    le_result_t result = le_arg_GetArg(argIndex, argBufferPtr, argBufferSize);

    if (result == LE_OVERFLOW)
    {
        fprintf(stderr,
                "Required parameter, %s, is too large for internal buffers.\n"
                "For more details please run:\n"
                "\t%s help\n\n",
                argNamePtr,
                ProgramName);

        return LE_FAULT;
    }
    else if (result == LE_NOT_FOUND)
    {
        fprintf(stderr,
                "Required parameter, %s, is missing.\n"
                "For more details please run:\n"
                "\t%s help\n\n",
                argNamePtr,
                ProgramName);

        return LE_FAULT;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Given an iterator, this function will return a string that describes the type of node that the
 *  iterator is pointed at.
 *
 *  @return A pointer to a name string for the given node type.
 */
// -------------------------------------------------------------------------------------------------
static const char* NodeTypeStr
(
    le_cfg_nodeType_t nodeType  ///< The iterator object to read.
)
// -------------------------------------------------------------------------------------------------
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

        case LE_CFG_TYPE_DOESNT_EXIST:
            return "** DOESN'T EXIST **";
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
    le_cfg_IteratorRef_t iterRef,  ///< Write out the tree pointed to by this iterator.
    size_t indent                  ///< The amount of indentation to use for this item.
)
// -------------------------------------------------------------------------------------------------
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
        le_cfg_GetNodeName(iterRef, "", strBuffer, STRING_MAX);
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");

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
                printf("%s<%s> == ", strBuffer, NodeTypeStr(le_cfg_GetNodeType(iterRef, "")));
                le_cfg_GetString(iterRef, "", strBuffer, STRING_MAX, "");
                printf("%s\n", strBuffer);
                break;
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the specified command line argument and get a type Id from it.
 *
 *  @return The specified type ID if successful, LE_CFG_TYPE_DOESNT_EXIST is returned if this
 *          function fails.
 */
// -------------------------------------------------------------------------------------------------
static le_cfg_nodeType_t GetNewNodeType
(
    size_t argIndex  ///< The index of the command line argument to read.
)
// -------------------------------------------------------------------------------------------------
{
    char typeName[COMMAND_MAX] = { 0 };
    const size_t typeNameSize = sizeof(typeName);

    // Let's see if a type was given.
    le_result_t result = le_arg_GetArg(argIndex, typeName, typeNameSize);

    if (result == LE_OVERFLOW)
    {
        // I don't know what was specified, but it was way too big.
        fprintf(stderr,
                "Parameter node type, '%s' is too large for internal buffers.\n"
                "For more details please run:\n"
                "\t%s help",
                typeName,
                ProgramName);

        return LE_CFG_TYPE_DOESNT_EXIST;
    }
    else if (result == LE_NOT_FOUND)
    {
        // Nothing was supplied, so go with our default.
        return LE_CFG_TYPE_STRING;
    }

    // Check the given name against what we're expecting.
    if (strncmp(typeName, "string", typeNameSize) == 0)
    {
        return LE_CFG_TYPE_STRING;
    }
    else if (strncmp(typeName, "bool", typeNameSize) == 0)
    {
        return LE_CFG_TYPE_BOOL;
    }
    else if (strncmp(typeName, "int", typeNameSize) == 0)
    {
        return LE_CFG_TYPE_INT;
    }
    else if (strncmp(typeName, "float", typeNameSize) == 0)
    {
        return LE_CFG_TYPE_FLOAT;
    }

    // Looks like we didn't get something useful, so return in error.
    fprintf(stderr, "Unexpected node type specified, '%s'\n", typeName);
    return LE_CFG_TYPE_DOESNT_EXIST;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Function to process the import and export parameters.  File path is also translated into an
 *  absolute path.
 *
 *  @return If either of the parameters is missing or too large this function will return LE_FAULT.
 *          LE_OK is returned otherwise.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t GetImpExpParams
(
    char* nodePathPtr,  ///< Buffer to hold the node path, must be at least STRING_MAX bytes in
                        ///<   size.
    char* filePathPtr   ///< Bufer to hold the file path, must be at least PATH_MAX bytes in size.
)
// -------------------------------------------------------------------------------------------------
{
    char relativePath[STRING_MAX] = { 0 };

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_IMP_EXP_NODE_PATH,
                             nodePathPtr,
                             STRING_MAX,
                             "node path") != LE_OK)
    {
        return LE_FAULT;
    }

    // Get the new value from the command line arguments.
    if (GetRequiredParameter(PARAM_IMP_EXP_FILE_PATH,
                             relativePath,
                             STRING_MAX,
                             "file path") != LE_OK)
    {
        return LE_FAULT;
    }

    // Convert the given path from a potentially relative path, to an absolute one.
    realpath(relativePath, filePathPtr);

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function takes the supplied result value and generates an error message for the user.
 */
// -------------------------------------------------------------------------------------------------
static void ReportImportExportFail
(
    le_result_t result,        ///< The result value we're checking.
    const char* operationPtr,  ///< Name of the attempted operation.
    const char* nodePathPtr,   ///< The path to the node in the config tree.
    const char* filePathPtr    ///< The path the file involved in the import/export.
)
// -------------------------------------------------------------------------------------------------
{
    fprintf(stderr,
            "%s failure, %d: %s.\nFile Path: %s\nNode Path: %s",
             operationPtr,
             result,
             LE_RESULT_TXT(result),
             filePathPtr,
             nodePathPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt read a value from the tree, and write it to standard out.  If the
 *  specifed node is a stem, then the tree structure will be printed, starting at the specifed node.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleGet
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = { 0 };
    const size_t nodePathSize = sizeof(nodePath);

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_GET_NODE_PATH, nodePath, nodePathSize, "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Start a read transaction at the specified node path.  Then dump the value, (if any.)
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);

    switch (le_cfg_GetNodeType(iterRef, ""))
    {
        case LE_CFG_TYPE_EMPTY:
            // Nothing to do here.
            break;

        case LE_CFG_TYPE_STEM:
            DumpTree(iterRef, 0);
            break;

        default:
            {
                char nodeValue[STRING_MAX] = { 0 };

                le_cfg_GetString(iterRef, "", nodeValue, STRING_MAX - 1, "");
                printf("%s\n", nodeValue);
            }
            break;
    }

    le_cfg_CancelTxn(iterRef);

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleSet
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = { 0 };
    const size_t nodePathSize = sizeof(nodePath);

    char nodeValue[STRING_MAX] = { 0 };
    const size_t nodeValueSize = sizeof(nodeValue);

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_SET_NODE_PATH, nodePath, nodePathSize, "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Get the new value from the command line arguments.
    if (GetRequiredParameter(PARAM_SET_VALUE, nodeValue, nodeValueSize, "new value") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Looks like we're trying to write a value to a node.  Get the node's current type and then
    // write the requested value to that node.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);

    le_cfg_nodeType_t originalType = le_cfg_GetNodeType(iterRef, "");
    le_cfg_nodeType_t newType = GetNewNodeType(PARAM_SET_TYPE);

    if (   (newType != originalType)
        && (originalType != LE_CFG_TYPE_DOESNT_EXIST))
    {
        printf("Converting node '%s' type from %s to %s.\n",
               nodePath,
               NodeTypeStr(originalType),
               NodeTypeStr(newType));
    }

    int result = EXIT_SUCCESS;

    switch (newType)
    {
        case LE_CFG_TYPE_STRING:
            le_cfg_SetString(iterRef, "", nodeValue);
            break;

        case LE_CFG_TYPE_BOOL:
            if (strncmp(nodeValue, "false", nodeValueSize) == 0)
            {
                le_cfg_SetBool(iterRef, "", false);
            }
            else if (strncmp(nodeValue, "true", nodeValueSize) == 0)
            {
                le_cfg_SetBool(iterRef, "", true);
            }
            else
            {
                fprintf(stderr, "Bad boolean value '%s'.\n", nodeValue);
            }
            break;

        case LE_CFG_TYPE_INT:
            le_cfg_SetInt(iterRef, "", atoi(nodeValue));
            break;

        case LE_CFG_TYPE_FLOAT:
            le_cfg_SetFloat(iterRef, "", atof(nodeValue));
            break;

        case LE_CFG_TYPE_DOESNT_EXIST:
            result = EXIT_FAILURE;
            break;

        default:
            fprintf(stderr, "Unexpected node type specified, %s.\n", NodeTypeStr(newType));
            result = EXIT_FAILURE;
            break;
    }

    // Finally, commit the value update, if the set was successful.
    if (result != EXIT_FAILURE)
    {
        le_cfg_CommitTxn(iterRef);
    }
    else
    {
        le_cfg_CancelTxn(iterRef);
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleImport
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = { 0 };
    char filePath[PATH_MAX] = { 0 };

    if (GetImpExpParams(nodePath, filePath) != LE_OK)
    {
        return EXIT_FAILURE;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);
    le_result_t result = le_cfgAdmin_ImportTree(iterRef, filePath, "");

    if (result != LE_OK)
    {
        ReportImportExportFail(result, "Import", nodePath, filePath);
        le_cfg_CancelTxn(iterRef);

        return EXIT_FAILURE;
    }

    le_cfg_CommitTxn(iterRef);
    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleExport
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = { 0 };
    char filePath[PATH_MAX] = { 0 };

    if (GetImpExpParams(nodePath, filePath) != LE_OK)
    {
        return EXIT_FAILURE;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);
    le_result_t result = le_cfgAdmin_ExportTree(iterRef, filePath, "");

    le_cfg_CancelTxn(iterRef);

    if (result != LE_OK)
    {
        ReportImportExportFail(result, "Export", nodePath, filePath);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Function called to handle deleting a node from the config tree.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleDelete
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = { 0 };
    const size_t nodePathSize = sizeof(nodePath);

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_DEL_NODE_PATH, nodePath, nodePathSize, "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Ok, delete the node.
    le_cfg_QuickDeleteNode(nodePath);
    return EXIT_SUCCESS;
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

    // Read out the program name so that we can better format our error and help messages.
    if (le_arg_GetProgramName(ProgramName, STRING_MAX, NULL) != LE_OK)
    {
        strncpy(ProgramName, "config", STRING_MAX);
    }

    // Get the name of the sub-command that the caller wants us to execute.
    char commandBuffer[COMMAND_MAX] = { 0 };
    const size_t bufferSize = sizeof(commandBuffer);

    if (GetRequiredParameter(PARAM_COMMAND_ID, commandBuffer, bufferSize, "command") != LE_OK)
    {
        exit(EXIT_FAILURE);
    }

    // Now dispatch to the appropriate sub-command.
    if (strncmp(commandBuffer, "help", bufferSize) == 0)
    {
        DumpHelpText();
        exit(EXIT_SUCCESS);
    }
    else if (strncmp(commandBuffer, "get", bufferSize) == 0)
    {
        exit(HandleGet());
    }
    else if (strncmp(commandBuffer, "set", bufferSize) == 0)
    {
        exit(HandleSet());
    }
    else if (strncmp(commandBuffer, "import", bufferSize) == 0)
    {
        exit(HandleImport());
    }
    else if (strncmp(commandBuffer, "export", bufferSize) == 0)
    {
        exit(HandleExport());
    }
    else if (strncmp(commandBuffer, "delete", bufferSize) == 0)
    {
        exit(HandleDelete());
    }
    else
    {
        fprintf(stderr,
                "Error, unrecognized command, '%s'.\n"
                "For more details please run:\n"
                "\t%s help\n\n",
                commandBuffer,
                ProgramName);

        exit(EXIT_FAILURE);
    }
}
