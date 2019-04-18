/**
 * @page c_signals Signals API
 *
 * @subpage le_signals.h "API Reference"
 *
 * <HR>
 *
 * Signals are software interrupts that can be sent to a running process or thread to
 * indicate exceptional situations. The action taken when an event is received depends on the
 * current settings for the signal and may be set to either:
 *
 *  - Operating systems default action.
 *  - Ignore the signal.
 *  - A custom handler.
 *
 * When a signal is received, unless it is ignored or blocked the action for the signal will preempt
 * any code that is currently executing.  Also, signals are asynchronous and may arrive at any time.
 * See http://man7.org/linux/man-pages/man7/signal.7.html for more details.
 *
 * The asynchronous and preemptive nature of signals can be difficult to deal with and is often a
 * source of race conditions.  Moreover asynchronous and preemptive signal handling is often
 * unnecessary so code often looks something like this:
 *
 * @code
 * // A global volatile atomic flag.
 * static volatile int GotSignal;
 *
 * void sigHandler(int sigNum)
 * {
 *      // Must only use asynch-signal-safe functions in this handler, see signals(7) man page for
 *      // more details.
 *
 *      // Set the flag.
 *      GotSignal = 1;
 * }
 *
 * int main (void)
 * {
 *      while(1)
 *      {
 *          // Do something.
 *          ...
 *
 *          if (GotSignal = 1)
 *          {
 *              // Clear the flag.
 *              GotSignal = 0;
 *
 *              // Process the signal.
 *              ...
 *          }
 *      }
 * }
 * @endcode
 *
 * In this code sample, the signal handler is only used to set a flag, while the main loop handles
 * the actual signal processing.  But handling signals this way requires the main loop to run
 * continuously.  This code is also prone to errors. For example, if the clearing of the flags was
 * done after processing of the signal, any signals received during processing of the signals will
 * be lost.
 *
 * @section c_signals_eventHandlers Signal Event Handlers
 *
 * The Legato signals API provides a simpler alternative, called signal events.  Signal events can
 * be used to receive and handle signals synchronously without the need for a sit-and-wait loop or
 * even a block-and-wait call.
 *
 * To use signal events, the desired signals must first be blocked using le_sig_Block() (see
 * @ref c_signals_blocking).  Then set a signal event handler for the desired signal using
 * @c le_sig_SetEventHandler().  Once a signal to the thread is received, the signal event handler
 * is called by the thread's Legato event loop (see @ref c_eventLoop for more details).  The handler
 * is called synchronously in the context of the thread that set the handler.  Be aware that if the
 * thread's event loop is not called or is blocked by some other code, the signal event handler will
 * also be blocked.
 *
 * Here is an example using signal events to handle the SIGCHLD signals:
 *
 * @code
 * // SIGCHILD event handler that will be called as a synchronous event.
 * static void SigChildEventHandler(int sigNum)
 * {
 *      // Handle SIGCHLD event.
 *      ...
 *
 *      // There is no need to limit ourselves to async-signal-safe functions because we are now in
 *      // a synchronous event handler.
 * }
 *
 *
 * COMPONENT_INIT
 * {
 *      // Block Signals that we are going to set event handlers for.
 *      le_sig_Block(SIGCHLD);
 *
 *      // Setup the signal event handler.
 *      le_sig_SetEventHandler(SIGCHILD, SigChildEventHandler);
 * }
 * @endcode
 *
 *
 * @section c_signals_mixedHandlers Mixing Asynchronous Signal Handlers with Synchronous Signal Event Handlers
 *
 * Signal events work well when dealing with signals synchronously, but when signals must be dealt
 * with in an asynchronously, traditional signal handlers are still preferred.  In fact, signal
 * event handlers are not allowed for certain signals like program error signals (ie. SIGFPE,
 * etc.) because they indicate a serious error in the program and all code outside of signal
 * handlers are considered unreliable.  This means that asynchronous signal handlers are the only
 * option when dealing with program error signals.
 *
 * Signal event handlers can be used in conjunction with asynchronous signal handlers but
 * only if they do not deal with the same signals.  In fact all signals that use signal events must
 * be blocked for every thread in the process.  The Legato framework takes care of this for you when
 * you set the signals you want to use in the Legato build system.
 *
 * If your code explicitly unblocks a signal where you currently have signal event handlers, the
 * signal event handlers will no longer be called until the signal is blocked again.
 *
 * @section c_signals_multiThread Multi-Threading Support
 *
 * In a multi-threaded system, signals can be sent to either the process or a specific thread.
 * Signals directed at a specific thread will be received by that thread; signals directed at
 * the process are received by one of the threads in the process that has a handler for the signal.
 *
 * It is unspecified which thread will actually receive the signal so it's recommended
 * to only have one signal event handler per signal.
 *
 * @section c_signals_limitations Limitations and Warnings
 *
 * A limitation of signals in general (not just with signal events) is called signal merging.
 * Signals that are received but not yet handled are said to be pending.  If another signal of the
 * same type is received while the first signal is pending, then the two signals will merge into a
 * single signal and there will be only one handler function call.  Consequently, it is not possible
 * to reliably know how many signals arrived.
 *
 *
 * @warning Signals are difficult to deal with in general because of their asynchronous nature and
 * although, Legato has simplified the situation with signal events certain limitations still exist.
 * If possible, avoid using them.
 *
 * @section c_signals_blocking Blocking signals
 *
 * Signals that are to be used with a signal event handlers must be blocked for the entire process.
 * To ensure this use le_sig_Block() to block signals in the process' first thread.  All other
 * threads will inherit the signal mask from the first thread.
 *
 * The example below shows how to use a signal event in a separate thread.
 *
 * @code
 * // SIGCHILD event handler that will be called as a synchronous event in the context of the
 * // workThread.
 * static void SigChildEventHandler(int sigNum)
 * {
 *      // Handle SIGCHILD event.
 *      ...
 *
 *      // There is no need to limit ourselves to async-signal-safe functions because we are now in
 *      // a synchronous event handler.
 * }
 *
 *
 * // Work thread's main function.
 * static void* WorkThreadMain(void* context)
 * {
 *      // Setup the signal event handler.
 *      le_sig_SetEventHandler(SIGCHILD, SigChildEventHandler);
 *
 *      // Start this thread's event loop.
 *      le_event_RunLoop();
 *
 *      return NULL;
 * }
 *
 *
 * // Main thread code.
 * COMPONENT_INIT
 * {
 *      // Block Signals that we are going to set event handlers for in the main thread so that all
 *      // subsequent threads will inherit the same signal mask.
 *      le_sig_Block(SIGCHLD);
 *
 *      // Create and start a work thread that will actually handle the signal.
 *      le_thread_Ref_t workThread = le_thread_Create("workThread", WorkThread, NULL);
 *
 *      le_thread_Start(workThread);
 * }
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_signals.h
 *
 * Legato @ref c_signals include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SIGNALS_INCLUDE_GUARD
#define LEGATO_SIGNALS_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 *  Prototype for the signal event handler functions.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_sig_EventHandlerFunc_t)
(
    int sigNum      ///< [IN] The signal that was received.
);


//--------------------------------------------------------------------------------------------------
/**
 * Blocks a signal in the calling thread.
 *
 * Signals that an event handler will be set for must be blocked for all threads in the process.  To
 * ensure that the signals are blocked in all threads call this function in the process' first
 * thread, all subsequent threads will inherit the signal mask.
 *
 * @note Does not return on failure.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_Block
(
    int sigNum              ///< [IN] Signal to block.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets a signal event handler for the calling thread.  Each signal can only have a single event
 * handler per thread.  The most recent event handler set will be called when the signal is
 * received.  sigEventHandler can be set to NULL to remove a previously set handler.
 *
 * @note sigNum Cannot be SIGKILL or SIGSTOP or any program error signals: SIGFPE, SIGILL,
 * SIGSEGV, SIGBUS, SIGABRT, SIGIOT, SIGTRAP, SIGEMT, SIGSYS.
 *
 * @note Does not return on failure.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_SetEventHandler
(
    int sigNum,                                 ///< [IN] Signal to set the event handler for.
                                                ///  See parameter documentation in comments above.
    le_sig_EventHandlerFunc_t sigEventHandler   ///< [IN] Event handler to call when a signal is
                                                ///  received.
);


//--------------------------------------------------------------------------------------------------
/**
 * Removes all signal event handlers for the calling thread and cleans up any resources used for
 * signal events.  This should be called before the thread exits.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_DeleteAll
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Install handler to show stack and signal informations on SIGSEGV/ILL/BUS/TRAP/FPE signals.
 * Called automatically by main().
 */
//--------------------------------------------------------------------------------------------------
void le_sig_InstallShowStackHandler
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Install a default handler to handle the SIGTERM signal.
 * Called automatically by main().
 */
//--------------------------------------------------------------------------------------------------
void le_sig_InstallDefaultTermHandler
(
    void
);


#endif // LEGATO_SIGNALS_INCLUDE_GUARD
