/**
 * strerror.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#ifndef _STRERROR_H
#define _STRERROR_H

char *le_strerror(int err);

#define strerror le_strerror

#endif /* le_strerror.h */
