/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "swi_sms.h"
#include "swi_log.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "testutils.h"

#define PHONE_NUMBER "33606060606"
#define MESSAGE "TEST MESSAGE"

static const char *lua_script =
"local sched = require 'sched'\n"
"local rpc = require 'rpc'\n"
"local os = require 'os'\n"
"function invoke(...)\n"
" local client = rpc.newclient()\n"
" client.call(client, ...)\n"
" os.exit(0)\n"
"end\n"
"sched.run(invoke, 'sched.signal', 'messaging', 'sms', {message=\"TEST MESSAGE\", address=\"33606060606\"})\n"
"sched.loop()\n";


static volatile uint8_t waiting_for_sms = 1;

static void exec_lua_code()
{
  lua_State* L;

  if (fork() != 0)
    return;
  L = lua_open();
  luaL_openlibs(L);
  (void)luaL_dostring(L, lua_script);
  lua_close(L);
}

static void sms_handler(const char *sender, const char *message)
{
  SWI_LOG("SMS_TEST", DEBUG, "%s: sender=%s, message=%s\n", __FUNCTION__, sender, message);
  if (!strcmp(sender, PHONE_NUMBER) && !strcmp(message, MESSAGE))
    {
      SWI_LOG("SMS_TEST", DEBUG, "%s: sms matched !\n", __FUNCTION__);
      waiting_for_sms = 0;
    }
}

static int test_sms_Init()
{
  rc_ReturnCode_t res;

  res = swi_sms_Init();
  if (res != RC_OK)
    return res;

  res = swi_sms_Init();
  if (res != RC_OK)
    return res;

  res = swi_sms_Init();
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static int test_sms_Destroy()
{
  rc_ReturnCode_t res;

  res = swi_sms_Destroy();
  if (res != RC_OK)
    return res;

  res = swi_sms_Destroy();
  if (res != RC_OK)
    return res;

  res = swi_sms_Destroy();
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static int test_sms_Register(swi_sms_ReceptionCB_t callback, const char* senderPatternPtr,
                 const char* messagePatternPtr, swi_sms_regId_t *regIdPtr)
{
  rc_ReturnCode_t res;

  res = swi_sms_Register(callback, senderPatternPtr, messagePatternPtr, regIdPtr);

  if (res != RC_OK)
    return res;

  exec_lua_code();
  while(waiting_for_sms)
    ;
  waiting_for_sms = 1;
  return RC_OK;
}

static int test_sms_Register_failure()
{
  rc_ReturnCode_t res;
  swi_sms_regId_t regId;

  res = swi_sms_Register((swi_sms_ReceptionCB_t)sms_handler, NULL, NULL, NULL);
  if (res != RC_BAD_PARAMETER)
    return res;

  res = swi_sms_Register(NULL, NULL, NULL, &regId);
  if (res != RC_BAD_PARAMETER)
    return res;

  res = swi_sms_Register(NULL, NULL, NULL, NULL);
  if (res != RC_BAD_PARAMETER)
    return res;
  return RC_OK;
}

static int test_sms_Send(const char *recipientPtr, const char* messagePtr, swi_sms_Format_t format)
{
  rc_ReturnCode_t res;

  res = swi_sms_Send(recipientPtr, messagePtr, format);
  return res;
}

static int test_sms_Unregister(swi_sms_regId_t regId)
{
  return swi_sms_Unregister(regId);
}

static int test_sms_Unregister_failure()
{
  rc_ReturnCode_t res;

  res = test_sms_Unregister((swi_sms_regId_t)0);
  if (res != RC_BAD_PARAMETER)
    return res;

  res = test_sms_Unregister((swi_sms_regId_t)-1);
   if (res != RC_BAD_PARAMETER)
    return res;

  res = test_sms_Unregister((swi_sms_regId_t)-2);
   if (res != RC_BAD_PARAMETER)
     return res;
   return RC_OK;
}

int main(void)
{
  swi_sms_regId_t regId;

  INIT_TEST("SMS_TEST");
  CHECK_TEST(test_sms_Init());

  CHECK_TEST(test_sms_Register((swi_sms_ReceptionCB_t)sms_handler, PHONE_NUMBER, MESSAGE, &regId));
  CHECK_TEST(test_sms_Unregister(regId));

  CHECK_TEST(test_sms_Register((swi_sms_ReceptionCB_t)sms_handler, NULL, NULL, &regId));
  CHECK_TEST(test_sms_Unregister(regId));

  CHECK_TEST(test_sms_Register_failure());
  CHECK_TEST(test_sms_Unregister_failure());

  CHECK_TEST(test_sms_Send(PHONE_NUMBER, MESSAGE, SWI_SMS_8BITS));
  CHECK_TEST(test_sms_Destroy());
  return 0;
}
