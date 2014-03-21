// -------------------------------------------------------------------------------------------------
/**
 *  Data Utility for command line control of data connection
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_data_interface.h"
#include "le_print.h"


//--------------------------------------------------------------------------------------------------
/**
 * Location of the file that contains the data connection ref
 *
 * It should be a safe assumption that the /tmp/legato directory already exists.
 */
//--------------------------------------------------------------------------------------------------
#define REF_FILE "/tmp/legato/dataref.txt"


//--------------------------------------------------------------------------------------------------
/**
 * Only one handler is registered, but ref needs to be shared between functions.
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t HandlerRef;


//--------------------------------------------------------------------------------------------------
/**
 * Callback for the connection state
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    LE_PRINT_VALUE("%s", intfName);
    LE_PRINT_VALUE("%i", isConnected);

    if ( isConnected )
    {
        LE_INFO("Network interface '%s' is connected", intfName);
    }
    else
    {
        LE_INFO("Network interface is not connected");

        // Delete the ref file, to indicate the data connection has been stopped.
        unlink(REF_FILE);
    }

    // De-register call-back and then exit
    le_data_RemoveConnectionStateHandler(HandlerRef);
    exit(0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a single command from the user.
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    char* command
)
{
    le_data_Request_Ref_t requestRef;
    char line[100] = "";
    FILE *filePtr;

    // Request data connection and write the ref to the temp file
    if ( strcmp(command, "start") == 0 )
    {
        // If the file already exists, then there is already a data connection requested
        if ( access(REF_FILE, F_OK) == 0 )
        {
            LE_INFO("Data connection already exists");
            exit(0);
        }
        else
        {
            requestRef = le_data_Request();
            LE_PRINT_VALUE("%p", requestRef);

            filePtr = fopen(REF_FILE, "a");
            if ( filePtr != NULL )
            {
                fprintf(filePtr, "%p\n", requestRef);
                fclose(filePtr);
            }
            else
            {
                LE_FATAL("Could not store requestRef %p to %s", requestRef, REF_FILE);
            }
        }
    }

    // Get the requestRef from the temp file and release the data connection
    else if ( strcmp(command, "stop") == 0 )
    {
        filePtr = fopen(REF_FILE, "r");
        if ( filePtr != NULL )
        {
            fgets(line, sizeof(line), filePtr);
            sscanf(line, "%p", &requestRef);

            LE_PRINT_VALUE("%p", requestRef);
            le_data_Release(requestRef);
        }
        else
        {
            LE_FATAL("Could not read requestRef from %s", REF_FILE);
        }
    }

    else
    {
        LE_FATAL("Invalid command %s", command);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Program init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    char command[100] = "";

    // Register a call-back
    HandlerRef = le_data_AddConnectionStateHandler(ConnectionStateHandler, NULL);
    LE_PRINT_VALUE("%p", HandlerRef);

    // Process the command
    if (le_arg_NumArgs() == 1)
    {
        le_arg_GetArg(0, command, sizeof(command));
        ProcessCommand(command);
    }
    else
    {
        LE_FATAL("=== You must specify a command ===");
    }
}


