/** @file appStopClient.c
 *
 * This is an AppStop Client program that's meant to be called by the cgroups "release_agent".
 * The program sends the app name to be stopped (supplied by release_agent) to the AppStop Server
 * hosted by the Supervisor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#define APPSTOP_SERVER_SOCKET_NAME       LE_CONFIG_RUNTIME_DIR "/AppStopServer"

COMPONENT_INIT
{
    struct sockaddr_un svaddr, claddr;
    int fd;
    size_t appNameLen;
    const char* appName = le_arg_GetArg(0);

    LE_FATAL_IF(appName == NULL, "App name not specified.");

    // Create client socket; bind to unique pathname (based on PID)
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    LE_FATAL_IF(fd == -1, "Error creating AppStop client socket.");

    memset(&claddr, 0, sizeof(struct sockaddr_un));
    claddr.sun_family = AF_UNIX;
    snprintf(claddr.sun_path, sizeof(claddr.sun_path),
             LE_CONFIG_RUNTIME_DIR "/AppStopClient__%ld__", (long)getpid());

    LE_FATAL_IF(bind(fd, (struct sockaddr *) &claddr, sizeof(struct sockaddr_un)) == -1,
                "Error binding AppStop client socket.");

    // Construct address of server
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, APPSTOP_SERVER_SOCKET_NAME, sizeof(svaddr.sun_path) - 1);

    appNameLen = strlen(appName);

    // Send name of app to be stopped to server.
    // Note that the app name being sent by the cgroups release_agent has a leading "/" which we
    // have to remove.
    ssize_t numBytesSent;
    do
    {
        numBytesSent = sendto(fd, &(appName[1]), appNameLen-1,
                              0, (struct sockaddr*)&svaddr, sizeof(struct sockaddr_un));
    }
    while ((numBytesSent == -1) && (errno == EINTR));

    LE_FATAL_IF(numBytesSent == -1, "Error sending app name to the AppStop server socket. %m");

    remove(claddr.sun_path);

    close(fd);

    exit(EXIT_SUCCESS);
}
