/**
 * This module implements tests for io controls related to serial port.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"
#include "interfaces.h"
#include "at_switch/atSwitchApi.h"

static char const *stateStr[] = {"INACTIVE", "ACTIVE"};
static char const *dtrCfgStr[] = {"IGNORED", "SWITCH MODE", "HANG"};
static char const *dcdCfgStr[] = {"ALWAYS ON", "DC STATE"};
static le_event_HandlerRef_t HdlRef;


static void ThisIsTheEnd
(
    void *param,
    void *dummy
)
{
    int fd = (int)param;
    le_fd_SerialIoCtlParam_t ioctlParam;

    LE_TEST_INFO("DTR event received");

    ioctlParam.dtrEvtHdlRef = HdlRef;
    LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_DEL_DTR_EVT_HDL, &ioctlParam),
                   "Remove DTR event handler");

    LE_TEST_EXIT;
}

static void DtrEventHandler
(
    bool dtrState,      ///< DTR state
    void *ctxPtr        ///< context pointer
)
{
    LE_TEST_INFO("DTR is %s", stateStr[dtrState]);
    le_event_QueueFunction(ThisIsTheEnd, ctxPtr, NULL);
}

static void AtCmd
(
    char *cmdStr
)
{
    char atCmdRespStr[128] = {'\0'};
    atCmd_rc rc;

    rc = AtCmdSend(cmdStr, atCmdRespStr, strlen(atCmdRespStr));
    LE_TEST_ASSERT((rc == AT_CMD_OK) || (rc == AT_CMD_LEN_TOO_SHORT), "Send '%s' rc=%d", cmdStr, rc);
}

COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_INFO("===== Starting serial port io controls test =====");

    le_port_DeviceRef_t portDevRef = le_port_Request("s1");
    LE_TEST_ASSERT(NULL != portDevRef, "Requested 's1' through port service");
    le_result_t res = le_port_Release(portDevRef);
    LE_TEST_ASSERT(LE_OK == res, "Released 's1' device through port service");
    LE_TEST_INFO("Ready to open AT device through atServer");

    int fd = le_fd_Open("atServerS1", 0);
    LE_TEST_ASSERT(-1 != fd, "Requested file descriptor on AT device");

    le_fd_SerialIoCtlParam_t ioctlParam;

    for(int i = 0; i < 2; i++)
    {
        ioctlParam.state = i;
        LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_SET_DCD, &ioctlParam),
                       "Set the DCD to '%s'", stateStr[ioctlParam.state]);

        ioctlParam.state = !i;
        LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_GET_DCD, &ioctlParam), "Get the DCD.");
        LE_TEST_OK(ioctlParam.state == i, " DCD is '%s'", stateStr[ioctlParam.state]);
    }

    LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_GET_DTR, &ioctlParam),
               "Get the DTR. DTR is '%s'", stateStr[ioctlParam.state]);

    for(int i = 0; i < 3; i++)
    {
        char cmdStr[10] = {'\0'};

        snprintf(cmdStr, sizeof(cmdStr), "AT&D%d\r\n", i);
        AtCmd(cmdStr);

        ioctlParam.dtrCfg = 5;
        LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_GET_DTR_CFG, &ioctlParam),
                   "Get the DTR configuration");
        LE_TEST_OK(ioctlParam.dtrCfg == i,"DTR cfg is '%s'", dtrCfgStr[ioctlParam.dtrCfg]);
    }

    for(int i = 0; i < 2; i++)
    {
        char cmdStr[10] = {'\0'};

        snprintf(cmdStr, sizeof(cmdStr), "AT&C%d\r\n", i);
        AtCmd(cmdStr);

        ioctlParam.dcdCfg = 5;
        LE_TEST_OK(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_GET_DCD_CFG, &ioctlParam),
                   "Get the DCD configuration");
        LE_TEST_OK(ioctlParam.dcdCfg == i,"DCD cfg is '%s'", dcdCfgStr[ioctlParam.dcdCfg]);
    }

    ioctlParam.dtrEvt.hdl = DtrEventHandler;
    ioctlParam.dtrEvt.ctxPtr = (void*)fd;
    LE_TEST_ASSERT(0 == le_fd_Ioctl(fd, LE_FD_SERIAL_SET_DTR_EVT_HDL, &ioctlParam),
                   "Set DTR event handler");

    HdlRef = ioctlParam.dtrEvtHdlRef;

    LE_TEST_INFO("==== Switch the DTR line ====>");
}
