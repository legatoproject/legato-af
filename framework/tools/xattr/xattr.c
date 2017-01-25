/** @file xattr.c
 *
 * This is a temporary extended attributes command line tool used to view and set extended
 * attributes.  This tool should be replaced by a third party tool if possible.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include <sys/xattr.h>


//--------------------------------------------------------------------------------------------------
/**
 * Prints help to stdout.
 */
//--------------------------------------------------------------------------------------------------
static void PrintHelp
(
    void
)
{
    puts(
        "NAME:\n"
        "    xattr - Gets or sets extended attributes of file system objects.\n"
        "\n"
        "DESCRIPTION:\n"
        "    xattr get OBJ_PATH\n"
        "       Prints all extended attributes and their values for OBJ_PATH.\n"
        "\n"
        "    xattr set ATTR_NAME ATTR_VALUE OBJ_PATH\n"
        "       Sets the attribute specified by ATTR_NAME to the value ATTR_VALUE for OBJ_PATH.\n"
        "\n"
        "    xattr delete ATTR_NAME OBJ_PATH\n"
        "       Deletes the attribute specified by ATTR_NAME for OBJ_PATH.\n"
        "\n"
        );
}


//--------------------------------------------------------------------------------------------------
/**
 * Prints a generic message on stderr so that the user is aware there is a problem, logs the
 * internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR(formatString, ...)                                                 \
            { fprintf(stderr, "Internal error check logs for details.\n");              \
              LE_FATAL(formatString, ##__VA_ARGS__); }


//--------------------------------------------------------------------------------------------------
/**
 * If the condition is true, print a generic message on stderr so that the user is aware there is a
 * problem, log the internal error message and exit.
 */
//--------------------------------------------------------------------------------------------------
#define INTERNAL_ERR_IF(condition, formatString, ...)                                   \
        if (condition) { INTERNAL_ERR(formatString, ##__VA_ARGS__); }


#define MAX_XATTR_LEN               10000

//--------------------------------------------------------------------------------------------------
/**
 * Print all extended attributes for the specified object.
 */
//--------------------------------------------------------------------------------------------------
static void PrintXAttrs
(
    void
)
{
    const char* pathPtr = le_arg_GetArg(1);

    if (pathPtr == NULL)
    {
        fprintf(stderr, "Please specify a file or directory.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    // Get the list of extended attributes for the object.
    char nameBuf[MAX_XATTR_LEN];

    ssize_t listLen = listxattr(pathPtr, nameBuf, sizeof(nameBuf));

    if (listLen < 0)
    {
        fprintf(stderr, "Could not read list of extended attributes.  %m.\n");
        exit(EXIT_FAILURE);
    }

    // Iterate through the list of attributes.
    int i;
    for (i = 0; i < listLen; i += strlen(&(nameBuf[i])) + 1)
    {
        char* namePtr = &(nameBuf[i]);

        // Get the value.
        char value[LIMIT_MAX_PATH_BYTES];

        ssize_t valueLen = getxattr(pathPtr, namePtr, value, sizeof(value));

        if (valueLen < 0)
        {
            fprintf(stderr, "Could not read extended attribute value for '%s'.  %m.\n", namePtr);
            exit(EXIT_FAILURE);
        }

        // Print both the name and value.
        printf("    name=%s; value=%.*s\n", namePtr, (int)valueLen, value);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the extended attribute for the object.
 */
//--------------------------------------------------------------------------------------------------
static void SetXAttr
(
    void
)
{
    const char* namePtr = le_arg_GetArg(1);

    if (namePtr == NULL)
    {
        fprintf(stderr, "Please specify an extended attribute name.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    const char* valuePtr = le_arg_GetArg(2);

    if (valuePtr == NULL)
    {
        fprintf(stderr, "Please specify an extended attribute value.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    const char* pathPtr = le_arg_GetArg(3);

    if (pathPtr == NULL)
    {
        fprintf(stderr, "Please specify a file or directory.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    if (setxattr(pathPtr, namePtr, valuePtr, strlen(valuePtr), 0) == -1)
    {
        fprintf(stderr, "Could not set extended attribute. %m\n");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the extended attribute for the object.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteXAttr
(
    void
)
{
    const char* namePtr = le_arg_GetArg(1);

    if (namePtr == NULL)
    {
        fprintf(stderr, "Please specify an extended attribute name.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    const char* pathPtr = le_arg_GetArg(2);

    if (pathPtr == NULL)
    {
        fprintf(stderr, "Please specify a file or directory.\n");
        PrintHelp();
        exit(EXIT_FAILURE);
    }

    if (removexattr(pathPtr, namePtr) == -1)
    {
        fprintf(stderr, "Could not delete extended attribute. %m\n");
        exit(EXIT_FAILURE);
    }
}


COMPONENT_INIT
{
    const char* cmdPtr = le_arg_GetArg(0);

    if (cmdPtr == NULL)
    {
        fprintf(stderr, "Please specify a command.\n");

        PrintHelp();
        exit(EXIT_FAILURE);
    }

    if (strcmp(cmdPtr, "get") == 0)
    {
        PrintXAttrs();
    }
    else if (strcmp(cmdPtr, "set") == 0)
    {
        SetXAttr();
    }
    else if (strcmp(cmdPtr, "delete") == 0)
    {
        DeleteXAttr();
    }
    else
    {
        fprintf(stderr, "Unknown command.\n");

        PrintHelp();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
