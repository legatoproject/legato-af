/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "serial.h"

#define MODULE_NAME "SERIAL_PORT"
#define READ_BUFFER_SIZE 512 // define how much it can read data at a time

typedef struct
{
  int fd;
} SerialPort;

static inline int check_port(lua_State* L, SerialPort* sp)
{
  if (sp->fd < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, "Port is closed");
    return 2;
  }

  return 0;
}

static const char* PAR_NONE  = "none";
static const char* PAR_ODD   = "odd";
static const char* PAR_EVEN  = "even";

static int setParity(const char* parity, struct termios* term)
{
  if (strcmp(parity, PAR_NONE) == 0)
    term->c_cflag &= ~PARENB;
  else if (strcmp(parity, PAR_ODD) == 0)
    term->c_cflag |= (PARENB | PARODD);
  else if (strcmp(parity, PAR_EVEN) == 0)
  {
    term->c_cflag &= ~PARODD;
    term->c_cflag |= PARENB;
  }
  else
    return -1;

  return 0;
}

static const char* getParity(struct termios* term)
{
  if (term->c_cflag & PARENB)
  {
    if (term->c_cflag & PARODD)
      return PAR_ODD;
    else
      return PAR_EVEN;
  }
  else
    return PAR_NONE;
}

static const char* FC_NONE    = "none";
static const char* FC_RTSCTS  = "rtscts";
static const char* FC_XONXOFF = "xon/xoff";

static int setFlowControl(const char* fc, struct termios* term)
{
  if (strcmp(fc, FC_RTSCTS) == 0)
  {
    term->c_cflag |= CRTSCTS;
    term->c_iflag &= ~(IXON | IXOFF);
  }
  else if (strcmp(fc, FC_XONXOFF) == 0)
  {
    term->c_cflag &= ~CRTSCTS;
    term->c_iflag |= (IXON | IXOFF);
  }
  else if (strcmp(fc, FC_NONE) == 0)
  {
    term->c_cflag &= ~CRTSCTS;
    term->c_iflag &= ~(IXON | IXOFF);
  }
  else
    return -1;

  return 0;
}

static const char* getFlowControl(struct termios* term)
{
  if (term->c_cflag & CRTSCTS)
  {
    if (term->c_iflag & IXON || term->c_iflag & IXOFF)
      printf("SERIAL: IXON/IXOFF flags are set in conjunction of CRTSCTS flag. Not harmfull but may be a bug. Check serial config !\n");
    return FC_RTSCTS;
  }
  else if (term->c_iflag & IXON || term->c_iflag & IXOFF)
    return FC_XONXOFF;
  else
    return FC_NONE;
}

static int setData(unsigned int data, struct termios* term)
{
  term->c_cflag &= ~CSIZE;

  switch (data)
  {
    case 5:
      term->c_cflag |= CS5;
      break;
    case 6:
      term->c_cflag |= CS6;
      break;
    case 7:
      term->c_cflag |= CS7;
      break;
    case 8:
      term->c_cflag |= CS8;
      break;
    default:
      return -1;
  }
  return 0;
}

static int getData(struct termios* term)
{
  int data;
  switch (term->c_cflag & CSIZE)
  {
    case CS5:
      data = 5;
      break;
    case CS6:
      data = 6;
      break;
    case CS7:
      data = 7;
      break;
    case CS8:
      data = 8;
      break;
    default:
      data = 0;
      break;
  }
  return data;
}


static int setStopBit(unsigned int stop_bit, struct termios* term)
{
  if (stop_bit == 1)
    term->c_cflag &= ~CSTOPB;
  else if (stop_bit == 2)
    term->c_cflag |= CSTOPB;
  else
    return -1;

  return 0;
}

static int getStopBit(struct termios* term)
{
  if (term->c_cflag & CSTOPB)
    return 2;
  else
    return 1;
}


static int setBaudrate(unsigned int baudrate, struct termios* term)
{
  switch (baudrate)
  {
    case 1200:
    {
      cfsetspeed(term, B1200);
      break;
    }
    case 9600:
    {
      cfsetspeed(term, B9600);
      break;
    }
    case 19200:
    {
      cfsetspeed(term, B19200);
      break;
    }
    case 38400:
    {
      cfsetspeed(term, B38400);
      break;
    }
    case 57600:
    {
      cfsetspeed(term, B57600);
      break;
    }
    case 115200:
    {
      cfsetspeed(term, B115200);
      break;
    }
    default:
      printf("SERIAL: unsupported baudrate: %d\n", baudrate);
      return -1;
  }
  return 0;
}

static int getBaudrate(struct termios* term)
{
  speed_t s = cfgetispeed(term);
  if (s != cfgetospeed(term))
    printf("SERIAL: Input and Output speed are different. Check the serial configuration !\n");

  switch(s)
  {
    case B1200:
      s = 1200;
      break;
    case B9600:
      s = 9600;
      break;
    case B19200:
      s = 19200;
      break;
    case B38400:
      s = 38400;
      break;
    case B57600:
      s = 57600;
      break;
    case B115200:
      s = 115200;
      break;
    default:
      printf("SERIAL: Unknown speed setting: %d!\n", s);
      s = 0;
      break;
  }
  return s;
}

static inline int checkps(lua_State* L, char* name, struct termios* term, int (*func)(const char* param, struct termios* term))
{
  lua_getfield(L, -1, name);
  if (!lua_isnil(L, -1))
  {
    const char* value = lua_tostring(L, -1);
    int s = func(value, term);
    if (s!=0)
    {
      lua_pushnil(L);
      lua_pushfstring(L, "Error while trying to set parameter %s, error=%d", name, s);
      return 2;
    }
  }
  lua_pop(L,1);
  return 0;
}
static inline int checkpi(lua_State* L, char* name, struct termios* term, int (*func)(unsigned int param, struct termios* term))
{
  lua_getfield(L, -1, name);
  if (!lua_isnil(L, -1))
  {
    unsigned int value = lua_tointeger(L, -1);
    int s = func(value, term);
    if (s!=0)
    {
      lua_pushnil(L);
      lua_pushfstring(L, "Error while trying to set parameter %s, error=%d", name, s);
      return 2;
    }
  }
  lua_pop(L,1);
  return 0;
}
static int setConfig(lua_State* L, SerialPort* sp)
{
  int fd = sp->fd;
  struct termios term;
  tcgetattr(fd, &term);

  int status;

  if ((status=checkps(L, "parity", &term, setParity)) != 0)
    return status;

  if ((status=checkps(L, "flowcontrol", &term, setFlowControl)) != 0)
    return status;

  if ((status=checkpi(L, "data", &term, setData)) != 0)
    return status;

  if ((status=checkpi(L, "stop", &term, setStopBit)) != 0)
    return status;

  if ((status=checkpi(L, "baudrate", &term, setBaudrate)) != 0)
    return status;

  tcsetattr(fd, TCSANOW, &term);

  return 0;
}

static int getConfig(lua_State* L, SerialPort* sp)
{
  int fd = sp->fd;
  struct termios term;
  tcgetattr(fd, &term);

  lua_newtable(L);
  lua_pushstring(L, getParity(&term));
  lua_setfield(L, -2, "parity");

  lua_pushstring(L, getFlowControl(&term));
  lua_setfield(L, -2, "flowcontrol");

  lua_pushinteger(L, getData(&term));
  lua_setfield(L, -2, "data");

  lua_pushinteger(L, getStopBit(&term));
  lua_setfield(L, -2, "stop");

  lua_pushinteger(L, getBaudrate(&term));
  lua_setfield(L, -2, "baudrate");

  return 1;
}

static int l_configure(lua_State* L)
{
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status = check_port(L, sp)) != 0)
    return status;

  if (lua_gettop(L) > 1) // user provided an additional parameter
  {
    luaL_checktype(L, 2, LUA_TTABLE);
    if ((status = setConfig(L, sp)) != 0)
      return status;
  }

  return getConfig(L, sp);
}

static int l_open(lua_State* L)
{
  size_t len;
  const char* port = luaL_checklstring(L, 1, &len);
  int config = 0;
  if (lua_gettop(L) > 1) // more than one argument on the stack: the second one must be a table that holds the config
  {
    luaL_checktype(L, 2, LUA_TTABLE);
    config = 1;
  }

  SerialPort* sp = lua_newuserdata(L, sizeof(*sp));

  int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  sp->fd = fd;

  struct termios term;
  bzero(&term, sizeof(term));
  cfmakeraw(&term);
  term.c_cflag |= CREAD;

  // Default config
  setParity("none", &term);
  setFlowControl("none", &term);
  setData(8, &term);
  setStopBit(1, &term);
  setBaudrate(115200, &term);
  tcsetattr(fd, TCSANOW, &term);

  // Use user provided config, if any
  if (config)
  {
    lua_pushvalue(L, 2); // setConfig, need to have the config table on the top of the stack...
    int status;
    if ((status = setConfig(L, sp)) != 0)
      return status;
    lua_pop(L, 1);
  }

  tcflush(fd, TCIOFLUSH);

  luaL_getmetatable(L,MODULE_NAME);
  lua_setmetatable(L, -2);

  return 1;
}

static int l_write(lua_State* L)
{
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  size_t buf_len;
  int status;

  if ((status=check_port(L, sp)) != 0)
    return status;

  const char* buf = luaL_checklstring(L, -1, &buf_len);

  int nb = write(sp->fd, buf, buf_len);

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
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status=check_port(L, sp)) != 0)
    return status;

  char buffer[READ_BUFFER_SIZE]; // it is not a real prb to allocate that amount of bytes on the stack because this code is designed for Linux with MMU
  int nb = read(sp->fd, buffer, READ_BUFFER_SIZE);

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

static int l_flush(lua_State* L)
{
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status = check_port(L, sp)) != 0)
    return status;

  status = tcflush(sp->fd, TCIOFLUSH);
  if (! status)
  {
    lua_pushliteral( L, "ok");
    return 1;
  }
  else
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(status));
    return 2;
  }
}


static inline void cleanup(SerialPort* sp)
{
  if (sp->fd >= 0)
    close(sp->fd);
  sp->fd = -1;
}
static int l_close(lua_State* L)
{
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status=check_port(L, sp)) != 0)
    return status;

  if (close(sp->fd) < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }
  sp->fd = -1;

  cleanup(sp);
  lua_pushliteral( L, "ok");
  return 1;
}

static int l_gc(lua_State* L)
{

  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);

  cleanup(sp);

  return 0;
}

static int l_getfd(lua_State* L)
{
  SerialPort* sp = (SerialPort*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status=check_port(L, sp)) != 0)
    return status;

  lua_pushinteger(L, sp->fd);
  return 1;
}

static const luaL_Reg module_functions[] =
{
{ "write", l_write },
{ "read", l_read },
{ "flush", l_flush },
{ "close", l_close },
{ "configure", l_configure },
{ "__gc", l_gc },
{ "getfd", l_getfd },
{ NULL, NULL } };

static const luaL_Reg R[] =
{
{ "open", l_open },
{ NULL, NULL } };

int luaopen_serial_core(lua_State* L) {
  luaL_register(L, "serial.core", R);       // m={ open=l_open }
  luaL_newmetatable(L, MODULE_NAME);        // m, mt
  lua_pushvalue(L, -1);                     // m, mt, mt
  lua_setfield(L, -2, "__index");           // m, mt[__index=mt]
  luaL_register(L, NULL, module_functions); // m, mt[key=value*]
  lua_setfield( L, -2, "__metatable");      // m[__metatable=mt]
  return 1;
}

