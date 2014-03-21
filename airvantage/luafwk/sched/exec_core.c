/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "luasignal.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define POPEN_USERDATA  "popenud"
#define READ_BUFFER_SIZE 256

typedef struct
{
  LuaSignalCtx* signalctx;
} SYS;

SYS sys;

typedef struct
{
  int fd;
  uint8_t gc;
  pid_t pid;
} popenctx;

static inline int check_file(lua_State* L, popenctx* pud)
{
  if (pud->fd < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, "File is closed");
    return 2;
  }

  return 0;
}

static int l_write(lua_State* L)
{
  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);
  size_t buf_len;
  int status;

  if ((status=check_file(L, pud)) != 0)
    return status;

  const char* buf = luaL_checklstring(L, -1, &buf_len);

  int nb = write(pud->fd, buf, buf_len);

  if (nb >= 0)
  {
    lua_pushinteger(L, nb);
    return 1;
  }

  lua_pushnil(L);
  lua_pushstring(L, strerror(errno));
  return 2;
}

static int l_read(lua_State* L)
{
  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);
  int status;

  if ((status=check_file(L, pud)) != 0)
    return status;

  char buffer[READ_BUFFER_SIZE];
  /* It's OK for READ_BUFFER_SIZE to be big, because we expect to have a MMU which
   * will only map it on demand. */
  int nb = read(pud->fd, buffer, READ_BUFFER_SIZE);

  if (nb == 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, "eof");
    return 2;
  }
  else if (nb < 0 && errno != EAGAIN)
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  else if (nb < 0)
    nb = 0;

  lua_pushlstring(L, buffer, nb);
  return 1;
}


static inline void cleanup(popenctx* pud)
{
  if (pud->fd >= 0)
    close(pud->fd);
  pud->fd = -1;
}

static int l_close(lua_State* L)
{
  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);
  int status;

  if ((status=check_file(L, pud)) != 0)
    return status;

  if (close(pud->fd) < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  pud->fd = -1;

  cleanup(pud);
  lua_pushboolean(L, 1);
  return 1;
}

static int l_gc(lua_State* L)
{

  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);

  cleanup(pud);

  return 0;
}

static int l_getpid(lua_State* L)
{
  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);

  lua_pushinteger(L, pud->pid);
  return 1;
}

static int l_getfd(lua_State* L)
{
  popenctx* pud = (popenctx*) luaL_checkudata(L, 1, POPEN_USERDATA);
  int status;

  if ((status=check_file(L, pud)) != 0)
    return status;

  lua_pushinteger(L, pud->fd);
  return 1;
}


static int fork_and_redirect_output(lua_State* L, uint8_t redirect)
{
  int sockets[2] = { -1, -1 };
  const char* cmd = lua_isnil(L, 1) == 1 ? "exit 1" : luaL_checkstring(L, 1);

  if (redirect)
  {
    // create the socket pair that is used to read and write to the spawned process
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
    }

    // Set the socket to non blocking (so it does not block the VM)
    int flags = fcntl(sockets[0], F_GETFL, 0);
    fcntl(sockets[0], F_SETFL, flags | O_NONBLOCK);
  }
  pid_t pid;
  if ((pid = fork()) == -1)
  {
    // Error case
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  else if (pid)
  { /* This is the parent. */

    close(sockets[1]); // fd are duplicated in the fork call, we do not need this end of the socket pair in that process.
    if (redirect)
    {
      popenctx* pud = lua_newuserdata(L, sizeof(*pud));
      pud->fd = sockets[0];
      pud->pid = pid;
      luaL_getmetatable(L, POPEN_USERDATA);
      lua_setmetatable(L, -2);
    }
    else
    {
      lua_pushfstring(L, "%d", pid);
    }
    return 1;
  }
  else
  { /* This is the child. */
      // Set stdout/stderr/stdin to use the socket pair for input/output
    if (redirect)
    {
      dup2(sockets[1], 0);
      dup2(sockets[1], 1);
      dup2(sockets[1], 2);
    }
    // closing inherited file descriptors but stdout/stderr/stdin
    // this will close sockets[0] among others
    int i;
    int maxfd = getdtablesize();
    for(i=3; i<maxfd; i++)
        close(i);

    int ret = execl("/bin/sh", "sh", "-c", cmd, NULL);
    int exitcode = (ret == -1 && errno == ENOENT) ? 1 : 127;
    close(sockets[1]);
    exit(exitcode);
  }
}

static int l_waitpid(lua_State *L)
{
  pid_t pid = luaL_checknumber(L, 1);
  int status;
  pid_t ret;

  ret = waitpid(pid, &status, WNOHANG);

  if (ret == 0)
  {
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
  }
  else if (ret == -1)
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  lua_pushinteger(L, WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status));
  return 1;
}

static int l_execute(lua_State *L)
{
  return fork_and_redirect_output(L, 0);
}

static int l_popen(lua_State *L)
{
  return fork_and_redirect_output(L, 1);
}


static const luaL_Reg module_functions[] =
{
 { "write", l_write },
 { "read", l_read },
 { "close", l_close },
 { "__gc", l_gc },
 { "getfd", l_getfd },
 { "getpid", l_getpid },
{ NULL, NULL } };

static const luaL_Reg R[] =
{
{ "execute", l_execute },
{ "popen", l_popen },
{ "waitpid", l_waitpid },
{ NULL, NULL } };

int luaopen_sched_exec_core(lua_State* L)
{
  luaL_newmetatable(L, POPEN_USERDATA);
  lua_pushvalue(L, -1); // push copy of the metatable
  lua_setfield(L, -2, "__index"); // set index file of the metatable
  luaL_register(L, NULL, module_functions); //Register functions in the metatable
  luaL_register(L, "exec.core", R);

  return 1;
}
