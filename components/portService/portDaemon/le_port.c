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
#include <sys/un.h>
#include <sys/socket.h>
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
/**
 * JSON configuration file path.
 */
//--------------------------------------------------------------------------------------------------
#ifdef LEGATO_EMBEDDED
#define JSON_CONFIG_FILE \
     "/legato/systems/current/apps/portService/read-only/usr/local/share/PortConfigurationFile.json"
#else
#define JSON_CONFIG_FILE le_arg_GetArg(0)
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
    le_atServer_DeviceRef_t atServerDevRef;                         ///< AT server device reference.
    char linkName[LINK_NAME_MAX_BYTES];                             ///< Link name.
    char path[PATH_MAX_BYTES];                                      ///< Path name.
    char openingType[OPEN_TYPE_MAX_BYTES];                          ///< Device opening type.
    char possibleMode[MAX_POSSIBLE_MODES][POSSIBLE_MODE_MAX_BYTES]; ///< Possible mode name.
    bool suspended;
}
LinkInformation_t;

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
 * Memory Pool for Port Service Client Handler.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PortPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for JSON object.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinksConfigPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for link information.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinkPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for list of links.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LinkListPoolRef;

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

//--------------------------------------------------------------------------------------------------
/**
 * JSON file descriptor.
 */
//--------------------------------------------------------------------------------------------------
static int JsonFd;

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

    if (-1 == close(fd))
    {
        LE_WARN("failed to close fd %d: %m", fd);
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
    // Stop JSON parsing and close the JSON file descriptor.
    le_json_Cleanup(JsonParsingSessionRef);

    // Close JSON file.
    CloseWarn(JsonFd);
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
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->atServerDevRef = NULL;
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkCounter]->suspended = false;

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

    fd = le_tty_Open(deviceName, O_RDWR);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open device");
        return -1;
    }

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

    return fd;
}

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
    le_atServer_DeviceRef_t atServerRef;

    if (POLLRDHUP & events)
    {
        LE_DEBUG("fd %d: connection reset by peer", clientFd);
    }
    else
    {
        LE_WARN("events %.8x not handled", events);
    }

    atServerRef = (le_atServer_DeviceRef_t)le_fdMonitor_GetContextPtr();
    if (atServerRef)
    {
        if (LE_OK != le_atServer_Close(atServerRef))
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
    int clientFd;
    char clientSocket[CLIENT_SOCKET_MAX_BYTES];
    le_fdMonitor_Ref_t fdMonitorRef;
    LinkInformation_t** linkInfoPtr;

    if (POLLIN & events)
    {
        clientFd = accept(sockFd, NULL, NULL);
        if (-1 == clientFd)
        {
            LE_ERROR("accepting socket failed: %m");
            goto exit_close_socket;
        }

        linkInfoPtr = (LinkInformation_t**)le_fdMonitor_GetContextPtr();

        if (0 == strcmp((*linkInfoPtr)->possibleMode[0], "DATA"))
        {
            LE_DEBUG("Socket opens in data mode.");
            (*linkInfoPtr)->dataModeFd = dup(clientFd);
            le_sem_Post(Semaphore);
            return;
        }

        (*linkInfoPtr)->fd = dup(clientFd);
        (*linkInfoPtr)->atServerDevRef = le_atServer_Open(dup(clientFd));

        if (NULL == (*linkInfoPtr)->atServerDevRef)
        {
            LE_ERROR("Cannot open the device!");
            CloseWarn(clientFd);
            goto exit_close_socket;
        }
        snprintf(clientSocket, sizeof(clientSocket) - 1, "socket-client-%d", clientFd);
        fdMonitorRef = le_fdMonitor_Create(clientSocket, clientFd, MonitorClient, POLLRDHUP);
        le_fdMonitor_SetContextPtr(fdMonitorRef, (*linkInfoPtr)->atServerDevRef);

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
    LinkInformation_t** linkInfoPtr     ///< [IN/OUT] Link information structure pointer.
)
{
    int sockFd;
    struct sockaddr_un myAddress;
    char socketName[SERVER_SOCKET_MAX_BYTES];
    le_fdMonitor_Ref_t fdMonitorRef;

    if ((-1 == unlink((*linkInfoPtr)->path)) && (ENOENT != errno))
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
    if (LE_OK != le_utf8_Copy(myAddress.sun_path, (*linkInfoPtr)->path,
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
    fdMonitorRef = le_fdMonitor_Create(socketName, sockFd, MonitorSocket, POLLIN);

    le_fdMonitor_SetContextPtr(fdMonitorRef, linkInfoPtr);

    return sockFd;

exit_close_socket:
    CloseWarn(sockFd);
    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the instance links.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t OpenInstanceLinks
(
    InstanceConfiguration_t* instanceConfigPtr ///< [IN] Instance configuaration.
)
{
    int i, j;

    le_dls_Link_t* linkPtr = le_dls_Peek(&(instanceConfigPtr->linkList));

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
                        instanceConfigPtr->linkInfo[i]->fd = OpenSerialDevice(instanceConfigPtr
                                                             ->linkInfo[i]->path);

                        if (0 > instanceConfigPtr->linkInfo[i]->fd)
                        {
                            LE_ERROR("Error in opening the device '%s': %m",
                                     instanceConfigPtr->instanceName);
                            return LE_FAULT;
                        }

                        instanceConfigPtr->linkInfo[i]->atServerDevRef =
                                          le_atServer_Open(dup(instanceConfigPtr->linkInfo[i]->fd));
                        if (NULL == instanceConfigPtr->linkInfo[i]->atServerDevRef)
                        {
                            LE_ERROR("atServerDevRef is NULL!");
                            return LE_FAULT;
                        }
                    }
                }
            }
            else if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket"))
            {
                for (j = 0; j < MAX_POSSIBLE_MODES; j++)
                {
                    if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
                    {
                        instanceConfigPtr->linkInfo[i]->atModeSockFd =
                                           OpenSocket(&(instanceConfigPtr->linkInfo[i]));
                        if (0 > instanceConfigPtr->linkInfo[i]->atModeSockFd)
                        {
                            LE_ERROR("Error in opening the device '%s': %m",
                                     instanceConfigPtr->instanceName);
                            return LE_FAULT;
                        }
                    }
                }
            }
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the instances which has "OpenByDefault" property as true.
 */
//--------------------------------------------------------------------------------------------------
static void OpenInstances
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

        if (true == instanceConfigPtr->openByDefault)
        {
            if (LE_OK != OpenInstanceLinks(instanceConfigPtr))
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
            LE_DEBUG("JSON parsing is completed.");

            // Open the instances which has "OpenByDefault" property as true.
            OpenInstances();
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
    InstanceConfiguration_t* instanceConfigPtr,    ///< [IN] Instance configuration pointer
    le_atServer_DeviceRef_t* atServerDeviceRef,    ///< [OUT] AtServer device reference.
    int* linkIndexPtr                              ///< [OUT] Link index of the link which contains
                                                   /// both AT and DATA mode.
)
{
    int i;
    bool allowSuspend = false;

    for (i = 0; i < (instanceConfigPtr->linkCounter); i++)
    {
        // Check if the same link supports AT and DATA as possiblemode.
        if (((0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "AT")) &&
            (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[1], "DATA"))) ||
            ((0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "DATA")) &&
            (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[1], "AT"))))
        {
            *linkIndexPtr = i;
            allowSuspend = true;
            break;
        }
    }

    if (true == allowSuspend)
    {
        if (NULL == instanceConfigPtr->linkInfo[*linkIndexPtr]->atServerDevRef)
        {
            LE_ERROR("atServerDeviceRef is NULL!");
            *atServerDeviceRef = NULL;
        }
        else
        {
            *atServerDeviceRef = instanceConfigPtr->linkInfo[*linkIndexPtr]->atServerDevRef;
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
le_port_DeviceRef_t le_port_Request
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

    OpenedInstanceCtx_t* openedInstanceCtxPtr = le_mem_ForceAlloc(PortPoolRef);

    // Save the client session.
    openedInstanceCtxPtr->sessionRef = le_port_GetClientSessionRef();

    le_port_DeviceRef_t devRef = le_ref_CreateRef(DeviceRefMap, openedInstanceCtxPtr);
    if (NULL == devRef)
    {
        LE_ERROR("devRef is NULL!");
        return NULL;
    }
    openedInstanceCtxPtr->deviceRef = devRef;
    openedInstanceCtxPtr->instanceConfigPtr = instanceConfigPtr;

    openedInstanceCtxPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&DeviceList, &(openedInstanceCtxPtr->link));

    // Check the openbydefault object value. If openbydefault true, then device is already opened.
    // If the openbydefault is false then open the instance.
    if (false == instanceConfigPtr->openByDefault)
    {
        if (LE_OK != OpenInstanceLinks(instanceConfigPtr))
        {
            LE_ERROR("Not able to open the instance links");
            return NULL;
        }
    }

    return devRef;
}

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
    linkInfo->dataModeSockFd = OpenSocket(&linkInfo);
    if (0 > linkInfo->dataModeSockFd)
    {
        LE_ERROR("Error in opening the device %m");
        return NULL;
    }

    le_event_RunLoop();
}

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
    int32_t* fdPtr                ///< [OUT] File descriptor of the device.
)
{
    le_result_t result = LE_FAULT;
    int i, j, linkIndex;
    le_atServer_DeviceRef_t atServerDeviceRef;
    le_clk_Time_t timeToWait = {10, 0};
    le_thread_Ref_t socketThreadRef;

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
    if (true == GetAtServerDeviceRef(instanceConfigPtr, &atServerDeviceRef, &linkIndex))
    {
        if (NULL == atServerDeviceRef)
        {
            LE_ERROR("atServerDeviceRef is NULL!");
            return LE_FAULT;
        }

        result = le_atServer_Suspend(atServerDeviceRef);
        if (LE_FAULT == result)
        {
            LE_ERROR("Device is already into data mode!");
            return LE_DUPLICATE;
        }
        else if (LE_OK != result)
        {
            return LE_FAULT;
        }

        instanceConfigPtr->linkInfo[linkIndex]->suspended = true;
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
                    *fdPtr = dup(instanceConfigPtr->linkInfo[i]->dataModeFd);
                }
                else
                {
                    *fdPtr = -1;
                }
            }
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
                    }
                    else if (0 == strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket"))
                    {
                        instanceConfigPtr->linkInfo[i]->atModeSockFd =
                                           OpenSocket(&(instanceConfigPtr->linkInfo[i]));
                        if (0 > instanceConfigPtr->linkInfo[i]->atModeSockFd)
                        {
                            LE_ERROR("Error in opening the device %s %m",
                                     instanceConfigPtr->instanceName);
                            return LE_FAULT;
                        }
                    }
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

    if (true == GetAtServerDeviceRef(instanceConfigPtr, deviceRefPtr, &linkIndex))
    {
        if (NULL == *deviceRefPtr)
        {
            LE_ERROR("deviceRefPtr is NULL!");
            return LE_FAULT;
        }

        if (instanceConfigPtr->linkInfo[linkIndex]->suspended == true)
        {
            result = le_atServer_Resume(*deviceRefPtr);
            if (LE_FAULT == result)
            {
                LE_ERROR("Device is not able to switch into command mode");
                return LE_FAULT;
            }

            instanceConfigPtr->linkInfo[linkIndex]->suspended = false;
        }
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
le_result_t le_port_Release
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
            // Check if the link supports AT as possiblemode.
            if (0 == strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "AT"))
            {
                linkIndex = i;
                break;
            }
        }
    }

    if (-1 == linkIndex)
    {
        LE_ERROR("Instance does not supports AT command mode!");
        return LE_FAULT;
    }

    if (NULL == instanceConfigPtr->linkInfo[linkIndex]->atServerDevRef)
    {
        LE_ERROR("atServerDevRef is NULL!");
        return LE_FAULT;
    }

    result = le_atServer_Close(instanceConfigPtr->linkInfo[linkIndex]->atServerDevRef);
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
            if (instanceConfigPtr->linkInfo[i]->atServerDevRef == atServerDevRef)
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
 * This function must be called to initialize the port service.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create safe reference map for request references.
    DeviceRefMap = le_ref_CreateMap("DeviceRef", MAX_APPS);

    // Create a pool for client objects.
    PortPoolRef = le_mem_CreatePool("PortPoolRef", sizeof(OpenedInstanceCtx_t));
    le_mem_ExpandPool(PortPoolRef, MAX_APPS);

    // Create a pool for JSON configuration.
    LinksConfigPoolRef = le_mem_CreatePool("LinksConfigPoolRef", sizeof(InstanceConfiguration_t));
    le_mem_ExpandPool(LinksConfigPoolRef, MAX_LINKS);

    // Create a pool for link configuration.
    LinkPoolRef = le_mem_CreatePool("LinkPoolRef", sizeof(LinkInformation_t));
    le_mem_ExpandPool(LinkPoolRef, MAX_LINKS);

    // Create a pool for list of links.
    LinkListPoolRef = le_mem_CreatePool("LinkListPoolRef", sizeof(LinkList_t));
    le_mem_ExpandPool(LinkListPoolRef, MAX_LINKS);

    // Add a handler to the close session service.
    le_msg_AddServiceCloseHandler(le_port_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Link list for device.
    DeviceList = LE_DLS_LIST_INIT;

    // Link list for JSON object.
    InstanceContextList = LE_DLS_LIST_INIT;

    // Open the JSON file.
    JsonFd = open(JSON_CONFIG_FILE, O_RDONLY);

    // Start the parser (and wait for callbacks).
    JsonParsingSessionRef = le_json_Parse(JsonFd, JsonEventHandler, JsonErrorHandler, NULL);

    // Create the semaphore for client socket connect request.
    Semaphore = le_sem_Create("ClientConnectSem", 0);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
