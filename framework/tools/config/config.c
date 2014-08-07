
// -------------------------------------------------------------------------------------------------
/**
 *  @file config/config.c
 *
 *  Utility to work with a config tree from the command line.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "jansson.h"
#include "interfaces.h"




/// Size of the string buffer used to communicate with the config tree.
#define STRING_MAX 513




/// Size of the command string used by the import/export command.
#define COMMAND_MAX 16




/// Maximum expected size of a config tree name.
#define MAX_TREE_NAME_BYTES LIMIT_MAX_USER_NAME_BYTES




/// Max size of a node name.
#define MAX_NODE_NAME (size_t)64




/// Json format string.
#define JSON_FORMAT "--format=json"




/// Json field names.
#define JSON_FIELD_TYPE "type"
#define JSON_FIELD_NAME "name"
#define JSON_FIELD_CHILDREN "children"
#define JSON_FIELD_VALUE "value"




/// Name used to launch this program.
static char ProgramName[STRING_MAX] = "";




// -------------------------------------------------------------------------------------------------
/**
 *  Indicies of the various command line parameters expected by the various sub-commands.
 */
// -------------------------------------------------------------------------------------------------
typedef enum ParamIndices
{
    PARAM_COMMAND_ID        = 0,  ///< Main command index.

    PARAM_GET_NODE_PATH     = 1,  ///< The path the read is occuring on.
    PARAM_GET_FORMAT        = 2,  ///< Expected format option.

    PARAM_SET_NODE_PATH     = 1,  ///< Path the write is occuring on.
    PARAM_SET_VALUE         = 2,  ///< Value being written to the node in question.
    PARAM_SET_TYPE          = 3,  ///< If specified, the type of value being written.

    PARAM_RN_NODE_PATH      = 1,  ///< Path to the node to rename.
    PARAM_RN_NEW_NAME       = 2,  ///< The new name for the node in question.

    PARAM_IMP_EXP_NODE_PATH = 1,  ///< Path to the node being imported or exported.
    PARAM_IMP_EXP_FILE_PATH = 2,  ///< The path in the filesystem the import/export is from/to.
    PARAM_IMP_EXP_FORMAT    = 3,  ///< Expected format option.

    PARAM_DEL_NODE_PATH     = 1,  ///< Path to the node being deleted.

    PARAM_RMTREE_NAME       = 1   ///< The name of the tree to delete.
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
           "\t%s get <tree path> [--format=json]\n\n"
           "To write a value:\n"
           "\t%s set <tree path> <new value> [<type>]\n\n"
           "To rename a node:\n"
           "\t%s rename <node path> <new name>\n\n"
           "To delete a node:\n"
           "\t%s delete <tree path>\n\n"
           "To import config data:\n"
           "\t%s import <tree path> <file path> [--format=json]\n\n"
           "To export config data:\n"
           "\t%s export <tree path> <file path> [--format=json]\n\n"
           "To list all config trees:\n"
           "\t%s list\n\n"
           "To delete a tree:\n"
           "\t%s rmtree <tree name>\n\n"
           "Where:\n"
           "\t<tree path>: Is a path to the tree and node to operate on.\n"
           "\t<tree name>: Is the name of a tree in the system, but without a path.\n"
           "\t<file path>: Path to the file to import from or export to.\n"
           "\t<new value>: Is a string value to write to the config tree.\n"
           "\t<type>:      Is optional and mmust be one of bool, int, float, or string.\n"
           "\t             If type is bool, then value must be either true or false.\n"
           "\t             If unspecified, the default type will be string.\n"
           "\n"
           "\tIf --format=json is specified, for imports, then properly formatted JSON will be\n"
           "\texpected.  If it is specified for exports, then the data will be generated as well.\n"
           "\tIt is also possible to specify JSON for the get sub-command.\n"
           "\n"
           "\tA tree path is specified similarly to a *nix path.  With the beginning slash\n"
           "\tbeing optional.\n"
           "\n"
           "\tFor example:\n"
           "\n"
           "\t    /a/path/to/somewhere\n"
           "\tor\n"
           "\t    a/path/to/somewhere\n"
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
 *  Create a JSON node reference, with name and type.
 *
 *  @return A newly allocated JSON node object for inserting in a document.
 */
// -------------------------------------------------------------------------------------------------
static json_t* CreateJsonNode
(
    const char* namePtr,  ///< Name of the new node.
    const char* typePtr   ///< configAPI type to insert.
)
// -------------------------------------------------------------------------------------------------
{
    json_t* nodePtr = json_object();

    json_object_set_new(nodePtr, JSON_FIELD_NAME, json_string(namePtr));
    json_object_set_new(nodePtr, JSON_FIELD_TYPE, json_string(typePtr));

    return nodePtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the iterator's current node and create a new JSON object from that info.
 */
// -------------------------------------------------------------------------------------------------
static json_t* CreateJsonNodeFromIterator
(
    le_cfg_IteratorRef_t iterRef  ///< The iterator to read from.
)
// -------------------------------------------------------------------------------------------------
{
    char nodeName[STRING_MAX] = "";

    le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");
    le_cfg_GetNodeName(iterRef, "", nodeName, sizeof(nodeName));

    json_t* nodePtr = CreateJsonNode(nodeName, NodeTypeStr(type));

    switch (type)
    {
        case LE_CFG_TYPE_EMPTY:
            json_object_set_new(nodePtr,
                                JSON_FIELD_TYPE,
                                json_string(NodeTypeStr(LE_CFG_TYPE_STEM)));
            json_object_set_new(nodePtr, JSON_FIELD_CHILDREN, json_array());
            break;

        case LE_CFG_TYPE_BOOL:
            json_object_set_new(nodePtr,
                                JSON_FIELD_VALUE,
                                json_boolean(le_cfg_GetBool(iterRef, "", false)));
            break;

        case LE_CFG_TYPE_STRING:
            {
                char strBuffer[STRING_MAX] = "";
                le_cfg_GetString(iterRef, "", strBuffer, STRING_MAX, "");
                json_object_set_new(nodePtr, JSON_FIELD_VALUE, json_string(strBuffer));
            }
            break;

        case LE_CFG_TYPE_INT:
            json_object_set_new(nodePtr,
                                JSON_FIELD_VALUE,
                                json_integer(le_cfg_GetInt(iterRef, "", false)));
            break;

        case LE_CFG_TYPE_FLOAT:
            json_object_set_new(nodePtr,
                                JSON_FIELD_VALUE,
                                json_real(le_cfg_GetFloat(iterRef, "", false)));
            break;

        case LE_CFG_TYPE_STEM:
        default:
            // Unknown type, nothing to do
            json_decref(nodePtr);
            nodePtr = NULL;
            break;
    }

    return nodePtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Dump tree data to a JSON object.  This function will start at the iterator's current location
 *  extract all tree data from there and insert it into the given JSON object.
 */
// -------------------------------------------------------------------------------------------------
static void DumpTreeJSON
(
    le_cfg_IteratorRef_t iterRef,  ///< Read the tree data from this iterator.
    json_t* jsonObject             ///< JSON object to hold the tree data.
)
// -------------------------------------------------------------------------------------------------
{
    // Note that becuse this is a recursive function, the buffer here is static in order to save on
    // stack space.  The implication here is that we then have to be careful how it is later
    // accessed.  Also, this makes the function not thread safe.  But this trade off was made as
    // this was not intended to be a multi-threaded program.
    static char strBuffer[STRING_MAX] = "";

    // Build up the child array.
    json_t* childArrayPtr = json_array();

    do
    {
        // Simply grab the name and the type of the current node.
        le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));
        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");

        switch (type)
        {
            // It's a stem object, so mark this item as being a stem and recurse into the stem's
            // sub-items.
            case LE_CFG_TYPE_STEM:
                {
                    json_t* nodePtr = CreateJsonNode(strBuffer, NodeTypeStr(type));

                    le_cfg_GoToFirstChild(iterRef);
                    DumpTreeJSON(iterRef, nodePtr);
                    le_cfg_GoToParent(iterRef);
                    json_array_append(childArrayPtr, nodePtr);
                }
                break;

            default:
                {
                    json_t* nodePtr = CreateJsonNodeFromIterator(iterRef);
                    if (nodePtr != NULL)
                    {
                        json_array_append(childArrayPtr, nodePtr);
                    }
                }
                break;
        }
    }
    while (le_cfg_GoToNextSibling(iterRef) == LE_OK);

    // Set children into the JSON document.
    json_object_set_new(jsonObject, JSON_FIELD_CHILDREN, childArrayPtr);
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
    static char strBuffer[STRING_MAX] = "";

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

            case LE_CFG_TYPE_BOOL:
                {
                    char* value = NULL;

                    if (le_cfg_GetBool(iterRef, "", false))
                    {
                        value = "true";
                    }
                    else
                    {
                        value = "false";
                    }

                    printf("%s<bool> == %s\n", strBuffer, value);
                }
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
 *  Given a type name string, convert it to a proper config type enumeration value.
 *
 *  @return The specified type ID if successful, LE_CFG_TYPE_DOESNT_EXIST is returned if this
 *          function fails.
 */
// -------------------------------------------------------------------------------------------------
static le_cfg_nodeType_t GetNodeTypeFromString
(
    const char* typeNamePtr  ///< The index of the command line argument to read.
)
// -------------------------------------------------------------------------------------------------
{
    // Check the given name against what we're expecting.
    if (strncmp(typeNamePtr, "string", COMMAND_MAX) == 0)
    {
        return LE_CFG_TYPE_STRING;
    }
    else if (strncmp(typeNamePtr, "bool", COMMAND_MAX) == 0)
    {
        return LE_CFG_TYPE_BOOL;
    }
    else if (strncmp(typeNamePtr, "int", COMMAND_MAX) == 0)
    {
        return LE_CFG_TYPE_INT;
    }
    else if (strncmp(typeNamePtr, "float", COMMAND_MAX) == 0)
    {
        return LE_CFG_TYPE_FLOAT;
    }
    else if (strncmp(typeNamePtr, "stem", COMMAND_MAX) == 0)
    {
        return LE_CFG_TYPE_STEM;
    }

    // Looks like we didn't get something useful, so return in error.
    fprintf(stderr, "Unexpected node type specified, '%s'\n", typeNamePtr);
    return LE_CFG_TYPE_DOESNT_EXIST;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the specified command line argument and get a type Id from it.
 *
 *  @return The specified type ID if successful, LE_CFG_TYPE_DOESNT_EXIST is returned if this
 *          function fails.
 */
// -------------------------------------------------------------------------------------------------
static le_cfg_nodeType_t GetNewNodeTypeFromParam
(
    size_t paramIndex  ///< Read the type name string from this command line parameter.
)
// -------------------------------------------------------------------------------------------------
{
    char typeName[COMMAND_MAX] = "";

    // Let's see if a type was given.
    le_result_t result = le_arg_GetArg(paramIndex, typeName, sizeof(typeName));

    if (result == LE_OVERFLOW)
    {
        // I don't know what was specified, but it was way too big.
        fprintf(stderr,
                "Parameter node type, '%s' is too large for internal buffers.\n"
                "For more details please run:\n"
                "\t%s help\n\n",
                typeName,
                ProgramName);

        return LE_CFG_TYPE_DOESNT_EXIST;
    }
    else if (result == LE_NOT_FOUND)
    {
        // Nothing was supplied, so go with our default.
        return LE_CFG_TYPE_STRING;
    }

    // Ok, convert the string into a proper type enum.
    return GetNodeTypeFromString(typeName);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Function to process the import and export parameters.  File path is also translated into an
 *  absolute path.
 *
 *  @return If either of the parameters is missing or too large this function will return LE_FAULT.
 *          LE_FAULT is also returned if an extra parameter for JSON format is given, but it's
 *          malformed.  LE_OK is returned otherwise.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t GetImpExpParams
(
    char* nodePathPtr,  ///< Buffer to hold the node path, must be at least STRING_MAX bytes in
                        ///<   size.
    char* filePathPtr,  ///< Bufer to hold the file path, must be at least PATH_MAX bytes in size.
    bool* isJSONPtr     ///< JSON format flag
)
// -------------------------------------------------------------------------------------------------
{
    char relativePath[STRING_MAX] = "";
    char format[STRING_MAX] = "";

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

    // Check to see if the supplied an extra parameter for the format.
    if (le_arg_GetArg(PARAM_IMP_EXP_FORMAT, format, sizeof(format)) == LE_OK)
    {
        // Looks like they did.  Make sure that the param is the JSON format specifier.  (That's
        // the only alternative output format supported.)
        if (strncmp(format, JSON_FORMAT, sizeof(JSON_FORMAT) != 0))
        {
            fprintf(stderr, "Bad format specifier, '%s'.", format);
            return LE_FAULT;
        }

        *isJSONPtr = true;
        return LE_OK;
    }

    *isJSONPtr = false;
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
            "%s failure, %d: %s.\nFile Path: %s\nNode Path: %s\n",
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
static int HandleGetUserFriendly
(
    const char* nodePathPtr  ///< Path to the node in the tree.
)
// -------------------------------------------------------------------------------------------------
{
    // Start a read transaction at the specified node path.  Then dump the value, (if any.)
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePathPtr);

    switch (le_cfg_GetNodeType(iterRef, ""))
    {
        case LE_CFG_TYPE_EMPTY:
            // Nothing to do here.
            break;

        case LE_CFG_TYPE_STEM:
            DumpTree(iterRef, 0);
            break;

        case LE_CFG_TYPE_BOOL:
            if (le_cfg_GetBool(iterRef, "", false))
            {
                printf("true\n");
            }
            else
            {
                printf("false\n");
            }
            break;

        default:
            {
                char nodeValue[STRING_MAX] = "";

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
 *  This function will attempt read a value from the tree, and write it to standard out, or to a
 *  file.  The tree data will be written in JSON format.
 *
 *  If the specifed node is a stem, then the tree structure will be dumped, starting at the
 *  specifed node.  If a '*' is given for a node path then all trees in the system will be dumped
 *  into a JSON document.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleGetJSON
(
    const char* nodePathPtr,  ///< Path to the node in the configTree.
    const char* filePathPtr   ///< Path to the file in the filesystem.  If NULL STDOUT is used
                              ///< instead of a file.
)
// -------------------------------------------------------------------------------------------------
{
    json_t* nodePtr = NULL;

    // Get the node path from our command line arguments.
    if (strcmp("*", nodePathPtr) == 0)
    {
        // Dump all trees
        // Create JSON root item
        json_t* rootPtr = CreateJsonNode("root", "root");
        json_t* treeListPtr = json_array();

        // Loop through the trees in the system.
        le_cfgAdmin_IteratorRef_t iteratorRef = le_cfgAdmin_CreateTreeIterator();
        while (le_cfgAdmin_NextTree(iteratorRef) == LE_OK)
        {
            // Allocate space for the tree name, plus space for a trailing :/ used when we create a
            // transaction for that tree.
            char treeName[MAX_TREE_NAME_BYTES + 2] = "";

            if (le_cfgAdmin_GetTreeName(iteratorRef, treeName, MAX_TREE_NAME_BYTES) != LE_OK)
            {
                continue;
            }

            // JSON node for the tree.
            json_t* treeNodePtr = CreateJsonNode(treeName, "tree");
            strcat(treeName, ":/");

            // Start a read transaction at the specified node path.  Then dump the value, (if any.)
            le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(treeName);
            le_cfg_GoToFirstChild(iterRef);

            // Dump tree to JSON
            DumpTreeJSON(iterRef, treeNodePtr);
            le_cfg_CancelTxn(iterRef);

            json_array_append(treeListPtr, treeNodePtr);
        }
        le_cfgAdmin_ReleaseTreeIterator(iteratorRef);

        // Finalize root object...
        json_object_set_new(rootPtr, "trees", treeListPtr);
        nodePtr = rootPtr;
    }
    else
    {
        // Start a read transaction at the specified node path.  Then dump the value, (if any.)
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePathPtr);

        le_cfg_nodeType_t type = le_cfg_GetNodeType(iterRef, "");
        switch (type)
        {
            case LE_CFG_TYPE_STEM:
                {
                    char strBuffer[STRING_MAX] = "";
                    char nodeType[STRING_MAX] = "";
                    le_cfg_GetNodeName(iterRef, "", strBuffer, STRING_MAX);

                    // If no name, we are dumping a complete tree.
                    if (strlen(strBuffer) == 0)
                    {
                        strcpy(nodeType, "tree");
                    }
                    else
                    {
                        strcpy(nodeType, NodeTypeStr(type));
                    }

                    nodePtr = CreateJsonNode(strBuffer, nodeType);
                    le_cfg_GoToFirstChild(iterRef);
                    DumpTreeJSON(iterRef, nodePtr);
                    le_cfg_GoToParent(iterRef);
                }
                break;

            default:
                nodePtr = CreateJsonNodeFromIterator(iterRef);
                break;
        }

        le_cfg_CancelTxn(iterRef);
    }

    if (nodePtr == NULL)
    {
        // Empty node
        nodePtr = json_object();
    }

    // Dump Json content
    // stdout mode?

    int result = EXIT_SUCCESS;

    if (filePathPtr == NULL)
    {
        printf("%s\n", json_dumps(nodePtr, JSON_COMPACT));
    }
    else if (json_dump_file(nodePtr, filePathPtr, JSON_COMPACT) != 0)
    {
        result = EXIT_FAILURE;
    }

    json_decref(nodePtr);
    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Function that handles the actual import of JSON data into the configTree.
 *
 *  @return LE_OK if the import is successful, LE_FAULT otherwise.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t HandleImportJSONIteration
(
    le_cfg_IteratorRef_t iterRef,  ///< Dump the JSON data into this iterator.
    json_t* nodePtr                ///< From this JSON object.
)
// -------------------------------------------------------------------------------------------------
{
    // Get value
    json_t* value = json_object_get(nodePtr, JSON_FIELD_VALUE);

    // Check type
    const char* typeStr = json_string_value(json_object_get(nodePtr, JSON_FIELD_TYPE));
    le_cfg_nodeType_t type = GetNodeTypeFromString(typeStr);

    switch (type)
    {
        case LE_CFG_TYPE_BOOL:
            le_cfg_SetBool(iterRef, "", json_is_true(value));
            break;

        case LE_CFG_TYPE_STRING:
            le_cfg_SetString(iterRef, "", json_string_value(value));
            break;

        case LE_CFG_TYPE_INT:
            le_cfg_SetInt(iterRef, "", json_integer_value(value));
            break;

        case LE_CFG_TYPE_FLOAT:
            le_cfg_SetFloat(iterRef, "", json_real_value(value));
            break;

        case LE_CFG_TYPE_STEM:
            {
                // Iterate on children
                json_t* childrenPtr = json_object_get(nodePtr, JSON_FIELD_CHILDREN);
                json_t* childPtr;
                int i;

                json_array_foreach(childrenPtr, i, childPtr)
                {
                    // Get name
                    const char* name = json_string_value(json_object_get(childPtr,
                                                                         JSON_FIELD_NAME));

                    // Is node exist with this name?
                    le_cfg_nodeType_t existingType = le_cfg_GetNodeType(iterRef, name);
                    switch (existingType)
                    {
                        case LE_CFG_TYPE_DOESNT_EXIST:
                        case LE_CFG_TYPE_STEM:
                        case LE_CFG_TYPE_EMPTY:
                            // Not existing, already a stem or empty node, nothing to do
                        break;

                        default:
                            // Issue with node creation
                            fprintf(stderr, "Node conflict when importing, at node %s", name);
                            return LE_NOT_POSSIBLE;
                        break;
                    }

                    // Iterate to this child
                    le_cfg_GoToNode(iterRef, name);

                    // Iterate
                    le_result_t subResult = HandleImportJSONIteration(iterRef, childPtr);
                    if (subResult != LE_OK)
                    {
                        // Something went wrong
                        return subResult;
                    }

                    // Go back to parent
                    le_cfg_GoToParent(iterRef);
                }
            }
            break;

        default:
            return LE_FAULT;
    }

    return LE_OK;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Load a JSON representation of some config data and import it into the configTree at the
 *  iterator's starting location.
 *
 *  @return LE_OK if the import is successful.  LE_FAULT otherwise.
 */
// -------------------------------------------------------------------------------------------------
static le_result_t HandleImportJSON
(
    le_cfg_IteratorRef_t iterRef,  ///< The iterator being written to.
    const char* filePathPtr        ///< Load the JSON from a file at this path.
)
// -------------------------------------------------------------------------------------------------
{
    // Attempt to load JSON file.  If it fails to validate, then bail out.
    json_error_t error;
    json_t* decodedRootPtr = json_load_file(filePathPtr, 0, &error);

    if (decodedRootPtr == NULL)
    {
        fprintf(stderr,
                "JSON import error: line: %d, column: %d, position: %d, source: '%s', error: %s",
                error.line,
                error.column,
                error.position,
                error.source,
                error.text);

        return LE_FAULT;
    }

    // Ok, looks like the JSON loaded, so iterate through it and dump it's contents into the
    // configTree.
    le_result_t result = HandleImportJSONIteration(iterRef, decodedRootPtr);
    json_decref(decodedRootPtr);

    return result;
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
    char nodePath[STRING_MAX] = "";
    char format[STRING_MAX] = "";

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_GET_NODE_PATH, nodePath, sizeof(nodePath), "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Check to see if the supplied an extra parameter.
    if (le_arg_GetArg(PARAM_GET_FORMAT, format, sizeof(format)) == LE_OK)
    {
        // Looks like they did.  Make sure that the param is the JSON format specifier.  (That's
        // the only alternative output format supported.)
        if (strncmp(format, JSON_FORMAT, sizeof(JSON_FORMAT) != 0))
        {
            fprintf(stderr, "Bad format specifier, '%s'.", format);
            return EXIT_FAILURE;
        }

        return HandleGetJSON(nodePath, NULL);
    }

    // Looks like we're just outputing the human readable format.
    return HandleGetUserFriendly(nodePath);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set a value in the configTree to sa new value as specified by the caller.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleSet
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = "";
    char nodeValue[STRING_MAX] = "";

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_SET_NODE_PATH, nodePath, sizeof(nodePath), "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Get the new value from the command line arguments.
    if (GetRequiredParameter(PARAM_SET_VALUE, nodeValue, sizeof(nodeValue), "new value") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Looks like we're trying to write a value to a node.  Get the node's current type and then
    // write the requested value to that node.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);

    le_cfg_nodeType_t originalType = le_cfg_GetNodeType(iterRef, "");
    le_cfg_nodeType_t newType = GetNewNodeTypeFromParam(PARAM_SET_TYPE);

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
            if (strncmp(nodeValue, "false", sizeof(nodeValue)) == 0)
            {
                le_cfg_SetBool(iterRef, "", false);
            }
            else if (strncmp(nodeValue, "true", sizeof(nodeValue)) == 0)
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
 *  Change the name of a given node to a new name.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleRename
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = "";
    char newName[MAX_NODE_NAME] = "";

    // Get the node path, and the new name for the node from the command line arguments.
    if (GetRequiredParameter(PARAM_RN_NODE_PATH, nodePath, sizeof(nodePath), "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    if (GetRequiredParameter(PARAM_RN_NEW_NAME, newName, sizeof(newName), "new name") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Attempt the rename, then report success or failure.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);

    le_result_t result = le_cfg_SetNodeName(iterRef, "", newName);
    int exitResult = EXIT_SUCCESS;

    switch (result)
    {
        case LE_OK:
            break;

        case LE_FORMAT_ERROR:
            fprintf(stderr, "Invalid node name specified, '%s'.\n", newName);
            exitResult = EXIT_FAILURE;
            break;

        case LE_DUPLICATE:
            fprintf(stderr, "Duplicate node name specified, '%s'.\n", newName);
            exitResult = EXIT_FAILURE;
            break;

        default:
            fprintf(stderr,
                    "An unexpected error occured, %d: %s.\n",
                    result,
                    LE_RESULT_TXT(result));

            exitResult = EXIT_FAILURE;
            break;
    }

    // Make sure that the change was successful, before we try to commit.
    if (result == LE_OK)
    {
        le_cfg_CommitTxn(iterRef);
    }
    else
    {
        le_cfg_CancelTxn(iterRef);
    }

    return exitResult;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Command to handle importing data into the tree.
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleImport
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = "";
    char filePath[PATH_MAX] = "";
    bool isJSON;

    if (GetImpExpParams(nodePath, filePath, &isJSON) != LE_OK)
    {
        return EXIT_FAILURE;
    }

    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(nodePath);
    le_result_t result;

    // Check requested format format.
    if (isJSON)
    {
        result = HandleImportJSON(iterRef, filePath);
    }
    else
    {
        result = le_cfgAdmin_ImportTree(iterRef, filePath, "");
    }

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
 *  Export data from the config tree, either in JSON or in the configTree's native format.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleExport
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    char nodePath[STRING_MAX] = "";
    char filePath[PATH_MAX] = "";
    bool isJSON;

    if (GetImpExpParams(nodePath, filePath, &isJSON) != LE_OK)
    {
        return EXIT_FAILURE;
    }

    le_result_t result;

    // Check required format.
    if (isJSON)
    {
        result = HandleGetJSON(nodePath, filePath);
    }
    else
    {
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(nodePath);
        result = le_cfgAdmin_ExportTree(iterRef, filePath, "");
        le_cfg_CancelTxn(iterRef);
    }

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
    char nodePath[STRING_MAX] = "";

    // Get the node path from our command line arguments.
    if (GetRequiredParameter(PARAM_DEL_NODE_PATH, nodePath, sizeof(nodePath), "node path") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    // Ok, delete the node.
    le_cfg_QuickDeleteNode(nodePath);
    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Handle the list command.  Simply call on the configAdmin API to create a tree iterator and then
 *  iterate through all available trees, while printing their name to standard out.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleList
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    le_cfgAdmin_IteratorRef_t iteratorRef = le_cfgAdmin_CreateTreeIterator();

    while (le_cfgAdmin_NextTree(iteratorRef) == LE_OK)
    {
        char treeName[MAX_TREE_NAME_BYTES] = "";

        if (le_cfgAdmin_GetTreeName(iteratorRef, treeName, sizeof(treeName)) == LE_OK)
        {
            printf("%s\n", treeName);
        }
    }

    le_cfgAdmin_ReleaseTreeIterator(iteratorRef);

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will delete the named tree.  Both from the configTree's memory and from the
 *  filesystem.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleDeleteTree
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Simply get the tree name from the command line parameters and let the configTree do the rest.
    char treeName[MAX_TREE_NAME_BYTES] = "";

    if (GetRequiredParameter(PARAM_RMTREE_NAME, treeName, sizeof(treeName), "tree name") != LE_OK)
    {
        return EXIT_FAILURE;
    }

    le_cfgAdmin_DeleteTree(treeName);

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
    // Read out the program name so that we can better format our error and help messages.
    if (le_arg_GetProgramName(ProgramName, STRING_MAX, NULL) != LE_OK)
    {
        strncpy(ProgramName, "config", STRING_MAX);
    }

    // Get the name of the sub-command that the caller wants us to execute.
    char commandBuffer[COMMAND_MAX] = "";
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
    else if (strncmp(commandBuffer, "rename", bufferSize) == 0)
    {
        exit(HandleRename());
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
    else if (strncmp(commandBuffer, "list", bufferSize) == 0)
    {
        exit(HandleList());
    }
    else if (strncmp(commandBuffer, "rmtree", bufferSize) == 0)
    {
        exit(HandleDeleteTree());
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
