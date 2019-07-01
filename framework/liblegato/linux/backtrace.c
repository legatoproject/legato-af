//--------------------------------------------------------------------------------------------------
/** @file backtrace.c
 *
 * Utility function for printing a backtrace.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "signals.h"

#include <execinfo.h>
#include <setjmp.h>
#include <ucontext.h>

#if defined(__arm__)
static const int SKIP_COUNT = 3;
#endif

#if LE_CONFIG_ENABLE_SEGV_HANDLER
//--------------------------------------------------------------------------------------------------
/**
 * Stack, signals mask and context environment for sigsetjmp() and siglongjmp()
 */
//--------------------------------------------------------------------------------------------------
static sigjmp_buf SigEnv;

//--------------------------------------------------------------------------------------------------
/**
 * SEGV handler used to handle a SEGV while executing the ShowStackSignalHandler handler. This will
 * abort the current dump and next dump will be run. This is to prevent the first handler to crash
 * while dumping a crushed stack and backtrace.
 * As sigsetjmp() and siglongjmp() are used, a proctection by counter is added to avoid infinite
 * loop. As two critical parts exists inside the ShowStackSignalHandler, this counter MUST NOT
 * be greater than 2. At the third call, there will be no recover from SEGV.
 */
//--------------------------------------------------------------------------------------------------
static __attribute__((unused)) void SigSegVHandler( int signum )
{
    static uint8_t sigSegVCounter = 0;

    SIG_WRITE("[...]\n", 6);
    if ((2 > (sigSegVCounter++)))
    {
        // Just have 2 tries for calling this handler...
        siglongjmp(SigEnv, 1);
    }
}

#endif // LE_CONFIG_ENABLE_SEGV_HANDLER

static inline void DumpContextStack(const void *infoPtr, int skip, char *buf, size_t bufLen)
{
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    struct sigaction sa, saveSaSegV;
    int ret;

    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))SigSegVHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags =  SA_RESETHAND | SA_NODEFER;
    ret = sigaction( SIGSEGV, &sa, &saveSaSegV);
    if (ret)
    {
        snprintf(buf, bufLen, "sigaction returns %d\n", ret);
        SIG_WRITE(buf, strlen(buf));
    }
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER

#if defined(__arm__)
    const struct sigcontext     *ctxPtr = (const struct sigcontext *)
                                          &(((const ucontext_t *) infoPtr)->uc_mcontext);
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    if (0 == sigsetjmp(SigEnv, 1))
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
    {
        int addr = ctxPtr->arm_pc;
        int* base = (int*)__builtin_frame_address(0);
        int* frame  = base;

        snprintf(buf, bufLen, "PC at %08lx\n",
                 ctxPtr->arm_pc);
        SIG_WRITE(buf, strlen(buf));

        // Here we first get LR from current FP. If FP is illegal, dump LR register instead.
        // This is because the LR register value passed to signal handler is unreliable.
        if ((frame < (int*)0x1000) || (frame > (base + 1024*1024)) || (frame < base))
        {
            snprintf(buf, bufLen, "LR at %08lx\n",
                     ctxPtr->arm_lr );
            SIG_WRITE(buf, strlen(buf));
        }
        else
        {
            for (int i = 0; ;i++)
            {
                // On arm, current frame points to previous LR. The previous frame is stored into
                // the word before PC. We have
                // FP[0] -> LR[1]
                //          FP[1] -> LR[2]
                //                   FP[2] -> ...
                if ((frame < (int*)0x1000) || (frame > (base + 1024*1024)) || (frame < base))
                {
                    // Exit if FP == 0 or FP[n] == 0 or if FP[n] is less than FP[0] or
                    // if FP[0] is outside 1MB
                    break;
                }

                if(i >= SKIP_COUNT)
                {
                    addr = *(frame);
                    snprintf(buf, bufLen, "LR at %08x [%p]\n",
                         addr, frame);
                    SIG_WRITE(buf, strlen(buf));
                }
                frame = (int *)*(frame-1);
            }
        }
    }
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    else
    {
        snprintf(buf, bufLen, "Abort while dumping the backtrace\n");
        SIG_WRITE(buf, strlen(buf));

        // When the code is long jumped back from SigSegVHandler(),
        // register handler again since current handler is already reset to default.
        ret = sigaction( SIGSEGV, &sa, NULL );
        if (ret)
        {
            snprintf(buf, bufLen, "sigaction returns %d\n", ret);
            SIG_WRITE(buf, strlen(buf));
        }
    }

    if (0 == sigsetjmp(SigEnv, 1))
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
    {
        snprintf(buf, bufLen,
                 "r0  %08lx r1  %08lx r2  %08lx r3  %08lx r4  %08lx  r5  %08lx\n",
                 ctxPtr->arm_r0, ctxPtr->arm_r1, ctxPtr->arm_r2,
                 ctxPtr->arm_r3, ctxPtr->arm_r4, ctxPtr->arm_r5);
        SIG_WRITE(buf, strlen(buf));
        snprintf(buf, bufLen,
                 "r6  %08lx r7  %08lx r8  %08lx r9  %08lx r10 %08lx cpsr %08lx\n",
                 ctxPtr->arm_r6, ctxPtr->arm_r7, ctxPtr->arm_r8,
                 ctxPtr->arm_r9, ctxPtr->arm_r10, ctxPtr->arm_cpsr);
        SIG_WRITE(buf, strlen(buf));
        snprintf(buf, bufLen,
                 "fp  %08lx ip  %08lx sp  %08lx lr  %08lx pc  %08lx\n",
                 ctxPtr->arm_fp, ctxPtr->arm_ip, ctxPtr->arm_sp,
                 ctxPtr->arm_lr, ctxPtr->arm_pc);
        SIG_WRITE(buf, strlen(buf));
        snprintf(buf, bufLen,
                 "STACK %08lx, FRAME %08lx\n", ctxPtr->arm_sp, ctxPtr->arm_fp);
        SIG_WRITE(buf, strlen(buf));

        int addr;
        int* frame;
        for (addr = 0, frame = (int*)ctxPtr->arm_sp-32;
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
             addr < 1024;
#else
             addr < 256;
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
             addr += 8, frame += 8)
        {
            snprintf(buf, bufLen,
                     "%08" PRIxPTR ": %08x %08x %08x %08x %08x %08x %08x %08x\n",
                     (uintptr_t) frame,
                     frame[0], frame[1], frame[2], frame[3],
                     frame[4], frame[5], frame[6], frame[7]);
            SIG_WRITE(buf, strlen(buf));
        }
    }
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    else
    {
        snprintf(buf, bufLen, "Abort while dumping the stack and registers\n");
        SIG_WRITE(buf, strlen(buf));
    }

    // All dumps are done, reset to default handler so that coredump
    // can be generated later.
    (void)sigaction( SIGSEGV, &saveSaSegV, NULL );
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
#else //!__arm__
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    if (0 == sigsetjmp(SigEnv, 1))
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
    {
        void *retSp[12];
        size_t nRetSp;
        int n;
        nRetSp = backtrace( &retSp[0], 12 );
        // Skip HandleSignal() and <signal handler called> frames
        for (n = skip; n < nRetSp; n++)
        {
            snprintf(buf, bufLen,
                     "#%d : %p\n", n-skip, (void*)retSp[n]);
            SIG_WRITE(buf, strlen(buf));
        }
    }
#if (defined(LE_CONFIG_ENABLE_SEGV_HANDLER))
    else
    {
        snprintf(buf, bufLen,
                 "Catching SEGV while dumping the backtrace\n");
        SIG_WRITE(buf, strlen(buf));
    }

    // All dumps are done, reset to default handler so that coredump
    // can be generated later.
    (void)sigaction( SIGSEGV, &saveSaSegV, NULL );
#endif //LE_CONFIG_ENABLE_SEGV_HANDLER
#endif //defined(__arm__)
}

void backtrace_DumpContextStack(const void *infoPtr, int skip, char *buf, size_t bufLen)
{
    snprintf(buf, bufLen, "BACKTRACE\n");
    SIG_WRITE(buf, strlen(buf));

    DumpContextStack(infoPtr, skip, buf, bufLen);

    snprintf(buf, bufLen, "DONE\n");
    SIG_WRITE(buf, strlen(buf));
}

#if LE_CONFIG_ENABLE_BACKTRACE

void _le_backtrace(const char *msg)
{
    char        buffer[256];
    ucontext_t  ctx;

    if (getcontext(&ctx) < 0)
    {
        // Do not use LE_ERRNO_TXT() here, as that uses TLS which may not be available
        // in a crash situation.
#if (_POSIX_C_SOURCE >= 200112L) && !  _GNU_SOURCE
        // XSI-compliant strerror_r
        if (strerror_r(errno, buffer, sizeof(buffer)) == 0)
        {
            // OK to return this since it's a static thread-local buffer.
            msg = buffer;
        }
        else
        {
            msg = "Unknown error";
        }
#else
        // GNU strerror_r
        msg = strerror_r(errno, buffer, sizeof(buffer));
#endif
        LE_ERROR("Failed to get context for backtrace: (%d) %s", errno, msg);
        return;
    }

    fprintf(stderr, "%s\n", msg);
    backtrace_DumpContextStack(&ctx, 1, buffer, sizeof(buffer));
}

#endif /* end LE_CONFIG_ENABLE_BACKTRACE */
