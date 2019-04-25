/**
 * @page c_fd File Descriptor API
 *
 * @subpage le_fd.h "API Reference"
 *
 * <HR>
 *
 * The File Descriptor (FD) service is intended as a helper to access file system resources.
 *
 * @section fd_dtr_api Available API
 *
 * It offers the following file operations:
 * - open a resource with @c le_fd_Open(),
 * - close a resource with @c le_fd_Close(),
 * - read from a resource with @c le_fd_Read(),
 * - write to a resource with @c le_fd_Write(),
 * - manage I/O with @c le_fd_Ioctl(),
 * - duplicate a file descriptor with @c le_fd_Dup(),
 * - create a FIFO with @c le_fd_MkFifo(),
 * - manipulate a file descriptor with @c le_fd_Fcntl().
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/** @file le_fd.h
 *
 * Legato @ref c_fd include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_FD_H_INCLUDE_GUARD
#define LEGATO_FD_H_INCLUDE_GUARD

#if !defined(le_fd_Close)   && \
    !defined(le_fd_Dup)     && \
    !defined(le_fd_Fcntl)   && \
    !defined(le_fd_Ioctl)   && \
    !defined(le_fd_MkFifo)  && \
    !defined(le_fd_Open)    && \
    !defined(le_fd_Read)    && \
    !defined(le_fd_Write)

//--------------------------------------------------------------------------------------------------
/**
 * Create or open an existing resource.
 *
 * @return
 *  The new file descriptor, or -1 if an error occurred.
 */
//--------------------------------------------------------------------------------------------------
int le_fd_Open
(
    const char* pathName,             ///< [IN] Pathname to the resource
    int flags                         ///< [IN] Resource access flags
);

//--------------------------------------------------------------------------------------------------
/**
 * Close a resource referred to by the file descriptor.
 *
 * @return
 *  Zero on success, or -1 if an error occurred.
 */
//--------------------------------------------------------------------------------------------------
int le_fd_Close
(
    int fd                            ///< [IN] File descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * Attempts to read up to count bytes from the resource referred to by the file descriptor.
 *
 * The data is read at the current offset.
 *
 * @return
 *  On success, the number of bytes read is returned, otherwise -1 is returned.
 */
//--------------------------------------------------------------------------------------------------
ssize_t le_fd_Read
(
    int fd,                           ///< [IN] File descriptor
    void* bufPtr,                     ///< [OUT] Buffer to store read data into
    size_t count                      ///< [IN] Number of bytes to read
);

//--------------------------------------------------------------------------------------------------
/**
 * Write up to count bytes to the resource referred to by the file descriptor.
 *
 * The data is written at the current offset.
 *
 * @return
 *  On success, the number of bytes written is returned, otherwise -1 is returned.
 */
//--------------------------------------------------------------------------------------------------
ssize_t le_fd_Write
(
    int fd,                           ///< [IN] File descriptor
    const void* bufPtr,               ///< [IN] Buffer containing data to be written
    size_t count                      ///< [IN] Number of bytes to write
);

//--------------------------------------------------------------------------------------------------
/**
 * Send a request to the resource referred to by the file descriptor.
 *
 * @return
 *  Zero on success, or -1 if an error occurred.
 */
//--------------------------------------------------------------------------------------------------
int le_fd_Ioctl
(
    int fd,                           ///< [IN] File descriptor
    int request,                      ///< [IN] Device dependent request code
    void* bufPtr                      ///< [INOUT] Pointer to request dependent data buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Make a FIFO.
 *
 * @return
 *  Zero on success, or -1 if an error occurred and errno is set.
 */
//--------------------------------------------------------------------------------------------------
int le_fd_MkFifo
(
    const char *pathname,       ///< [IN] pathname of the fifo
    mode_t mode                 ///< [IN] permissions of the file
);

//--------------------------------------------------------------------------------------------------
/**
 * Create a copy of the file descriptor.
 *
 * @return
 *  On success return the new descriptor, or -1 if an error occurred and errno is set.
 */
//--------------------------------------------------------------------------------------------------
int le_fd_Dup
(
    int oldfd                         ///< [IN] File descriptor
);

//--------------------------------------------------------------------------------------------------
/**
 * Manipulate a file descriptor.
 *
 * Implements a subset of the commands supported by fcntl(2).
 *
 * The following subcommands are guaranteed to be implemented on all platforms:
 *    - F_GETFL
 *    - F_SETFL
 */
//--------------------------------------------------------------------------------------------------
int le_fd_Fcntl
(
    int fd,                       ///< [IN] File descriptor
    int cmd,                      ///< [IN] Command
    ... /* arg */                 ///< [IN] Argument (optional)
);

#else /* le_fd macros defined */

// If any le_fd macro is overridden, all le_fd macros must be overridden.

#if !defined(le_fd_Open)
#  error "File descriptor macros are overridden, but le_fd_Open not defined.  Please define it."
#endif
#if !defined(le_fd_Close)
#  error "File descriptor macros are overridden, but le_fd_Close not defined.  Please define it."
#endif
#if !defined(le_fd_Read)
#  error "File descriptor macros are overridden, but le_fd_Read not defined.  Please define it."
#endif
#if !defined(le_fd_Write)
#  error "File descriptor macros are overridden, but le_fd_Write not defined.  Please define it."
#endif
#if !defined(le_fd_Ioctl)
#  error "File descriptor macros are overridden, but le_fd_Ioctl not defined.  Please define it."
#endif
#if !defined(le_fd_MkFifo)
#  error "File descriptor macros are overridden, but le_fd_MkFifo not defined.  Please define it."
#endif
#if !defined(le_fd_Fcntl)
#  error "File descriptor macros are overridden, but le_fd_Fcntl not defined.  Please define it."
#endif
#if !defined(le_fd_Dup)
#  error "File descriptor macros are overridden, but le_fd_Dup not defined.  Please define it."
#endif

#endif /* end le_fd macros defined */

#endif /* end LEGATO_FD_H_INCLUDE_GUARD */
