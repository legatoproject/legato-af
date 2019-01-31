//--------------------------------------------------------------------------------------------------
/** @file fd.h
 *
 * Legato File Descriptor inter-module include file.
 *
 * Inter-module interface definitions exported by the File Descriptor module to other
 * modules within the framework.
 *
 * The File Descriptor module is part of the @ref c_fd implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_FD_H_INCLUDE_GUARD
#define LEGATO_FD_H_INCLUDE_GUARD

typedef mode_t le_fd_Mode_t;

#define le_fd_Open         open
#define le_fd_Close        close
#define le_fd_Read         read
#define le_fd_Write        write
#define le_fd_Ioctl        ioctl
#define le_fd_MkFifo(p,m)  mkfifo(p,(mode_t)m)
#define le_fd_Fcntl        fcntl
#define le_fd_Dup          dup

//--------------------------------------------------------------------------------------------------
/** Request codes for le_fd_Ioctl API
 */
//--------------------------------------------------------------------------------------------------
#define LE_FD_FLUSH 0x7F00            // Send AT command to ATSERVER device.

#endif // LEGATO_FD_H_INCLUDE_GUARD
