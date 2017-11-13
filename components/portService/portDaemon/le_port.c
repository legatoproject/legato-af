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
#define BLOCKING_MODE     true
#define NON_BLOCKING_MODE false

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of link name.
 */
//--------------------------------------------------------------------------------------------------
#define LINK_NAME_MAX_BYTES       10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of possible mode string.
 */
//--------------------------------------------------------------------------------------------------
#define POSSIBLE_MODE_MAX_BYTES   10

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of links.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LINKS              2

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
#define OPEN_TYPE_MAX_BYTES       20

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of clients
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CLIENTS            1

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL       8

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
    char linkName[LINK_NAME_MAX_BYTES];                               ///< Link name.
    char path[PATH_MAX_BYTES];                                        ///< Path name.
    char openingType[OPEN_TYPE_MAX_BYTES];                            ///< Device opening type.
    char possibleMode[MAX_LINKS][POSSIBLE_MODE_MAX_BYTES];            ///< Possible mode name.
}
LinkInformation_t;

//--------------------------------------------------------------------------------------------------
/**
 * JSON structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char                instanceNamePtr[LE_PORT_MAX_LEN_DEVICE_NAME]; ///< Instance name.
    char                linkList[MAX_LINKS][LINK_NAME_MAX_BYTES];     ///< List of link name.
    bool                openByDefault;                                ///< Indicates whether device
                                                                      ///< opens at system startup.
    LinkInformation_t*  linkInfo[MAX_LINKS];                          ///< Device link information
                                                                      ///< structure pointer.
    int8_t              linkNumber;                                   ///< Device link count.
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
    char                        deviceNamePtr[LE_PORT_MAX_LEN_DEVICE_NAME]; ///< The device name.
    int32_t                     fd;                                         ///< The device
                                                                            ///< indentifier.
    int32_t                     dataModeFd;                                 ///< The device
                                                                            ///< indentifier
                                                                            ///< specific to data
                                                                            ///< mode.
    le_port_DeviceRef_t         deviceRef;                                  ///< Device reference
                                                                            ///< for client.
    le_atServer_DeviceRef_t     atServerDevRef;                             ///< AT server device
                                                                            ///< reference.
    le_msg_SessionRef_t         sessionRef;                                 ///< Client session
                                                                            ///< identifier.
    le_dls_Link_t               link;                                       ///< Object node link.
}
OpenedInstanceCtx_t;

//--------------------------------------------------------------------------------------------------
/**
 * Device mode structure.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AT_CMD_MODE = 0,  ///< Device opens in AT command mode.
    DATA_MODE = 1     ///< Device opens in data mode.
}
DeviceMode_t;

//--------------------------------------------------------------------------------------------------
/**
 * FdMonitor context pointer structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    OpenedInstanceCtx_t* openedInstanceCtxPtr;  ///< Opened instance context pointer.
    DeviceMode_t deviceMode;                    ///< Device mode.
    le_msg_SessionRef_t sessionRef;             ///< Client session.
}
FdMonitorContext_t;

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
 * Memory pool for fdMonitor context pointer.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FdMonitorCtxRef;

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
 * Thread and semaphore for JSON parsing.
 */
//--------------------------------------------------------------------------------------------------
static bool JsonParseComplete;


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

        if (0 == strcmp(instanceConfigPtr->instanceNamePtr, deviceNamePtr))
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
 * Get opened instance from device name.
 */
//--------------------------------------------------------------------------------------------------
static OpenedInstanceCtx_t* GetOpenedInstanceCtxPtr
(
    const char* deviceNamePtr    ///< [IN] Device name.
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&DeviceList);

    while (linkPtr)
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = CONTAINER_OF(linkPtr,
                                                                 OpenedInstanceCtx_t,
                                                                 link);
        linkPtr = le_dls_PeekNext(&DeviceList, linkPtr);

        if (0 == strcmp(openedInstanceCtxPtr->deviceNamePtr, deviceNamePtr))
        {
            LE_DEBUG("Device found: %p", openedInstanceCtxPtr);
            return openedInstanceCtxPtr;
        }
    }

    LE_ERROR("Not able to get the device");
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
    int linkNumber = 0;

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
            if (NULL != memberName)
            {
                le_utf8_Copy(instanceConfigPtr->linkList[linkNumber++], memberName,
                             LINK_NAME_MAX_BYTES, NULL);
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

            le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkNumber]->path,
                         memberName, PATH_MAX_BYTES, NULL);

            le_json_SetEventHandler(DeviceEventHandler);
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

            le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkNumber]->openingType,
                         memberName, OPEN_TYPE_MAX_BYTES, NULL);
            le_json_SetEventHandler(DeviceEventHandler);
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
    int linkNumber = 0;

    switch (event)
    {
        case LE_JSON_ARRAY_START:
            break;

        case LE_JSON_ARRAY_END:
            le_json_SetEventHandler(DeviceEventHandler);
            break;

        case LE_JSON_STRING:
        {
            const char* memberName = le_json_GetString();
            le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkNumber]
                         ->possibleMode[linkNumber], memberName, POSSIBLE_MODE_MAX_BYTES, NULL);
            linkNumber++;
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

                instanceConfigPtr->linkNumber++;
                instanceConfigPtr->linkInfo[instanceConfigPtr->linkNumber] =
                                           (LinkInformation_t*)le_mem_ForceAlloc(LinkPoolRef);

                le_utf8_Copy(instanceConfigPtr->linkInfo[instanceConfigPtr->linkNumber]->linkName,
                             linkName, LINK_NAME_MAX_BYTES, NULL);
            }
            else if (strcmp(memberName, "path") == 0)
            {
                le_json_SetEventHandler(PathEventHandler);
            }
            else if (strcmp(memberName, "openingType") == 0)
            {
                le_json_SetEventHandler(OpeningTypeEventHandler);
            }
            else if (strcmp(memberName, "possibleMode") == 0)
            {
                le_json_SetEventHandler(PossibleModeEventHandler);
            }
            else if (strcmp(memberName, "OpenByDefault") == 0)
            {
                le_json_SetEventHandler(OpenByDefaultEventHandler);
            }
            else if (strcmp(memberName, "OpenLinks") == 0)
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
    char* deviceName,  ///< [IN] Device name.
    bool blokingMode   ///< [IN] Device blocking mode. 0 -> non-blocking mode, 1 -> blocking mode.
)
{
    int32_t fd;

    fd = le_tty_Open(deviceName, O_RDWR | O_NOCTTY);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open device");
        return -1;
    }

    if (LE_OK != le_tty_SetBaudRate(fd, LE_TTY_SPEED_115200))
    {
        LE_ERROR("Failed to configure TTY baud rate");
        goto error;
    }

    if (LE_OK != le_tty_SetFraming(fd, 'N', 8, 1))
    {
        LE_ERROR("Failed to configure TTY framing");
        goto error;
    }

    if (LE_OK != le_tty_SetFlowControl(fd, LE_TTY_FLOW_CONTROL_NONE))
    {
        LE_ERROR("Failed to configure TTY flow control");
        goto error;
    }

    if (blokingMode == false)
    {
        // Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters,
        // disables local echo, numChars = 0 and timeout = 0: Read will be completetly non-blocking.
        if (LE_OK != le_tty_SetRaw(fd, 0, 0))
        {
            LE_ERROR("Failed to configure TTY raw");
            goto error;
        }
    }
    else
    {
        // Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters,
        // disables local echo, numChars = 50 and timeout = 50: Read will be completetly blocking.
        if (LE_OK != le_tty_SetRaw(fd, 50, 50))
        {
            LE_ERROR("Failed to configure TTY raw");
            goto error;
        }
    }
    return fd;

error:
    // Close the TTY
    le_tty_Close(fd);

    return -1;
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
    le_result_t result;
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
        result = le_atServer_Close(atServerRef);
        if (LE_OK != result)
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
    FdMonitorContext_t* fdMonitorContextPtr;

    if (POLLIN & events)
    {
        clientFd = accept(sockFd, NULL, NULL);
        if (-1 == clientFd)
        {
            LE_ERROR("accepting socket failed: %m");
            goto exit_close_socket;
        }

        fdMonitorContextPtr = (FdMonitorContext_t*)le_fdMonitor_GetContextPtr();

        if (fdMonitorContextPtr->deviceMode == DATA_MODE)
        {
            LE_DEBUG("Socket opens in data mode.");
            return;
        }

        fdMonitorContextPtr->openedInstanceCtxPtr->atServerDevRef = le_atServer_Open(dup(clientFd));

        if (NULL == fdMonitorContextPtr->openedInstanceCtxPtr->atServerDevRef)
        {
            LE_ERROR("Cannot open the device!");
            CloseWarn(clientFd);
            goto exit_close_socket;
        }
        snprintf(clientSocket, sizeof(clientSocket) - 1, "socket-client-%d", clientFd);
        fdMonitorRef = le_fdMonitor_Create(clientSocket, clientFd, MonitorClient, POLLRDHUP);
        le_fdMonitor_SetContextPtr(fdMonitorRef,
                                   fdMonitorContextPtr->openedInstanceCtxPtr->atServerDevRef);
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
    char* deviceName,                              ///< [IN] The device name.
    OpenedInstanceCtx_t* openedInstanceCtxPtr,     ///< [IN] Opened instance context.
    DeviceMode_t deviceMode                        ///< [IN] Device mode.
)
{
    int sockFd;
    struct sockaddr_un myAddress;
    char socketName[SERVER_SOCKET_MAX_BYTES];
    le_fdMonitor_Ref_t fdMonitorRef;
    FdMonitorContext_t* fdMonitorContextPtr;

    if ( (-1 == unlink(deviceName)) && (ENOENT != errno) )
    {
        LE_ERROR("unlink socket failed: %m");
        return -1;
    }

    // Create the socket
    sockFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (-1 == sockFd)
    {
        LE_ERROR("creating socket failed: %m");
        return -1;
    }

    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sun_family = AF_UNIX;
    le_utf8_Copy(myAddress.sun_path, deviceName, sizeof(myAddress.sun_path) - 1, NULL);

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

    // Create the memory for fdMonitor context pointer inforamtion.
    fdMonitorContextPtr = le_mem_ForceAlloc(FdMonitorCtxRef);

    // Fill the fdMonitor context pointer inforamtion.
    fdMonitorContextPtr->openedInstanceCtxPtr = openedInstanceCtxPtr;
    fdMonitorContextPtr->deviceMode = deviceMode;

    le_fdMonitor_SetContextPtr(fdMonitorRef, fdMonitorContextPtr);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

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
le_result_t OpenInstanceLinks
(
    InstanceConfiguration_t* instanceConfigPtr, ///< [IN] Instance configuaration.
    OpenedInstanceCtx_t* openedInstanceCtxPtr   ///< [IN] Opened instance context.
)
{
    int i;

    for (i = 0; i < MAX_LINKS; i++)
    {
        if ((strstr(instanceConfigPtr->linkList[i], "link")!= NULL) &&
            (strcmp(instanceConfigPtr->linkInfo[i]->linkName, instanceConfigPtr->linkList[i]) == 0))
        {
            if (strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink") == 0)
            {
                if (strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "DATA") == 0)
                {
                    // Open the links specified in OpenLinks object.
                    openedInstanceCtxPtr->fd = OpenSerialDevice(instanceConfigPtr
                                                                ->linkInfo[i]->path, BLOCKING_MODE);
                }
                else if (strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "AT") == 0)
                {
                    // Open the links specified in OpenLinks object.
                    openedInstanceCtxPtr->fd = OpenSerialDevice(instanceConfigPtr
                                                                ->linkInfo[i]->path,
                                                                NON_BLOCKING_MODE);
                }
                else
                {
                    LE_ERROR("The device mode is not valid!");
                    return LE_FAULT;
                }
                if (0 > openedInstanceCtxPtr->fd)
                {
                    LE_ERROR("Error in opening the device '%s': %m",
                             openedInstanceCtxPtr->deviceNamePtr);
                    return LE_FAULT;
                }
                else
                {
                    openedInstanceCtxPtr->atServerDevRef =
                                          le_atServer_Open(openedInstanceCtxPtr->fd);
                }
                if (NULL == openedInstanceCtxPtr->atServerDevRef)
                {
                    LE_ERROR("atServerDevRef is NULL!");
                    return LE_FAULT;
                }
                if (strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "DATA") == 0)
                {
                    if (LE_FAULT == le_atServer_Suspend(openedInstanceCtxPtr->atServerDevRef))
                    {
                        LE_ERROR("Device is not able to open into data mode");
                        return LE_FAULT;
                    }
                }
            }
            else if (strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket") == 0)
            {
                if (strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "DATA") == 0)
                {
                    openedInstanceCtxPtr->fd = OpenSocket(instanceConfigPtr->linkInfo[i]->path,
                                                          openedInstanceCtxPtr, DATA_MODE);
                }
                else if (strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[0], "AT") == 0)
                {
                    openedInstanceCtxPtr->fd = OpenSocket(instanceConfigPtr->linkInfo[i]->path,
                                                          openedInstanceCtxPtr, AT_CMD_MODE);
                }
                else
                {
                    LE_ERROR("The device mode is not valid!");
                    return LE_FAULT;
                }
                if (0 > openedInstanceCtxPtr->fd)
                {
                    LE_ERROR("Error in opening the device '%s': %m",
                             openedInstanceCtxPtr->deviceNamePtr);
                    return LE_FAULT;
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
            OpenedInstanceCtx_t* openedInstanceCtxPtr = le_mem_ForceAlloc(PortPoolRef);

            // Initialization of openedInstanceCtxPtr.
            openedInstanceCtxPtr->dataModeFd = 0;
            openedInstanceCtxPtr->fd = 0;
            openedInstanceCtxPtr->atServerDevRef = NULL;

            // Save the device name.
            le_utf8_Copy(openedInstanceCtxPtr->deviceNamePtr, instanceConfigPtr->instanceNamePtr,
                         LE_PORT_MAX_LEN_DEVICE_NAME, NULL);

            le_port_DeviceRef_t devRef = le_ref_CreateRef(DeviceRefMap, openedInstanceCtxPtr);
            if (NULL == devRef)
            {
                LE_ERROR("devRef is NULL!");
                continue;
            }

            openedInstanceCtxPtr->deviceRef = devRef;
            openedInstanceCtxPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(&DeviceList, &(openedInstanceCtxPtr->link));

            if (LE_OK != OpenInstanceLinks(instanceConfigPtr, openedInstanceCtxPtr))
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
                le_utf8_Copy(instanceConfigurationPtr->instanceNamePtr, memberName,
                             sizeof(instanceConfigurationPtr->instanceNamePtr), NULL);

                // Initialize the link number.
                instanceConfigurationPtr->linkNumber = -1;

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

    // Check the openbydefault object value. If openbydefault true, then device is already opened.
    // Return device reference.
    if (true == instanceConfigPtr->openByDefault)
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = GetOpenedInstanceCtxPtr(deviceNamePtr);
        return openedInstanceCtxPtr->deviceRef;
    }
    else
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = le_mem_ForceAlloc(PortPoolRef);

        // Initialization of openedInstanceCtxPtr.
        openedInstanceCtxPtr->dataModeFd = 0;
        openedInstanceCtxPtr->fd = 0;
        openedInstanceCtxPtr->atServerDevRef = NULL;

        // Save the device name.
        le_utf8_Copy(openedInstanceCtxPtr->deviceNamePtr, deviceNamePtr,
                     LE_PORT_MAX_LEN_DEVICE_NAME, NULL);

        // Save the client session.
        openedInstanceCtxPtr->sessionRef = le_port_GetClientSessionRef();

        le_port_DeviceRef_t devRef = le_ref_CreateRef(DeviceRefMap, openedInstanceCtxPtr);
        if (NULL == devRef)
        {
            LE_ERROR("devRef is NULL!");
            return NULL;
        }

        openedInstanceCtxPtr->deviceRef = devRef;
        openedInstanceCtxPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&DeviceList, &(openedInstanceCtxPtr->link));

        if (LE_OK != OpenInstanceLinks(instanceConfigPtr, openedInstanceCtxPtr))
        {
            LE_ERROR("Not able to open the instance links");
            return NULL;
        }

        return devRef;
    }
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int32_t* fdPtr                ///< [OUT] File descriptor of the device.
)
{
    le_result_t result = LE_FAULT;
    int i,j;

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

    if (NULL == fdPtr)
    {
        LE_ERROR("fdPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    le_atServer_DeviceRef_t atServerDeviceRef = openedInstanceCtxPtr->atServerDevRef;
    if (NULL == atServerDeviceRef)
    {
        LE_ERROR("atServerDeviceRef is NULL!");
        return LE_FAULT;
    }

    result = le_atServer_Suspend(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to switch into data mode");
        return LE_FAULT;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr =
                             GetInstanceFromDeviceName(openedInstanceCtxPtr->deviceNamePtr);
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return LE_FAULT;
    }

    result = LE_FAULT;

    // Check all the links. Open the link which contains "DATA" as possibleMode.
    for (i = 0; i < MAX_LINKS; i++)
    {
        if ((strstr(instanceConfigPtr->linkList[i], "link") != NULL) &&
            (strcmp(instanceConfigPtr->linkInfo[i]->linkName, instanceConfigPtr->linkList[i]) == 0))
        {
            for (j = 0; j < MAX_LINKS; j++)
            {
                if ((strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "DATA") == 0) &&
                   (strcmp(instanceConfigPtr->linkInfo[i]->openingType, "serialLink") == 0))
                {
                    // Open the links specified in OpenLinks object.
                    openedInstanceCtxPtr->dataModeFd = OpenSerialDevice(instanceConfigPtr->
                                                                        linkInfo[i]->path,
                                                                        BLOCKING_MODE);
                    if (openedInstanceCtxPtr->dataModeFd != -1)
                    {
                        *fdPtr = (openedInstanceCtxPtr->dataModeFd);
                        result = LE_OK;
                    }
                    else
                    {
                        *fdPtr = -1;
                        result = LE_FAULT;
                    }
                    return result;
                }
                else if ((strcmp(instanceConfigPtr->linkInfo[i]->possibleMode[j], "DATA") == 0) &&
                   (strcmp(instanceConfigPtr->linkInfo[i]->openingType, "unixSocket") == 0))
                {
                    openedInstanceCtxPtr->dataModeFd = OpenSocket(instanceConfigPtr->
                                                                  linkInfo[i]->path,
                                                                  openedInstanceCtxPtr,
                                                                  DATA_MODE);
                    if (openedInstanceCtxPtr->dataModeFd != -1)
                    {
                        *fdPtr = (openedInstanceCtxPtr->dataModeFd);
                        result = LE_OK;
                    }
                    else
                    {
                        *fdPtr = -1;
                        result = LE_FAULT;
                    }
                    return result;
                }
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into AT command mode.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetCommandMode
(
    le_port_DeviceRef_t devRef    ///< [IN] Device reference.
)
{
    le_result_t result;

    OpenedInstanceCtx_t* openedInstanceCtxPtr = le_ref_Lookup(DeviceRefMap, devRef);
    if (NULL == openedInstanceCtxPtr)
    {
        LE_ERROR("devRef is invalid!");
        return LE_BAD_PARAMETER;
    }

    le_atServer_DeviceRef_t atServerDeviceRef = openedInstanceCtxPtr->atServerDevRef;
    if (NULL == atServerDeviceRef)
    {
        LE_ERROR("atServerDeviceRef is NULL!");
        return LE_FAULT;
    }

    result = le_atServer_Resume(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to switch into command mode");
        return LE_FAULT;
    }

    return LE_OK;
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

    le_atServer_DeviceRef_t atServerDeviceRef = openedInstanceCtxPtr->atServerDevRef;
    if (NULL == atServerDeviceRef)
    {
        LE_ERROR("atServerDeviceRef is NULL!");
        return LE_FAULT;
    }

    result = le_atServer_Close(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to close");
        return LE_FAULT;
    }

    // Get the instance configuration.
    InstanceConfiguration_t* instanceConfigPtr =
                             GetInstanceFromDeviceName(openedInstanceCtxPtr->deviceNamePtr);
    if (NULL == instanceConfigPtr)
    {
        LE_ERROR("instanceConfigPtr is NULL!");
        return LE_FAULT;
    }

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
    if (NULL == devRefPtr)
    {
        LE_ERROR("devRefPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&DeviceList);

    while (linkPtr)
    {
        OpenedInstanceCtx_t* openedInstanceCtxPtr = CONTAINER_OF(linkPtr,
                                                                 OpenedInstanceCtx_t,
                                                                 link);
        linkPtr = le_dls_PeekNext(&DeviceList, linkPtr);

        if (openedInstanceCtxPtr->atServerDevRef == atServerDevRef)
        {
            *devRefPtr = openedInstanceCtxPtr->deviceRef;
            return LE_OK;
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

            // Delete the link from DeviceList.
            le_dls_Remove(&DeviceList, &(openedInstanceCtxPtr->link));

            // Release memory and delete reference of device.
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

    // Create a pool for fdMonitor context pointer.
    FdMonitorCtxRef = le_mem_CreatePool("FdMonitorCtxRef", sizeof(FdMonitorContext_t));
    le_mem_ExpandPool(FdMonitorCtxRef, MAX_LINKS);

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
}
