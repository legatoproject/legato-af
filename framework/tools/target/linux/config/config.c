
// -------------------------------------------------------------------------------------------------
/**
 *  @file config/config.c
 *
 *  Utility to work with a config tree from the command line.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"
#include "jansson.h"
#include "interfaces.h"




/// Size of the command string used by the import/export command.
#define COMMAND_MAX 16




/// Maximum expected size of a config tree name.
#define MAX_TREE_NAME_BYTES LIMIT_MAX_USER_NAME_BYTES




/// Max size of a node name.
#define MAX_NODE_NAME (size_t)64




/// Json field names.
#define JSON_FIELD_TYPE "type"
#define JSON_FIELD_NAME "name"
#define JSON_FIELD_CHILDREN "children"
#define JSON_FIELD_VALUE "value"



/// Name used to launch this program.
static const char* ProgramName;



/// Configuration tree node path.
static const char* NodePath;


/// Destination path for copy and move operations.
static const char* NodeDestPath;



/// Configuration tree node value.
static const char* NodeValue;



/// Node's data type (default = string).
static le_cfg_nodeType_t DataType = LE_CFG_TYPE_STRING;



/// File system path (absolute).
static char FilePath[PATH_MAX];



/// Name of a configuration tree.
static const char* TreeName;



/// true = do import or export using JSON format.
static bool UseJson = false;



/// If true, delete the original node after a copy, false leave the original alone.
static bool DeleteAfterCopy = false;



/// Function to be used to handle the command.
static int (*CommandHandler)(void);



// -------------------------------------------------------------------------------------------------
/**
 *  Simply write the usage text to the console.
 */
// -------------------------------------------------------------------------------------------------
static void PrintHelpAndExit
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
           "To move a node:\n"
           "\t%s move <node path> <new name>\n\n"
           "To copy a node:\n"
           "\t%s copy <node path> <new name>\n\n"
           "To delete a node:\n"
           "\t%s delete <tree path>\n\n"
           "To clear or create a new, empty node:\n"
           "\t%s clear <tree path>\n\n"
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
           "\t<type>:      Is optional and must be one of bool, int, float, or string.\n"
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
           ProgramName,
           ProgramName,
           ProgramName);

    exit(EXIT_SUCCESS);
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
    char nodeName[LE_CFG_NAME_LEN_BYTES] = "";

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
                char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
                le_cfg_GetString(iterRef, "", strBuffer, LE_CFG_STR_LEN_BYTES, "");
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
    // Note that because this is a recursive function, the buffer here is static in order to save on
    // stack space.  The implication here is that we then have to be careful how it is later
    // accessed.  Also, this makes the function not thread safe.  But this trade off was made as
    // this was not intended to be a multi-threaded program.
    static char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

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
    // Note that because this is a recursive function, the buffer here is static in order to save on
    // stack space.  The implication here is that we then have to be careful how it is later
    // accessed.  Also, this makes the function not thread safe.  But this trade off was made as
    // this was not intended to be a multi-threaded program.
    static char strBuffer[LE_CFG_STR_LEN_BYTES] = "";

    do
    {
        // Quick and dirty way to indent the tree item.
        size_t i;

        for (i = 0; i < indent; i++)
        {
            printf(" ");
        }

        // Simply grab the name and the type of the current node.
        le_cfg_GetNodeName(iterRef, "", strBuffer, LE_CFG_STR_LEN_BYTES);
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
                le_cfg_GetString(iterRef, "", strBuffer, LE_CFG_STR_LEN_BYTES, "");
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
 *  @return The specified type ID if successful.  Prints an error message and exits on error.
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
    fprintf(stderr, "Unrecognized node type specified, '%s'\n", typeNamePtr);
    exit(EXIT_FAILURE);
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
 *  specified node is a stem, then the tree structure will be printed, starting at the specified
 *  node.
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
                char nodeValue[LE_CFG_STR_LEN_BYTES] = "";

                le_cfg_GetString(iterRef, "", nodeValue, LE_CFG_STR_LEN_BYTES, "");
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
 *  If the specified node is a stem, then the tree structure will be dumped, starting at the
 *  specified node.  If a '*' is given for a node path then all trees in the system will be dumped
 *  into a JSON document.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleGetJSON
(
    const char* nodePathPtr,  ///< Path to the node in the configTree.
    const char* filePathPtr   ///< Path to the file in the file system.  If NULL STDOUT is used
                              ///< instead of a file.
)
// -------------------------------------------------------------------------------------------------
{
    json_t* nodePtr = NULL;
    char *json_dumps_result;

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
                    char strBuffer[LE_CFG_STR_LEN_BYTES] = "";
                    char nodeType[LE_CFG_STR_LEN_BYTES] = "";
                    le_cfg_GetNodeName(iterRef, "", strBuffer, sizeof(strBuffer));

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
        json_dumps_result = json_dumps(nodePtr, JSON_COMPACT);
        if (!json_dumps_result)
        {
            fprintf(stderr,"json_dumps failed");
            result = EXIT_FAILURE;
        }
        else
        {
            printf("%s\n", json_dumps_result);
            free(json_dumps_result);
        }
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
                        case LE_CFG_TYPE_STRING:
                        case LE_CFG_TYPE_BOOL:
                        case LE_CFG_TYPE_INT:
                        case LE_CFG_TYPE_FLOAT:
                            // If not existing, already a stem, empty node,
                            // or any expected type, do nothing
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

    // OK, looks like the JSON loaded, so iterate through it and dump it's contents into the
    // configTree.
    le_result_t result = HandleImportJSONIteration(iterRef, decodedRootPtr);
    json_decref(decodedRootPtr);

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt read a value from the tree, and write it to standard out.  If the
 *  specified node is a stem, then the tree structure will be printed, starting at the specified
 *  node.
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
    if (UseJson)
    {
        return HandleGetJSON(NodePath, NULL);
    }

    // Looks like we're just outputing the human readable format.
    return HandleGetUserFriendly(NodePath);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set a value in the configTree to a new value as specified by the caller.
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
    // Looks like we're trying to write a value to a node.  Get the node's current type and then
    // write the requested value to that node.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(NodePath);

    le_cfg_nodeType_t originalType = le_cfg_GetNodeType(iterRef, "");
    le_cfg_nodeType_t newType = DataType;

    if (   (newType != originalType)
        && (originalType != LE_CFG_TYPE_DOESNT_EXIST))
    {
        printf("Converting node '%s' type from %s to %s.\n",
               NodePath,
               NodeTypeStr(originalType),
               NodeTypeStr(newType));
    }

    int result = EXIT_SUCCESS;

    switch (newType)
    {
        case LE_CFG_TYPE_STRING:
            le_cfg_SetString(iterRef, "", NodeValue);
            break;

        case LE_CFG_TYPE_BOOL:
            if (strcmp(NodeValue, "false") == 0)
            {
                le_cfg_SetBool(iterRef, "", false);
            }
            else if (strcmp(NodeValue, "true") == 0)
            {
                le_cfg_SetBool(iterRef, "", true);
            }
            else
            {
                fprintf(stderr, "Bad boolean value '%s'.\n", NodeValue);
            }
            break;

        case LE_CFG_TYPE_INT:
            {
                char *endIntp;

                errno = 0;
                int32_t value = strtol(NodeValue, &endIntp, 10);

                if (errno != 0)
                {
                    fprintf(stderr, "Integer '%s' out of range\n", NodeValue);
                    result = EXIT_FAILURE;
                }
                else if (*endIntp != '\0')
                {
                    fprintf(stderr, "Invalid character in integer '%s'\n", NodeValue);
                    result = EXIT_FAILURE;
                }
                else
                {
                    le_cfg_SetInt(iterRef, "", value);
                }
                break;
            }

        case LE_CFG_TYPE_FLOAT:
            {
                char *endFloatp;

                errno = 0;
                double floatVal = strtod(NodeValue, &endFloatp);

                if (errno != 0)
                {
                    fprintf(stderr, "Float value '%s' out of range\n", NodeValue);
                    result = EXIT_FAILURE;
                }
                else if (*endFloatp != '\0')
                {
                    fprintf(stderr, "Invalid character in float value '%s'\n", NodeValue);
                    result = EXIT_FAILURE;
                }
                else
                {
                    le_cfg_SetFloat(iterRef, "", floatVal);
                }
                break;
            }

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
 *  Command-line argument handler called when a node name is found on the command-line.
 *
 *  Stores a pointer to the node path in the NodeDestPath variable.
 */
// -------------------------------------------------------------------------------------------------
static void NodeDestPathArgHandler
(
    const char* nodeDestPath
)
// -------------------------------------------------------------------------------------------------
{
    NodeDestPath = nodeDestPath;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Change the name of a given node to a new name.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleCopy
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Create a temp file to export the tree to.
    char tempFilePath[] = "/tmp/configExport-XXXXXX";
    int tempFd;

    do
    {
        tempFd = mkstemp(tempFilePath);
    }
    while ((tempFd == -1) && (errno == EINTR));

    if (tempFd == -1)
    {
        fprintf(stderr, "Could not create temp file. Reason, %s (%d).", strerror(errno), errno);
        return 1;
    }

    // Unlink the file now so that we can make sure that it will end up being deleted, no matter how
    // we exit.
    if (unlink(tempFilePath) == -1)
    {
        printf("Could not unlink temporary file. Reason, %s (%d).", strerror(errno), errno);
    }

    // Create a transaction and export the data from the config tree.
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(NodePath);
    le_result_t result = le_cfgAdmin_ExportTree(iterRef, tempFilePath, "");

    if (result != LE_OK)
    {
        fprintf(stderr,
                "An I/O error occurred while updating the config tree.  "
                "Tree has been left untouched.\n");
        goto txnDone;
    }

    if (DeleteAfterCopy != false)
    {
        // Since this is a rename, then delete the node at the original location.
        le_cfg_DeleteNode(iterRef, "");
    }

    // Now, move the iterator to the node's new name, then attempt to reload the data.
    le_cfg_GoToNode(iterRef, "..");
    result = le_cfgAdmin_ImportTree(iterRef, tempFilePath, NodeDestPath);

    if (result != LE_OK)
    {
        switch (result)
        {
            case LE_FAULT:
                fprintf(stderr,
                        "An I/O error occurred while updating the config tree.  "
                        "Tree has been left untouched.\n");
                break;

            case LE_FORMAT_ERROR:
                fprintf(stderr,
                        "Import/export corruption detected.  Tree has been left untouched.\n");
                break;

            default:
                fprintf(stderr,
                        "An unexpected error has occurred: %s, (%d).\n",
                        LE_RESULT_TXT(result),
                        result);
                break;
        }
    }

 txnDone:
    // Make sure that the change was successful, and either commit or discard any changes that were
    // made.
    if (result == LE_OK)
    {
        le_cfg_CommitTxn(iterRef);
    }
    else
    {
        le_cfg_CancelTxn(iterRef);
    }

    // Was the operation successful?
    int exitResult = (result == LE_OK) ? EXIT_SUCCESS : EXIT_FAILURE;

    // Finally, clean up our temp file and report our results.
    int closeRetVal;

    closeRetVal = close(tempFd);

    if ((closeRetVal == -1) && (errno != EINTR))
    {
        fprintf(stderr, "Could not close temp file (%m).");
        exitResult = EXIT_FAILURE;
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
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateWriteTxn(NodePath);
    le_result_t result;

    // Check requested format format.
    if (UseJson)
    {
        result = HandleImportJSON(iterRef, FilePath);
    }
    else
    {
        result = le_cfgAdmin_ImportTree(iterRef, FilePath, "");
    }

    if (result != LE_OK)
    {
        ReportImportExportFail(result, "Import", NodePath, FilePath);
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
    le_result_t result;

    // Check required format.
    if (UseJson)
    {
        result = HandleGetJSON(NodePath, FilePath);
    }
    else
    {
        le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn(NodePath);
        result = le_cfgAdmin_ExportTree(iterRef, FilePath, "");
        le_cfg_CancelTxn(iterRef);
    }

    if (result != LE_OK)
    {
        ReportImportExportFail(result, "Export", NodePath, FilePath);
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
    le_cfg_QuickDeleteNode(NodePath);

    return EXIT_SUCCESS;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Function called to handle clearing a node in the config tree.
 *
 *  @return EXIT_SUCCESS if the command completes properly.  EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
static int HandleClear
(
    void
)
// -------------------------------------------------------------------------------------------------
{
    // Clear the node by setting it empty.
    le_cfg_QuickSetEmpty(NodePath);

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
 *  file system.
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
    le_cfgAdmin_DeleteTree(TreeName);

    return EXIT_SUCCESS;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Function called when a data type is found on the command line.
 */
// -------------------------------------------------------------------------------------------------
static void DataTypeArgHandler
(
    const char* dataType
)
// -------------------------------------------------------------------------------------------------
{
    // Convert the string into a proper type enum.
    DataType = GetNodeTypeFromString(dataType);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Command-line argument handler called when a --format=X option appears on the command-line.
 */
// -------------------------------------------------------------------------------------------------
static void FormatArgHandler
(
    const char* format
)
// -------------------------------------------------------------------------------------------------
{
    if (strcmp(format, "json") == 0)
    {
        UseJson = true;
    }
    else
    {
        fprintf(stderr, "Bad format specifier, '%s'.\n", format);
        exit(EXIT_FAILURE);
    }
}


// -------------------------------------------------------------------------------------------------
/**
 *  Command-line argument handler called when a file path is found on the command-line.
 *
 *  Converts the path to an absolute path and stores it in FilePath.
 */
// -------------------------------------------------------------------------------------------------
static void FilePathArgHandler
(
    const char* filePath
)
// -------------------------------------------------------------------------------------------------
{
    // Convert the given path from a potentially relative path, to an absolute, canonical one
    // and store it in the FilePath static variable.
    if (!realpath(filePath, FilePath))
    {
        // Since the file does not exist, compose an absolute path based
        // on the absolute directory resolved through realpath concatenated
        // with the filename initially provided.

        // Copy string to obtain dir
        char filePathTmp[PATH_MAX];

        LE_ASSERT(LE_OK == le_utf8_Copy(filePathTmp, filePath, sizeof(filePathTmp), NULL));
        if (NULL != realpath(dirname(filePathTmp), FilePath))
        {
            // Copy string to obtain basename
            LE_ASSERT(LE_OK == le_utf8_Copy(filePathTmp, filePath, sizeof(filePathTmp), NULL));

            size_t length = strlen(FilePath);
            LE_ASSERT(FilePath[length] == '\0');
            snprintf(&FilePath[length], sizeof(FilePath)-length, "/%s", basename(filePathTmp));

            return;
        }

        fprintf(stderr, "Cannot find path '%s': %s", filePath, strerror(errno));
        exit(EXIT_FAILURE);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler for configuration tree node path argument.
 **/
//--------------------------------------------------------------------------------------------------
static void NodePathArgHandler
(
    const char* nodePath
)
//--------------------------------------------------------------------------------------------------
{
    NodePath = nodePath;
}



//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler for configuration tree node value argument.
 **/
//--------------------------------------------------------------------------------------------------
static void NodeValueArgHandler
(
    const char* nodeValue
)
//--------------------------------------------------------------------------------------------------
{
    NodeValue = nodeValue;

    // Could optionally have a node type argument after the node value.
    le_arg_AddPositionalCallback(DataTypeArgHandler);
    le_arg_AllowLessPositionalArgsThanCallbacks();
}



//--------------------------------------------------------------------------------------------------
/**
 * Command-line argument handler for configuration tree name argument.
 **/
//--------------------------------------------------------------------------------------------------
static void TreeNameArgHandler
(
    const char* treeName
)
//--------------------------------------------------------------------------------------------------
{
    TreeName = treeName;
}



//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called by the command-line argument scanner when it sees the command
 * on the command line.
 **/
//--------------------------------------------------------------------------------------------------
static void CommandArgHandler
(
    const char* command
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(command, "get") == 0)
    {
        CommandHandler = HandleGet;

        // Get the node path from our command line arguments and accept an optional --format=X arg.
        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_SetStringCallback(FormatArgHandler, NULL, "format");
    }
    else if (strcmp(command, "set") == 0)
    {
        CommandHandler = HandleSet;

        // Get the node path and value from our command line arguments.
        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_AddPositionalCallback(NodeValueArgHandler);
    }
    else if (strcmp(command, "move") == 0)
    {
        CommandHandler = HandleCopy;

        DeleteAfterCopy = true;

        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_AddPositionalCallback(NodeDestPathArgHandler);
    }
    else if (strcmp(command, "copy") == 0)
    {
        CommandHandler = HandleCopy;

        DeleteAfterCopy = false;

        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_AddPositionalCallback(NodeDestPathArgHandler);
    }
    else if (strcmp(command, "import") == 0)
    {
        CommandHandler = HandleImport;

        // Expect a node path and a file path, with an optional --format= argument.
        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_AddPositionalCallback(FilePathArgHandler);
        le_arg_SetStringCallback(FormatArgHandler, NULL, "format");
    }
    else if (strcmp(command, "export") == 0)
    {
        CommandHandler = HandleExport;

        // Expect a node path and a file path, with an optional --format= argument.
        le_arg_AddPositionalCallback(NodePathArgHandler);
        le_arg_AddPositionalCallback(FilePathArgHandler);
        le_arg_SetStringCallback(FormatArgHandler, NULL, "format");
    }
    else if (strcmp(command, "delete") == 0)
    {
        CommandHandler = HandleDelete;

        // Need a node path from our command line arguments.
        le_arg_AddPositionalCallback(NodePathArgHandler);
    }
    else if (strcmp(command, "clear") == 0)
    {
        CommandHandler = HandleClear;

        // Need a node path from our command line arguments.
        le_arg_AddPositionalCallback(NodePathArgHandler);
    }
    else if (strcmp(command, "list") == 0)
    {
        CommandHandler = HandleList;

        // No additional command-line parameters for this command.
    }
    else if (strcmp(command, "rmtree") == 0)
    {
        CommandHandler = HandleDeleteTree;

        // The only parameter is the tree name.
        le_arg_AddPositionalCallback(TreeNameArgHandler);
    }
    else if (strcmp(command, "help") == 0)
    {
        PrintHelpAndExit();
    }
    else
    {
        fprintf(stderr,
                "Error, unrecognized command, '%s'.\n"
                "For more details please run:\n"
                "\t%s help\n\n",
                command,
                ProgramName);

        exit(EXIT_FAILURE);
    }
}



// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the component.  This initializer will extract the number of command line arguments
 *  given to the executable and determine what operation to perform.  Once that is done, we exit
 *  and report success or failure to the process that started the executable.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Read out the program name so that we can better format our error and help messages.
    ProgramName = le_arg_GetProgramName();
    if (ProgramName == NULL)
    {
        ProgramName = "config";
    }

    // The first positional argument is the command that the caller wants us to execute.
    le_arg_AddPositionalCallback(CommandArgHandler);

    // Print help and exit if the "-h" or "--help" options are given.
    le_arg_SetFlagCallback(PrintHelpAndExit, "h", "help");

    // Scan the argument list.  This will set the CommandHandler and its parameters.
    le_arg_Scan();

    // Run the command handler.
    exit(CommandHandler());
}
