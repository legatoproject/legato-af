//--------------------------------------------------------------------------------------------------
/** @file addr.c
 *
 * Utility functions for examining a process's virtual memory address space.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets the address of the .data section of the specified library for the specified process.
 * The address is in the address space of the specified process.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if the library was not found.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t addr_GetLibDataSection
(
    pid_t pid,              ///< [IN] Pid to get the address for.  Zero means the calling process.
    const char* libNamePtr, ///< [IN] Name of the library.
    off_t* libAddressPtr    ///< [OUT] Address of the .data section of the library.
)
{
    // Build the path to the maps file.
    char fileName[LIMIT_MAX_PATH_BYTES];
    size_t snprintfSize;

    if (pid == 0)
    {
        snprintfSize = snprintf(fileName, sizeof(fileName), "/proc/self/maps");
    }
    else
    {
        snprintfSize = snprintf(fileName, sizeof(fileName), "/proc/%d/maps", pid);
    }

    if (snprintfSize >= sizeof(fileName))
    {
        LE_ERROR("Path to file '%s' is too long.", fileName);
        return LE_FAULT;
    }

    // Open the maps file.
    int fd = open(fileName, O_RDONLY);

    if (fd == -1)
    {
        LE_ERROR("Could not open %s.  %m.", fileName);
        return LE_FAULT;
    }

    // Search each line of the file to find the liblegato.so section.
    off_t address;
    char line[LIMIT_MAX_PATH_BYTES * 2];

    while (1)
    {
        le_result_t result = fd_ReadLine(fd, line, sizeof(line));

        if (result == LE_OK)
        {
            // The line is the section we are looking for if it specifies our library and has an
            // access mode of "rw-p" which we infer is the text section.
            if ( (strstr(line, libNamePtr) != NULL) &&
                 ( (strstr(line, "rw-p") != NULL) || (strstr(line, "rwxp") != NULL) ) )
            {
                // The line should begin with the starting address of the section.
                // Convert it to an address value.
                char* endPtr;
                errno = 0;
                address = strtoll(line, &endPtr, 16);

                if ( (errno != 0) || (endPtr == line) )
                {
                    LE_ERROR("Error reading file %s.", fileName);
                    fd_Close(fd);
                    return LE_FAULT;
                }

                // Got the address.
                break;
            }
        }
        else if (result == LE_OUT_OF_RANGE)
        {
            // The end of the file is reached.
            fd_Close(fd);
            return LE_NOT_FOUND;
        }
        else
        {
            LE_ERROR("Error reading '%s' while looking for '%s'.", fileName, libNamePtr);
            fd_Close(fd);
            return LE_FAULT;
        }
    }

    fd_Close(fd);

    *libAddressPtr = address;
    return LE_OK;
}
