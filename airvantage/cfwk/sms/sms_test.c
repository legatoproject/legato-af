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
static swi_sms_regId_t regId;

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

DEFINE_TEST(test_sms_Init)
{
  rc_ReturnCode_t res;

  res = swi_sms_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_sms_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_sms_Init();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_sms_Destroy)
{
  rc_ReturnCode_t res;

  res = swi_sms_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_sms_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_sms_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

static void sms_Register(swi_sms_ReceptionCB_t callback, const char* senderPatternPtr,
                         const char* messagePatternPtr, swi_sms_regId_t *regIdPtr)
{
  rc_ReturnCode_t res;

  res = swi_sms_Register(callback, senderPatternPtr, messagePatternPtr, regIdPtr);
  ASSERT_TESTCASE_IS_OK(res);

  exec_lua_code();
  while(waiting_for_sms)
    ;
  waiting_for_sms = 1;
}

DEFINE_TEST(test_sms_Register_for_one)
{
    sms_Register((swi_sms_ReceptionCB_t)sms_handler, PHONE_NUMBER, MESSAGE, &regId);
}

DEFINE_TEST(test_sms_Register_for_all)
{
    sms_Register((swi_sms_ReceptionCB_t)sms_handler, NULL, NULL, &regId);
}

DEFINE_TEST(test_sms_Register_failure)
{
  rc_ReturnCode_t res;
  swi_sms_regId_t regId;

  res = swi_sms_Register((swi_sms_ReceptionCB_t)sms_handler, NULL, NULL, NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_sms_Register(NULL, NULL, NULL, &regId);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_sms_Register(NULL, NULL, NULL, NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
}

DEFINE_TEST(test_sms_Send)
{
  rc_ReturnCode_t res;

  res = swi_sms_Send(PHONE_NUMBER, MESSAGE, SWI_SMS_8BITS);
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_sms_Unregister)
{
  ASSERT_TESTCASE_IS_OK(swi_sms_Unregister(regId));
}

DEFINE_TEST(test_sms_Unregister_failure)
{
  rc_ReturnCode_t res;

  res = swi_sms_Unregister((swi_sms_regId_t)0);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_sms_Unregister((swi_sms_regId_t)-1);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_sms_Unregister((swi_sms_regId_t)-2);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
}

int main(void)
{
  INIT_TEST("SMS_TEST");
  test_sms_Init();

  test_sms_Register_for_one();
  test_sms_Unregister();

  test_sms_Register_for_all();
  test_sms_Unregister();

  test_sms_Register_failure();
  test_sms_Unregister_failure();

  test_sms_Send();
  test_sms_Destroy();
  return 0;
}
