//--------------------------------------------------------------------------------------------------
/** @file le_fd_linux.h
 *
 * Legato File Descriptor Linux-specific definitions.
 *
 * For individual function documentation, see the corresponding Linux man pages, as these macros are
 * direct mappings to the underlying calls.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_FD_LINUX_H_INCLUDE_GUARD
#define LEGATO_FD_LINUX_H_INCLUDE_GUARD

#define le_fd_Close(fd)                   close(fd)
#define le_fd_Dup(oldfd)                  dup(oldfd)
#define le_fd_Fcntl                       fcntl
#define le_fd_Ioctl(fd, request, argp)    ioctl((fd), (request), (argp))
#define le_fd_Fstat(fd, buf)              fstat((fd), (buf))
#define le_fd_MkFifo(pathname, mode)      mkfifo((pathname), (mode))
#define le_fd_MkPipe(pathname, mode)      (-1)
#define le_fd_Open(pathname, flags, ...)  open((pathname), (flags), ##__VA_ARGS__)
#define le_fd_Read(fd, buf, count)        read((fd), (buf), (count))
#define le_fd_Write(fd, buf, count)       write((fd), (buf), (count))

#endif /* end LEGATO_FD_LINUX_H_INCLUDE_GUARD */
