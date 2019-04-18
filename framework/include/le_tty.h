/**
 * @page c_tty tty API
 *
 * @subpage le_tty.h "API Reference"
 *
 * <HR>
 *
 * This API provides routines to configure serial ports.
 *
 * @section c_tty_open_close Open/Close serial ports
 *
 * - @c le_tty_Open() opens a serial port device and locks it for exclusive use.
 *
 * @code
 * fd = le_tty_Open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
 * @endcode
 *
 * - @c le_tty_Close() closes and unlocks a serial port file descriptor.
 *
 *
 * @section c_tty_settings Settings serial ports
 *
 * - Setting baud rate is done with @c le_tty_SetBaudRate(), value available are listed
 * by #tty_Speed_t
 *
 * - Getting baud rate is done with @c le_tty_GetBaudRate(). when le_tty_SetBaudRate()
 * failed with LE_UNSUPPORTED, use @c le_tty_GetBaudRate() to retrieve the real
 * value sets by the driver.
 *
 * - Setting framing on serial port is done with @c le_tty_SetFraming().
 * Parity value can be :
 *  - "N" for No parity
 *  - "O" for Odd parity
 *  - "E" for Even parity
 *
 * - Setting flow control on serial port is done with @c le_tty_SetFlowControl().
 * Flow control options are:
 *  - LE_TTY_FLOW_CONTROL_NONE - flow control disabled
 *  - LE_TTY_FLOW_CONTROL_XON_XOFF - software flow control (XON/XOFF)
 *  - LE_TTY_FLOW_CONTROL_HARDWARE - hardware flow control (RTS/CTS)
 *
 * - Setting serial port into terminal mode is done with @c le_tty_SetCanonical().
 * it converts EOL characters to unix format, enables local echo, line mode.
 *
 * - Setting serial port into raw (non-canonical) mode is done with @c le_tty_SetRaw().
 * it disables conversion of EOL characters, disables local echo, sets character mode,
 * read timeouts.
 *
 *   Different use cases for numChars and timeout parameters in @c
 * le_tty_SetRaw(int fd,int numChars,int timeout)
 *   - numChars = 0 and timeout = 0: Read will be completetly non-blocking.
 *   - numChars = 0 and timeout > 0: Read will be a pure timed read. If the timer expires without
 * data, zero is returned.
 *   - numChars > 0 and timeout > 0: Read will return when numChar have been transferred to the
 * caller's buffer or when timeout expire between characters.
 *   - numChars > 0 and timeout = 0: Read will return only when exactly numChars have been
 * transferred to the caller's buffer. This can wait and block indefinitely, when
 * read(fd,buffer,nbytes) is performed and that nbytes is smaller than the numChars setted.
 *
 * To switch between 'cannonical' and 'raw' mode, just call @c le_tty_SetCanonical() and
 * @c le_tty_SetRaw() respectively
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_tty.h
 *
 * Legato @ref c_tty include file
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_TTY_H_INCLUDE_GUARD
#define LEGATO_TTY_H_INCLUDE_GUARD

typedef enum
{
    LE_TTY_FLOW_CONTROL_NONE = 0,
    LE_TTY_FLOW_CONTROL_XON_XOFF = 1,
    LE_TTY_FLOW_CONTROL_HARDWARE = 2
}
tty_FlowControl_t;

//--------------------------------------------------------------------------------------------------
/*
 * Use Bxxxxxx constants from termbits.h to indicate baud rate.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_TTY_SPEED_0 = 0,
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
    LE_TTY_SPEED_4000000
}
tty_Speed_t;

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Close and unlock a serial port file descriptor.
 */
//--------------------------------------------------------------------------------------------------
void le_tty_Close
(
    int fd  ///< [IN] File descriptor
);

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
);

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
);

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
);

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
);

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
    int fd  ///< [iIN] File Descriptor
);

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
);

#endif // LEGATO_TTY_H_INCLUDE_GUARD
