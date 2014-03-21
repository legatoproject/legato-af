/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/

/**
 The gpio library enables to interact with GPIOs.

 It provides `read`, `write`, and `register` methods,
 as well as GPIO configuration and listing features.

 This module is mainly based on Linux kernel userspace mapping to act on GPIOs.<br />
 See [Kernel GPIO doc](http://kernel.org/doc/Documentation/gpio.txt), "Sysfs Interface
 for Userspace" chapter.<br />
 Please check you device system comes with this capability before using this module.
 Also, pay attention to access rights to /sys/class/gpio files, and check that the process
 running this module has correct user rights to access those files.

 @module gpio
 */

//#include <stdio.h>
#include <string.h>
//#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//Lua includes
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "returncodes.h"

//#include "swi_log.h"

#define MODULE_NAME "GPIO_FILE"
#define GPIO_PATH_BASE "/sys/class/gpio/"
#define GPIO_PATH_EXPORT GPIO_PATH_BASE "export"
#define GPIO_PATH_UNEXPORT GPIO_PATH_BASE "unexport"
#define GPIO_ID_SIZE (5) //1024 GPIOmax; -> 4 chars for 1024, +1
//util macro to return from Lua C API func
//it follows the nil, "error msg" common pattern
#define L_RETURN_ERROR(...)              \
        do {                             \
        lua_pushnil(L);                  \
        lua_pushfstring(L, __VA_ARGS__); \
        return 2;                        \
        } while(0)

typedef struct Gpio
{
  int fd;
  int id;
} Gpio_t;

static rc_ReturnCode_t setparam(int id, const char* param, const char* value);
static rc_ReturnCode_t enable(int id);
static int isenable(int id);
static int enable_config(lua_State* L, int id, const char* direction, const char* edge, const char* active);

/*
 * @return allocated char *, the id of the gpio.
 * Note: the returned char* must be freed by the caller!
 * @return NULL in case of error
 *
 * getgpioidstr(14, "edge") -> "14"
 */
static char* getgpioidstr(int id)
{
  char* buf = NULL;
  //snprintf returns the size needed to write the formated str, without '\0'
  int res = snprintf(buf, 0, "%d", id);
  if (res <= 0)
  {
    return NULL;
  }
  buf = malloc(res + 1);
  if (NULL == buf)
  {
    return NULL;
  }
  //actually do the formated print
  int res2 = snprintf(buf, res + 1, "%d", id);
  if (res2 != res)
  {
    free(buf);
    buf = NULL;
    return NULL;
  }
  return buf;
}

/*
 * @param id: gpio id
 * @param func; char*, likely to be "edge", "direction", "active_low"
 *
 * @return allocated char *, the full path to corresponding gpio file.
 * Note: the returned char* must be freed by the caller!
 * @return NULL in case of error
 *
 * getgpiopath(4, "edge") -> "/sys/class/gpio/gpio4/edge"
 */
static char* getgpiopath(int id, const char* func)
{
  int res = 0;
  char* buf;
  size_t len = 0;
  char* gpioidstr = getgpioidstr(id);
  if (NULL == gpioidstr)
  {
    return NULL;
  }

  //"/sys/class/gpio/" + "gpio" + strlen(gpioidstr) + "/" "value" or "edge" etc
  len = strlen(GPIO_PATH_BASE) + 4 + strlen(gpioidstr) + 1 + strlen(func);
  buf = malloc(len + 1);
  if (NULL == buf)
  {
    free(gpioidstr);
    gpioidstr = NULL;
    return NULL;
  }
  //do the formated print
  res = snprintf(buf, len + 1, "%sgpio%s/%s", GPIO_PATH_BASE, gpioidstr, func);
  //clean ressources
  free(gpioidstr);
  gpioidstr = NULL;

  if (res != len)
  {
    free(buf);
    buf = NULL;
    return NULL;
  }

  return buf;
}

/***************************************************************************
 Registers a GPIO for monitoring it for changes.

 Using `nil` for `userhook` parameter, this function also enables to cancel
 previous registration.

 GPIO Configuration:

 *    Default configuration: <br>
 If the gpio has not been configured (using @{#gpio.configure} API) prior to
 call this function, then the gpio will be automatically configured using:<br>
 `{direction="in", edge="both", activelow="0"}`.


 *    Required configuration:

 If the gpio have been previously set as input, its the configuration
 will not be changed.

 The behavior of notification  (i.e. when the user callback will be called)
 is impacted by the configuration of the GPIO:
 *   `edge` parameter: "rising", "falling" and "both" will enable the notification
 and the callback will be called according to the behavior of `edge` value.
 Using "none" will disable any notification.

 *   `activelow` parameter: this parameter will modify the way to interpret
 `edge` values ("rising", "falling").


 @function [parent=#gpio] register
 @param id: a number, the gpio id.
 @param userhook: a function that will be called when a notification is available for the GPIO.

 This param can be set to nil to cancel previous registration.
 userhook signature: hook(gpioid, value)

 @return "ok" when successful
 @return `nil` followed by an error message otherwise

 @usage
 local function myhook(id, value)
 print(string.format("GPIO %d has changed! New value = %s", id, value))
 end
 gpio.register(42, myhook)
 -- this may produce this print:
 "GPIO 42 has changed! New value = '1'"
 */

//TODO: move doc about register into Lua source where top level API is implemented.
// this l_newgpio is just a part of implementation of register
// this doc can be moved when luadocumentor support module doc generation from several/various
// source files.
// Main point here is to create the userdata containing GPIO fd.
static int l_newgpio(lua_State* L)
{
  int errnotmp = 0;
  int res = 0;
  char* filepath = NULL;
  Gpio_t* io = NULL;

  int gpioid = luaL_checkinteger(L, 1);

  //check GPIO is enabled otherwise set appropriate config.
  if ((res = enable_config(L, gpioid, "in", "both", "0")))
    return res;

  filepath = getgpiopath(gpioid, "value");
  if (NULL == filepath)
    L_RETURN_ERROR("failed to get gpio path");

  io = lua_newuserdata(L, sizeof(*io));

  int fd = open(filepath, O_RDWR | O_NONBLOCK);
  errnotmp = errno;
  // clean resources
  free(filepath);
  filepath = NULL;

  if (fd < 0)
    L_RETURN_ERROR("failed to open gpio file", strerror(errnotmp));

  io->fd = fd;
  io->id = gpioid;

  luaL_getmetatable(L, MODULE_NAME);
  lua_setmetatable(L, -2);

  //leave fd open on purpose, will be closed when gpio is not monitored anymore

  return 1;
}

/***************************************************************************
 Reads a GPIO value.

 GPIO Configuration:

 *    Default configuration: <br>
 If the gpio has not been configured (using @{#gpio.configure} API) prior to
 call this function, then the gpio will be automatically configured using:<br>
 `{direction="in", edge="none", activelow="0"}`.


 *    Required configuration:<br>
 If the gpio have been previously set as output, its the configuration
 will not be changed, it is very likely to be readable without any error.


 The value returned is impacted by the configuration of the GPIO:<br>
 *   `activelow` parameter: this parameter will modify the way to interpret
 `edge` values "rising", "falling".


 @function [parent=#gpio] read
 @param id: a number, the gpio id.

 @return GPIO value as a "string" when successful
 @return `nil` followed by an error message otherwise

 @usage
 local gpio = require"gpio"
 local val = gpio.read(42)
 */
static int l_read(lua_State* L)
{
  int errnotmp = 0;
  int res = 0;
  int gpioid = luaL_checkint(L, -1);

  //check GPIO is enabled otherwise set appropriate config.
  if ((res = enable_config(L, gpioid, "in", "none", "0")))
    return res;

  int fd = -1;
  char * valuepath = getgpiopath(gpioid, "value");
  if (NULL == valuepath)
    L_RETURN_ERROR("can't get gpio path");

  //fd will be used only for this read action
  fd = open(valuepath, O_RDONLY);
  if (fd < 0)
  {
    free(valuepath);
    valuepath = NULL;
    L_RETURN_ERROR("failed to open gpio file: errno: %s", strerror(errno));
  }
  char val = 0;
  int nb = read(fd, &val, 1);
  errnotmp = errno;
  //clean unnecessary resources
  close(fd);
  free(valuepath);
  valuepath = NULL;

  if (nb == 0)
    L_RETURN_ERROR("eof");
  else if (nb < 0 && errnotmp != EAGAIN)
    L_RETURN_ERROR("read error: errno: %s", strerror(errnotmp));
  else if (nb < 0)
    nb = 0;

  if (val == '0')
    lua_pushinteger(L, 0);
  else
    lua_pushinteger(L, 1);

  return 1;
}

/** Writes a GPIO value.

 GPIO Configuration:

 *   Default configuration:<br>
 If the gpio has not been configured (using @{#gpio.configure} API) prior to
 call this function, then the gpio will be automatically configured using:<br>
 `{direction="out", edge="none", activelow="0"}`.

 *    Required configuration:<br>
 If the GPIO have been previously set as input, its configuration
 will not be changed and the write call will return an error.

 @function [parent=#gpio] write
 @param id: a number, the GPIO id
 @param value: string or number :"0" or 0,"1" or 1, the value to set the GPIO.

 @return "ok" on success
 @return nil, error string never returns (throws a Lua error instead)

 @usage
 local gpio = require"gpio"
 gpio.write(42, 1)
 */
static int l_write(lua_State* L)
{
  int errnotmp = 0;
  int res = 0;
  //param check
  int gpioid = luaL_checkint(L, 1);
  size_t buf_len = 0;
  const char* buf = luaL_checklstring(L, 2, &buf_len);
  if (buf_len > 1)
  {
    lua_pushnil(L);
    lua_pushstring(L, "write only one byte!");
    return 2;
  }

  //check GPIO is enabled otherwise set appropriate config.
  if ((res = enable_config(L, gpioid, "out", "none", "0")))
    return res;

  int fd = -1;
  char * valuepath = getgpiopath(gpioid, "value");
  if (NULL == valuepath)
  {
    lua_pushnil(L);
    lua_pushstring(L, "can't get gpio path");
    return 2;
  }

  fd = open(valuepath, O_WRONLY);
  errnotmp = errno;
  if (fd < 0)
  {
    free(valuepath);
    valuepath = NULL;
    L_RETURN_ERROR("failed to open gpio file, errno: %s", strerror(errnotmp));
  }

  int nb = write(fd, buf, buf_len);
  errnotmp = errno;
  //clean unnecessary resources
  close(fd);
  free(valuepath);
  valuepath = NULL;
  if (nb >= 0)
  {
    lua_pushstring(L, "ok");
    return 1;
  }

  lua_pushnil(L);
  lua_pushstring(L, strerror(errnotmp));
  return 2;

}

/***
 * check parameter string
 *
 * @return -1 when parameter not found,
 *         0 on success ,
 *         2 when error occurred (nil and an error msg are push onto the Lua stack)
 */
static inline int checkps(lua_State* L, const char* table_field, const char* gpio_param, int id)
{
  int res = 0;
  lua_getfield(L, -1, table_field);
  if (!lua_isnil(L, -1))
  {
    const char* value = lua_tostring(L, -1);
    rc_ReturnCode_t s = setparam(id, gpio_param, value);
    if (s != RC_OK)
      L_RETURN_ERROR("Error while trying to set parameter %s, error=%s", gpio_param, rc_ReturnCodeToString(s));
    res = 0; //success
  }
  else
  {
    res = -1; //parameter not found
  }
  lua_pop(L, 1);
  return res;
}

/**
 Configures the GPIO parameters.

 The parameters that can be set are:

 * `direction`: the direction of the GPIO; input or output
 * `edge`: this setting describes when embedded application will be notified for changes:
   never, on rising edge (from inactive to active) or falling edge (from active to inactive).
 * `activelow`: This setting describes how to interpret the GPIO value:
   Is high voltage value (e.g.: 3.3V or 5V) to be interpreted as "active"/"on" (the default behavior i.e. `activelow` set to "0")
   or as "inactive"/"off" (`activelow` set to "1").

 Before calling this function:

 *    Required configuration:<br>
 If no previous configuration (neither automatic nor explicit) has been done for
 this GPIO, then the first explicit call to @{#gpio.configure} must contain the `direction`
 parameter.

 @function [parent=#gpio] configure
 @param id GPIO id as a number

 @param config a map with fields:

 * `edge`: accepted value: "none", "rising", "falling", "both".
  Beware: changing edge value on a GPIO monitored for change (using @{#gpio.register} API).
 will alter the way notifications are done for this GPIO.
 * `direction`: accepted values (string): "in", "out".
 * `activelow`: accepted values (string): "0", "1".

 @return "ok" when successful
 @return `nil` followed by an error message otherwise

 @usage:
 local gpio = require"gpio"
 local val = gpio.configure(42, {direction="in", edge="both", activelow="0"})
 */
static int l_configure(lua_State* L)
{
  rc_ReturnCode_t res = RC_OK;
  int status = 0;
  int gpioid = luaL_checkint(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  if (!isenable(gpioid))
  {
    res = enable(gpioid);
    if ( RC_OK != res )
      L_RETURN_ERROR("Error while enabling gpio, error=%s", rc_ReturnCodeToString(res) );

    status = checkps(L, "direction", "direction", gpioid);
    if (status < 0) //direction not found
      L_RETURN_ERROR("direction must be set for first configure call on a gpio");
    else if (status > 0) //failed to set direction
      return status;
    //if direction was found and successfully set then go on with other params.
  }
  else
  {
    if ((status = checkps(L, "direction", "direction", gpioid)) > 0)
      return status;
  }

  if ((status = checkps(L, "edge", "edge", gpioid)) > 0)
    return status;

  if ((status = checkps(L, "activelow", "active_low", gpioid)) > 0)
    return status;

  lua_pushstring(L, "ok");
  return 1;
}

static int readconfigfile(lua_State* L, int gpioid, const char* param)
{
  int errnotmp = 0;
  int fd = -1;
  char * valuepath = getgpiopath(gpioid, param);
  if (NULL == valuepath)
    L_RETURN_ERROR("can't get gpio path");

  fd = open(valuepath, O_RDONLY);
  if (fd < 0)
  {
    free(valuepath);
    valuepath = NULL;
    L_RETURN_ERROR("failed to open gpio file: errno: %s", strerror(errno));
  }
  char val[256];
  memset(val, 0, 256);
  int nb = read(fd, &val, 256);
  errnotmp = errno;
  //clean unnecessary resources
  close(fd);
  free(valuepath);
  valuepath = NULL;

  if (nb == 0)
    L_RETURN_ERROR("eof");
  else if (nb < 0 && errnotmp != EAGAIN)
    L_RETURN_ERROR("read error: errno: %s", strerror(errnotmp));
  else if (nb < 0)
    L_RETURN_ERROR("read error: errno EAGAIN: %s", strerror(errnotmp));

  //read all config params as string, even though activelow could
  //have been returned as an integer, no need for it for now.
  lua_pushlstring(L, val, nb - 1);

  return 1;
}

/***************************************************************************
 Retrieves GPIO configuration.

 See @{#gpio.configure} for available settings.

 @function [parent=#gpio] getconfig

 @param id: a number, the gpio id.

 @return GPIO configuration as a table
 @return `nil` followed by an error message otherwise

 @usage
 local gpio = require"gpio"
 local res = gpio.getconfig(4)
 -- res = { direction="in", edge="none", activelow="0" }
 */

static int l_getconfig(lua_State* L)
{
  int gpioid = luaL_checkint(L, 1);
  int res = 0;
  if (!isenable(gpioid))
    L_RETURN_ERROR("GPIO %d not enabled yet", gpioid);
  else
  {
    lua_newtable(L);

    if (1 != (res = readconfigfile(L, gpioid, "direction")))
      return res; //error pushed by readconfigfile
    lua_setfield(L, -2, "direction");

    if (1 != (res = readconfigfile(L, gpioid, "edge")))
      return res; //error pushed by readconfigfile
    lua_setfield(L, -2, "edge");

    if (1 != (res = readconfigfile(L, gpioid, "active_low")))
      return res; //error pushed by readconfigfile
    lua_setfield(L, -2, "activelow");

    return 1;
  }
}

/**
 * based on /sys/class/gpio/gpioid/ folder existence
 *
 * @return 1 when the gpio is enabled
 * @return 0 when the gpio is disabled
 */
int isenable(int id)
{
  char* path = getgpiopath(id, "");
  struct stat buf;
  int res = stat(path, &buf);
  free(path);
  path = NULL;
  return !res;
}

/**
 * Enables a GPIO, doesn"t test if already enabled,
 * this needs to be done be by caller.
 *
 * @return RC_OK on success
 *         other rc_ReturnCode_t on error
 */
rc_ReturnCode_t enable(int id)
{
  int res = 0;

  char* id_buf = getgpioidstr(id);
  if (NULL == id_buf)
    return RC_UNSPECIFIED_ERROR;

  int fd = open(GPIO_PATH_EXPORT, O_WRONLY); //can't read "export" file
  if (fd < 0)
  {
    int errnotmp = errno;
    free(id_buf);
    id_buf = NULL;
    if (EPERM == errnotmp || EACCES == errnotmp)
      return RC_NOT_PERMITTED;
    if (ENOENT == errnotmp)
      return RC_NOT_AVAILABLE;
    return RC_UNSPECIFIED_ERROR;
  }

  size_t len = strlen(id_buf);
  res = write(fd, id_buf, len);
  //clean resources
  free(id_buf);
  id_buf = NULL;
  close(fd);

  if (res <= 0 || res < len)
    return RC_UNSPECIFIED_ERROR;

  return RC_OK;
}

/* Checks a GPIO is already activated;
 * if not, set a default config for each param.
 *
 * @return 0 on success
 *         2 on error (nil, err msg pushed on Lua stack)
 */
int enable_config(lua_State* L, int id, const char* direction, const char* edge, const char* active)
{
  //if the GPIO is already enabled, then don't change its config
  if (isenable(id))
    return 0;
  else
  {
    rc_ReturnCode_t res = RC_OK;
    if (RC_OK != (res = enable(id)))
      L_RETURN_ERROR("Error while enabling gpio, error=%s", rc_ReturnCodeToString(res));
    if ((res = setparam(id, "direction", direction)))
      L_RETURN_ERROR("Error while setting direction, error=%s", rc_ReturnCodeToString(res));
    if ((res = setparam(id, "edge", edge)))
      L_RETURN_ERROR("Error while setting edge, error=%s", rc_ReturnCodeToString(res));
    if ((res = setparam(id, "active_low", active)))
      L_RETURN_ERROR("Error while setting active_low, error=%s", rc_ReturnCodeToString(res));

    return 0;
  }
  return 0;
}

/**
 * internal function, must be called with gpio sub files exact name
 * (cf param description).
 *
 * @param param: the gpio parameter *file* to act on: "edge", "direction", "active_low"
 *
 * @param value: depends on the parameter being set:
 *   - edge: "none", "rising", "falling", "both".
 *   - direction: "in", "out"
 *   - active_low: "0", "1"
 *
 *  @see `configure` public API for complete parameter documentation.
 *
 *  @return RC_OK on success, other rc_ReturnCode_t on error
 */
rc_ReturnCode_t setparam(int id, const char* param, const char* value)
{
  int res = 0;
  int fd = -1;
//create the full path to edge file
  char* file = getgpiopath(id, param);
  if (NULL == file)
    return RC_UNSPECIFIED_ERROR;

  fd = open(file, O_RDWR);
  if (fd < 0)
  {
    int errnotmp = errno;
    free(file);
    file = NULL;
    if (EPERM == errnotmp || EACCES == errnotmp)
      return RC_NOT_PERMITTED;
    if (ENOENT == errnotmp)
      return RC_NOT_AVAILABLE;
    return RC_UNSPECIFIED_ERROR;
  }
  //write the "command"
  size_t len = strlen(value);
  res = write(fd, value, len);

  //clean unnecessary resources
  close(fd);
  free(file);
  file = NULL;

  if (res != len)
  {
    return RC_UNSPECIFIED_ERROR;
  }

  return RC_OK;
}

static inline int check_io(lua_State* L, Gpio_t* io)
{
  if (io->fd < 0)
  {
    lua_pushnil(L);
    lua_pushstring(L, "File is closed");
    return 2;
  }

  return 0;
}

/*
 * Read function to be used to pushed value to Lua, after
 * a interrupt/exception was detected by sched.fd
 *
 * Must use the fd of the userdate registered in sched.fd to
 * clear the interrupt.
 *
 * @return 1 on success, new GPIO value pushed as an integer on Lua stack
 *         2 on error (nil, err msg pushed on Lua stack)
 */
static int l_readinterrupt(lua_State* L)
{
  Gpio_t* io = (Gpio_t*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status = check_io(L, io)) != 0)
    return status;

  //use GPIO userdata fd
  int fd = io->fd;

  int nb = lseek(fd, 0, SEEK_SET);
  if (nb)
  { //lseek must return 0, the requested position, otherwise exit
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
  }

  char val = 0;
  nb = read(fd, &val, 1);

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

  if (val == '0')
    lua_pushinteger(L, 0);
  else
    lua_pushinteger(L, 1);

//leave fd open on purpose, will be closed when gpio is not monitored anymore

  return 1;
}

/*
 * Method of GPIO object, so that it's possible to
 * plug it into sched.
 *
 * @param io gpio userdata
 *
 * @return integer, the file descriptor linked to this GPIO.
 * @return `nil` followed by an error message otherwise
 */
static int l_getfd(lua_State* L)
{
  Gpio_t* io = (Gpio_t*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status = check_io(L, io)) != 0)
    return status;

  lua_pushinteger(L, io->fd);
  return 1;
}

/*
 * Method of GPIO object, used in register/notify implementation
 * (made in Lua).
 *
 * @param io gpio userdata
 *
 * @return integer, the id of this GPIO.
 * @return `nil` followed by an error message otherwise
 */
static int l_getid(lua_State* L)
{
  Gpio_t* io = (Gpio_t*) luaL_checkudata(L, 1, MODULE_NAME);
  int status;

  if ((status = check_io(L, io)) != 0)
    return status;

  lua_pushinteger(L, io->id);
  return 1;
}

/*
 * Method of GPIO object, used to clean it when there is no more
 * reference on it.
 *
 * @param io gpio userdata to clean
 *
 * @return nothing
 */
static int l_gc(lua_State* L)
{
  Gpio_t* io = (Gpio_t*) luaL_checkudata(L, 1, MODULE_NAME);

  if (io->fd)
    close(io->fd);
  io->fd = -1;

  return 0;
}

/***************************************************************************
 Lists available GPIO on the system.

 The list contains the GPIOs ids that the system claims to be able to manage.
 The list content is not modified by calling other API. In particular, the
 list contains GPIOs that haven't been configured yet (using @{#gpio.configure} API).

 Beware, this doesn't mean that every GPIO in the list should be used by a user
 application, some can be already used by hardware drivers or other applications.
 Please check the device and the system documentation to get more precise info.


 @function [parent=#gpio] availablelist

 @return available GPIOs list when successful
 @return `nil` followed by an error message otherwise

 @usage
 local gpio = require"gpio"
 local res = gpio.availablelist()
 --On a Raspberry-Pi, res is likely to be:
 -- res = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53 }
 */

/***************************************************************************
 Lists enabled GPIOs.

 The list contains the GPIOs ids that have already been activated/configured.
 Using this library, once a GPIO have been activated/configured, it will remain
 "enabled" until system reboot.

 Beware, this doesn't mean that every GPIO in the list should be used by a user
 application, some can be already used by hardware drivers or other applications.
 Please check the device and the system documentation to get more precise info.


 @function [parent=#gpio] enabledlist

 @return available GPIOs list when successful
 @return `nil` followed by an error message otherwise

 @usage
 local gpio = require"gpio"
 gpio.read(4)
 local res = gpio.enabledlist()
 -- If no other application have been using GPIO since last system reboot
 -- then the is likely to be:
 -- res = { 4 }
 */

static const luaL_Reg R[] = {
        { "read", l_read },
        { "configure", l_configure },
        { "getconfig", l_getconfig },
        { "write", l_write },
        { "newgpio", l_newgpio }, //not part of the public API
        { "readinterrupt", l_readinterrupt }, //not part of the public API
        { NULL, NULL }
};

static const luaL_Reg module_functions[] = {
        { "getfd", l_getfd },
        { "getid", l_getid },
        { "__gc", l_gc },
        { NULL, NULL }
};

int luaopen_gpio_core(lua_State* L)
{

  luaL_register(L, "gpio.core", R); // m

  luaL_newmetatable(L, MODULE_NAME); // m, mt
  lua_pushvalue(L, -1); // m, mt, mt
  lua_setfield(L, -2, "__index"); // m, mt[__index=mt]
  luaL_register(L, NULL, module_functions); // m, mt[key=value*]
  lua_setfield(L, -2, "__metatable"); // m[__metatable=mt]
  return 1;
}

