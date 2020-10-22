/**
 * @file le_cfg.c
 *
 * Config Tree APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_atomFile.h"
#include <stdio.h>
#include <errno.h>

/** Configuration directory. */
#define MAX_APP_NAME_SIZE              100

/** Space needed to store a config path (file.option.key). */
#define FULL_PATH_BUFFER_SIZE          512

/** Common warning for write calls. */
#define WARN_READ_IGNORE()      LE_WARN("Ignoring %s call in read transaction", __func__)

/** Base Path of Config Tree */
#define CONFIG_TREE_BASE_PATH   "/config/"

/** Base Path of Config Tree */
#define NODE_EXTENSION   ".node"

// TODO: convert this to a KConfig value once master is next merged back
#define LE_CONFIG_MAX_CFG_ITERATORS 8

//--------------------------------------------------------------------------------------------------
/**
 * Node Types
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    NODE_TYPE_EMPTY,
    NODE_TYPE_BOOL,
    NODE_TYPE_INTEGER,
    NODE_TYPE_FLOAT,
    NODE_TYPE_STRING,
    NODE_TYPE_BINARY_ARRAY
}
NodeType_t;

//--------------------------------------------------------------------------------------------------
/**
 * Transaction iterator type for ALT1250/ThreadX config interface.
 */
//--------------------------------------------------------------------------------------------------
struct le_cfg_IteratorImpl
{
    bool    isWrite;                          ///< Is this a write transaction?
    char    fileStr[FULL_PATH_BUFFER_SIZE];   ///< Config file name.
    char    pathStr[FULL_PATH_BUFFER_SIZE];   ///< Iterator path.
};

//--------------------------------------------------------------------------------------------------
/**
 * Static memory for the config iterator pool.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ConfigIterator, LE_CONFIG_MAX_CFG_ITERATORS,
    sizeof(struct le_cfg_IteratorImpl));

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for config iterator objects.
 *
 * On RTOS this is shared across all components.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ConfigIteratorPool;

//--------------------------------------------------------------------------------------------------
/**
 * Static safe reference map for config iterators.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(ConfigIterator, LE_CONFIG_MAX_CFG_ITERATORS);

//--------------------------------------------------------------------------------------------------
/**
 * The reference map for config iterator objects.
 *
 * On RTOS this is shared across all components.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ConfigIteratorMap;

//--------------------------------------------------------------------------------------------------
/**
 * Get's the Node Type text
 */
//--------------------------------------------------------------------------------------------------
static const char* _Get_Node_Type_Txt
(
    NodeType_t nodeCode  ///< [in] The node code value to be translated.
)
{
    switch (nodeCode)
    {
        case NODE_TYPE_EMPTY:
            return "NODE_TYPE_EMPTY";
        case NODE_TYPE_BOOL:
            return "NODE_TYPE_BOOL";
        case NODE_TYPE_INTEGER:
            return "NODE_TYPE_INTEGER";
        case NODE_TYPE_FLOAT:
            return "NODE_TYPE_FLOAT";
        case NODE_TYPE_STRING:
            return "NODE_TYPE_STRING";
        case NODE_TYPE_BINARY_ARRAY:
            return "NODE_TYPE_BINARY_ARRAY";
        default:
            return "UNKNOWN";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Create a new transaction iterator.
 *
 *  @return The new iterator.
 */
//--------------------------------------------------------------------------------------------------
static struct le_cfg_IteratorImpl *OpenSession
(
    const char  *basePathPtr,   ///< Iterator config tree path.
    bool         isWrite        ///< Is this a write (true) or a read (false)?
)
{
    char                         appNameStr[MAX_APP_NAME_SIZE];
    const char                  *c;
    le_result_t                  result;
    pid_t                        clientPid;
    size_t                       length;
    struct le_cfg_IteratorImpl  *it = le_mem_AssertAlloc(ConfigIteratorPool);

    it->isWrite = isWrite;
    c = strchr(basePathPtr, ':');
    if (c == NULL)
    {
        // No tree name at beginning of path, so use app name
        c = basePathPtr;
        LE_ASSERT(*c == '/');

        result = le_msg_GetClientProcessId(le_cfg_GetClientSessionRef(), &clientPid);
        LE_ASSERT(result == LE_OK);
        result = le_appInfo_GetName(clientPid, appNameStr, sizeof(appNameStr));
        LE_ASSERT(result == LE_OK);

        LE_DEBUG("Using app name '%s' as file name", appNameStr);
        strlcpy(it->fileStr, appNameStr, sizeof(it->fileStr));
    }
    else
    {
        // Copy out tree name from beginning of path and advance to path proper
        length = c - basePathPtr;
        LE_ASSERT(length < sizeof(it->fileStr));

        if (length)
        {
            strncpy(it->fileStr, basePathPtr, length);
        }
        it->fileStr[length] = '\0';
        ++c;
        LE_ASSERT(*c != '\0' && strlen(c) < sizeof(it->pathStr));
    }

    strlcpy(it->pathStr, c, sizeof(it->pathStr));
    LE_DEBUG("From iterator base path '%s', determined file name '%s' and path '%s'",
        basePathPtr, it->fileStr, it->pathStr);

    return it;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Create all the directories for a given path
 */
//--------------------------------------------------------------------------------------------------
static void BuildPathDirectories
(
    char     *pathBufferStr   ///< [IN]  Buffer to write composed config
                              ///<       path into.  Assumed to be at least
                              ///<       FULL_PATH_BUFFER_SIZE bytes in size.
)
{
    // Check if there's a directory part of the path passed in
    char *c = strrchr(pathBufferStr, '/');

    // There has to be a single directory in the path
    LE_ASSERT(c != NULL);
    *c = '\0';

    // Check if directory exists, if it doesn't create it
    if (!le_dir_IsDir(pathBufferStr))
    {
        le_dir_MakePath(pathBufferStr, S_IRWXU);
    }

    // Reset value to '/'
    *c = '/';
}

//--------------------------------------------------------------------------------------------------
/**
 *  Format a config path from config tree components.
 */
//--------------------------------------------------------------------------------------------------
static void FormatPath
(
    struct le_cfg_IteratorImpl  *it,            ///< [IN]  Transaction iterator.
    const char                  *pathStr,       ///< [IN]  Request path (relative or absolute).
    char                        *pathBufferStr  ///< [OUT] Buffer to write composed config
                                                ///<       path into.  Assumed to be at least
                                                ///<       FULL_PATH_BUFFER_SIZE bytes in size.
)
{
    // Copy config tree base folder to path
    strlcpy(pathBufferStr, CONFIG_TREE_BASE_PATH, FULL_PATH_BUFFER_SIZE);

    // Add the path provided by starting the transaction
    if (it->fileStr[0] != '\0')
    {
        strlcat(pathBufferStr, it->fileStr, FULL_PATH_BUFFER_SIZE);
    }

    if (pathStr == NULL || pathStr[0] == '\0')
    {
        // No path passed in, just append iterator's
        strlcat(pathBufferStr, it->pathStr, FULL_PATH_BUFFER_SIZE);
    }
    else
    {
        if (pathStr[0] == '/')
        {
            // Absolute path passed in, override iterator's and append parameter
            strlcat(pathBufferStr, pathStr, FULL_PATH_BUFFER_SIZE);
        }
        else
        {
            // Relative path passed in, append iterator's followed by parameter. Do not add
            // iterator path if it's a single forward slash
            if (!((strlen(it->pathStr) == 1) && (*(it->pathStr) == '/')))
            {
                strlcat(pathBufferStr, it->pathStr, FULL_PATH_BUFFER_SIZE);
            }

            strlcat(pathBufferStr, "/", FULL_PATH_BUFFER_SIZE);
            strlcat(pathBufferStr, pathStr, FULL_PATH_BUFFER_SIZE);
        }
    }

    LE_DEBUG("From iterator file name '%s', iterator path '%s', and request path '%s',"
        "constructed config path '%s'", it->fileStr, it->pathStr, pathStr != NULL ? pathStr : "",
        pathBufferStr);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Read a value from persistent storage.
 *
 *  @return
 *      - Number of bytes read
 *      - LE_NOT_FOUND  - Lookup failed or requested entry was not found.
 */
//--------------------------------------------------------------------------------------------------
static int ReadValue
(
    struct le_cfg_IteratorImpl  *it,            ///< [IN]  Transaction iterator.
    const char                  *pathPtr,       ///< [IN]  Request path (relative or absolute).
    NodeType_t                   nodeType,      ///< [IN]  Node type to read
    size_t                       bufferSize,    ///< [IN]  Size of output buffer in bytes.
    void                        *bufferPtr      ///< [OUT] Buffer to store read value in.  May be
                                                ///<       NULL to just test for the existence of
                                                ///<       the entry.
)
{
    char        fullPathStr[FULL_PATH_BUFFER_SIZE];
    NodeType_t  nodeTypeRead = NODE_TYPE_EMPTY;
    ssize_t     bytesRead = 0;
    int         fd = 0;
    le_result_t result = LE_OK;
    struct stat fileStat = {0};
    int result_atomfile = 0;

    LE_ASSERT(bufferPtr != NULL);
    memset(bufferPtr, 0, bufferSize);
    FormatPath(it, pathPtr, fullPathStr);

    // Add NODE_EXTENSION to the end of the file
    strlcat(fullPathStr, NODE_EXTENSION, FULL_PATH_BUFFER_SIZE);

    fd = le_atomFile_Open(fullPathStr, LE_FLOCK_READ);
    if (fd < 0)
    {
        LE_CRIT("Failed to open file %s", fullPathStr);
        return LE_NOT_FOUND;
    }

    le_fd_Fstat(fd, &fileStat);
    if (fileStat.st_size > (bufferSize + 1))
    {
        LE_CRIT("Buffer is not large enough to read %s", fullPathStr);
        return LE_OVERFLOW;
    }

    // Read the first byte which specifies what is stored by the node
    if (le_fd_Read(fd, &nodeTypeRead, 1) < 0)
    {
        LE_CRIT("Error in reading node type of file '%s'", fullPathStr);
        result = LE_FAULT;
        goto error;
    }
    // Return LE_NOT_FOUND if the node type is wrong for the API
    else if (nodeTypeRead != nodeType)
    {
        LE_CRIT("Node \'%s\' is of type %s but API called is for type %s",
            fullPathStr, _Get_Node_Type_Txt(nodeTypeRead), _Get_Node_Type_Txt(nodeType));
        result = LE_NOT_FOUND;
        goto error;
    }

    bytesRead = le_fd_Read(fd, (uint8 *)bufferPtr, bufferSize);
    if (bytesRead < 0)
    {
        LE_CRIT("Error (%s) reading buffer from file '%s'", strerror(errno), fullPathStr);
        result = LE_FAULT;
        goto error;
    }

    result_atomfile = le_atomFile_Close(fd);
    if (result_atomfile < 0)
    {
        LE_ERROR("Failed to close atomic file '%s'", fullPathStr);
        return LE_FAULT;
    }

    return bytesRead;

error:
    le_atomFile_Cancel(fd);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Write a value to persistent storage.
 *
 *  @return
 *      - LE_OK     - Value written successfully.
 *      - LE_FAULT  - Error writing value.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteValue
(
    struct le_cfg_IteratorImpl  *it,        ///< [IN]  Transaction iterator.
    const char                  *pathPtr,   ///< [IN]  Request path (relative or absolute).
    NodeType_t                  nodeType,   ///< [IN]  Node type to write
    void*                       *bufPtr,    ///< [IN]  Pointer to data buffer to write to file
    size_t                      bufSize     ///< [IN]  Size of buffer to write
)
{
    char fullPathStr[FULL_PATH_BUFFER_SIZE];
    int bytesWr = 0;
    int fd;
    le_flock_AccessMode_t accessMode = LE_FLOCK_WRITE;

    FormatPath(it, pathPtr, fullPathStr);
    BuildPathDirectories(fullPathStr);

    // Add NODE_EXTENSION to the end of the file
    strlcat(fullPathStr, NODE_EXTENSION, FULL_PATH_BUFFER_SIZE);

    fd = le_atomFile_Create(fullPathStr,
                            accessMode,
                            LE_FLOCK_REPLACE_IF_EXIST,
                            S_IRWXU);
    if (fd < 0)
    {
        LE_CRIT("Failed to open file %s", fullPathStr);
        return LE_FAULT;
    }

    if ((nodeType != NODE_TYPE_EMPTY) && bufPtr)
    {
        // Write the first byte as the node type
        bytesWr = le_fd_Write(fd, (const uint8 *)&nodeType, 1);
        if (bytesWr == -1)
        {
            LE_CRIT("Error (%d) while writing node type to the file '%s'", errno, fullPathStr);
            goto error;
        }

        // If size of the buffer to write is 0, no need to write to fd
        if (bufSize != 0)
        {
            bytesWr = le_fd_Write(fd, (const uint8 *)bufPtr, bufSize);
            if (bytesWr == -1)
            {
                LE_CRIT("Error (%d) while writing buffer to file '%s'", errno, fullPathStr);
                goto error;
            }
        }
    }

    return le_atomFile_Close(fd);

error:
    le_atomFile_Cancel(fd);
    return LE_FAULT;
}

// -------------------------------------------------------------------------------------------------
// Implemented API Functions
// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 *
 *  If the path is empty, the iterator's current node is deleted.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_DeleteNode
(
    le_cfg_IteratorRef_t externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char *pathPtr               ///< [IN] Absolute or relative path to the node to delete.
                                      ///<      If absolute path is given, it's rooted off of the
                                      ///<      user's root node.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    char fullPathStr[FULL_PATH_BUFFER_SIZE];

    LE_ASSERT(pathPtr != NULL);

    FormatPath(it, pathPtr, fullPathStr);

    // Remove stem
    // TODO: Change this to atomic remove recursive
    le_result_t result = le_dir_RemoveRecursive(fullPathStr);
    LE_ASSERT(result == LE_OK);

    // Remove leaf
    strlcat(fullPathStr, NODE_EXTENSION, FULL_PATH_BUFFER_SIZE);
    le_atomFile_Delete(fullPathStr);

    return;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @note:  On ThreadX, the configuration mechanism does not support transactions.  In this case the
 *          reads and writes are just done directly.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char *basePathPtr ///< [IN] Path to the location to create the new iterator.
)
{
    return le_ref_CreateRef(ConfigIteratorMap, OpenSession(basePathPtr, false));
}

// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 *
 *  @note:  On ThreadX, the configuration mechanism does not support transactions.  In this case the
 *          reads and writes are just done directly.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *basePathPtr ///< [IN] Path to the location to create the new iterator.
)
{
    return le_ref_CreateRef(ConfigIteratorMap, OpenSession(basePathPtr, true));
}

// -------------------------------------------------------------------------------------------------
/**
 *  Close the write iterator and commit the write transaction.  This updates the config tree
 *  with all of the writes that occured using the iterator.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator object to commit.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    le_ref_DeleteRef(ConfigIteratorMap, externalRef);
    le_mem_Release(it);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Close and free the given iterator object.  If the iterator is a write iterator, the transaction
 *  will be canceled.  If the iterator is a read iterator, the transaction will be closed.
 *
 *  @note: This operation will also delete the iterator object.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t externalRef ///< [IN] Iterator object to close.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        LE_WARN("Configuration write transactions are not cancellable on RTOS Devices");
    }
    le_ref_DeleteRef(ConfigIteratorMap, externalRef);
    le_mem_Release(it);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Check if the given node is empty.  A node is considered empty if it has no value.  A node is
 *  also considered empty if it doesn't yet exist.
 *
 *  If the path is empty, the iterator's current node is queried for emptiness.
 *
 *  Valid for both read and write transactions.
 *
 *  @return True if the node is considered empty, false if not.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_IsEmpty
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    char                         fullPathStr[FULL_PATH_BUFFER_SIZE];
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    struct stat                  fileStat;
    int                          result;

    if (it == NULL)
    {
        return true;
    }

    FormatPath(it, pathPtr, fullPathStr);
    strlcat(fullPathStr, NODE_EXTENSION, FULL_PATH_BUFFER_SIZE);

    result = stat(fullPathStr, &fileStat);

    if (-1 == result)
    {
        return true;
    }
    else if(fileStat.st_size > 0)
    {
        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the node's value.  If it doesn't exist it will be created, but have no value.
 *
 *  If the path is empty, the iterator's current node will be cleared.  If the node is a stem,
 *  all children will be removed from the tree.
 *
 *  Only valid during a write transaction.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetEmpty
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, pathPtr, NODE_TYPE_EMPTY, NULL, 0);
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node in the configuration tree exists.
 *
 *  @return True if the specified node exists in the tree.  False if not.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr      ///< [IN] Absolute or relative path to read from.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return false;
    }

    char fullPathStr[FULL_PATH_BUFFER_SIZE];
    FormatPath(it, pathPtr, fullPathStr);

    // Check if it's a stem
    if (le_dir_IsDir(fullPathStr))
    {
        return true;
    }

    // Check if it's a leaf
    strlcat(fullPathStr, NODE_EXTENSION, FULL_PATH_BUFFER_SIZE);
    return (access(fullPathStr, F_OK) == 0);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a binary data from the configuration tree.  If the the node has a wrong type, is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  \b Responds \b With:
 *
 *  This function will respond with one of the following values:
 *
 *          - LE_OK             - Read was completed successfully.
 *          - LE_FORMAT_ERROR   - if data can't be decoded.
 *          - LE_OVERFLOW       - Supplied buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetBinary
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char* LE_NONNULL path,      ///< [IN] Path to the target node. Can be an absolute path,
                                      ///<      or a path relative from the iterator's current
                                      ///<      position.
    uint8_t* valuePtr,                ///< [OUT] Buffer to write the value into.
    size_t* valueSizePtr,             ///< [INOUT]
    const uint8_t* defaultValuePtr,   ///< [IN] Default value to use if the original can't be
                                      ///<      read.
    size_t defaultValueSize           ///< [IN] Default value size.
)
{
    int                          bytesRead;
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, iteratorRef);

    if (it == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    bytesRead = ReadValue(it, path, NODE_TYPE_BINARY_ARRAY, *valueSizePtr, (void *)valuePtr);

    if (bytesRead < 0)
    {
        if (defaultValueSize <= *valueSizePtr)
        {
            memcpy(valuePtr, defaultValuePtr, defaultValueSize);
            *valueSizePtr = defaultValueSize;
        }

        if (bytesRead == LE_OVERFLOW)
        {
            return LE_OVERFLOW;
        }
        else
        {
            return LE_OK;
        }
    }
    else
    {
        *valueSizePtr = bytesRead;
        return LE_OK;
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary data to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBinary
(
    le_cfg_IteratorRef_t iteratorRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char* LE_NONNULL path,      ///< [IN] Path to the target node. Can be an absolute path, or
                                      ///<      a path relative from the iterator's current position.
    const uint8_t* valuePtr,          ///< [IN] Value to write.
    size_t valueSize                  ///< [IN] Size of the binary data.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, iteratorRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, path, NODE_TYPE_BINARY_ARRAY, (void *)valuePtr, valueSize);
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a string value from the configuration tree.  If the value isn't a string, or if the node is
 *  empty or doesn't exist, the default value will be returned.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return
 *          - LE_OK            - Read was completed successfully.
 *          - LE_OVERFLOW      - Supplied string buffer was not large enough to hold the value.
 *          - LE_BAD_PARAMETER - Config iterator was invalid.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t     externalRef,       ///< [IN]  Iterator to use as a basis for the
                                                ///<       transaction.
    const char              *pathPtr,           ///< [IN]  Absolute or relative path to read from.
    char                    *valueStr,          ///< [OUT] Buffer to write the value into.
    size_t                   valueNumElements,  ///< [IN]  Size of value buffer.
    const char              *defaultValueStr    ///< [IN]  Default value to use if none can be read.
)
{
    int                          bytesRead;
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);

    if (it == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    bytesRead = ReadValue(it, pathPtr, NODE_TYPE_STRING, valueNumElements, (void *)valueStr);

    if (bytesRead < 0)
    {
        strlcpy(valueStr, defaultValueStr, valueNumElements);
        valueStr[valueNumElements - 1] = '\0';

        if (bytesRead == LE_OVERFLOW)
        {
            return LE_OVERFLOW;
        }
        else
        {
            return LE_OK;
        }
    }

    return LE_OK;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a string value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetString
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    const char              *valueStr     ///< [IN] Value to write.
)
{
    struct le_cfg_IteratorImpl *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, pathPtr, NODE_TYPE_STRING, (void *)valueStr, strlen(valueStr));
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a signed integer value from the configuration tree.
 *
 *  If the underlying value is not an integer, the default value will be returned instead.  The
 *  default value is also returned if the node does not exist or if it's empty.
 *
 *  If the value is a floating point value, it will be truncated and returned as an integer.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The integer value.
 */
// -------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    int32_t                  defaultValue ///< [IN] Default value to use if none can be read.
)
{
    int32_t                      value;
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);

    if (it == NULL)
    {
        return defaultValue;
    }

    if (ReadValue(it, pathPtr, NODE_TYPE_INTEGER, sizeof(value), (void *)&value) < 0)
    {
        return defaultValue;
    }

    return value;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree.  Only valid during a
 *  write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    int32_t                  value        ///< [IN] Value to write.
)
{
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, pathPtr, NODE_TYPE_INTEGER, (void *)&value, sizeof(value));
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a 64-bit floating point value from the configuration tree.
 *
 *  If the value is an integer, the value will be promoted to a float.  Otherwise, if the
 *  underlying value is not a float or integer, the default value will be returned.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The double value.
 */
// -------------------------------------------------------------------------------------------------
double le_cfg_GetFloat
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    double                   defaultValue ///< [IN] Default value to use if none can be read.
)
{
    double                       value;
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);

    if (it == NULL)
    {
        return defaultValue;
    }

    if (ReadValue(it, pathPtr, NODE_TYPE_FLOAT, sizeof(value), (void *)&value) < 0)
    {
        return defaultValue;
    }

    return value;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a 64-bit floating point value to the configuration tree.  Only valid
 *  during a write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetFloat
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    double                   value        ///< [IN] Value to write.
)
{
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, pathPtr, NODE_TYPE_FLOAT, (void *)&value, sizeof(value));
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Read a value from the tree as a boolean.  If the node is empty or doesn't exist, the default
 *  value is returned.  The default value is also returned if the node is of a different type than
 *  expected.
 *
 *  Valid for both read and write transactions.
 *
 *  If the path is empty, the iterator's current node will be read.
 *
 *  @return The boolean value.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_GetBool
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to read.
    bool                     defaultValue ///< [IN] Default value to use if none can be read.
)
{
    bool                         value;
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);

    if (it == NULL)
    {
        return defaultValue;
    }

    if (ReadValue(it, pathPtr, NODE_TYPE_BOOL, sizeof(value), (void *)&value) < 0)
    {
        return defaultValue;
    }

    return value;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a boolean value to the configuration tree.  Only valid during a write
 *  transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetBool
(
    le_cfg_IteratorRef_t     externalRef, ///< [IN] Iterator to use as a basis for the transaction.
    const char              *pathPtr,     ///< [IN] Full or relative path to the value to write.
    bool                     value        ///< [IN] Value to write.
)
{
    struct le_cfg_IteratorImpl  *it = le_ref_Lookup(ConfigIteratorMap, externalRef);
    if (it == NULL)
    {
        return;
    }

    if (it->isWrite)
    {
        WriteValue(it, pathPtr, NODE_TYPE_BOOL, (void *)&value, sizeof(value));
    }
    else
    {
        WARN_READ_IGNORE();
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Delete the node specified by the path.  If the node doesn't exist, nothing happens.  All child
 *  nodes are also deleted.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickDeleteNode
(
    const char *pathPtr ///< [IN] Path to the node to delete.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathPtr);
    le_cfg_DeleteNode(ref, pathPtr);
    le_cfg_CommitTxn(ref);
}


// -------------------------------------------------------------------------------------------------
/**
 * Clears the current value of a node. If the node doesn't currently exist then it is created as a
 * new empty node.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetEmpty
(
    const char *pathStr ///< Absolute or relative path to read from.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathStr);
    le_cfg_SetEmpty(ref, NULL);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * @return - LE_OK       - Commit was completed successfully.
 *         - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_QuickGetString
(
    const char  *pathStr,           ///< [IN]  Absolute or relative path to read from.
    char        *valueStr,          ///< [OUT] Buffer to write the value into.
    size_t       valueNumElements,  ///< [IN]  Size of value buffer.
    const char  *defaultValueStr    ///< [IN]  Default value to use if none can be read.
)
{
    le_cfg_IteratorRef_t    ref = le_cfg_CreateReadTxn(pathStr);
    le_result_t             result = le_cfg_GetString(ref, NULL, valueStr, valueNumElements,
                                defaultValueStr);

    le_cfg_CancelTxn(ref);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a string value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetString
(
    const char *pathStr,    ///< Absolute or relative path to read from.
    const char *valueStr    ///< Value to write.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathStr);
    le_cfg_SetString(ref, NULL, valueStr);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary value to the configuration tree.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_cfg_QuickGetBinary
(
    const char* LE_NONNULL path,     ///< [IN] Path to the target node.
    uint8_t* valuePtr,               ///< [OUT] Buffer to write the value into.
    size_t* valueSizePtr,            ///< [INOUT]
    const uint8_t* defaultValuePtr,  ///< [IN] Default value to use if the original can't be read
    size_t defaultValueSize          ///< [IN] Size of default value
)
{
    le_cfg_IteratorRef_t    ref = le_cfg_CreateReadTxn(path);
    le_result_t             result = le_cfg_GetBinary(ref, NULL, valuePtr, valueSizePtr,
                                defaultValuePtr, defaultValueSize);
    le_cfg_CancelTxn(ref);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a binary value to the configuration tree.
 *
 *  @note Binary data cannot be written to the 'system' tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBinary
(
    const char* LE_NONNULL path, ///< [IN] Path to the target node.
    const uint8_t* valuePtr,     ///< [IN] Value to write.
    size_t valueSize             ///< [IN] Size of value
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(path);
    le_cfg_SetBinary(ref, NULL, valuePtr, valueSize);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a signed integer value from the config tree. If the value is a floating point
 * value, then it will be rounded and returned as an integer. Otherwise If the underlying value is
 * not an integer or a float, the default value will be returned instead.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
int32_t le_cfg_QuickGetInt
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    int32_t      defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    le_cfg_IteratorRef_t    ref = le_cfg_CreateReadTxn(pathStr);
    int32_t                 result = le_cfg_GetInt(ref, NULL, defaultValue);

    le_cfg_CancelTxn(ref);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a signed integer value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetInt
(
    const char  *pathStr,   ///< [IN]   Absolute or relative path to read from.
    int32_t      value      ///< [IN]   Value to write.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathStr);
    le_cfg_SetInt(ref, NULL, value);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a 64-bit floating point value from the config tree. If the value is an integer,
 * then it is promoted to a float. Otherwise, if the underlying value is not a float, or an
 * integer the default value will be returned.
 *
 * If the value is empty or the node doesn't exist, the default value is returned.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
// -------------------------------------------------------------------------------------------------
double le_cfg_QuickGetFloat
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    double       defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    le_cfg_IteratorRef_t    ref = le_cfg_CreateReadTxn(pathStr);
    double                  result = le_cfg_GetFloat(ref, NULL, defaultValue);

    le_cfg_CancelTxn(ref);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a 64-bit floating point value to the config tree.
 *
 * @note Floating point values will only be stored up to 6 digits of precision.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetFloat
(
    const char  *pathStr,   ///< Absolute or relative path to read from.
    double       value      ///< Value to write.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathStr);
    le_cfg_SetFloat(ref, NULL, value);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
/**
 * Reads a value from the tree as a boolean. If the node is empty or doesn't exist, the default
 * value is returned. This is also true if the node is a different type than expected.
 *
 * If the value is empty or the node doesn't exist, the default value is returned instead.
 */
// -------------------------------------------------------------------------------------------------
bool le_cfg_QuickGetBool
(
    const char  *pathStr,       ///< [IN]  Absolute or relative path to read from.
    bool         defaultValue   ///< [IN]  Default value to use if none can be read.
)
{
    le_cfg_IteratorRef_t    ref = le_cfg_CreateReadTxn(pathStr);
    bool                    result = le_cfg_GetBool(ref, NULL, defaultValue);

    le_cfg_CancelTxn(ref);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Writes a boolean value to the config tree.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_QuickSetBool
(
    const char  *pathStr,   ///< Absolute or relative path to read from.
    bool         value      ///< Value to write.
)
{
    le_cfg_IteratorRef_t ref = le_cfg_CreateWriteTxn(pathStr);
    le_cfg_SetBool(ref, NULL, value);
    le_cfg_CommitTxn(ref);
}

// -------------------------------------------------------------------------------------------------
// Component Initialization
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    LE_INFO("Initializing config tree");
    ConfigIteratorPool = le_mem_InitStaticPool(ConfigIterator, LE_CONFIG_MAX_CFG_ITERATORS,
                            sizeof(struct le_cfg_IteratorImpl));
    ConfigIteratorMap = le_ref_InitStaticMap(ConfigIterator, LE_CONFIG_MAX_CFG_ITERATORS);
}

COMPONENT_INIT
{
}
