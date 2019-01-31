/**
 * Simple test for le_fd API with "dataPort" device on RTOS.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "serialMngr/serialMngrApi.h"

#define INPUT_SIZE 64
#define INPUT_TIMEOUT 10

static le_fdMonitor_Ref_t fdMonitor = NULL;
static char buf[INPUT_SIZE] = {'\0'};
static int bufEnd = 0;
static bool SawPollout;

static void SerialPortHandler
(
    int fd,
    short events
)
{
    ssize_t retBytes;

    if (events & POLLOUT)
    {
        LE_TEST_INFO("Got POLLOUT event");
        SawPollout = true;
        le_fdMonitor_Disable(le_fdMonitor_GetMonitor(), POLLOUT);
    }

    if (events & POLLIN)
    {
        LE_TEST_INFO("Got POLLIN event");
        retBytes = le_fd_Read(fd, buf + bufEnd, sizeof(buf) - bufEnd);
        if (retBytes < 0)
        {
            LE_TEST_FATAL("Got error from le_fd_Read: %d", errno);
        }
        else if (retBytes == 0)
        {
            LE_TEST_FATAL("Got unexpected EOF from le_fd_Read");
        }
        else
        {
            bufEnd += retBytes;
        }

        if (buf[bufEnd -1] == '\r' || retBytes >= INPUT_SIZE)
        {
            // Chop carriage return from buffer
            buf[bufEnd - 1] = '\0';
            retBytes = le_fd_Write(fd, buf, strlen(buf));
            LE_TEST_ASSERT(-1 != retBytes, "Wrote %d bytes back to serial data port (input: '%s')",
                           (int) bufEnd - 1, buf);
            le_fd_Write(fd, "\r\n", 2);

            int ret = le_fd_Close(fd);
            LE_TEST_ASSERT(-1 != ret, "Data port device closed");

            le_fdMonitor_Delete(fdMonitor);
            LE_TEST_OK(SawPollout, "At least one POLLOUT event was triggered");
            LE_TEST_INFO("===== Successfully passed FD dataPort test =====");
            LE_TEST_EXIT;
        }
    }
}

COMPONENT_INIT
{
    int fd = -1;
    SawPollout = false;

    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_INFO("===== Starting FD dataPort test =====");

    fd = le_fd_Open("dataPortS1", O_RDWR);
    LE_TEST_ASSERT(-1 != fd, "Data port device opened");
    fdMonitor = le_fdMonitor_Create("SerialDataPort",
                                    fd,
                                    SerialPortHandler,
                                    POLLIN|POLLOUT);
    LE_TEST_ASSERT(NULL != fdMonitor,
                   "Waiting for ENTER-terminated user input on serial port (max input is %d bytes)",
                   INPUT_SIZE);
}
