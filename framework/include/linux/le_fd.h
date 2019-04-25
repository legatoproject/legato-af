//--------------------------------------------------------------------------------------------------
/** @file le_fd.h
 *
 * Legato File Descriptor Linux-specific definitions.
 *
 * For individual function documentation, see the corresponding Linux man pages, as these macros are
 * direct mappings to the underlying calls.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LINUX_LE_FD_H_INCLUDE_GUARD
#define LINUX_LE_FD_H_INCLUDE_GUARD

#define le_fd_Close(fd)                 close(fd)
#define le_fd_Dup(oldfd)                dup(oldfd)
#define le_fd_Fcntl                     fcntl
#define le_fd_Ioctl(fd, request, argp)  ioctl((fd), (request), (argp))
#define le_fd_MkFifo(pathname, mode)    mkfifo((pathname), (mode))
#define le_fd_Open(pathname, flags)     open((pathname), (flags))
#define le_fd_Read(fd, buf, count)      read((fd), (buf), (count))
#define le_fd_Write(fd, buf, count)     write((fd), (buf), (count))

#endif /* end LINUX_LE_FD_H_INCLUDE_GUARD */
