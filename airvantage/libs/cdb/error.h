#ifndef ERROR_H
#define ERROR_H

/* On some compilers including recent GCC versions,
 * errno is a macro, not a genuine variable.
 * You have to include <errno.h>. */
// extern int errno;
#include <errno.h>

extern int error_intr;
extern int error_nomem;
extern int error_noent;
extern int error_txtbsy;
extern int error_io;
extern int error_exist;
extern int error_timeout;
extern int error_inprogress;
extern int error_wouldblock;
extern int error_again;
extern int error_pipe;
extern int error_perm;
extern int error_acces;
extern int error_nodevice;
extern int error_proto;

extern char *error_str(int);
extern int error_temp(int);

#endif
