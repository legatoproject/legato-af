//--------------------------------------------------------------------------------------------------
/** @file appSmack.c
 *
 * This component implements the appSmack API which manages the SMACK label for applications.
 *
 *  - @ref c_appSmack_Labels
 *
 * @section c_appSmack_Labels Application SMACK Labels
 *
 * Each Legato application is assigned a SMACK label.  This label is given to all the application's
 * processes and bundled files in the app's sandbox.  The most logical label to use for an app would
 * be the app's name because it is unique.  However, presently, SMACK labels are limited to 23
 * characters, much shorter than an app name can be.  To handle this a mapping between app names and
 * SMACK labels is created and maintained by this module.  The labels used are based on unique
 * strings.
 *
 * A hash of the app name was also considered but was rejected due to the possibility that a hash
 * may collide.
 *
 * The mapping between app names and smack labels is maintained persistently in a file.  To speed
 * look-up the mapping is read into a hashmap in memory on startup.  Modifications are written back
 * to the map file.
 *
 * An application does not need to exist before it is given a SMACK label.  In fact to satisfy
 * bindings between a client app and a server app, where the server app is not yet installed, a
 * SMACK label must be generated for the server before the server exists.  This leads to a
 * restriction where mappings cannot be simply deleted once an app is uninstalled.  This causes a
 * memory leak and is an attack vector.
 *
 * @todo Delete mappings when applications are uninstalled and there are no more bindings to the
 *       uninstalled app.
 *
 * The mappings maintained in this module are only for applications.  SMACK labels for other objects
 * in the system such as device files are not kept in this mapping.  However, it is important that
 * the SMACK label for different objects do not collide.  Therefore the application labels are all
 * name spaced with the "app." prefix.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "smack.h"
#include "limit.h"
#include "fileDescriptor.h"


//--------------------------------------------------------------------------------------------------
/**
 * Path to the security directory.
 */
//--------------------------------------------------------------------------------------------------
#define SECURITY_DIR                        "/opt/legato/security/"


//--------------------------------------------------------------------------------------------------
/**
 * File used to store the mapping between application names and smack labels.
 */
//--------------------------------------------------------------------------------------------------
#define MAPPING_FILE                        SECURITY_DIR "labels"


//--------------------------------------------------------------------------------------------------
/**
 * Backup file for mappings.
 */
//--------------------------------------------------------------------------------------------------
#define MAPPING_FILE_BACKUP                 SECURITY_DIR "labels.bak"


//--------------------------------------------------------------------------------------------------
/**
 * Estimated maximum number of applications.
 */
//--------------------------------------------------------------------------------------------------
#define ESTIMATED_MAX_NUM_APPS              31


//--------------------------------------------------------------------------------------------------
/**
 * Maximum SMACK label base length.  The maximum SMACK label length not including the 'app.' prefix.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_BASE_LABEL_BYTES                LIMIT_MAX_SMACK_LABEL_BYTES - 4

//--------------------------------------------------------------------------------------------------
/**
 * Stores the current highest order string that is used for the base of smack labels.  This is used
 * to generate additional application smack labels that are guaranteed to be unique.
 */
//--------------------------------------------------------------------------------------------------
static char HighestOrderStr[MAX_BASE_LABEL_BYTES];


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool of app names.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppNamePool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool of smack labels.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LabelPool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map that maintains the mapping between app names and smack labels.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t LabelMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Imports app name to smack label mappings from the map file into a hash map.
 *
 * @note
 *      This function will kill the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void ImportMapping
(
    le_hashmap_Ref_t map            ///< [IN/OUT] The hash map to import the mappings to.
)
{
    // Open the map file.
    int fd;

    do
    {
        fd = open(MAPPING_FILE, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if (fd == -1)
    {
        if (errno == ENOENT)
        {
            // The mapping file does not exist yet.
            HighestOrderStr[0] = '\0';
            return;
        }

        LE_FATAL("Could not read SMACK label mappings file. %s.  %m.", MAPPING_FILE);
    }

    // The first line is the highest order string.
    le_result_t result = fd_ReadLine(fd, HighestOrderStr, sizeof(HighestOrderStr));

    if (result == LE_OUT_OF_RANGE)
    {
        // The mapping file is still empty.
        HighestOrderStr[0] = '\0';
        return;
    }

    LE_FATAL_IF(result == LE_OVERFLOW, "The smack map file may be corrupted.  The highest order \
string is too long.");

    LE_FATAL_IF(result == LE_FAULT, "Error reading smack map file.");

    // Read the app name and smack label from the map file.
    while (1)
    {
        // Read the appName.
        char* appNamePtr = le_mem_ForceAlloc(AppNamePool);

        result = fd_ReadLine(fd, appNamePtr, LIMIT_MAX_APP_NAME_BYTES);

        if (result == LE_OUT_OF_RANGE)
        {
            // Nothing more to read.
            le_mem_Release(appNamePtr);
            break;
        }

        LE_FATAL_IF(result == LE_OVERFLOW, "The app name '%s...' in the smack map file is too \
long.  The smack map file may be corrupted.", appNamePtr);

        LE_FATAL_IF(result == LE_FAULT, "Error reading smack map file.");

        // Read the smack label.
        char* labelPtr = le_mem_ForceAlloc(LabelPool);

        result = fd_ReadLine(fd, labelPtr, LIMIT_MAX_SMACK_LABEL_BYTES);

        LE_FATAL_IF(result == LE_OVERFLOW, "The smack label '%s...' in the smack map file is too \
long.  The smack map file may be corrupted.", labelPtr);

        LE_FATAL_IF(result == LE_FAULT, "Error reading smack map file.");

        // Store the appname and smack label in our hash map.
        LE_ASSERT(le_hashmap_Put(LabelMap, labelPtr, appNamePtr) == NULL);
    }

    fd_Close(fd);
};


//--------------------------------------------------------------------------------------------------
/**
 * Exports the app name to smack label mapping in the hash map to the map file.
 *
 * @note
 *      This function will kill the calling process if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static void ExportMapping
(
    le_hashmap_Ref_t map            ///< [IN] The hash map containing the mappings.
)
{
    if (access(MAPPING_FILE, F_OK) == 0)
    {
        // Create a backup of the map file if the map file exists.
        LE_FATAL_IF(rename(MAPPING_FILE, MAPPING_FILE_BACKUP) != 0,
                    "Could not move smack map file from %s to %s.  %m",
                    MAPPING_FILE,
                    MAPPING_FILE_BACKUP);
    }

    // Open the map file for writing.
    FILE* filePtr;

    do
    {
        filePtr = fopen(MAPPING_FILE, "w");
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Could not create file '%s'.  %m.", MAPPING_FILE);

    // Write the highest order string first.
    LE_FATAL_IF(fprintf(filePtr, "%s\n", HighestOrderStr) < 0,
                "Could not write '%s' to file '%s'.  %m.", HighestOrderStr, MAPPING_FILE);

    // Iterate over the hash map and write the app names and smack labels to the file.
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(map);

    while(le_hashmap_NextNode(iter) == LE_OK)
    {
        // Write the app name to the file.
        char* appNamePtr = (char*)le_hashmap_GetValue(iter);
        LE_FATAL_IF(fprintf(filePtr, "%s\n", appNamePtr) < 0,
                    "Could not write app name '%s' to file '%s'.  %m.", appNamePtr, MAPPING_FILE);

        // Write the label name to the file.
        char* labelPtr = (char*)le_hashmap_GetKey(iter);
        LE_FATAL_IF(fprintf(filePtr, "%s\n", labelPtr) < 0,
                    "Could not write label '%s' to file '%s'.  %m.", labelPtr, MAPPING_FILE);
    }

    // Flush stream data to file.
    int result;
    do
    {
        result = fflush(filePtr);
    }
    while ( (result != 0) && (errno == EINTR) );

    LE_FATAL_IF(result != 0, "Could not flush buffered data to file '%s'.  %m.", MAPPING_FILE);

    // Flush the file to disk.
    int fd = fileno(filePtr);
    LE_FATAL_IF(fd == -1, "Could not get file descriptor of file stream '%s'.  %m", MAPPING_FILE);
    LE_FATAL_IF(fsync(fd) == -1, "Could not flush file '%s' to disk.  %m.", MAPPING_FILE);

    // Close the file.
    do
    {
        result = fclose(filePtr);
    }
    while ( (result != 0) && (errno == EINTR) );

    LE_FATAL_IF(result != 0, "Could not close file '%s'.  %m.", MAPPING_FILE);

    // Delete the back-up file we created.
    LE_FATAL_IF((unlink(MAPPING_FILE_BACKUP) == -1) && (errno != ENOENT),
                "Could not delete file '%s'.  %m.", MAPPING_FILE_BACKUP);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application name of the process with the specified PID.
 *
 * @return
 *      LE_OK if the application name was successfully found.
 *      LE_OVERFLOW if the application name could not fit in the provided buffer.
 *      LE_NOT_FOUND if the process is not part of an application.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t appSmack_GetName
(
    int pid,                ///< [IN] PID of the process.
    char* bufPtr,           ///< [OUT] Buffer to hold the name of the app.
    size_t bufSize          ///< [IN] Size of the buffer.
)
{
    // Get the SMACK label for the process.
    char smackLabel[LIMIT_MAX_SMACK_LABEL_BYTES];

    le_result_t result = smack_GetProcLabel(pid, smackLabel, sizeof(smackLabel));

    if (result != LE_OK)
    {
        return result;
    }

    // Find the smack label in the hashmap.
    char* appNamePtr = le_hashmap_Get(LabelMap, smackLabel);

    if (appNamePtr == NULL)
    {
        return LE_NOT_FOUND;
    }

    // Copy the app name to the user's buffer.
    return le_utf8_Copy(bufPtr, appNamePtr, bufSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the application's SMACK label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 */
//--------------------------------------------------------------------------------------------------
void appSmack_GetLabel
(
    const char* appName,
        ///< [IN]
        ///< The name of the application.

    char* label,
        ///< [OUT]
        ///< The smack label for the application.

    size_t labelNumElements
        ///< [IN]
)
{
    // Look for the app by iteratoring over the hashmap.
    le_hashmap_It_Ref_t iter = le_hashmap_GetIterator(LabelMap);

    while (le_hashmap_NextNode(iter) == LE_OK)
    {
        if (strcmp(appName, le_hashmap_GetValue(iter)) == 0)
        {
            // Found the app.  Copy the label to the user's buffer.
            const char* appLabelPtr = le_hashmap_GetKey(iter);

            if (le_utf8_Copy(label, appLabelPtr, labelNumElements, NULL) != LE_OK)
            {
                LE_KILL_CLIENT("User buffer is too small to hold SMACK label %s for app %s.",
                               appLabelPtr, appName);
            }

            return;
        }
    }

    // The app does not have a SMACK label yet, we need to create one.
    // Get a unique string to base the label on.  The unique string must be shorter than the max
    // label size so that we can prepend the app prefix to it.
    char uniqueStr[MAX_BASE_LABEL_BYTES];

    LE_FATAL_IF(le_utf8_GetMonotonicString(HighestOrderStr, uniqueStr, sizeof(uniqueStr)) != LE_OK,
                "Could not create SMACK label for application '%s'.", appName);

    // Update the HighestOrderStr.
    LE_ASSERT(le_utf8_Copy(HighestOrderStr, uniqueStr, sizeof(HighestOrderStr), NULL) == LE_OK);

    // Create the label.
    char* labelPtr = le_mem_ForceAlloc(LabelPool);

    LE_FATAL_IF(snprintf(labelPtr, LIMIT_MAX_SMACK_LABEL_BYTES, "app.%s", uniqueStr) >= LIMIT_MAX_SMACK_LABEL_BYTES,
                "Smack label 'app.%s' is too long.", uniqueStr);

    // Store the app name.
    char* namePtr = le_mem_ForceAlloc(AppNamePool);

    if (le_utf8_Copy(namePtr, appName, LIMIT_MAX_APP_NAME_BYTES, NULL) != LE_OK)
    {
        LE_KILL_CLIENT("App name '%s' is too long.", appName);
        return;
    }

    // Put the app name and label in the hash map.
    LE_ASSERT(le_hashmap_Put(LabelMap, labelPtr, namePtr) == NULL);

    // Export the mapping to the mappings file.
    ExportMapping(LabelMap);

    // Copy the label to the user buffer.
    if (le_utf8_Copy(label, labelPtr, labelNumElements, NULL) != LE_OK)
    {
        LE_KILL_CLIENT("Smack label '%s' for app '%s' is too long.", labelPtr, appName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get's the application's smack label with the access mode appended to it as a string.  For
 * example, if the accessMode is ACCESS_FLAG_READ | ACCESS_FLAG_WRITE then "rw" will be appended to
 * the application's smack label.  If the accessMode is 0 (empty) then "-" will be appended to the
 * app's smack label.
 *
 * @note
 *      The application need not be installed for this function to succeed.
 */
//--------------------------------------------------------------------------------------------------
void appSmack_GetAccessLabel
(
    const char* appName,
        ///< [IN]
        ///< The name of the application.

    appSmack_AccessFlags_t accessMode,
        ///< [IN]
        ///< The access mode flags.

    char* label,
        ///< [OUT]
        ///< The smack label for the application.

    size_t labelNumElements
        ///< [IN]
)
{
    // Get the app label.
    char appLabel[LIMIT_MAX_SMACK_LABEL_LEN];
    appSmack_GetLabel(appName, appLabel, sizeof(appLabel));

    // Translate the accessMode to a string.
    char modeStr[4] = "-";
    int index = 0;

    if ((accessMode & APPSMACK_ACCESS_FLAG_READ) > 0)
    {
        modeStr[index++] = 'r';
    }
    if ((accessMode & APPSMACK_ACCESS_FLAG_WRITE) > 0)
    {
        modeStr[index++] = 'w';
    }
    if ((accessMode & APPSMACK_ACCESS_FLAG_EXECUTE) > 0)
    {
        modeStr[index++] = 'x';
    }

    modeStr[index] = '\0';

    // Create the access label.
    if (snprintf(label, labelNumElements, "%s%s", appLabel, modeStr) >= labelNumElements)
    {
        LE_KILL_CLIENT("User buffer is too small to hold SMACK access label %s for app %s.",
                       appLabel, appName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * App info's initialization function.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create the security directory if it doesn't exist.
    LE_FATAL_IF(le_dir_MakePath(SECURITY_DIR, S_IRWXU) != LE_OK,
                "Could not create directory %s.  %m.", SECURITY_DIR);

    // Create the memory pools.
    AppNamePool = le_mem_CreatePool("AppNamePool", LIMIT_MAX_APP_NAME_BYTES);
    LabelPool = le_mem_CreatePool("LabelPool", LIMIT_MAX_SMACK_LABEL_BYTES);

    // Create the hash map.
    LabelMap = le_hashmap_Create("AppsSmackLabels", ESTIMATED_MAX_NUM_APPS,
                                 le_hashmap_HashString, le_hashmap_EqualsString);

    // Restore the map back up file if there is one.
    if (access(MAPPING_FILE_BACKUP, F_OK) == 0)
    {
        LE_FATAL_IF(rename(MAPPING_FILE_BACKUP, MAPPING_FILE) != 0,
                    "Could not move smack map file from %s to %s.  %m",
                    MAPPING_FILE_BACKUP,
                    MAPPING_FILE);

    }

    // Load the initial appName-label mappings into our hash map.
    ImportMapping(LabelMap);
}
