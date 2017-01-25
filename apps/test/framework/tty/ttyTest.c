/*
 * Test serial port.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "fileDescriptor.h"
#include <termios.h>

/// Number of array members.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

static char* DevTty = NULL;

static tty_Speed_t SpeedTestTable[] =
{
    LE_TTY_SPEED_0,
    LE_TTY_SPEED_50,
    LE_TTY_SPEED_75,
    LE_TTY_SPEED_110,
    LE_TTY_SPEED_134,
    LE_TTY_SPEED_150,
    LE_TTY_SPEED_200,
    LE_TTY_SPEED_300,
    LE_TTY_SPEED_600,
    LE_TTY_SPEED_1200,
    LE_TTY_SPEED_1800,
    LE_TTY_SPEED_2400,
    LE_TTY_SPEED_4800,
    LE_TTY_SPEED_9600,
    LE_TTY_SPEED_19200,
    LE_TTY_SPEED_38400,
    LE_TTY_SPEED_57600,
    LE_TTY_SPEED_115200,
    LE_TTY_SPEED_230400,
    LE_TTY_SPEED_460800,
    LE_TTY_SPEED_500000,
    LE_TTY_SPEED_576000,
    LE_TTY_SPEED_921600,
    LE_TTY_SPEED_1000000,
    LE_TTY_SPEED_1152000,
    LE_TTY_SPEED_1500000,
    LE_TTY_SPEED_2000000,
    LE_TTY_SPEED_2500000,
    LE_TTY_SPEED_3000000,
    LE_TTY_SPEED_3500000,
    LE_TTY_SPEED_4000000,
};

//--------------------------------------------------------------------------------------------------
/**
 * Wrapper to le_tty_Open
 */
//--------------------------------------------------------------------------------------------------
static int TestTtyOpen
(
    void
)
{
    return le_tty_Open(DevTty, O_RDWR | O_NOCTTY | O_NDELAY);
}

//--------------------------------------------------------------------------------------------------
/**
 * Wrapper to le_tty_close
 */
//--------------------------------------------------------------------------------------------------
static void TestTtyClose
(
    int fd
)
{
    return le_tty_Close(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test open, lock, and close
 */
//--------------------------------------------------------------------------------------------------
static void TestTtyOpenClose
(
    void
)
{
    int fd = 0;
    struct flock ttyLock = {0};

    fd = TestTtyOpen();
    LE_ASSERT(fd > -1)

    // Test lock
    if (0 > fcntl(fd, F_GETLK, &ttyLock))
    {
        LE_ERROR("Error: '%s' locked by process %d: %m.", DevTty, ttyLock.l_pid);
        fd_Close(fd);
        exit(EXIT_FAILURE);
    }
    LE_ASSERT(ttyLock.l_type == F_UNLCK)

    TestTtyClose(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test all settings baud rate
 */
//--------------------------------------------------------------------------------------------------
static void TestTtySettingBaudRate
(
    void
)
{
    int i=0;
    int fd = TestTtyOpen();

    for(i=0;i<ARRAY_SIZE(SpeedTestTable);i++)
    {
        tty_Speed_t ispeed, ospeed;
        le_result_t result = le_tty_SetBaudRate(fd,SpeedTestTable[i]);
        if ( LE_OK == result )
        {
            LE_ASSERT(LE_OK == le_tty_GetBaudRate(fd, &ispeed, &ospeed))
            LE_ASSERT((i == ispeed) && (i == ospeed))
        }
        else
        {
            LE_ASSERT(LE_UNSUPPORTED == result)
            LE_ASSERT(LE_OK == le_tty_GetBaudRate(fd, &ispeed, &ospeed))
            LE_ASSERT(!((i == ispeed) && (i == ospeed)))
        }
    }

    TestTtyClose(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test Framing
 */
//--------------------------------------------------------------------------------------------------
static void TestTtySetFraming
(
    void
)
{
    const char parity[] = { 'N', 'O', 'E'};
    const int dataBits[] = { 5 , 6 , 7 ,8 };
    const int stopBits[] = { 1, 2 };
    int p,d,s;
    le_result_t result;

    int fd = TestTtyOpen();

    for( p=0 ; p<ARRAY_SIZE(parity) ; p++)
    {
        for( d=0; d<ARRAY_SIZE(dataBits) ; d++)
        {
            for( s=0; s<ARRAY_SIZE(stopBits) ; s++)
            {
                result = le_tty_SetFraming(fd, parity[p],dataBits[d],stopBits[s]);
                if ( LE_OK != result )
                {
                    LE_ASSERT(LE_UNSUPPORTED == result)
                }
            }
        }
    }

    LE_ASSERT(LE_NOT_FOUND == le_tty_SetFraming(fd, 'Z', 8, 1))
    LE_ASSERT(LE_NOT_FOUND == le_tty_SetFraming(fd, 'N', 9, 1))
    LE_ASSERT(LE_NOT_FOUND == le_tty_SetFraming(fd, 'N', 8, 0))

    LE_ASSERT(LE_OK == le_tty_SetFraming(fd, 'N', 8, 1))

    TestTtyClose(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test flow control settings
 */
//--------------------------------------------------------------------------------------------------
static void TestTtySetFlowControl
(
    void
)
{
    int fd = TestTtyOpen();

    LE_ASSERT(LE_OK == le_tty_SetFlowControl(fd, LE_TTY_FLOW_CONTROL_NONE));
    LE_ASSERT(LE_OK == le_tty_SetFlowControl(fd, LE_TTY_FLOW_CONTROL_XON_XOFF));
    LE_ASSERT(LE_OK == le_tty_SetFlowControl(fd, LE_TTY_FLOW_CONTROL_HARDWARE));

    LE_ASSERT(LE_NOT_FOUND == le_tty_SetFlowControl(fd, 3));

    TestTtyClose(fd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test canonical
 */
//--------------------------------------------------------------------------------------------------
static void TestTtySetCanonical
(
    void
)
{
    int fd = TestTtyOpen();

    LE_ASSERT(LE_OK == le_tty_SetCanonical(fd));

    TestTtyClose(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * test raw
 */
//--------------------------------------------------------------------------------------------------
static void TestTtySetRaw
(
    void
)
{
    int fd = TestTtyOpen();

    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, 1));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 1, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 1, 1));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, 255));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 255, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 255, 255));

    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, INT_MAX));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, INT_MAX, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, INT_MAX, INT_MAX));

    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, UINT_MAX));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, UINT_MAX, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, UINT_MAX, UINT_MAX));

    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, 0, -1));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, -1, 0));
    LE_ASSERT(LE_OK == le_tty_SetRaw(fd, -1, -1));

    TestTtyClose(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    struct termios portSettings;

    LE_INFO("======== Starting Tty Test ========");

    le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO("PRINT USAGE => testTty /dev/ttyS0");

    if (le_arg_NumArgs() == 1)
    {
        // This function gets the test band fom the User (interactive case).
        const char* tty = le_arg_GetArg(0);
        if (tty != NULL )
        {
            LE_INFO("testTty argument %s", tty);
            DevTty = strdup(tty);
        }
    }
    else
    {
        LE_INFO("PRINT USAGE => testTty /dev/ttyS0");
        exit(EXIT_FAILURE);
    }

    // Save configuration
    int fd = TestTtyOpen();
    if (-1 == tcgetattr(fd, &portSettings))
    {
        LE_DEBUG("Cannot retrieve port settings");
        exit(EXIT_FAILURE);
    }
    TestTtyClose(fd);

    TestTtyOpenClose();
    TestTtySettingBaudRate();
    TestTtySetFraming();
    TestTtySetFlowControl();
    TestTtySetCanonical();
    TestTtySetRaw();

    // restore configuration
    fd = TestTtyOpen();
    if (-1 == tcsetattr(fd, TCSANOW, &portSettings))
    {
        LE_DEBUG("Cannot set port settings");
        exit(EXIT_FAILURE);
    }
    TestTtyClose(fd);

    LE_INFO("======== Tty Test Completed Successfully ========");
    exit(EXIT_SUCCESS);
}
