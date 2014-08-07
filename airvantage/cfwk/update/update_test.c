/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "swi_update.h"
#include "swi_log.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "testutils.h"

static const char *lua_script =
"local sched = require 'sched'\n"
"local rpc = require 'rpc'\n"
"local os = require 'os'\n"
"function invoke(...)\n"
" local client = rpc.newclient(\"localhost\", 2012)\n"
" client.call(client, ...)\n"
" os.exit(0)\n"
"end\n"
"sched.run(invoke, 'agent.update.localupdate', '/tmp/update_package.tar.gz', false)\n"
"sched.loop()\n";


static const char *manifest_content =
"{\n"
"  version = \"1.1\",\n"
"\n"
"  components =\n"
"  {\n"
"    {   name = \"@sys.update.my_app\",\n"
"        location = \".\",\n"
"        depends = {},\n"
"        provides = { my_app=\"1.1\"},\n"
"        version = \"1.1\"\n"
"    }\n"
"  }\n"
"}\n";


static volatile uint8_t waiting_update_notification = 1;


static rc_ReturnCode_t statusNotification(swi_update_Notification_t* indPtr)
{

  SWI_LOG("UPDATE_TEST", INFO, "statusNotification: %p\n", indPtr->eventDetails);
  const char* details = indPtr->eventDetails != NULL ? indPtr->eventDetails : "";
  switch (indPtr->event)
  {
    case SWI_UPDATE_NEW:
      SWI_LOG("UPDATE_TEST", INFO, "new update\n");
      break;
      case SWI_UPDATE_DOWNLOAD_IN_PROGRESS:
      SWI_LOG("UPDATE_TEST", INFO, "download in progress: [%s]\n", details);
      break;
    case SWI_UPDATE_DOWNLOAD_OK:
      SWI_LOG("UPDATE_TEST", INFO, "download ok\n");
      break;
    case SWI_UPDATE_CRC_OK:
      SWI_LOG("UPDATE_TEST", INFO, "crc ok\n");

      //      swi_update_Request_t request = (count++)%2;
      //SWI_LOG("UPDATE_TEST", INFO,  "sending req:%d\n", request);
      //res = swi_update_Request(request);
      //SWI_LOG("UPDATE_TEST", INFO, "swi_update_Request: %d\n", res)
      break;
    case SWI_UPDATE_UPDATE_IN_PROGRESS:
      SWI_LOG("UPDATE_TEST", INFO, "update in progress for [%s]\n", details);
      break;
    case SWI_UPDATE_FAILED:
      //update finished, end test
      waiting_update_notification = 0;
      SWI_LOG("UPDATE_TEST", ERROR, "update failed: [%s]\n", details);
      break;
    case SWI_UPDATE_SUCCESSFUL:
      //update finished, end test
      waiting_update_notification = 0;
      SWI_LOG("UPDATE_TEST", INFO, "update successful: [%s]\n", details);
      break;
    case SWI_UPDATE_PAUSED:
      SWI_LOG("UPDATE_TEST", INFO, "update paused\n");
      break;
    default:
      //update event "error", end test
      waiting_update_notification = 0;
      SWI_LOG("UPDATE_TEST", WARNING, "unknown event: [%d]\n", indPtr->event);
      break;
  }

//  rc_ReturnCode_t res = swi_update_Request(SWI_UPDATE_REQ_RESUME);
//  SWI_LOG("UPDATE_TEST", INFO, "swi_update_Request: %d\n", res)

  return RC_OK;
}

DEFINE_TEST(test_update_Init)
{
  rc_ReturnCode_t res;

  res = swi_update_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_update_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_update_Init();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_update_Destroy)
{
  rc_ReturnCode_t res;

  res = swi_update_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_update_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

// swi_update_Request must be used when an update is in progress, so
// its test is likely to be put in  statusNotification callback test/use.

//DEFINE_TEST(test_update_Request)
//{
//  rc_ReturnCode_t res;
//
//  res = swi_update_Request(SWI_UPDATE_REQ_PAUSE);
//  ASSERT_TESTCASE_IS_OK(res);
//}

DEFINE_TEST(test_update_RegisterStatusNotification)
{
  rc_ReturnCode_t res;

  res = swi_update_RegisterStatusNotification(NULL);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_update_RegisterStatusNotification(statusNotification);
  ASSERT_TESTCASE_IS_OK(res);
}

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

static void generate_package()
{
  FILE *file;

  file = fopen("/tmp/Manifest", "w");
  if (file == NULL) {
    fprintf(stderr, "/tmp/Manifest: %s\n", strerror(errno));
    exit(1);
  }
  fwrite(manifest_content, 1, strlen(manifest_content), file);
  fclose(file);

  file = fopen("/tmp/install.lua", "w");
  if (file == NULL) {
    fprintf(stderr, "/tmp/install.lua: %s\n", strerror(errno));
    exit(1);
  }
  const char* script = "print 'install script for C update API unittest' ";
  fwrite(script, 1, strlen(script), file);
  fclose(file);

  int ret = system("cd /tmp; tar czpf update_package.tar.gz Manifest install.lua");
  if (ret == -1 || WEXITSTATUS(ret) != 0) {
    SWI_LOG("UPDATE_TEST", ERROR, "package packing internal error: %s\n", (ret == -1) ? strerror(errno) : "wrong status code");
    exit(1);
  }
}

int main(void)
{
  INIT_TEST("UPDATE_TEST");

  test_update_Init();
  //test_update_Request();
  test_update_RegisterStatusNotification();

  //waiting_update_notification is set to 0 at the end of an update and on update event error
  generate_package();
  exec_lua_code();
  while(waiting_update_notification)
    usleep(1000 * 100);
  test_update_Destroy();

  return 0;
}
