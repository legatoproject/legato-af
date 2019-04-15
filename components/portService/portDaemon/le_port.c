/**
 * This module implements the port service application.
 *
 * This application manages a list of serial links (physical or emulated).
 * It provides the APIs to open/close the devices.
 * Handles the devices which are opened by default.
 * Manages devices modes (AT command and data modes).
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#if LE_CONFIG_LINUX
#   include <sys/un.h>
#endif
#include <sys/socket.h>
#include "le_port_local.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * JSON configuration file path.
 */
//--------------------------------------------------------------------------------------------------
#ifdef LEGATO_EMBEDDED
#       define JSON_CONFIG_FILE LE_CONFIG_PORT_JSON_CONFIG_FILE
#else
#       define JSON_CONFIG_FILE le_arg_GetArg(0)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of client applications.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_APPS 2

//--------------------------------------------------------------------------------------------------
/**
 * Max length for error string
 */
//--------------------------------------------------------------------------------------------------
#define ERR_MSG_MAX 256

//--------------------------------------------------------------------------------------------------
/**
 * Device mode flag for blocking / non-blocking.
 */
//--------------------------------------------------------------------------------------------------
#define BLOCKING_MODE            true
#define NON_BLOCKING_MODE        false

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of link name.
 */
//--------------------------------------------------------------------------------------------------
#define LINK_NAME_MAX_BYTES      10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of possible mode string.
 */
//--------------------------------------------------------------------------------------------------
#define POSSIBLE_MODE_MAX_BYTES  10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of ports.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PORTS                6


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of links.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LINKS                2

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of possible modes.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_POSSIBLE_MODES       2

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of device path string.
 */
//--------------------------------------------------------------------------------------------------
#define PATH_MAX_BYTES           50

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of opentype string.
 */
//--------------------------------------------------------------------------------------------------
#define OPEN_TYPE_MAX_BYTES      20

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of clients
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CLIENTS              1

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL         8

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of client socket.
 */
//--------------------------------------------------------------------------------------------------
#define CLIENT_SOCKET_MAX_BYTES  30

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of server socket.
 */
//--------------------------------------------------------------------------------------------------
#define SERVER_SOCKET_MAX_BYTES  30


//--------------------------------------------------------------------------------------------------
/**
 * Link information structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t fd;                                                     ///< The device indentifier.
    int32_t dataModeFd;                                             ///< The device indentifier
                                                                    ///< specific to data mode.
    int32_t atModeSockFd;                                           ///< Socket fd created in AT
                                                                    ///< command mode.
    int32_t dataModeSockFd;                                         ///< Socket fd created in data
                                                                    ///< mode.
    char linkName[LINK_NAME_MAX_BYTES];                             ///< Link name.
    char path[PATH_MAX_BYTES];                                      ///< Path name.
    char openingType[OPEN_TYPE_MAX_BYTES];                          ///< Device opening type.
    char possibleMode[MAX_POSSIBLE_MODES][POSSIBLE_MODE_MAX_BYTES]; ///< Possible mode name.
}
LinkInformation_t;

//--------------------------------------------------------------------------------------------------
/**
 * Lookup information for matching AT server instances to clients and links.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t      clientRef;     ///< Port service client.
    LinkInformation_t       *linkPtr;       ///< Link to the current AT server.
    le_atServer_DeviceRef_t  atServerRef;   ///< AT server reference.
    bool                     suspended;     ///< State of AT server instance.
}
ClientLinkAtServer_t;

//--------------------------------------------------------------------------------------------------
/**
 * Store the link name in the linkList. It stores the link names which are present in "OpenLinks"
 * object of JSON configuration.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char linkName[LINK_NAME_MAX_BYTES];         ///< Link name.
    le_dls_Link_t link;                         ///< Link for linkList.
}
LinkList_t;

//--------------------------------------------------------------------------------------------------
/**
 * JSON structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char                instanceName[LE_PORT_MAX_LEN_DEVICE_NAME];    ///< Instance name.
    le_dls_List_t       linkList;                                     ///< List of the links to be
                                                                      ///< opened by default.
    bool                openByDefault;                                ///< Indicates whether device
                                                                      ///< opens at system startup.
    LinkInformation_t*  linkInfo[MAX_LINKS];                          ///< Device link information
                                                                      ///< structure pointer.
    int8_t              linkCounter;                                  ///< Device link counter.
    le_dls_Link_t       link;                                         ///< Link of Instance.
}
InstanceConfiguration_t;

//--------------------------------------------------------------------------------------------------
/**
 * Device object structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_port_DeviceRef_t         deviceRef;                   ///< Device reference for client.
    InstanceConfiguration_t*    instanceConfigPtr;           ///< Instance configuration.
    le_msg_SessionRef_t         sessionRef;                  ///< Client session identifier.
    le_dls_Link_t               link;                        ///< Object node link.
}
OpenedInstanceCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * JSON parsing session.
 */
//--------------------------------------------------------------------------------------------------
static le_json_ParsingSessionRef_t JsonParsingSessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory Pool for Port Service Client Handler.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PortPool, MAX_APPS, sizeof(OpenedInstanceCtx_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Port Service Client Handler.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PortPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for JSON object
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LinksConfigPool, MAX_PORTS, sizeof(InstanceConfiguration_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for JSON object.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinksConfigPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for link information
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LinkPool, MAX_PORTS*MAX_LINKS, sizeof(LinkInformation_t));


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for link information.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinkPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for list of links.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(LinkListPool, MAX_PORTS, sizeof(LinkList_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for list of links.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinkListPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference for mapping of client/links to AT servers.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ClientLinkAtServerPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for mapping of client/links to AT servers.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ClientLinkAtServerPool, MAX_APPS * MAX_LINKS,
                          sizeof(ClientLinkAtServer_t));

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for device.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(DeviceRef, MAX_APPS);

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for device.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DeviceRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Device object list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t DeviceList;

//--------------------------------------------------------------------------------------------------
/**
 * Instance session list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t InstanceContextList;

#ifndef LE_CONFIG_PORT_CONFIG_IS_STATIC
//--------------------------------------------------------------------------------------------------
/**
 * JSON file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static int JsonFd;
#endif

//--------------------------------------------------------------------------------------------------
/**
 * JSON parsing status flag.
 */
//--------------------------------------------------------------------------------------------------
static bool JsonParseComplete;

//--------------------------------------------------------------------------------------------------
/**
 * Current open link number.
 */
//--------------------------------------------------------------------------------------------------
static int OpenLinkNumber;

//--------------------------------------------------------------------------------------------------
/**
 * Current possible mode number.
 */
//--------------------------------------------------------------------------------------------------
static int PossibleModeNumber;

//--------------------------------------------------------------------------------------------------
/**
 * Semaphore to be used for client socket connection.
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t Semaphore;

//--------------------------------------------------------------------------------------------------
/**
 * AT server references for each client/link combination.
 */
//--------------------------------------------------------------------------------------------------
LE_HASHMAP_DEFINE_STATIC(AtServerDevRefs, MAX_APPS * MAX_LINKS);

//--------------------------------------------------------------------------------------------------
/**
 * Hash map reference to AT server references for each client/link combination.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t AtServerDevRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Device member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void DeviceEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
);

//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called by the JSON parser when it encounters things during parsing.
 */
//--------------------------------------------------------------------------------------------------
static void JsonEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get current instance pointer.
 */
//--------------------------------------------------------------------------------------------------
static InstanceConfiguration_t* GetCurrentInstance
(
    void
)
{
    le_dls_Link_t* linkPtr = le_dls_PeekTail(&InstanceContextList);

    if (NULL == linkPtr)
    {
        LE_ERROR("No instances are present in the list");
        return NULL;
    }

    InstanceConfiguration_t* instanceConfigPtr = CONTAINER_OF(linkPtr,
                                                              InstanceConfiguration_t,
                                                              link);
    return instanceConfigPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get instance pointer from device name.
 */
//--------------------------------------------------------------------------------------------------
static InstanceConfiguration_t* GetInstanceFromDeviceName
(
    const char* deviceNamePtr    ///< [IN] Device name.
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&InstanceContextList);

    while (linkPtr)
    {
        InstanceConfiguration_t* instanceConfigPtr = CONTAINER_OF(linkPtr,
                                                                  InstanceConfiguration_t,
                                                                  link);
        linkPtr = le_dls_PeekNext(&InstanceContextList, linkPtr);

        if (0 == strcmp(instanceConfigPtr->instanceName, deviceNamePtr))
        {
            LE_DEBUG("Instance found: %p", instanceConfigPtr);
            return instanceConfigPtr;
        }
    }

    LE_ERROR("Not able to get the instance");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close a fd and log a warning message if an error occurs
 */
//--------------------------------------------------------------------------------------------------
static void CloseWarn
(
    int fd    ///< [IN] File descriptor.
)
{
    if (-1 == fd)
    {
        LE_ERROR("File descriptor is not valid!");
        return;
    }

    if (-1 == le_fd_Close(fd))
    {
        LE_WARN("failed to close fd %d", fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Cleanup JSON configuration.
 */
//--------------------------------------------------------------------------------------------------
static void CleanJsonConfig
(
    void
)
{
    if (JsonParsingSessionRef)
    {
        // Stop JSON parsing and close the JSON file descriptor.
        le_json_Cleanup(JsonParsingSessionRef);
        JsonParsingSessionRef = NULL;
    }
    else
    {
        LE_WARN("JSON parser cleanup called twice");
    }

#ifndef LE_CONFIG_PORT_CONFIG_IS_STATIC
    if (JsonFd != -1)
    {
        // Close JSON file.
        CloseWarn(JsonFd);
        JsonFd = -1;
    }
    else
    {
        LE_WARN("Closing JSON FD twice");
    }
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Error handling function called by the JSON parser when an error occurs.
 */
//--------------------------------------------------------------------------------------------------
static void JsonErrorHandler
(
    le_json_Error_t error,    ///< [IN] Error enum from JSON.
    const char* msg           ///< [IN] Error message in human readable format.
)
{
    switch (error)
    {
        case LE_JSON_SYNTAX_ERROR:
            LE_ERROR("JSON error message: %s", msg);
            CleanJsonConfig();
            break;

        case LE_JSON_READ_ERROR:
            LE_ERROR("JSON error message: %s", msg);
            CleanJsonConfig();
            break;

        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Open links parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void OpenLinksEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    // Get the instance config pointer from the list.
    InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

    switch (event)
    {
        case LE_JSON_ARRAY_START:
            break;

        case LE_JSON_ARRAY_END:
        {
            le_json_SetEventHandler(JsonEventHandler);
            break;
        }

        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();
            memberName = strstr(memberName, "link");
            if (instanceConfigPtr == NULL)
            {
                CleanJsonConfig();
                break;
            }
            if (NULL != memberName)
            {
                LinkList_t* linkListPtr = (LinkList_t*)le_mem_ForceAlloc(LinkListPoolRef);

                linkListPtr->link = LE_DLS_LINK_INIT;
                if (LE_OK != le_utf8_Copy(linkListPtr->linkName, memberName, LINK_NAME_MAX_BYTES,
                                          NULL))
                {
                    LE_ERROR("linkName is not set properly!");
                    CleanJsonConfig();
                }
                else
                {
                    le_dls_Queue(&(instanceConfigPtr->linkList), &(linkListPtr->link));
                    OpenLinkNumber++;
                }
            }
            else
            {
                LE_ERROR("JSON file not created in proper order");
                CleanJsonConfig();
            }
            break;
        }

        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_NUMBER:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * OpenByDefault parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void OpenByDefaultEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    switch (event)
    {
        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();

            // Get the JSON configuration pointer from the list.
            InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

            LE_ASSERT(instanceConfigPtr != NULL);

            if (0 == strcmp(memberName, "true"))
            {
                instanceConfigPtr->openByDefault = 1;
                le_json_SetEventHandler(DeviceEventHandler);
            }
            else if (0 == strcmp(memberName, "false"))
            {
                instanceConfigPtr->openByDefault = 0;
                le_json_SetEventHandler(DeviceEventHandler);
            }
            else
            {
                LE_ERROR("JSON file not created in proper order");
                CleanJsonConfig();
            }
            break;
        }

        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_NUMBER:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Device path string parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void PathEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    switch (event)
    {
        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();

            // Get the instance config pointer from the list.
            InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

            LE_ASSERT(instanceConfigPtr != NULL);

            if (LE_OK != le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]
                                      ->path, memberName, PATH_MAX_BYTES, NULL))
            {
                LE_ERROR("path is not set properly!");
                CleanJsonConfig();
            }
            else
            {
                le_json_SetEventHandler(DeviceEventHandler);
            }
            break;
        }

        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_NUMBER:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * OpeningType string parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void OpeningTypeEventHandler
(
    le_json_Event_t event     ///< [IN] JSON event.
)
{
    switch (event)
    {
        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();

            // Get the instance config pointer from the list.
            InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

            LE_ASSERT(instanceConfigPtr != NULL);

            if (LE_OK != le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]
                                      ->openingType, memberName, OPEN_TYPE_MAX_BYTES, NULL))
            {
                LE_ERROR("openingType is not set properly!");
                CleanJsonConfig();
            }
            else
            {
                le_json_SetEventHandler(DeviceEventHandler);
            }
            break;
        }

        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_NUMBER:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * PossibleMode string parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void PossibleModeEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    // Get the instance config pointer from the list.
    InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

    LE_ASSERT(instanceConfigPtr != NULL);

    switch (event)
    {
        case LE_JSON_ARRAY_START:
            break;

        case LE_JSON_ARRAY_END:
            // Increment link counter here, as all link information are captured.
            instanceConfigPtr->linkCounter++;
            le_json_SetEventHandler(DeviceEventHandler);
            break;

        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();
            if (LE_OK != le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]
                                      ->possibleMode[PossibleModeNumber], memberName,
                                       POSSIBLE_MODE_MAX_BYTES, NULL))
            {
                LE_ERROR("possibleMode is not set properly!");
                CleanJsonConfig();
            }
            else
            {
                PossibleModeNumber++;
            }
            break;
        }

        case LE_JSON_OBJECT_MEMBER:
        case LE_JSON_OBJECT_START:
        case LE_JSON_OBJECT_END:
        case LE_JSON_NUMBER:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Device member parsing event function.
 */
//--------------------------------------------------------------------------------------------------
static void DeviceEventHandler
(
    le_json_Event_t event     ///< [IN] JSON event.
)
{
    switch (event)
    {
        case LE_JSON_OBJECT_START:
            break;

        case LE_JSON_OBJECT_END:
            break;

        case LE_JSON_OBJECT_MEMBER:
        {
            const char* memberName = le_json_GetString();
            const char* linkName = strstr(memberName, "link");
            if (NULL != linkName)
            {
                // Get the instance config pointer from the list.
                InstanceConfiguration_t* instanceConfigPtr = GetCurrentInstance();

                if (NULL == instanceConfigPtr)
                {
                    LE_ERROR("instanceConfigPtr is NULL!");
                    CleanJsonConfig();
                    break;
                }
                // Check if the instance contains more than MAX_LINKS links.
                if (instanceConfigPtr->linkCounter > (MAX_LINKS - 1))
                {
                    LE_ERROR("JSON file not created in proper order");
                    CleanJsonConfig();
                    break;
                }

                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter] =
                                           (LinkInformation_t*)le_mem_ForceAlloc(LinkPoolRef);

                // Initialization of link information structure.
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->fd = -1;
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->dataModeFd = -1;
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->atModeSockFd = -1;
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->dataModeSockFd = -1;

                // Initialize the counter before parsing of new link.
                PossibleModeNumber = 0;

                if (LE_OK != le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr
                                          ->linkCounter]->linkName, linkName, LINK_NAME_MAX_BYTES,
                                          NULL))
                {
                    LE_ERROR("linkName is not set properly!");
                    CleanJsonConfig();
                    break;
                }
            }
            else if (0 == strcmp(memberName, "path"))
            {
                le_json_SetEventHandler(PathEventHandler);
            }
            else if (0 == strcmp(memberName, "openingType"))
            {
                le_json_SetEventHandler(OpeningTypeEventHandler);
            }
            else if (0 == strcmp(memberName, "possibleMode"))
            {
                le_json_SetEventHandler(PossibleModeEventHandler);
            }
            else if (0 == strcmp(memberName, "OpenByDefault"))
            {
                le_json_SetEventHandler(OpenByDefaultEventHandler);
            }
            else if (0 == strcmp(memberName, "OpenLinks"))
            {
                le_json_SetEventHandler(OpenLinksEventHandler);
            }
            break;
        }

        case LE_JSON_STRING:
        case LE_JSON_NUMBER:
        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
        case LE_JSON_DOC_END:
            LE_ERROR("JSON file not created in proper order");
            CleanJsonConfig();
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens the serial device.
 *
 * @return
 *      - -1 if function failed.
 *      - Valid file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static int32_t OpenSerialDevice
(
    char* deviceName  ///< [IN] Device name.
)
{
    int32_t fd;

    fd = le_fd_Open(deviceName, O_RDWR);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open device");
        return -1;
    }

#if LE_CONFIG_LINUX
    // TODO: how to set a specific configuration ?

    // Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters,
    // disables local echo, numChars = 0 and timeout = 0: Read is completetly non-blocking.
    if (LE_OK != le_tty_SetRaw(fd, 0, 0))
    {
        LE_ERROR("Failed to configure TTY raw");
        // Close the TTY
        le_tty_Close(fd);
        return -1;
    }
#endif
    return fd;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create an AT server entry corresponding to the unique combination of link and client session.
 *
 * @return
 *      LE_OK           - Success.
 *      LE_DUPLICATE    - Entry already exists.
 *      LE_FAULT        - Could not open AT server instance.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddAtServer
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *newEntryPtr;

    if (le_hashmap_Get(AtServerDevRefMap, &key) != NULL)
    {
        return LE_DUPLICATE;
    }

    newEntryPtr = le_mem_ForceAlloc(ClientLinkAtServerPoolRef);
    newEntryPtr->clientRef = clientRef;
    newEntryPtr->linkPtr = infoPtr;
    newEntryPtr->suspended = false;
    newEntryPtr->atServerRef = le_atServer_Open(le_fd_Dup(infoPtr->fd));
    if (newEntryPtr->atServerRef == NULL)
    {
        le_mem_Release(newEntryPtr);
        return LE_FAULT;
    }

    LE_ASSERT(le_hashmap_Put(AtServerDevRefMap, newEntryPtr, newEntryPtr) == NULL);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find the AT server instance corresponding to a link/client pair.
 *
 * @return  The found instance, or NULL if there was no match.
 */
//--------------------------------------------------------------------------------------------------
static le_atServer_DeviceRef_t FindAtServer
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *found;

    found = le_hashmap_Get(AtServerDevRefMap, &key);
    if (found == NULL)
    {
        return NULL;
    }
    return found->atServerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Suspend the AT server session for a link/client combination.
 *
 * @return
 *      LE_OK               - Success.
 *      LE_NOT_FOUND        - No entry found for client/link combination.
 *      LE_BAD_PARAMETER    - Invalid device reference.
 *      LE_FAULT            - Device not monitored.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SuspendAtServer
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *found;
    le_result_t              result;

    found = le_hashmap_Get(AtServerDevRefMap, &key);
    if (found == NULL)
    {
        return LE_NOT_FOUND;
    }

    result = le_atServer_Suspend(found->atServerRef);
    found->suspended = (result == LE_OK);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Determine if an AT server instance is suspended.
 *
 * @return  False if the AT server exists and is not suspended.
 */
//--------------------------------------------------------------------------------------------------
static bool IsAtServerSuspended
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *found;

    found = le_hashmap_Get(AtServerDevRefMap, &key);
    if (found == NULL)
    {
        return true;
    }

    return found->suspended;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume the AT server session for a link/client combination.
 *
 * @return
 *      LE_OK               - Success.
 *      LE_NOT_FOUND        - No entry found for client/link combination.
 *      LE_BAD_PARAMETER    - Invalid device reference.
 *      LE_FAULT            - Device not monitored.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ResumeAtServer
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *found;
    le_result_t              result;

    found = le_hashmap_Get(AtServerDevRefMap, &key);
    if (found == NULL)
    {
        return LE_NOT_FOUND;
    }

    result = le_atServer_Resume(found->atServerRef);
    found->suspended = (result != LE_OK);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the AT server session for a link/client combination.
 *
 * @return
 *      LE_OK               - Success.
 *      LE_NOT_FOUND        - No entry found for client/link combination.
 *      LE_BAD_PARAMETER    - Invalid device reference.
 *      LE_BUSY             - The requested device is busy.
 *      LE_FAULT            - Failed to stop the server, check logs for more information.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveAtServer
(
    LinkInformation_t   *infoPtr,   ///< Link info corresponding to port.
    le_msg_SessionRef_t  clientRef  ///< Client identifier.
)
{
    ClientLinkAtServer_t     key = { clientRef, infoPtr, NULL, false };
    ClientLinkAtServer_t    *removed;
    le_result_t              result;

    removed = le_hashmap_Remove(AtServerDevRefMap, &key);
    if (removed == NULL)
    {
        return LE_NOT_FOUND;
    }

    result = le_atServer_Close(removed->atServerRef);
    le_mem_Release(removed);
    return result;
}

#if LE_CONFIG_LINUX
//--------------------------------------------------------------------------------------------------
/**
 * Monitor the client's fd
 */
//--------------------------------------------------------------------------------------------------
static void MonitorClient
(
    int clientFd,    ///< [IN] Client file descriptor.
    short events     ///< [IN] Socket event.
)
{
    LinkInformation_t *linkInfoPtr;

    if (POLLRDHUP & events)
    {
        LE_DEBUG("fd %d: connection reset by peer", clientFd);
    }
    else
    {
        LE_WARN("events %.8x not handled", events);
    }

    linkInfoPtr = le_fdMonitor_GetContextPtr();
    if (linkInfoPtr != NULL)
    {
        if (RemoveAtServer(linkInfoPtr, NULL))
        {
            LE_ERROR("failed to close atServer device");
        }
    }
    else
    {
        LE_ERROR("failed to get atServer device reference");
    }

    le_fdMonitor_Delete(le_fdMonitor_GetMonitor());

    CloseWarn(clientFd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Monitor the socket's fd
 */
//--------------------------------------------------------------------------------------------------
static void MonitorSocket
(
    int sockFd,     ///< [IN] Socket file descriptor.
    short events    ///< [IN] Socket event.
)
{
    char                 clientSocket[CLIENT_SOCKET_MAX_BYTES];
    int                  clientFd;
    le_fdMonitor_Ref_t   fdMonitorRef;
    LinkInformation_t   *linkInfoPtr;

    if (POLLIN & events)
    {
        clientFd = accept(sockFd, NULL, NULL);
        if (-1 == clientFd)
        {
            LE_ERROR("accepting socket failed: %m");
            goto exit_close_socket;
        }

        linkInfoPtr = le_fdMonitor_GetContextPtr();

        if (0 == strcmp(linkInfoPtr->possibleMode[0], "DATA"))
        {
            LE_DEBUG("Socket opens in data mode.");
            linkInfoPtr->dataModeFd = dup(clientFd);
            le_sem_Post(Semaphore);
            return;
        }

        linkInfoPtr->fd = dup(clientFd);
        if (AddAtServer(linkInfoPtr, NULL) != LE_OK)
        {
            LE_ERROR("Cannot open the device!");
            CloseWarn(clientFd);
            goto exit_close_socket;
        }
        snprintf(clientSocket, sizeof(clientSocket) - 1, "socket-client-%d", clientFd);
        fdMonitorRef = le_fdMonitor_Create(clientSocket, clientFd, &MonitorClient, POLLRDHUP);
        le_fdMonitor_SetContextPtr(fdMonitorRef, linkInfoPtr);

        return;
    }

    LE_WARN("events %.8x not handled", events);

exit_close_socket:
    le_fdMonitor_Delete(le_fdMonitor_GetMonitor());
    CloseWarn(sockFd);
    LE_ERROR("Not able to monitor the socket");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens the socket.
 *
 * @return
 *      - -1 if function failed.
 *      - Valid file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static int32_t OpenSocket
(
    LinkInformation_t   *linkInfoPtr    ///< [IN] Link information structure pointer.
)
{
    int sockFd;
    struct sockaddr_un myAddress;
    char socketName[SERVER_SOCKET_MAX_BYTES];
    le_fdMonitor_Ref_t fdMonitorRef;

    if ((-1 == unlink(linkInfoPtr->path)) && (ENOENT != errno))
    {
        LE_ERROR("unlink socket failed: %m");
        return -1;
    }

    // Create the socket
    sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == sockFd)
    {
        LE_ERROR("creating socket failed: %m");
        return -1;
    }

    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sun_family = AF_UNIX;
    if (LE_OK != le_utf8_Copy(myAddress.sun_path, linkInfoPtr->path,
                              sizeof(myAddress.sun_path) - 1, NULL))
    {
        LE_ERROR("Socket path is not set properly!");
        goto exit_close_socket;
    }

    if (-1 == bind(sockFd, (struct sockaddr *) &myAddress, sizeof(myAddress)))
    {
        LE_ERROR("binding socket failed: %m");
        goto exit_close_socket;
    }

    // Listen on the socket
    if (-1 == listen(sockFd, MAX_CLIENTS))
    {
        LE_ERROR("listening socket failed: %m");
        goto exit_close_socket;
    }

    snprintf(socketName, sizeof(socketName) - 1, "unixSocket-%d", sockFd);
    fdMonitorRef = le_fdMonitor_Create(socketName, sockFd, &MonitorSocket, POLLIN);

    le_fdMonitor_SetContextPtr(fdMonitorRef, linkInfoPtr);

    return sockFd;

exit_close_socket:
    CloseWarn(sockFd);
    return -1;
}
#endif /* end LE_CONFIG_LINUX */

//--------------------------------------------------------------------------------------------------
/**
 * Open the instance links.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenInstanceLinks
(
    OpenedInstanceCtx_t *openedInstanceCtxPtr ///< [IN] Opened instance configuaration.
)
{
    InstanceConfiguration_t *instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
    int                      i;
    int                      j;
    le_dls_Link_t           *linkPtr = le_dls_Peek(&(instanceConfigPtr->linkList));

    for (i = 0; (i < (instanceConfigPtr->linkCounter)) && (NULL != linkPtr); i++)
    {
        LinkList_t* linkListPtr = CONTAINER_OF(linkPtr, LinkList_t, link);

        if ((NULL != strstr(linkListPtr->linkName, "link")) &&
            (0 == strcmp(instanceConfigPtr->linkInfo[i]->linkName, linkListPtr->linkName)))
        {
            linkPtr = le_dls_PeekNext(&(instanceConfigPtr->linkList), linkPtr);
            if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink"))
            {
                for (j = 0; j < MAX_POSSIBLE_MODES; j++)
                {
                    if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
                    {
                        if (instanceConfigPtr->linkInfo[i]->fd == -1)
                        {
                            instanceConfigPtr->linkInfo[i]->fd = OpenSerialDevice(
                                instanceConfigPtr->linkInfo[i]->path);
                        }
                        if (-1 == instanceConfigPtr->linkInfo[i]->fd)
                        {
                            LE_ERROR("Error in opening the device '%s': %s %d",
                                     instanceConfigPtr->instanceName,
                                     instanceConfigPtr->linkInfo[i]->path,
                                     (int)instanceConfigPtr->linkInfo[i]->fd);
                            return LE_FAULT;
                        }

                        le_result_t result = AddAtServer(instanceConfigPtr->linkInfo[i],
                            openedInstanceCtxPtr->sessionRef);
                        if (result != LE_OK)
                        {
                            LE_ERROR("Failed to open AT server: %s", LE_RESULT_TXT(result));
                            return result;
                        }
                    }
                }
            }
#if LE_CONFIG_LINUX
            else if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket"))
            {
                for (j = 0; j < MAX_POSSIBLE_MODES; j++)
                {
                    if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
                    {
                        if (instanceConfigPtr->linkInfo[i]->atModeSockFd == -1)
                        {
                            instanceConfigPtr->linkInfo[i]->atModeSockFd =
                                           OpenSocket(instanceConfigPtr->linkInfo[i]);
                        }
                        if (0 > instanceConfigPtr->linkInfo[i]->atModeSockFd)
                        {
                            LE_ERROR("Error in opening the device '%s': %m",
                                     instanceConfigPtr->instanceName);
                            return LE_FAULT;
                        }
                    }
                }
            }
#endif /* end LE_CONFIG_LINUX */
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open a port instance.
 *
 * @return  New opened port instance context.
 */
//--------------------------------------------------------------------------------------------------
static OpenedInstanceCtx_t *OpenInstance
(
    InstanceConfiguration_t *instanceConfigPtr, ///< [IN] Port instance to open.
    le_msg_SessionRef_t      sessionRef         ///< [IN] Client session reference.
)
{
    le_port_DeviceRef_t  devRef;
    OpenedInstanceCtx_t *openedInstanceCtxPtr = le_mem_ForceAlloc(PortPoolRef);
    LE_ASSERT(openedInstanceCtxPtr != NULL);

    devRef = le_ref_CreateRef(DeviceRefMap, openedInstanceCtxPtr);
    LE_ASSERT(devRef != NULL);

    // Save the client session.
    openedInstanceCtxPtr->sessionRef = sessionRef;

    openedInstanceCtxPtr->deviceRef = devRef;
    openedInstanceCtxPtr->instanceConfigPtr = instanceConfigPtr;

    openedInstanceCtxPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&DeviceList, &openedInstanceCtxPtr->link);
    return openedInstanceCtxPtr;
}

//--------------------------------------------------------------------------------------------------
 /**
 * Open the instances which has "OpenByDefault" property as true.
 */
//--------------------------------------------------------------------------------------------------
static void OpenDefaultInstances
(
    void
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&InstanceContextList);

    while (linkPtr)
    {
        InstanceConfiguration_t* instanceConfigPtr = CONTAINER_OF(linkPtr,
                                                                  InstanceConfiguration_t,
                                                                  link);
        linkPtr = le_dls_PeekNext(&InstanceContextList, linkPtr);

        if (instanceConfigPtr->openByDefault)
        {
            // NOTE: NULL is used to indicate the instances opened by the port service itself rather
            //       than those opened on behalf of a client, as is the case of the "opened by
            //       default" ports.
            OpenedInstanceCtx_t *openedInstanceCtxPtr = OpenInstance(instanceConfigPtr, NULL);
            if (OpenInstanceLinks(openedInstanceCtxPtr) != LE_OK)
            {
                LE_ERROR("Not able to open the instance links");
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Event handler function called by the JSON parser when it encounters things during parsing.
 */
//--------------------------------------------------------------------------------------------------
static void JsonEventHandler
(
    le_json_Event_t event    ///< [IN] JSON event.
)
{
    switch (event)
    {
        case LE_JSON_OBJECT_START:
            break;

        case LE_JSON_OBJECT_END:
            break;

        case LE_JSON_DOC_END:
            CleanJsonConfig();
            JsonParseComplete = true;
            LE_INFO("JSON parsing is completed.");

            // Open the instances which has "OpenByDefault" property as true.
            OpenDefaultInstances();
            break;

        case LE_JSON_OBJECT_MEMBER:
        {
            const char* memberName = le_json_GetString();
            if (NULL != memberName)
            {
                // Create the safe reference and allocate the memory for JSON object.
                InstanceConfiguration_t* instanceConfigurationPtr;
                instanceConfigurationPtr =
                        (InstanceConfiguration_t*)le_mem_ForceAlloc(LinksConfigPoolRef);

                // Save the instance name.
                if (LE_OK != le_utf8_Copy(instanceConfigurationPtr->instanceName, memberName,
                                          sizeof(instanceConfigurationPtr->instanceName), NULL))
                {
                    LE_ERROR("instanceName is not set properly!");
                    CleanJsonConfig();
                    break;
                }

                // Initialize the link counter.
                instanceConfigurationPtr->linkCounter = 0;

                // Initialization of linkList.
                instanceConfigurationPtr->linkList = LE_DLS_LIST_INIT;

                // Initialize the counter before parsing of new instance.
                OpenLinkNumber = 0;

                // Add instance into InstanceContextList.
                instanceConfigurationPtr->link = LE_DLS_LINK_INIT;
                le_dls_Queue(&InstanceContextList, &(instanceConfigurationPtr->link));

                // Switch to DeviceEventHandler to parse device information from JSON file.
                le_json_SetEventHandler(DeviceEventHandler);
            }
            break;
        }

        case LE_JSON_STRING:
        case LE_JSON_NUMBER:
        case LE_JSON_ARRAY_START:
        case LE_JSON_ARRAY_END:
        case LE_JSON_TRUE:
        case LE_JSON_FALSE:
        case LE_JSON_NULL:
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get atServer device reference for the links which supports both AT and DATA mode.
 */
//--------------------------------------------------------------------------------------------------
static bool GetAtServerDeviceRef
(
    OpenedInstanceCtx_t     *openedInstanceCtxPtr, ///< [IN] Opened instance configuration pointer.
    le_atServer_DeviceRef_t *atServerDeviceRef,    ///< [OUT] AtServer device reference.
    int                     *linkIndexPtr          ///< [OUT] Link index of the link which contains
                                                   /// both AT and DATA mode.
)
{
    bool                     allowSuspend = false;
    InstanceConfiguration_t *instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
    int                      i;
    int                      j;

    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        // Check if the link supports AT as a possible mode.
        for (j = 0; j < MAX_POSSIBLE_MODES; j++)
        {
            if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
            {
                *linkIndexPtr = i;
                allowSuspend = true;
                break;
            }
        }
        if (true == allowSuspend)
        {
            // Found a link that supports AT, stop searching.
            break;
        }
    }

    if (true == allowSuspend)
    {
        *atServerDeviceRef = FindAtServer(instanceConfigPtr->linkInfo[*linkIndexPtr],
            openedInstanceCtxPtr->sessionRef);
        if (*atServerDeviceRef == NULL)
        {
            LE_ERROR("No AT server device reference found");
        }
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function requests to open a configured device. If the device was not opened, it opens
 * the device.
 *
 * @return
 *      - Reference to the device.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_port_DeviceRef_t le_port_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
)
{
    if ((NULL == deviceNamePtr) || !(strcmp(deviceNamePtr, "")))
    {
        LE_ERROR("deviceNamePtr is not valid!");
        return NULL;
    }

    if (false == JsonParseComplete)
    {
        LE_ERROR("JSON parsing is not completed!");
        return NULL;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr = GetInstanceFromDeviceName(deviceNamePtr);
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return NULL;
    }

    OpenedInstanceCtx_t *openedInstanceCtxPtr = OpenInstance(instanceConfigPtr,
        le_port_GetClientSessionRef());
    if (OpenInstanceLinks(openedInstanceCtxPtr) != LE_OK)
    {
        LE_ERROR("Not able to open the instance links");
        return NULL;
    }

    return openedInstanceCtxPtr->deviceRef;
}

#if LE_CONFIG_LINUX
//--------------------------------------------------------------------------------------------------
/**
 * This function creates the socket and waiting for the client socket connection.
 */
//--------------------------------------------------------------------------------------------------
static void* SocketThread
(
    void* ctxPtr
)
{
    LinkInformation_t* linkInfo = (LinkInformation_t*)ctxPtr;
    if (linkInfo->dataModeSockFd == -1)
    {
        linkInfo->dataModeSockFd = OpenSocket(linkInfo);
    }
    if (0 > linkInfo->dataModeSockFd)
    {
        LE_ERROR("Error in opening the device %m");
        return NULL;
    }

    le_event_RunLoop();
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into data mode.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_UNAVAILABLE   JSON parsing is not completed.
 *      - LE_DUPLICATE     Device already opened in data mode
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int* fdPtr                    ///< [OUT] File descriptor of the device.
)
{
    le_result_t result = LE_FAULT;
    int i, j, linkIndex;
    le_atServer_DeviceRef_t atServerDeviceRef;
#if LE_CONFIG_LINUX
    le_clk_Time_t timeToWait = {10, 0};
    le_thread_Ref_t socketThreadRef;
#endif

    if (false == JsonParseComplete)
    {
        LE_ERROR("JSON parsing is not completed!");
        return LE_UNAVAILABLE;
    }

    if (NULL == fdPtr)
    {
        LE_ERROR("fdPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    OpenedInstanceCtx_t* openedInstanceCtxPtr = le_ref_Lookup(DeviceRefMap, devRef);
    if (NULL == openedInstanceCtxPtr)
    {
        LE_ERROR("devRef is invalid!");
        return LE_BAD_PARAMETER;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return LE_FAULT;
    }

    // Check whether same link contains AT and DATA mode.
    // If both links contains AT and DATA mode then suspend atServer.
    if (GetAtServerDeviceRef(openedInstanceCtxPtr, &atServerDeviceRef, &linkIndex))
    {
        if (NULL == atServerDeviceRef)
        {
            LE_ERROR("atServerDeviceRef is NULL!");
            return LE_FAULT;
        }

        result = SuspendAtServer(instanceConfigPtr->linkInfo[linkIndex],
            openedInstanceCtxPtr->sessionRef);
        if (result == LE_FAULT)
        {
            LE_ERROR("Device is already into data mode!");
            return LE_DUPLICATE;
        }
        else if (result != LE_OK)
        {
            return result;
        }
    }

    *fdPtr = -1;

    // Check all the links. Open the link which contains "DATA" as possibleMode.
    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        for (j = 0; j < MAX_POSSIBLE_MODES; j++)
        {
            if ((0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "DATA")) &&
               (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink")))
            {
                // If link is not opened in data mode then open the link in data mode.
                if (-1 == instanceConfigPtr->linkInfo[i]->dataModeFd)
                {
                    instanceConfigPtr->linkInfo[i]->dataModeFd =
                                       OpenSerialDevice(instanceConfigPtr->linkInfo[i]->path);
                }

                if (-1 != instanceConfigPtr->linkInfo[i]->dataModeFd)
                {
                    *fdPtr = le_fd_Dup(instanceConfigPtr->linkInfo[i]->dataModeFd);
                }
                else
                {
                    *fdPtr = -1;
                }
            }
#if LE_CONFIG_LINUX
            else if ((0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "DATA")) &&
                     (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket")))
            {
                // If link is not opened in data mode then open the link in data mode.
                if (-1 == instanceConfigPtr->linkInfo[i]->dataModeFd)
                {
                    // Create the socket thread which waits for the client connection request
                    // and filled the file descriptor for data mode.
                    socketThreadRef = le_thread_Create("SocketThread", SocketThread,
                                                       (void*)(instanceConfigPtr->linkInfo[i]));
                    le_thread_Start(socketThreadRef);

                    // Wait for the server to accept the client connect request.
                    result = le_sem_WaitWithTimeOut(Semaphore, timeToWait);

                    // Stop socket thread.
                    le_thread_Cancel(socketThreadRef);

                    if (LE_TIMEOUT == result)
                    {
                        return LE_FAULT;
                    }
                }

                if (-1 != instanceConfigPtr->linkInfo[i]->dataModeFd)
                {
                    *fdPtr = (instanceConfigPtr->linkInfo[i]->dataModeFd);
                }
                else
                {
                    *fdPtr = -1;
                }
            }
#endif
        }
    }

    if (-1 == *fdPtr)
    {
        LE_ERROR("Unable to open the device in data mode!");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into AT command mode and returns At server device reference.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetCommandMode
(
    le_port_DeviceRef_t devRef,           ///< [IN] Device reference.
    le_atServer_DeviceRef_t* deviceRefPtr ///< [OUT] AT server device reference.
)
{
    le_result_t result;
    int i, j, linkIndex;
    bool atLinkDetect = false;
    bool dataLinkDetect = false;

    if (false == JsonParseComplete)
    {
        LE_ERROR("JSON parsing is not completed!");
        return LE_UNAVAILABLE;
    }

    OpenedInstanceCtx_t* openedInstanceCtxPtr = le_ref_Lookup(DeviceRefMap, devRef);
    if (NULL == openedInstanceCtxPtr)
    {
        LE_ERROR("DevRef is invalid!");
        return LE_BAD_PARAMETER;
    }

    if (NULL == deviceRefPtr)
    {
        LE_ERROR("deviceRefPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return LE_FAULT;
    }

    // Check all links to close any that may be opened in DATA mode.
    for (i = 0; i < instanceConfigPtr->linkCounter; i++)
    {
        for (j = 0; j < MAX_POSSIBLE_MODES; j++)
        {
            if ((0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "DATA")) &&
                (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink")))
            {
                // If link is opened in data mode then close it.
                if (-1 != instanceConfigPtr->linkInfo[i]->dataModeFd)
                {
                    CloseWarn(instanceConfigPtr->linkInfo[i]->dataModeFd);
                    instanceConfigPtr->linkInfo[i]->dataModeFd = -1;
                }
                dataLinkDetect = true;
                break;
            }
        }
        if(dataLinkDetect == true)
        {
            break;
        }
    }

    // Check all the links. Open the link which contains "AT" as possibleMode if not opened in AT
    // command mode.
    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        for (j = 0; j < MAX_POSSIBLE_MODES; j++)
        {
            if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
            {
                // If link is not opened in AT mode then open the link in AT mode.
                if (-1 == instanceConfigPtr->linkInfo[i]->fd)
                {
                    if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink"))
                    {
                        instanceConfigPtr->linkInfo[i]->fd =
                                           OpenSerialDevice(instanceConfigPtr->linkInfo[i]->path);

                        if (-1 == instanceConfigPtr->linkInfo[i]->fd)
                        {
                            LE_ERROR("Error in opening the device '%s': %d",
                                     instanceConfigPtr->instanceName, errno);
                            return LE_FAULT;
                        }
                    }
#if LE_CONFIG_LINUX
                    else if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket"))
                    {
                        if (instanceConfigPtr->linkInfo[i]->atModeSockFd == -1)
                        {
                            instanceConfigPtr->linkInfo[i]->atModeSockFd =
                                           OpenSocket(instanceConfigPtr->linkInfo[i]);
                        }
                        if (0 > instanceConfigPtr->linkInfo[i]->atModeSockFd)
                        {
                            LE_ERROR("Error in opening the device %s %m",
                                     instanceConfigPtr->instanceName);
                            return LE_FAULT;
                        }
                    }
#endif /* end LE_CONFIG_LINUX */
                }

                // One instance supports only AT link.
                atLinkDetect = true;
                break;
            }
        }

        if (true == atLinkDetect)
        {
            break;
        }
    }

    if (GetAtServerDeviceRef(openedInstanceCtxPtr, deviceRefPtr, &linkIndex))
    {
        if (NULL == *deviceRefPtr)
        {
            LE_ERROR("deviceRefPtr is NULL!");
            return LE_FAULT;
        }

        if (IsAtServerSuspended(instanceConfigPtr->linkInfo[linkIndex],
            openedInstanceCtxPtr->sessionRef))
        {
            result = ResumeAtServer(instanceConfigPtr->linkInfo[linkIndex],
                openedInstanceCtxPtr->sessionRef);
            if (LE_FAULT == result)
            {
                LE_ERROR("Device is not able to switch into command mode");
                return LE_FAULT;
            }
        }
    }
    else if (true == atLinkDetect)
    {
        LE_ERROR("Failed to get atServer device reference");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes all file descriptors.
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllFd
(
    InstanceConfiguration_t* instanceConfigPtr
)
{
    int i;

    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        if (-1 != instanceConfigPtr->linkInfo[i]->fd)
        {
            CloseWarn(instanceConfigPtr->linkInfo[i]->fd);
            instanceConfigPtr->linkInfo[i]->fd = -1;
        }
        if (-1 != instanceConfigPtr->linkInfo[i]->dataModeFd)
        {
            CloseWarn(instanceConfigPtr->linkInfo[i]->dataModeFd);
            instanceConfigPtr->linkInfo[i]->dataModeFd = -1;
        }
        if (-1 != instanceConfigPtr->linkInfo[i]->atModeSockFd)
        {
            CloseWarn(instanceConfigPtr->linkInfo[i]->atModeSockFd);
            instanceConfigPtr->linkInfo[i]->atModeSockFd = -1;
        }
        if (-1 != instanceConfigPtr->linkInfo[i]->dataModeSockFd)
        {
            CloseWarn(instanceConfigPtr->linkInfo[i]->dataModeSockFd);
            instanceConfigPtr->linkInfo[i]->dataModeSockFd = -1;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the device and releases the resources.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_UNAVAILABLE   JSON parsing is not completed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_port_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
)
{
    le_result_t result;
    int i, j, linkIndex = -1;

    if (false == JsonParseComplete)
    {
        LE_ERROR("JSON parsing is not completed!");
        return LE_UNAVAILABLE;
    }

    OpenedInstanceCtx_t* openedInstanceCtxPtr = le_ref_Lookup(DeviceRefMap, devRef);
    if (NULL == openedInstanceCtxPtr)
    {
        LE_ERROR("devRef is invalid!");
        return LE_BAD_PARAMETER;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return LE_FAULT;
    }

    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        for (j = 0; j < MAX_POSSIBLE_MODES; j++)
        {
            // Check if the link supports AT as a possible mode.
            if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
            {
                linkIndex = i;
                break;
            }
        }
        if (-1 != linkIndex)
        {
            // Found a link that supports AT, stop searching.
            break;
        }
    }

    if (-1 == linkIndex)
    {
        LE_ERROR("Instance does not supports AT command mode!");
        return LE_FAULT;
    }

    // Resume device before closing if it was suspended.
    if (IsAtServerSuspended(instanceConfigPtr->linkInfo[linkIndex],
        openedInstanceCtxPtr->sessionRef))
    {
        result = ResumeAtServer(instanceConfigPtr->linkInfo[linkIndex],
            openedInstanceCtxPtr->sessionRef);
        if (LE_FAULT == result)
        {
            LE_ERROR("Failed to resume device before closing");
            return LE_FAULT;
        }
    }

    result = RemoveAtServer(instanceConfigPtr->linkInfo[linkIndex],
        openedInstanceCtxPtr->sessionRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to close");
        return LE_FAULT;
    }

    // Close all file descriptors.
    CloseAllFd(instanceConfigPtr);

    // Delete the link from DeviceList.
    le_dls_Remove(&DeviceList, &(openedInstanceCtxPtr->link));

    le_ref_DeleteRef(DeviceRefMap, devRef);
    le_mem_Release(openedInstanceCtxPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device reference regarding to a given reference coming from the AT server.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
)
{
    int i;

    if (NULL == devRefPtr)
    {
        LE_ERROR("devRefPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (false == JsonParseComplete)
    {
        LE_ERROR("JSON parsing is not completed!");
        return LE_UNAVAILABLE;
    }

    le_dls_Link_t* deviceLinkPtr = le_dls_Peek(&DeviceList);

    while (deviceLinkPtr)
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = CONTAINER_OF(deviceLinkPtr,
                                                                 OpenedInstanceCtx_t,
                                                                 link);
        deviceLinkPtr = le_dls_PeekNext(&DeviceList, deviceLinkPtr);

        InstanceConfiguration_t* instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
        if (NULL == instanceConfigPtr)
        {
            LE_ERROR("instanceConfigPtr is NULL!");
            return LE_FAULT;
        }

        for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
        {
            if (FindAtServer(instanceConfigPtr->linkInfo[i], openedInstanceCtxPtr->sessionRef) ==
                atServerDevRef)
            {
                 *devRefPtr = openedInstanceCtxPtr->deviceRef;
                 return LE_OK;
            }
        }
    }

    *devRefPtr = NULL;
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close session event handler of port service.
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Session reference of client application.
    void*               contextPtr   ///< [IN] Context pointer of CloseSessionEventHandler.
)
{
    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Clean session context.
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    le_ref_IterRef_t iterRef = le_ref_GetIterator(DeviceRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (LE_OK == result)
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = (OpenedInstanceCtx_t*)le_ref_GetValue(iterRef);

        // Check if the session reference saved matches with the current session reference.
        if (openedInstanceCtxPtr->sessionRef == sessionRef)
        {
            le_port_DeviceRef_t devRef = (le_port_DeviceRef_t)le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release device reference 0x%p, sessionRef 0x%p", devRef, sessionRef);

            InstanceConfiguration_t* instanceConfigPtr = openedInstanceCtxPtr->instanceConfigPtr;
            if (NULL == instanceConfigPtr)
            {
                LE_ERROR("instanceConfigPtr is NULL!");
            }
            else
            {
                int i;

                // Close AT server handle.
                for (i = 0; i < instanceConfigPtr->linkCounter; ++i)
                {
                    RemoveAtServer(instanceConfigPtr->linkInfo[i], sessionRef);
                }

                // Close all file descriptors.
                CloseAllFd(instanceConfigPtr);
            }

            // Delete the link from DeviceList.
            le_dls_Remove(&DeviceList, &(openedInstanceCtxPtr->link));

            // Release memory and delete reference of device and instance.
            le_ref_DeleteRef(DeviceRefMap, devRef);
            le_mem_Release(openedInstanceCtxPtr);
        }

        // Get the next value in the reference map.
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Calculate hash of client and link pointers for map.
 *
 *  @return Hash value.
 */
//--------------------------------------------------------------------------------------------------
static size_t HashClientAndLink
(
    const ClientLinkAtServer_t *entryPtr    ///< Hash map entry to calculate hash over.
)
{
    const uint8_t   *ptr;
    size_t           hash = 0;

    for (ptr = (const uint8_t *) entryPtr; ptr < (const uint8_t *) &entryPtr->atServerRef; ++ptr)
    {
        hash = *ptr + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Determine equality of two client/link to server map entries.
 *
 *  @return Equality of entries.
 */
//--------------------------------------------------------------------------------------------------
static bool EqualClientAndLink
(
    const ClientLinkAtServer_t *entry1Ptr,  ///< First entry to compare.
    const ClientLinkAtServer_t *entry2Ptr   ///< Second entry to compare.
)
{
    return (entry1Ptr->clientRef == entry2Ptr->clientRef &&
        entry1Ptr->linkPtr == entry2Ptr->linkPtr);
}

#if LE_CONFIG_PORT_CONFIG_IS_STATIC
//--------------------------------------------------------------------------------------------------
/**
 *  Initialize the JSON parsing session with a static configuration
 *
 */
//--------------------------------------------------------------------------------------------------
void le_portLocal_InitStaticCfg
(
    const char *cfgPtr   ///< static configuration to parse
)
{
    JsonParsingSessionRef = le_json_ParseString(cfgPtr, JsonEventHandler, JsonErrorHandler,
                                                NULL);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the port service.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create safe reference map for request references.
    DeviceRefMap = le_ref_InitStaticMap(DeviceRef, MAX_APPS);

    AtServerDevRefMap = le_hashmap_InitStatic(AtServerDevRefs, MAX_APPS * MAX_LINKS,
        (le_hashmap_HashFunc_t) &HashClientAndLink, (le_hashmap_EqualsFunc_t) &EqualClientAndLink);
    ClientLinkAtServerPoolRef = le_mem_InitStaticPool(ClientLinkAtServerPool, MAX_APPS * MAX_LINKS,
        sizeof(ClientLinkAtServer_t));

    // Create a pool for client objects.
    PortPoolRef = le_mem_InitStaticPool(PortPool, MAX_APPS, sizeof(OpenedInstanceCtx_t));

    // Create a pool for JSON configuration.
    LinksConfigPoolRef = le_mem_InitStaticPool(LinksConfigPool,
                                               MAX_PORTS,
                                               sizeof(InstanceConfiguration_t));

    // Create a pool for link configuration.
    LinkPoolRef = le_mem_InitStaticPool(LinkPool,
                                        MAX_PORTS*MAX_LINKS,
                                        sizeof(LinkInformation_t));

    // Create a pool for list of links.
    LinkListPoolRef = le_mem_InitStaticPool(LinkListPool, MAX_PORTS, sizeof(LinkList_t));

    // Add a handler to the close session service.
    le_msg_AddServiceCloseHandler(le_port_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Link list for device.
    DeviceList = LE_DLS_LIST_INIT;

    // Link list for JSON object.
    InstanceContextList = LE_DLS_LIST_INIT;

#if !defined LE_CONFIG_PORT_CONFIG_IS_STATIC
    // Open the JSON file.
    JsonFd = open(JSON_CONFIG_FILE, O_RDONLY);

    // Start the parser (and wait for callbacks).
    JsonParsingSessionRef = le_json_Parse(JsonFd, JsonEventHandler, JsonErrorHandler, NULL);
#endif

    // Create the semaphore for client socket connect request.
    Semaphore = le_sem_Create("ClientConnectSem", 0);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
