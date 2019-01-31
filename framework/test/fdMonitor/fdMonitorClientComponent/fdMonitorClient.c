//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#if LE_CONFIG_LINUX
#   include <sys/socket.h>
#   include <netinet/ip.h>
#   include <arpa/inet.h>
#endif

#define LOCAL_IP_ADDR "127.0.0.1"
#define SERVER_PORT_NUM 5000

static const char PingMsg[] = "ping";
static const char PongMsg[] = "pong";

COMPONENT_INIT
{
    struct sockaddr_in srvAddress;
    int ret, readMsgSize, writeMsgSize;
    int sockFd;
    int connectErrno;
    char msg[2*sizeof(PingMsg)];

    LE_INFO("Initializing client component");
    // Create the socket
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        LE_ERROR("creating socket failed (err %d)", sockFd);
        return;
    }

    memset(&srvAddress, 0, sizeof(srvAddress));
    srvAddress.sin_family = AF_INET;
    srvAddress.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDR);
    srvAddress.sin_port = htons(SERVER_PORT_NUM);

    do
    {
        ret = connect(sockFd, (struct sockaddr*) &srvAddress, sizeof(srvAddress));
        connectErrno = errno;
        if(ret < 0)
        {
            LE_WARN("connect failed (err %d, %s)", ret, strerror(connectErrno));
            if (ECONNREFUSED == connectErrno)
            {
                // Connection refused?  Wait a bit and try again.
                sleep(1);
            }
        }
    } while ((ret < 0) && ((EINTR == connectErrno) || (ECONNREFUSED == connectErrno)));

    if (ret < 0)
    {
        LE_WARN("Failed to connect");
        goto end;
    }

    readMsgSize = read(sockFd, msg, sizeof(msg));

    if (readMsgSize <= 0)
    {
        LE_WARN("Failed to read ping");
        goto end;
    }

    // If we get a ping, write back pong.  Otherwise
    // write back message so server can report it.
    if (strncmp(msg, PingMsg, readMsgSize) == 0)
    {
        writeMsgSize = write(sockFd, PongMsg, sizeof(PongMsg));
    }
    else
    {
        writeMsgSize = write(sockFd, msg, readMsgSize);
    }

    if (writeMsgSize <= 0)
    {
        LE_WARN("Failed to send response message");
        goto end;
    }

end:
    close(sockFd);
    return;
}
