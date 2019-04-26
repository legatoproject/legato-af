/**
 * @file legato.h
 *
 * This file includes all the commonly-used Linux-specific header files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LINUX_LEGATO_H_INCLUDE_GUARD
#define LINUX_LEGATO_H_INCLUDE_GUARD

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include <dirent.h>
#include <fcntl.h>
#include <fts.h>
#include <grp.h>
#include <libgen.h>
#include <mntent.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <syslog.h>
#include <unistd.h>

// Local includes -- definitions which are different between Linux & RTOS
#include "le_fd.h"
#include "le_fs.h"
#include "le_thread.h"

#endif // LINUX_LEGATO_H_INCLUDE_GUARD
