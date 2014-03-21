/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Gilles Cannenterre for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/*
 * lposixsignal.c
 *
 *  Created on: 21 fev 2012
 *      Author: Gilles
 */

#include "lposixsignal.h"

//#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>


#define MAX_SIGNALS     64

typedef int t_socket;
typedef t_socket *p_socket;

/* timeout control structure */
typedef struct t_timeout_ {
    double block;          /* maximum time for blocking calls */
    double total;          /* total number of miliseconds for operation */
    double start;          /* time of start of operation */
} t_timeout;
typedef t_timeout *p_timeout;

#define SOCKET_INVALID (-1)

/*
 SIGABRT        -    Process abort signal.
 SIGALRM        -    Alarm clock.
 SIGFPE            -    Erroneous arithmetic operation.
 SIGHUP            -    Hangup.
 SIGILL            -    Illegal instruction.
 SIGINT            -    Terminal interrupt signal.
 SIGPIPE        -    Write on a pipe with no one to read it.
 SIGQUIT        -    Terminal quit signal.
 SIGSEGV        -    Invalid memory reference.
 SIGTERM        -    Termination signal.
 SIGUSR1        -    User-defined signal 1.
 SIGUSR2        -    User-defined signal 2.
 SIGCHLD        -    Child process terminated or stopped.
 SIGCONT        -    Continue executing, if stopped.
 SIGTSTP        -    Terminal stop signal.
 SIGTTIN        -    Background process attempting read.
 SIGTTOU        -    Background process attempting write.
 SIGBUS            -    Access to an undefined portion of a memory object.
 SIGPOLL        -    Pollable event.
 SIGPROF        -    Profiling timer expired.
 SIGSYS            -    Bad system call.
 SIGTRAP        -    Trace/breakpoint trap.
 SIGURG            -    High bandwidth data is available at a socket.
 SIGVTALRM        -    Virtual timer expired.
 SIGXCPU        -    CPU time limit exceeded.
 SIGXFSZ        -    File size limit exceeded.
 */

typedef struct PosixSignal_ {
    char* name;
    int sig;
} PosixSignal;

static const PosixSignal posixSignals[] = { { "0", 0 },
#ifdef SIGABRT
        {   "SIGABRT", SIGABRT},
#endif
#ifdef SIGALRM
        {   "SIGALRM", SIGALRM},
#endif
#ifdef SIGFPE
        {   "SIGFPE", SIGFPE},
#endif
#ifdef SIGHUP
        {   "SIGHUP", SIGHUP},
#endif
#ifdef SIGILL
        {   "SIGILL", SIGILL},
#endif
#ifdef SIGINT
        {   "SIGINT", SIGINT},
#endif
#ifdef SIGPIPE
        {   "SIGPIPE", SIGPIPE},
#endif
#ifdef SIGQUIT
        {   "SIGQUIT", SIGQUIT},
#endif
#ifdef SIGSEGV
        {   "SIGSEGV", SIGSEGV},
#endif
#ifdef SIGTERM
        {   "SIGTERM", SIGTERM},
#endif
#ifdef SIGUSR1
        {   "SIGUSR1", SIGUSR1},
#endif
#ifdef SIGUSR2
        {   "SIGUSR2", SIGUSR2},
#endif
#ifdef SIGCHLD
        {   "SIGCHLD", SIGCHLD},
#endif
#ifdef SIGCONT
        {   "SIGCONT", SIGCONT},
#endif
#ifdef SIGTSTP
        {   "SIGTSTP", SIGTSTP},
#endif
#ifdef SIGTTIN
        {   "SIGTTIN", SIGTTIN},
#endif
#ifdef SIGTTOU
        {   "SIGTTOU", SIGTTOU},
#endif
#ifdef SIGBUS
        {   "SIGBUS", SIGBUS},
#endif
#ifdef SIGPOLL
        {   "SIGPOLL", SIGPOLL},
#endif
#ifdef SIGPROF
        {   "SIGPROF", SIGPROF},
#endif
#ifdef SIGSYS
        {   "SIGSYS", SIGSYS},
#endif
#ifdef SIGTRAP
        {   "SIGTRAP", SIGTRAP},
#endif
#ifdef SIGURG
        {   "SIGURG", SIGURG},
#endif
#ifdef SIGALRM
        {   "SIGALRM", SIGALRM},
#endif
#ifdef SIGXCPU
        {   "SIGXCPU", SIGXCPU},
#endif
#ifdef SIGXFSZ
        {   "SIGXFSZ", SIGXFSZ},
#endif
        { NULL, 0 } };

typedef struct SignalBuffer_ {
    int lastI;
    int nSig;
    int sigs[MAX_SIGNALS];
} SignalBuffer;

static SignalBuffer sigBuffer = {-1, 0, {0}};

static struct sigaction sa;

static void signal_handler(int sig) {
    sigBuffer.lastI = (sigBuffer.lastI + 1) % MAX_SIGNALS;
    sigBuffer.sigs[sigBuffer.lastI] = sig;
    sigBuffer.nSig = ((sigBuffer.nSig + 1) >= MAX_SIGNALS) ? MAX_SIGNALS : (sigBuffer.nSig + 1);
    //printf("from C '%d/%d'->'%d'\n", sigBuffer.lastI, sigBuffer.nSig, sig);
}

static int retrieve_signal_sig(lua_State* L, int index) {
    lua_getglobal(L, "sched");
    lua_getfield(L, -1, "posixsignal");
    lua_getfield(L, -1, "__sigdef");

    lua_pushvalue(L, index);
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {

        if (lua_isnumber(L, index))
            return lua_tointeger(L, index);

        if (lua_isnumber(L, -1))
            return lua_tointeger(L, -1);

    }
    return luaL_error(L, "cannot retrieve signal number: unknown signal\n");
}

/**
 * signal
 *  sig: signal number or name
 *  [true]: register to sig
 * returns:
 *  "ok" on success
 *  nil, <error> on failure.
 */
static int l_signal(lua_State* L) {
    int params = lua_gettop(L);
    int sig = retrieve_signal_sig(L, 1);

    if (params == 1) {
        sa.sa_handler = SIG_IGN;
    } else {
        sa.sa_handler = signal_handler;
    }
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, NULL) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushstring(L, "ok");
    return 1;
}

/**
 * raise
 *  sig: signal number or name
 * returns:
 *  "ok" on success
 *  nil, <error> on failure.
 */
static int l_raise(lua_State* L) {
    int sig = retrieve_signal_sig(L, 1);
    if (raise(sig) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushstring(L, "ok");
    return 1;
}

/**
 * kill
 *  pid: process pid number
 *  sig: signal number or name
 * returns:
 *  "ok" on success
 *  nil, <error> on failure.
 */
static int l_kill(lua_State* L) {
    int pid = luaL_checkint(L, 1);
    int sig = retrieve_signal_sig(L, 2);
    if (kill((pid_t) pid, sig) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushstring(L, "ok");
    return 1;
}

static int step(lua_State* L) {
    if (sigBuffer.nSig == 0)
        return 0;

    int top = lua_gettop(L);
    lua_getglobal(L, "sched"); // sched
    lua_getfield(L, -1, "posixsignal"); //sched, posixsignal
    lua_getfield(L, -1, "__sigdef"); // sched, posixsignal, __sigdef
    lua_getfield(L, -3, "signal"); // sched, posixsignal, __sigdef, signal

    int tmpI = ((sigBuffer.lastI - sigBuffer.nSig + 1) < 0) ? (MAX_SIGNALS + (sigBuffer.lastI - sigBuffer.nSig + 1)) : (sigBuffer.lastI - sigBuffer.nSig + 1);
    //printf("from LUA (step) (%d) '%d'->'%d'\n", sigBuffer.nSig, tmpI, sigBuffer.lastI);
    while (sigBuffer.nSig > 0) {
        //printf("  + '%d'='%d'\n", tmpI, sigBuffer.sigs[tmpI]);
        lua_pushvalue(L, -1);
        lua_pushstring(L, "posixsignal"); // sched, posixsignal, __sigdef, signal, posixsignal
        lua_pushinteger(L, sigBuffer.sigs[tmpI]); // sched, posixsignal, __sigdef, signal, posixsignal, sigs[i]
        lua_gettable(L, -5); // sched, posixsignal, __sigdef, signal, posixsignal, lastSignalName
        lua_call(L, 2, 0);
        sigBuffer.nSig--;
        tmpI = (tmpI + 1) % MAX_SIGNALS;
    }
    //printf("from LUA (step) exit\n");
    lua_settop(L, top);
    return 1;
}

/* LUA SOCKET EXPORT*/
static t_socket getfd(lua_State *L) {
    t_socket fd = SOCKET_INVALID;
    lua_pushstring(L, "getfd");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        if (lua_isnumber(L, -1))
            fd = (t_socket) lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return fd;
}

static int dirty(lua_State *L) {
    int is = 0;
    lua_pushstring(L, "dirty");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        is = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    return is;
}

static t_socket collect_fd(lua_State *L, int tab, t_socket max_fd,
        int itab, fd_set *set) {
    int i = 1;
    if (lua_isnil(L, tab))
        return max_fd;
    while (1) {
        t_socket fd;
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        fd = getfd(L);
        if (fd != SOCKET_INVALID) {
            FD_SET(fd, set);
            if (max_fd == SOCKET_INVALID || max_fd < fd)
                max_fd = fd;
            lua_pushnumber(L, fd);
            lua_pushvalue(L, -2);
            lua_settable(L, itab);
        }
        lua_pop(L, 1);
        i = i + 1;
    }
    return max_fd;
}

static int check_dirty(lua_State *L, int tab, int dtab, fd_set *set) {
    int ndirty = 0, i = 1;
    if (lua_isnil(L, tab))
        return 0;
    while (1) {
        t_socket fd;
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        fd = getfd(L);
        if (fd != SOCKET_INVALID && dirty(L)) {
            lua_pushnumber(L, ++ndirty);
            lua_pushvalue(L, -2);
            lua_settable(L, dtab);
            FD_CLR(fd, set);
        }
        lua_pop(L, 1);
        i = i + 1;
    }
    return ndirty;
}

static void return_fd(lua_State *L, fd_set *set, t_socket max_fd,
        int itab, int tab, int start) {
    t_socket fd;
    for (fd = 0; fd < max_fd; fd++) {
        if (FD_ISSET(fd, set)) {
            lua_pushnumber(L, ++start);
            lua_pushnumber(L, fd);
            lua_gettable(L, itab);
            lua_settable(L, tab);
        }
    }
}

static void make_assoc(lua_State *L, int tab) {
    int i = 1, atab;
    lua_newtable(L); atab = lua_gettop(L);
    while (1) {
        lua_pushnumber(L, i);
        lua_gettable(L, tab);
        if (!lua_isnil(L, -1)) {
            lua_pushnumber(L, i);
            lua_pushvalue(L, -2);
            lua_settable(L, atab);
            lua_pushnumber(L, i);
            lua_settable(L, atab);
        } else {
            lua_pop(L, 1);
            break;
        }
        i = i+1;
    }
}

/* min and max macros */
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? x : y)
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? x : y)
#endif

static void timeout_init(p_timeout tm, double block, double total) {
    tm->block = block;
    tm->total = total;
}

static double timeout_gettime(void) {
    struct timeval v;
    gettimeofday(&v, (struct timezone *) NULL);
    /* Unix Epoch time (time since January 1, 1970 (UTC)) */
    return v.tv_sec + v.tv_usec/1.0e6;
}

static double timeout_getretry(p_timeout tm) {
    if (tm->block < 0.0 && tm->total < 0.0) {
        return -1;
    } else if (tm->block < 0.0) {
        double t = tm->total - timeout_gettime() + tm->start;
        return MAX(t, 0.0);
    } else if (tm->total < 0.0) {
        double t = tm->block - timeout_gettime() + tm->start;
        return MAX(t, 0.0);
    } else {
        double t = tm->total - timeout_gettime() + tm->start;
        return MIN(tm->block, MAX(t, 0.0));
    }
}

static p_timeout timeout_markstart(p_timeout tm) {
    tm->start = timeout_gettime();
    return tm;
}

int socket_select(t_socket n, fd_set *rfds, fd_set *wfds, fd_set *efds,
        p_timeout tm, const sigset_t* mask) {
    int ret;
    //do
    {
        struct timespec ts;
        double t = timeout_getretry(tm);
        ts.tv_sec = (int) t;
        ts.tv_nsec = (int) ((t - ts.tv_sec) * 1.0e9);
        /* timeout = 0 means no wait */
        ret = pselect(n, rfds, wfds, efds, (const struct timespec*) (t >= 0.0 ? &ts: NULL), mask);
    }
    //while (ret < 0 && errno == EINTR);
    return ret;
}

static int global_select(lua_State *L, const sigset_t* mask, int sigreceived) {
    int rtab, wtab, etab, itab, ret, ndirty;
    t_socket max_fd;
    fd_set rset, wset, eset;
    t_timeout tm;
    double t = luaL_optnumber(L, 4, -1);
    FD_ZERO(&rset); FD_ZERO(&wset); FD_ZERO(&eset);
    lua_settop(L, 4);
    lua_newtable(L); itab = lua_gettop(L);
    lua_newtable(L); rtab = lua_gettop(L);
    lua_newtable(L); wtab = lua_gettop(L);
    lua_newtable(L); etab = lua_gettop(L);
    max_fd = collect_fd(L, 1, SOCKET_INVALID, itab, &rset);
    ndirty = check_dirty(L, 1, rtab, &rset);
    t = ndirty > 0? 0.0: t;
    timeout_init(&tm, t, -1);
    timeout_markstart(&tm);
    max_fd = collect_fd(L, 2, max_fd, itab, &wset);
    max_fd = collect_fd(L, 3, max_fd, itab, &eset);
    //printf("+enter select\n");
    if (sigreceived) {
        ret = -1;
    } else {
        ret = socket_select(max_fd+1, &rset, &wset, &eset, &tm, mask);
    }
    //printf("+exit select\n");
    if (ret > 0 || ndirty > 0) {
        return_fd(L, &rset, max_fd+1, itab, rtab, ndirty);
        return_fd(L, &wset, max_fd+1, itab, wtab, 0);
        return_fd(L, &eset, max_fd+1, itab, etab, 0);
        make_assoc(L, rtab);
        make_assoc(L, wtab);
        make_assoc(L, etab);
        return 3; //3 values pushed: 3 result tables
    } else if (ret == 0) {
        lua_pushstring(L, "timeout");
        return 4; //4 values pushed: 3 result tables + timeout msg
    } else {
        lua_pushstring(L, strerror(errno));
        return 4; //4 values pushed: 3 result tables + errno msg
    }
}
/* LUA SOCKET EXPORT*/

static int l_select(lua_State *L) {
    sigset_t orig_mask;
    sigset_t mask;
    sigfillset(&mask);

    sigprocmask(SIG_SETMASK, &mask, &orig_mask);
    //printf("############# BLOCKING ##############\n");
    int sigreceived = step(L);
    sigset_t emptymask;
    sigemptyset(&emptymask);
    int top = global_select(L, (const sigset_t*) &emptymask, sigreceived);
    sigprocmask(SIG_SETMASK, &orig_mask, NULL);
    //printf("######### UN BLOCKING '%X' ##########\n", orig_mask);
    return top;
}

/**
 * Register functions.
 */
static const luaL_Reg R[] = {
        { "signal", l_signal },
        { "raise", l_raise },
        { "kill", l_kill },
        { "select", l_select},
        { NULL, NULL } };

int luaopen_sched_posixsignal(lua_State* L) {
    luaL_register(L, "sched.posixsignal", R);

    // put signal number and name table in the registry
    lua_pushstring(L, "__sigdef");
    lua_newtable(L);
    int i = 0;
    while (posixSignals[i].name != NULL) {
        // t[name] = sig
        lua_pushstring(L, posixSignals[i].name);
        lua_pushnumber(L, posixSignals[i].sig);
        lua_settable(L, -3);
        // t[sig] = name
        lua_pushnumber(L, posixSignals[i].sig);
        lua_pushstring(L, posixSignals[i].name);
        lua_settable(L, -3);
        i++;
    }
    lua_settable(L, -3);
    return 1;
}
