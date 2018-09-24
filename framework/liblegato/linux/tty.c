//--------------------------------------------------------------------------------------------------
/**
 * @file tty.c
 *
 * Routines for dealing with serial ports. Port configuration, simple reads and writes are handled
 * here.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "smack.h"
#include "fileDescriptor.h"
#include "le_tty.h"
#include <termios.h>

// ==============================================
//  PRIVATE DATA
// ==============================================

/// Number of array members.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

/// Flags to enable local echo in termios struct.
#define ECHO_FLAGS (ECHO | ECHOE | ECHOK | ECHONL)

static tty_Speed_t SpeedTable[] =
{
    B0, // LE_TTY_SPEED_0
    B50, // LE_TTY_SPEED_50
    B75, // LE_TTY_SPEED_75
    B110, // LE_TTY_SPEED_110
    B134, // LE_TTY_SPEED_134
    B150, // LE_TTY_SPEED_150
    B200, // LE_TTY_SPEED_200
    B300, // LE_TTY_SPEED_300
    B600, // LE_TTY_SPEED_600
    B1200, // LE_TTY_SPEED_1200
    B1800, // LE_TTY_SPEED_1800
    B2400, // LE_TTY_SPEED_2400
    B4800, // LE_TTY_SPEED_4800
    B9600, // LE_TTY_SPEED_9600
    B19200, // LE_TTY_SPEED_19200
    B38400, // LE_TTY_SPEED_38400
    B57600, // LE_TTY_SPEED_57600
    B115200, // LE_TTY_SPEED_115200
    B230400, // LE_TTY_SPEED_230400
    B460800, // LE_TTY_SPEED_460800
    B500000, // LE_TTY_SPEED_500000
    B576000, // LE_TTY_SPEED_576000
    B921600, // LE_TTY_SPEED_921600
    B1000000, // LE_TTY_SPEED_1000000
    B1152000, // LE_TTY_SPEED_1152000
    B1500000, // LE_TTY_SPEED_1500000
    B2000000, // LE_TTY_SPEED_2000000
    B2500000, // LE_TTY_SPEED_2500000
    B3000000, // LE_TTY_SPEED_3000000
    B3500000, // LE_TTY_SPEED_3500000
    B4000000 // LE_TTY_SPEED_4000000
};

//--------------------------------------------------------------------------------------------------
/**
 * Set parity bits in termios struct.
 * Uses chars such as 'N', 'O' and 'E' to indicate no-parity, odd-parity or even-parity,
 * respectively.
 *
 * @return
 *  - LE_OK if succeed
 *  - LE_BAD_PARAMETER if parameter is not supported
 *
 **/
//--------------------------------------------------------------------------------------------------
static inline le_result_t SetParity
(
    struct termios *portSettings,   ///< [IN/OUT] termios settings
    char ttyParity                  ///< [IN] parity bits
)
{
    le_result_t result = LE_OK;
    // Clear settings.
    portSettings->c_cflag &= ~(PARENB | PARODD);
    portSettings->c_iflag &= ~(INPCK | ISTRIP);
    switch (ttyParity)
    {
    case 'N':
    case 'n':
        // No parity - nothing to set.
        break;

    case 'O':
    case 'o':
        // Odd parity.
        portSettings->c_cflag |= (PARENB | PARODD);
        portSettings->c_iflag |= INPCK;
        break;

    case 'E':
    case 'e':
        // Even parity.
        portSettings->c_cflag |= PARENB;
        portSettings->c_iflag |= INPCK;
        break;

    default:
        LE_ERROR("Unexpected parity setting (%c).", ttyParity);
        result = LE_BAD_PARAMETER;
        break;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the word size bits in termios struct. Use simple integer to indicate word size.
 *
 * @return
 *  - LE_OK if succeed
 *  - LE_BAD_PARAMETER if parameter is not supported
 **/
//--------------------------------------------------------------------------------------------------
static inline le_result_t SetWordSize
(
    struct termios *portSettings,   ///< [IN/OUT] termios settings
    int wordSize                    ///< [IN] data bits size
)
{
    le_result_t result = LE_OK;
    portSettings->c_cflag &= ~CSIZE;
    switch (wordSize)
    {
    case 5:
        break;

    case 6:
        portSettings->c_cflag |= CS6;
        break;

    case 7:
        portSettings->c_cflag |= CS7;
        break;

    case 8:
        portSettings->c_cflag |= CS8;
        break;

    default:
        LE_ERROR("Unexpected char size (%d).", wordSize);
        result = LE_BAD_PARAMETER;
        break;
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the stop bit mask in termios struct. Only 1 and 2 stop bit options are supported in termios.
 *
 * @return
 *  - LE_OK if succeed
 *  - LE_BAD_PARAMETER if parameter is not supported
 **/
//--------------------------------------------------------------------------------------------------
static inline le_result_t SetStopBits
(
    struct termios *portSettings,   ///< [IN/OUT] termios settings
    int stopBits                    ///< [IN] stop bits {1,2}
)
{
    if ((1 > stopBits) || (2 < stopBits)) {
        LE_ERROR("Unexpected stop bits (%d).", stopBits);
        return LE_BAD_PARAMETER;
    }

    portSettings->c_cflag |= (2 == stopBits ? CSTOPB : 0);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Translate system baud rate into legato speed.
 *
 * @return
 *  - LE_OK if succeed
 *  - LE_NOT_FOUND if there is no correspondance
 **/
//--------------------------------------------------------------------------------------------------
static inline le_result_t ConvertBaudIntoSpeed
(
    int baud,               ///< [in] baud rate
    tty_Speed_t *speedPtr   ///< [out] legato speed
)
{
    int i = 0;

    for (i=0; i<ARRAY_SIZE(SpeedTable); i++) {
        if (baud == SpeedTable[i]) {
            *speedPtr = i;
            return LE_OK;
        }
    }
    return LE_NOT_FOUND;
}

// ==============================================
//  PUBLIC API FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Open a serial port device and locks it for exclusive use.
 *
 * @return
 *  - Serial port file descriptor number on success.
 *  - -1 on failure.
 */
//--------------------------------------------------------------------------------------------------
int le_tty_Open
(
    const char *ttyDev, ///< [IN] Path name
    int flags           ///< [IN] flags used in open(2)
)
{
    int fd;
    struct stat ttyStatus;
    struct flock ttyLock = {0};

    // Open path and check if it is a serial device
    fd = open(ttyDev, flags);
    LE_FATAL_IF(0 > fd, "Error opening serial device '%s': %m", ttyDev);

    LE_FATAL_IF(0 > fstat(fd, &ttyStatus),
                "Error checking status of serial device '%s': %m", ttyDev);

    LE_FATAL_IF(!S_ISCHR(ttyStatus.st_mode), "Error: '%s' is not a character device.", ttyDev);

    // Place a lock on the serial device
    ttyLock.l_type = F_WRLCK;
    ttyLock.l_whence = SEEK_SET;
    if (0 > fcntl(fd, F_SETLK, &ttyLock)) {
        LE_ERROR("Error: '%s' locked by process %d: %m.", ttyDev, ttyLock.l_pid);
        fd_Close(fd);
        return -1;
    }

    LE_DEBUG("Serial device '%s' acquired by pid %d.", ttyDev, getpid());
    return fd;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close and unlock a serial port file descriptor.
 */
//--------------------------------------------------------------------------------------------------
void le_tty_Close
(
    int fd  ///< [IN] File descriptor
)
{
   // Just close the descriptor; it would release any locks held.
   fd_Close(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set baud rate of serial port.
 *
* @return
 *  - LE_OK if successful
 *  - LE_UNSUPPORTED if value cannot be set
 *  - LE_NOT_FOUND if value is not supported
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_SetBaudRate
(
    int fd,                 ///< [IN] File Descriptor
    tty_Speed_t ttyRate     ///< [IN] Baud rate
)
{
    struct termios portSettings;

    if (ttyRate >= ARRAY_SIZE(SpeedTable)) {
        LE_ERROR(" %d speed rate in not permitted ", ttyRate);
        return LE_NOT_FOUND;
    }

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    // Assume full-duplex, symmetrical.
    if (-1 == cfsetospeed(&portSettings, SpeedTable[ttyRate])) {
        LE_ERROR("Cannot set ospeed");
        return LE_FAULT;
    }
    if (-1 == cfsetispeed(&portSettings, SpeedTable[ttyRate])) {
        LE_ERROR("Cannot set ispeed");
        return LE_FAULT;
    }

    if (-1 == tcsetattr(fd, TCSANOW, &portSettings)) {
        LE_ERROR("Cannot set port settings");
        return LE_FAULT;
    }
    if (-1 == tcflush(fd, TCIOFLUSH)) {
        LE_ERROR("Cannot flush termios");
        return LE_FAULT;
    }

    // Test if value is supported
    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if ( (SpeedTable[ttyRate] != cfgetispeed(&portSettings))
            ||
         (SpeedTable[ttyRate] != cfgetospeed(&portSettings))
       ) {
        LE_ERROR("Speed rate was not setted, %d/%d not supported",ttyRate,SpeedTable[ttyRate]);
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get baud rate of serial port.
 *
 * @return
 *  - LE_OK if successful
 =  - LE_NOT_FOUND if speed is not supported by legato
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_GetBaudRate
(
    int fd,                     ///< [IN] File Descriptor
    tty_Speed_t *ttyInRatePtr,  ///< [OUT] input baud rate
    tty_Speed_t *ttyOutRatePtr  ///< [OUT] output baud rate
)
{
    le_result_t result;
    struct termios portSettings;

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    result = ConvertBaudIntoSpeed(cfgetispeed(&portSettings),ttyInRatePtr);
    if (LE_OK != result) {
        LE_ERROR("Cannot retrieve/convert ispeed");
        return result;
    }

    result = ConvertBaudIntoSpeed(cfgetispeed(&portSettings),ttyOutRatePtr);
    if (LE_OK != result) {
        LE_ERROR("Cannot retrieve/convert ospeed");
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set framing on serial port. Use human-readable characters/numbers such as 'N', 8, 1 to indicate
 * parity, word size and stop bit settings.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_UNSUPPORTED if value cannot be set
 *  - LE_NOT_FOUND if value is not supported
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_SetFraming
(
    int fd,         ///< [IN] File Descriptor
    char parity,    ///< [IN] Parity ('N','O','E')
    int wordSize,   ///< [IN] Data bits (5,6,7,8)
    int stopBits    ///< [IN] Stop bits (1,2)
)
{
    struct termios portSettings, portSettingsSetted;

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if (LE_OK != SetParity(&portSettings, parity)) {
        return LE_NOT_FOUND;
    }

    if (LE_OK != SetWordSize(&portSettings, wordSize)) {
        return LE_NOT_FOUND;
    }

    if (LE_OK != SetStopBits(&portSettings, stopBits)) {
       return LE_NOT_FOUND;
    }

    if (-1 == tcsetattr(fd, TCSANOW, &portSettings)) {
        LE_ERROR("Cannot set port settings");
        return LE_FAULT;
    }
    if (-1 == tcflush(fd, TCIOFLUSH)) {
        LE_ERROR("Cannot flush termios");
        return LE_FAULT;
    }

    // Test if value is supported
    if (-1 == tcgetattr(fd, &portSettingsSetted)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if ( (portSettings.c_cflag != portSettingsSetted.c_cflag)
        ||
         (portSettings.c_iflag != portSettingsSetted.c_iflag)
       ) {
        LE_ERROR("Could not set framing, parity '%c' data bits '%d' stop bits '%d' not supported",
                 parity, wordSize, stopBits);
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set flow control option on serial port. Flow control options are:
 *    LE_TTY_FLOW_CONTROL_NONE - flow control disabled
 *    LE_TTY_FLOW_CONTROL_XON_XOFF - software flow control (XON/XOFF)
 *    LE_TTY_FLOW_CONTROL_HARDWARE - hardware flow control (RTS/CTS)
 *
 * @return
 *  - LE_OK if successful
 *  - LE_UNSUPPORTED if value cannot be set
 *  - LE_NOT_FOUND if value is not supported
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_SetFlowControl
(
    int fd,                             ///< [IN] File decriptor
    tty_FlowControl_t ttyFlowControl    ///< [IN] Flow control option
)
{
    struct termios portSettings, portSettingsSetted;

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }
    // Clear flow control settings.
    portSettings.c_cflag &= ~CRTSCTS;
    portSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
    switch (ttyFlowControl)
    {
    case LE_TTY_FLOW_CONTROL_NONE:
        // No flow control, nothing to set.
        break;

    case LE_TTY_FLOW_CONTROL_XON_XOFF:
        // XON/XOFF flow control.
        portSettings.c_iflag |= (IXON | IXOFF | IXANY);
        break;

    case LE_TTY_FLOW_CONTROL_HARDWARE:
        // Hardware (RTS/CTS) flow control.
        portSettings.c_cflag |= CRTSCTS;
        break;

    default:
        LE_ERROR("Unexpected flow control option (%d) for descriptor %d.", ttyFlowControl, fd);
        return LE_NOT_FOUND;
    }

    if (-1 == tcsetattr(fd, TCSANOW, &portSettings)) {
        LE_ERROR("Cannot set port settings");
        return LE_FAULT;
    }
    if (-1 == tcflush(fd, TCIOFLUSH)) {
        LE_ERROR("Cannot flush termios");
        return LE_FAULT;
    }

    // Test if value is supported
    if (-1 == tcgetattr(fd, &portSettingsSetted)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if ( (portSettings.c_cflag != portSettingsSetted.c_cflag)
        ||
         (portSettings.c_iflag != portSettingsSetted.c_iflag)
       ) {
        LE_ERROR("Could not set FlowControl, %d not supported",ttyFlowControl);
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set serial port into terminal mode. Converts EOL characters to unix format, enables local echo,
 * line mode, etc.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_UNSUPPORTED if canonical mode cannot be set
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_SetCanonical
(
    int fd  ///< [IN] File Descriptor
)
{
    struct termios portSettings, portSettingsSetted;

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    // Strip <CR> from <CR><LF> on input, enable break condition.
    portSettings.c_iflag &= ~(INLCR | ICRNL);
    portSettings.c_iflag |= (BRKINT | IGNCR);

    // Enable canonical, enable local echo.
    portSettings.c_lflag |= (ICANON | IEXTEN | ISIG | ECHO_FLAGS);

    // Enable post-processed output.
    portSettings.c_oflag |= OPOST;

    if (-1 == tcsetattr(fd, TCSANOW, &portSettings)) {
        LE_ERROR("Cannot set port settings");
        return LE_FAULT;
    }
    if (-1 == tcflush(fd, TCIOFLUSH)) {
        LE_ERROR("Cannot flush termios");
        return LE_FAULT;
    }

    // Test if value is supported
    if (-1 == tcgetattr(fd, &portSettingsSetted)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if ( (portSettings.c_iflag != portSettingsSetted.c_iflag)
        ||
         (portSettings.c_lflag != portSettingsSetted.c_lflag)
        ||
         (portSettings.c_oflag != portSettingsSetted.c_oflag)
       ) {
        LE_ERROR("Could not set canonical, mode not supported");
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters, disables
 * local echo, sets character mode, read timeouts, etc.
 *
 * @return
 *  - LE_OK if successful
 *  - LE_UNSUPPORTED if raw mode cannot be set
 *  - LE_FAULT for any other error
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_tty_SetRaw
(
    int fd,         ///< [IN] File Descriptor
    int numChars,   ///< [IN] Number of bytes returned by read(2) when a read is performed.
    int timeout     ///< [IN] when a read(2) is performed return after that timeout.
                    ///<      The timeout value is given with 1 decimal places.
)
{
    struct termios portSettings, portSettingsSetted;

    if (-1 == tcgetattr(fd, &portSettings)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    // Disable break and EOL character conversions.
    portSettings.c_iflag &= ~(BRKINT | IGNCR | INLCR | ICRNL);

    // Disable canonical, signal handling and local echo.
    portSettings.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO_FLAGS);

    // Disable post-processing and character conversion.
    portSettings.c_oflag &= ~(OPOST | ONLCR | OCRNL);

    // Set read timeout.
    portSettings.c_cc[VMIN] = numChars;
    portSettings.c_cc[VTIME] = timeout;

    if (-1 == tcsetattr(fd, TCSANOW, &portSettings)) {
        LE_ERROR("Cannot set port settings");
        return LE_FAULT;
    }
    if (-1 == tcflush(fd, TCIOFLUSH)) {
        LE_ERROR("Cannot flush termios");
        return LE_FAULT;
    }

    // Test if value is supported
    if (-1 == tcgetattr(fd, &portSettingsSetted)) {
        LE_ERROR("Cannot retrieve port settings");
        return LE_FAULT;
    }

    if ( (portSettings.c_iflag != portSettingsSetted.c_iflag)
        ||
         (portSettings.c_lflag != portSettingsSetted.c_lflag)
        ||
         (portSettings.c_oflag != portSettingsSetted.c_oflag)
        ||
         (portSettings.c_cc[VMIN] != portSettingsSetted.c_cc[VMIN])
        ||
         (portSettings.c_cc[VTIME] != portSettingsSetted.c_cc[VTIME])
       ) {
        LE_ERROR("Could not set raw, mode not supported");
        return LE_UNSUPPORTED;
    }

    return LE_OK;
}
