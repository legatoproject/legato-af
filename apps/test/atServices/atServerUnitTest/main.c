/**
 * This module implements the unit tests for AT server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "defs.h"
#include "strerror.h"

#define DSIZE           1024     // default buffer size
#define SERVER_TIMEOUT  10000    // server timeout in milliseconds

//--------------------------------------------------------------------------------------------------
/**
 * shared data between threads
 *
 */
//--------------------------------------------------------------------------------------------------
static SharedData_t SharedData;

//--------------------------------------------------------------------------------------------------
/**
 * convert \r\n into <>
 * example: at\r => at<
 *          \r\nOK\r\n => <>OK<>
 *
 */
//--------------------------------------------------------------------------------------------------
static char* PrettyPrint
(
    char* strPtr
)
{
    LE_ASSERT(strlen(strPtr) <= (DSIZE-1));

    static char copy[DSIZE];
    char* swapPtr;

    memset(copy, 0, DSIZE);
    strncpy(copy, strPtr, strlen(strPtr));

    swapPtr = copy;

    while(*swapPtr)
    {
        switch (*swapPtr)
        {
            case '\r':
                *swapPtr = '<';
            break;
            case '\n':
                *swapPtr = '>';
            break;
            case '\x1a':
            case '\x1b':
                *swapPtr = '#';
            break;
          default:
            if (!isprint(*swapPtr))
            {
                *swapPtr = '@';
            }
            break;
        }
        swapPtr++;
    }

    return copy;
}

//--------------------------------------------------------------------------------------------------
/**
 * send text
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendText
(
    int fd,
    int epollFd,
    const char* textPtr
)
{
    LE_INFO("Text: %s", PrettyPrint((char *)textPtr));

    if (write(fd, textPtr, strlen(textPtr)) == -1)
    {
        LE_ERROR("write failed: %s", strerror(errno));
        return LE_IO_ERROR;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * host thread function
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AtHost
(
    void* contextPtr
)
{
    SharedData_t* sharedDataPtr;
    char buf[DSIZE];
    struct sockaddr_un addr;
    int socketFd;
    int epollFd;
    struct epoll_event event;

    sharedDataPtr = (SharedData_t *)contextPtr;

    LE_DEBUG("Host Started");

    le_clk_Time_t timeToWait = {SERVER_TIMEOUT/1000,0};
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(sharedDataPtr->semRef, timeToWait));

    memset(buf, 0, DSIZE);

    epollFd = epoll_create1(0);
    LE_ASSERT(epollFd != -1);

    socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    LE_ASSERT(socketFd != -1);

    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = socketFd;

    LE_ASSERT(epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFd, &event) == 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sharedDataPtr->devPathPtr, sizeof(addr.sun_path)-1);

    LE_ASSERT(connect(socketFd, (struct sockaddr *)&addr, sizeof(addr)) != -1);

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(sharedDataPtr->semRef, timeToWait));

    // activate echo
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+ECHO=1",
                "\r\n+ECHO TYPE: PARA\r\n"
                "\r\nOK\r\n"));


    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT",
               "AT\r"
                "\r\n TYPE: ACT\r\n"
                "\r\nOK\r\n"));

    // deactivate echo
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+ECHO=0",
               "AT+ECHO=0\r"
                "\r\n+ECHO TYPE: PARA\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT",
                "\r\n TYPE: ACT\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "ATI",
                "\r\nManufacturer: Sierra Wireless, Incorporated\r\n"
                "Model: WP8548\r\n"
                "Revision: SWI9X15Y_07.10.04.00 "
                "12c1700 jenkins 2016/06/02 02:52:45\r\n"
                "IMEI: 359377060009700\r\n"
                "IMEI SV: 42\r\n"
                "FSN: LL542500111503\r\n"
                "+GCAP: +CGSM\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "ATASVE",
                "\r\nA TYPE: ACT\r\n"
                "\r\nS TYPE: ACT\r\n"
                "\r\nV TYPE: ACT\r\n"
                "\r\nE TYPE: ACT\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "ATASVEB",
                "\r\nA TYPE: ACT\r\n"
                "\r\nS TYPE: ACT\r\n"
                "\r\nV TYPE: ACT\r\n"
                "\r\nE TYPE: ACT\r\n"
                "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+ABCD;+ABCD=?;+ABCD?",
                "\r\n+ABCD TYPE: ACT\r\n"
                "\r\n+ABCD TYPE: TEST\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                "ATE0S3?;+ABCD?;S0?S0=2E1;V0S0=\"3\"+ABCD;+ABCD?5",
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 0\r\n"
                "\r\nS TYPE: READ\r\n"
                "S PARAM 0: 3\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "\r\nS TYPE: READ\r\n"
                "S PARAM 0: 0\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 2\r\n"
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 1\r\n"
                "\r\nV TYPE: PARA\r\n"
                "V PARAM 0: 0\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 3\r\n"
                "\r\n+ABCD TYPE: ACT\r\n"
                "\r\n+ABCD TYPE: READ\r\n"
                "+ABCD PARAM 0: 5\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,"AT&FE0V1&C1&D2S95=47S0=0",
                "\r\n&F TYPE: ACT\r\n"
                "\r\nE TYPE: PARA\r\n"
                "E PARAM 0: 0\r\n"
                "\r\nV TYPE: PARA\r\n"
                "V PARAM 0: 1\r\n"
                "\r\n&C TYPE: PARA\r\n"
                "&C PARAM 0: 1\r\n"
                "\r\n&D TYPE: PARA\r\n"
                "&D PARAM 0: 2\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 95\r\n"
                "S PARAM 1: 47\r\n"
                "\r\nS TYPE: PARA\r\n"
                "S PARAM 0: 0\r\n"
                "S PARAM 1: 0\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CBC=?",
                "\r\n+CBC: (0-2),(1-100),(voltage)\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CBC",
                "\r\n+CBC: 1,50,4190\r\n"
                "\r\nOK\r\n"
                "\r\n+CBC: 1,70,4190\r\n"
                "\r\n+CBC: 2,100,4190\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+EEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
                "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
                "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+DATA=?",
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE",
                "\r\nERROR\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=?",
                "\r\n+CMEE: (0-2)\r\n"
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=0",
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE?",
                "\r\n+CMEE: 0\r\n"
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=1",
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE?",
                "\r\n+CMEE: 1\r\n"
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=2",
                "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE?",
                "\r\n+CMEE: 2\r\n"
                "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+DATA",
                "\r\nCONNECT\r\n"
                "testing the data mode"
                "\r\nNO CARRIER\r\n"
                "\r\nCONNECTED\r\n"
                "\r\nCONNECTED\r\n"
                "\r\nCONNECTED\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CBC",
                "\r\n+CBC: 1,50,4190\r\n"
                "\r\nOK\r\n"
                "\r\n+CBC: 1,70,4190\r\n"
                "\r\n+CBC: 2,100,4190\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=0",
                "\r\nOK\r\n"));

    // Test bridge feature
    LE_ASSERT_OK(Testle_atServer_Bridge(socketFd, epollFd, sharedDataPtr));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+DEL="
                "\"AT\",\"ATI\",\"AT+CBC\",\"AT+ABCD\",\"ATA\",\"AT&F\","
                "\"ATS\",\"ATV\",\"AT&C\",\"AT&D\",\"ATE\",\"AT+DATA\"",
                "\r\nOK\r\n"));

    // ATD handler echoes the received parameter in intermediate response. The goal is to test
    // here the expected parameter of dial command.
    // AT server should bypass useless/unknown characters, and keep the ones belonging to the D
    // command: T,P,W,!,@,>,',',;,0 to 9, A to D, I,i,G,g. It makes also the uppercase when
    // possible.
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "ATD.T(+-33)1,-23-P-45-67-W-890-!tABCDabcde*#2pw@IiGg$:;",
                                     "\r\nT+331,23P4567W890!TABCDABCD*#2PW@IiGg;\r\n"
                                     "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "ATD>me\"John\"IG;D>1ig;D>ME1",
                                     "\r\n>ME\"John\"IG;\r\n"
                                     "\r\n>1ig;\r\n"
                                     "\r\n>ME1\r\n"
                                     "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1c"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_FORMAT_ERROR\r\n"
                                                  "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1b"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntesting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x08\x08\x08\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntest"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t\x08t\x08t\x08ting\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\nting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));
    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1ctesting"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_FORMAT_ERROR\r\n"
                                                  "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1btesting\x1b"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_FORMAT_ERROR\r\n"
                                                  "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1atesting\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_FORMAT_ERROR\r\n"
                                                  "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x0a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntesting\ntesting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x08"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntestintesting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "testing\x08testing\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntestintesting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "\bt\be\bs\bting\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\nting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "e"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "s"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "i"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "n"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "g"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "\x11"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_FORMAT_ERROR\r\n"
                                                  "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "e"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "s"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "i"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "n"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "g"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "\x1b"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\n"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+TEXT", "\r\n> "));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "e"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "s"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "t"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "i"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "n"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "g"));
    LE_ASSERT_OK(SendText(socketFd, epollFd, "\x1a"));
    LE_ASSERT_OK(TestResponses(socketFd, epollFd, "\r\ntesting"
                                                  "\r\nLE_OK\r\n"
                                                  "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+ERRCODE?", "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=1", "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"513\",\"CUSTOM_ERROR: \"",
                                     "\r\nCUSTOM_ERROR: 513\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=2", "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"514\",\"CUSTOM_ERROR: \",\"VERBOSE_MSG\"",
                                     "\r\nCUSTOM_ERROR: VERBOSE_MSG\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"159\",\"+CME ERROR: \"",
                                     "\r\n+CME ERROR: Uplink busy\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"159\",\"+CMS ERROR: \"",
                                     "\r\n+CMS ERROR: Unspecified TP-DCS error\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"127\",\"+CME ERROR: \"",
                                    "\r\n+CME ERROR: Missing or unknown APN\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"127\",\"+CMS ERROR: \"",
                                    "\r\n+CMS ERROR: Interworking, unspecified\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd,
                                     "AT+ERRCODE=\"128\",\"+UNDEF ERROR: \"",
                                    "\r\n+UNDEF ERROR: 128\r\n"
                                     ));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CMEE=0", "\r\nOK\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CLOSE?", "\r\nERROR\r\n"));

    LE_ASSERT_OK(SendCommandsAndTest(socketFd, epollFd, "AT+CLOSE", ""));


    LE_INFO("======== ATServer unit test PASSED ========");

    exit(0);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test on an expected result
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t TestResponses
(
    int fd,
    int epollFd,
    const char* expectedResponsePtr
)
{
    char buf[DSIZE];
    struct epoll_event ev;
    int ret = 0;
    int offset = 0;
    int count = 0;
    ssize_t size = 0;

    if (!expectedResponsePtr)
    {
        LE_ERROR("null expected response");
        return LE_FAULT;
    }

    count = strlen(expectedResponsePtr);
    memset(buf, 0 , DSIZE);

    while (count > 0)
    {
        do
        {
            ret = epoll_wait(epollFd, &ev, 1, SERVER_TIMEOUT);
        }
        while ((-1 == ret) && (EINTR == errno));

        if (-1 == ret)
        {
            LE_ERROR("epoll wait failed: %s", strerror(errno));
            return LE_IO_ERROR;
        }

        if (!ret)
        {
            LE_ERROR("Timed out waiting for server's response");
            return LE_TIMEOUT;
        }

        if (ev.data.fd != fd)
        {
            LE_ERROR("%s", strerror(EBADF));
            return LE_IO_ERROR;
        }

        if (ev.events & EPOLLRDHUP)
        {
            LE_ERROR("%s", strerror(ECONNRESET));
            return LE_TERMINATED;
        }

        size = read(fd, buf+offset, DSIZE);
        if (size == -1)
        {
            LE_ERROR("read failed: %s", strerror(errno));
            return LE_IO_ERROR;
        }

        offset += size;
        count -= size;

    }

    LE_DEBUG("Response: %s", PrettyPrint(buf));
    LE_DEBUG("Expected: %s", PrettyPrint((char *)expectedResponsePtr));

    if (strcmp(buf, expectedResponsePtr))
    {
        LE_ERROR("response %s", PrettyPrint(buf));
        LE_ERROR("expected %s", PrettyPrint((char*)expectedResponsePtr));
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * send an AT command and test on an expected result
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t SendCommandsAndTest
(
    int fd,
    int epollFd,
    const char* commandsPtr,
    const char* expectedResponsePtr
)
{
    char buf[DSIZE];
    int size;

    size = strlen(commandsPtr);
    if (DSIZE <= size)
    {
        LE_ERROR("command is too long: %d", size);
        return LE_FAULT;
    }

    memset(buf, 0 , DSIZE);

    snprintf(buf, strlen(commandsPtr)+2, "%s\r", commandsPtr);

    LE_INFO("Commands: %s", PrettyPrint(buf));

    if (write(fd, buf, strlen(buf)) == -1)
    {
        LE_ERROR("write failed: %s", strerror(errno));
        return LE_IO_ERROR;
    }

    return TestResponses(fd, epollFd, expectedResponsePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t atHostThread;

    // To reactivate for all DEBUG logs
#ifdef DEBUG
    LE_INFO("DEBUG MODE");
    le_log_SetFilterLevel(LE_LOG_DEBUG);
#endif

    memset(&SharedData, 0, sizeof(SharedData));
    SharedData.devPathPtr = "\0at-dev";
    SharedData.semRef = le_sem_Create("AtUnitTestSem", 0);
    SharedData.atServerThread = le_thread_GetCurrent();

    atHostThread = le_thread_Create("atHostThread", AtHost, (void *)&SharedData);
    le_thread_Start(atHostThread);

    AtServer(&SharedData);
}
