/**
 * strerror.c returns standard error codes or legato error codes string
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "strerror.h"

#define ERR_MSG_LEN 128 /* error message max length */

char *le_strerror
(
    int err
)
{
    static char errMsg[ERR_MSG_LEN];

    /* list of legato error messages */
    const char* le_errMsg[] =
    {
        [0] = "Successful.",
        [1] = "Referenced item does not exist.",
        [2] = "LE_NOT_POSSIBLE",
        [3] = "Value out of range.",
        [4] = "Out of memory.",
        [5] = "Current user does not have permission"
              " to perform requested action.",
        [6] = "Unspecified internal error.",
        [7] = "Communications error.",
        [8] = "A time-out occurred.",
        [9] = "An overflow occurred.",
        [10] = "An underflow occurred.",
        [11] = "Would have blocked if non-blocking"
               " behaviour was not requested.",
        [12] = "Would have caused a deadlock.",
        [13] = "Format error.",
        [14] = "Duplicate entry found.",
        [15] = "Parameter is invalid.",
        [16] = "The resource is closed.",
        [17] = "The resource is busy.",
        [18] = "The underlying resource does not support this operation.",
        [19] = "An IO operation failed.",
        [20] = "Unimplemented functionality.",
        [21] = "Temporary loss of a service or resource.",
        [22] = "The process, operation, data stream,"
               " session, etc. has stopped.",
    };

    memset(errMsg, 0, ERR_MSG_LEN);

    if (err < 0)
    {
        snprintf(errMsg, ERR_MSG_LEN, "%s", le_errMsg[-err]);
        return errMsg;
    }

#ifdef __USE_GNU
    snprintf(errMsg, ERR_MSG_LEN, "%s", strerror_r(err, errMsg, ERR_MSG_LEN));
#else /* XSI-compliant */
    strerror_r(err, errMsg, ERR_MSG_LEN);
#endif

    return errMsg;
}
