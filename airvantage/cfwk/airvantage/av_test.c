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
 *******************************************************************************/

#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include "swi_log.h"
#include "swi_airvantage.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "testutils.h"

#define ASSET_ID "av_test_asset_id"
#define NUMBER_MAX_ID 10000
#define SIZE_ALLOC 5 /* size allocated for id number */
static volatile char waiting_notification = 1;
static int result = 1;

rc_ReturnCode_t get_path_element(int first, const char* pathPtr, char** remainingPath, char** varName);

/* add random id number in to str: */
char* addTicketId(const char* str)
{
  sleep(1);  /*delay 1 second for random id number*/
  srand ( time(NULL) );
  int size = strlen(str);
  char *cmd = malloc(sizeof(char)*size + SIZE_ALLOC + 1);
  if(!cmd)
  {
    printf("av_test : Malloc cmd failed \n");
    return NULL;
  }
  unsigned int id = rand() % NUMBER_MAX_ID;  /*generate random id number between 0-NUMBER_MAX_ID*/
  sprintf(cmd, str, id);
  return cmd;
}

static void exec_lua_code(const char* cmd)
{
  lua_State* L;
  const char* str1 =
      "local sched = require 'sched'\n"
      "local rpc = require 'rpc'\n"
      "local os = require 'os'\n"
      "function invoke(...)\n"
      "local client = rpc.newclient(\"localhost\", 2012)\n"
      "client.call(client, ...)\n"
      "os.exit(0)\n"
      "end\n"
      "sched.run(invoke, 'agent.asscon.sendcmd', 'av_test_asset_id', ";
  const char* str3 = "sched.loop()\n";

  /*concatenate str1 + cmd + str2 in other to create lua_script*/
  int size = strlen(str1) + strlen(cmd) + strlen(str3);
  char* lua_script = malloc(sizeof(char)*size+1);
  if(!lua_script)
  {
    printf("av_test : Malloc lua_script failed \n");
    return;
  }
  strcpy(lua_script, str1);
  strcat(lua_script, cmd);
  strcat(lua_script, str3);

  if (fork() != 0)
  {
    //parent process, just return now
    return;
  }
  //child process: execute the lua script
  L = lua_open();
  luaL_openlibs(L);
  int res = luaL_dostring(L, (const char*)lua_script);
  if (res)
    printf("exec_lua_code failed: %d\n", res);

  lua_close(L);
  free(lua_script);
  exit(0);
}

//sendDataCommandMap: not used for now
//"sched.run(invoke, 'agent.asscon.sendcmd', assetID, 'SendData', { Path = 'av_test_asset_id.sub.path', Body = { Command = 'plop',  Args = {foo = 'bar'}, __class = 'AWT-DA::Command' },  TicketId = 75, Type = 2,  __class = 'AWT-DA::Message' })\n"


static void updateNotificationCb(swi_av_Asset_t *asset, const char* componentNamePtr, const char* versionPtr,
    const char *updateFilePathPtr, swi_dset_Iterator_t* customParams, void *userDataPtr)
{
  if (strcmp("my_pkg", componentNamePtr) || strcmp("my_version", versionPtr)
      || strcmp(updateFilePathPtr, "/toto/my_file") || strcmp((const char *) userDataPtr, "userData"))
  {
    result = 81;
    waiting_notification = 0;
    return;
  }

  if (!customParams)
  {
    result = 82;
    waiting_notification = 0;
    return;
  }
  rc_ReturnCode_t res;

  /*check received values*/
  double float_val = 0;
  res = swi_dset_GetFloatByName(customParams, "float", &float_val);
  if (res != RC_OK || float_val != 0.23)
  {
    result = 83;
    waiting_notification = 0;
    return;
  }

  const char* string_val = NULL;
  res = swi_dset_GetStringByName(customParams, "foo", &string_val);
  if (res != RC_OK || strcmp(string_val, "bar"))
  {
    result = 83;
    waiting_notification = 0;
    return;
  }

  int64_t int_val = 0;
  res = swi_dset_GetIntegerByName(customParams, "num", &int_val);
  if (res != RC_OK || int_val != 42)
  {
    result = 83;
    waiting_notification = 0;
    return;
  }

  result = 0;
  waiting_notification = 0;
}

static void dwcb_DataWritting(swi_av_Asset_t *asset, ///< [IN] the asset receiving the data
    const char *pathPtr, ///< [IN] the path targeted by the data sent by the server.
    swi_dset_Iterator_t* data, ///< [IN] the data iterator containing the received data.
                               ///<      The data contained in the iterator will be automatically released when the callback returns.
    int ack_id,                              ///< [IN] the id to be used to acknowledge the received data.
                                             ///<      If ack_id=0 then there is no need to acknowledge.
    void *userDataPtr)
{
  SWI_LOG("AV_TEST", DEBUG, "dwcb_DataWritting: pathPtr=%s, ack_id=%d\n", pathPtr, ack_id);

  if (strcmp(pathPtr, "sub.path"))
  {
    result = 102;
    waiting_notification = 0;
    return;
  }

  if (data == NULL )
  {
    result = 104;
    waiting_notification = 0;
    return;
  }
  char *val1 = NULL;

  if (RC_OK != swi_dset_GetStringByName(data, "foo", (const char**) &val1))
  {
    result = 105;
    waiting_notification = 0;
    return;
  }

  if (val1 == NULL || strcmp(val1, "bar"))
  {
    result = 106;
    waiting_notification = 0;
    return;
  }

  if (ack_id)
    swi_av_Acknowledge(ack_id, 42, "some error msg", "now", 0);

  swi_dset_Destroy(data);
  result = 0;
  waiting_notification = 0;
  return;
}

static void dwcb_DataWrittingList(swi_av_Asset_t *asset, ///< [IN] the asset receiving the data
    const char *pathPtr, ///< [IN] the path targeted by the data sent by the server.
    swi_dset_Iterator_t* data, ///< [IN] the data iterator containing the received data.
                               ///<      The data contained in the iterator will be automatically released when the callback returns.
    int ack_id,                              ///< [IN] the id to be used to acknowledge the received data.
                                             ///<      If ack_id=0 then there is no need to acknowledge.
    void *userDataPtr)
{
  SWI_LOG("AV_TEST", DEBUG, "dwcb_DataWrittingList: pathPtr=%s, ack_id=%d\n", pathPtr, ack_id);

  if (strcmp(pathPtr, "sub.path"))
  {
    result = 112;
    waiting_notification = 0;
    return;
  }

  if (data == NULL )
  {
    result = 114;
    waiting_notification = 0;
    return;
  }

  rc_ReturnCode_t res = RC_OK;
  bool valueOK = true;
  res = swi_dset_Next(data);
  while (res == RC_OK && valueOK)
  {
    swi_dset_Type_t type = swi_dset_GetType(data);

    switch (type)
    {
      case SWI_DSET_INTEGER:
        if (swi_dset_ToInteger(data) != 42)
          valueOK = false;
        break;

      case SWI_DSET_STRING:
        if (strcmp(swi_dset_ToString(data), "bar"))
          valueOK = false;
        break;

      default:
        break;
    }
    res = swi_dset_Next(data);
  }

  if (!valueOK)
  {
    result = 125;
    waiting_notification = 0;
    return;
  }

  if (ack_id)
    swi_av_Acknowledge(ack_id, 42, "some error msg", "now", 0);

  swi_dset_Destroy(data);
  result = 0;
  waiting_notification = 0;
  return;
}

static void dwcb_DataCommand(swi_av_Asset_t *asset, ///< [IN] the asset receiving the data
    const char *pathPtr, ///< [IN] the path targeted by the data sent by the server.
    swi_dset_Iterator_t* data, ///< [IN] the data iterator containing the received data.
                               ///<      The data contained in the iterator will be automatically released when the callback returns.
    int ack_id,                              ///< [IN] the id to be used to acknowledge the received data.
                                             ///<      If ack_id=0 then there is no need to acknowledge.
    void *userDataPtr)
{
  SWI_LOG("AV_TEST", DEBUG, "dwcb_DataCommand: pathPtr=%s, ack_id=%d\n", pathPtr, ack_id);
  //init result
  result = 0;

  if (strcmp(pathPtr, "commands.avTestCommand"))
  {
    result = 122;
    waiting_notification = 0;
    return;
  }

  if (data == NULL )
  {
    result = 124;
    waiting_notification = 0;
    return;
  }


  rc_ReturnCode_t res = RC_OK;
  int value1 = -1, value2 = -1, pname1 = -1, pname2 = -1; //init at -1 meaning: not received

  res = swi_dset_Next(data);


  while (res == RC_OK && result == 0)
  {
    swi_dset_Type_t type = swi_dset_GetType(data);

    switch (type)
    {
      case SWI_DSET_INTEGER:
        if (strcmp(swi_dset_GetName(data), "param1"))
          result = 125;
        else
          pname1 = 0;
        if (swi_dset_ToInteger(data) != 42)
          result = 126;
        else
          value1 =  0;
        break;
      case SWI_DSET_STRING:
        if (strcmp(swi_dset_GetName(data), "param2"))
          result = 127;
        else
          pname2 = 0;
        if (strcmp(swi_dset_ToString(data), "bar"))
          result = 128;
        else
          value2 = 0;
        break;
      default:
        result = 129; //unexpected data!
        break;
    }
    res = swi_dset_Next(data);
  }

  //check all param were received
  if ( result == 0 && (value1 == -1 || value2 == -1 || pname1 == -1 || pname2 == -1)){
    result = 130;
    SWI_LOG("AV_TEST", ERROR, "at least one value was missing (i.e. -1): value1 = %d, value2 = %d, pname1 = %d, pname2 = %d\n", value1, value2, pname1, pname2);
  }

  if (ack_id)
    swi_av_Acknowledge(ack_id, result, "some error msg", "now", 0);

  swi_dset_Destroy(data);
  waiting_notification = 0;
  return;
}

DEFINE_TEST(test_1_Init_Destroy)
{
  rc_ReturnCode_t res;

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_2_TriggerPolicy)
{
  rc_ReturnCode_t res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  // trigger default policy
  res = swi_av_TriggerPolicy(NULL);
  ASSERT_TESTCASE_IS_OK(res);

  // trigger one existing policy
  res = swi_av_TriggerPolicy("now");
  ASSERT_TESTCASE_IS_OK(res);

  // trigger "never" policy: this must fail
  res = swi_av_TriggerPolicy("never");
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  // test using unknown policy
  res = swi_av_TriggerPolicy("plop");
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_3_ConnectToServer)
{
  rc_ReturnCode_t res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

//test using requesting SYNC connexion
  res = swi_av_ConnectToServer(SWI_AV_CX_SYNC);
  ASSERT_TESTCASE_IS_OK(res);

  SWI_LOG("AV_TEST", DEBUG, "sync done\n");

//test using 0 latency: async but "immediate" connection
  res = swi_av_ConnectToServer(0);
  ASSERT_TESTCASE_IS_OK(res);

//test using correct latency
  res = swi_av_ConnectToServer(10);
  ASSERT_TESTCASE_IS_OK(res);

//test using too big latency:
//expected behavior here: rejected
  res = swi_av_ConnectToServer((unsigned int) INT_MAX + 1);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_4_asset_Create_Start_Destroy)
{
  rc_ReturnCode_t res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  swi_av_Asset_t* asset;
  /*
   res = swi_av_asset_Create(&asset, NULL);
   ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

   res = swi_av_asset_Create(NULL, "test_asset");
   ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
   */
  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(NULL );
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Destroy(NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_5_asset_pushData)
{

  rc_ReturnCode_t res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  swi_av_Asset_t* asset;

  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

//"long" path
  res = swi_av_asset_PushInteger(asset, "titi.test.toto1", "now", SWI_AV_TSTAMP_AUTO, 42);
  ASSERT_TESTCASE_IS_OK(res);

//"short" path
  res = swi_av_asset_PushInteger(asset, "titi.toto2", "now", SWI_AV_TSTAMP_AUTO, 43);
  ASSERT_TESTCASE_IS_OK(res);

//"shortest" path
  res = swi_av_asset_PushInteger(asset, "toto3", "now", SWI_AV_TSTAMP_AUTO, 44);
  ASSERT_TESTCASE_IS_OK(res);

//"shortest" path, no timestamp
  res = swi_av_asset_PushInteger(asset, "toto4", "now", SWI_AV_TSTAMP_NO, 45);
  ASSERT_TESTCASE_IS_OK(res);

//"shortest" path, no timestamp, no policy
  res = swi_av_asset_PushInteger(asset, "toto5", NULL, SWI_AV_TSTAMP_AUTO, 46);
  ASSERT_TESTCASE_IS_OK(res);

//"shortest" path, manual timestamp, no policy
  res = swi_av_asset_PushInteger(asset, "toto6", NULL, 23, 47);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_PushFloat(asset, "toto7", "now", SWI_AV_TSTAMP_AUTO, 47.455555);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_PushString(asset, "toto8", "now", SWI_AV_TSTAMP_AUTO, "foo");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_PushString(asset, "toto8", "now", SWI_AV_TSTAMP_AUTO, NULL );
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_asset_PushString(asset, "toto8", "plop", SWI_AV_TSTAMP_AUTO, "foo");
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_av_TriggerPolicy("*");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_6_Acknowledge)
{
  rc_ReturnCode_t res;

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Acknowledge(0, 0, "BANG BANG BANG", "now", 0);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_TriggerPolicy("now");
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_7_path_utils)
{

  rc_ReturnCode_t res = RC_OK;

  char* var = NULL;
  char *remain = NULL;
  const char* path = "toto.titi.tata";
  res = get_path_element(0, path, &remain, &var);
  SWI_LOG("AV_TEST", DEBUG, "last: var=[%s], remain=[%s]\n", var, remain);
  ASSERT_TESTCASE_IS_OK(res);

  if (strcmp("tata", var))
    ABORT("Invalid var content");
  if (strcmp("toto.titi", remain))
    ABORT("Invalid remain content");

  if (var)
    free(var);
  if (remain)
    free(remain);

  res = get_path_element(1, path, &remain, &var);
  SWI_LOG("AV_TEST", DEBUG, "first: var=[%s], remain=[%s]\n", var, remain);
  ASSERT_TESTCASE_IS_OK(res);

  if (strcmp("titi.tata", remain))
    ABORT("Invalid remain content");
  if (strcmp("toto", var))
    ABORT("Invalid var content");

  if (var)
    free(var);
  if (remain)
    free(remain);

  path = "foobarfoobar";

  res = get_path_element(1, path, &remain, &var);
  SWI_LOG("AV_TEST", DEBUG, "first: var=[%s], remain=[%s]\n", var, remain);
  ASSERT_TESTCASE_IS_OK(res);

  if (strcmp("", remain))
    ABORT("Invalid remain content");
  if (strcmp("foobarfoobar", var))
    ABORT("Invalid var content");

  if (var)
    free(var);
  if (remain)
    free(remain);

  res = get_path_element(0, path, &remain, &var);
  SWI_LOG("AV_TEST", DEBUG, "last: var=[%s], remain=[%s]\n", var, remain);
  ASSERT_TESTCASE_IS_OK(res);

  if (strcmp("", remain))
    ABORT("Invalid remain content");
  if (strcmp("foobarfoobar", var))
    ABORT("Invalid var content");

  if (var)
    free(var);
  if (remain)
    free(remain);

//todo test cas with bad content in path: .toto, toto., ttu..titi, etc
}

DEFINE_TEST(test_8_UpdateNotification)
{
  rc_ReturnCode_t res;
  swi_av_Asset_t* asset = NULL;
  waiting_notification = 1;

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_RegisterUpdateNotification(asset, (swi_av_updateNotificationCB) updateNotificationCb, "userData");
  ASSERT_TESTCASE_IS_OK(res);

  const char *cmd_SoftwareUpdate ="'SoftwareUpdate', { 'av_test_asset_id.my_pkg', 'my_version', '/toto/my_file', {foo='bar', num=42, float=0.23}})\n";
  exec_lua_code(cmd_SoftwareUpdate);
  SWI_LOG("AV_TEST", DEBUG, "exec_lua_code SoftwareUpdate done\n");
  while (waiting_notification)
    ;

  swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_9_TableManipulation)
{
  rc_ReturnCode_t res;
  swi_av_Asset_t* asset = NULL;
  swi_av_Table_t *table = NULL;
  const char *columns[] = { "column1", "column2", "column3" };

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_Create(asset, &table, "test", 3, columns, "now", STORAGE_RAM, 0);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_PushInteger(table, 1234);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_PushFloat(table, 1234.1234);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_PushString(table, "test");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_PushString(table, "fake push");
  ASSERT_TESTCASE_EQUAL(RC_OUT_OF_RANGE, res);

  res = swi_av_table_PushRow(table);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_table_Destroy(table);
  ASSERT_TESTCASE_IS_OK(res);

  swi_av_asset_Destroy(asset);
}

DEFINE_TEST(test_10_asset_receiveDataWriting)
{
  rc_ReturnCode_t res = RC_OK;
  waiting_notification = 1;

  res = swi_av_Init();
   ASSERT_TESTCASE_IS_OK(res);

  swi_av_Asset_t* asset;

  res = swi_av_asset_Create(&asset, ASSET_ID);
   ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_RegisterDataWrite(asset, dwcb_DataWritting, NULL );
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  /*command sends to av_test_asset_id asset*/
  const char* str = "'SendData', { path = 'av_test_asset_id.sub.path', body = { foo = 'bar' }, ticketid = %u, Type = 5, __class = 'Message' })\n";
  char* cmd_SendDataWriting = addTicketId(str);

  exec_lua_code(cmd_SendDataWriting);
  SWI_LOG("AV_TEST", DEBUG, "exec_lua_code SendDataWriting done\n");
  while (waiting_notification)
    ;

  res = swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  free(cmd_SendDataWriting);
}

DEFINE_TEST(test_11_asset_receiveDataWritingList)
{
  rc_ReturnCode_t res = RC_OK;
  waiting_notification = 1;

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  swi_av_Asset_t* asset;

  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_RegisterDataWrite(asset, dwcb_DataWrittingList, NULL );
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  const char* str = "'SendData', { path = 'av_test_asset_id.sub.path', body = { 42, 'bar' }, ticketid = %u, Type = 5, __class = 'Message' })\n";
  char* cmd_SendDataWrittingList = addTicketId(str);

  exec_lua_code(cmd_SendDataWrittingList);
  SWI_LOG("AV_TEST", DEBUG, "exec_lua_code SendDataWrittingList done\n");
  while (waiting_notification)
    ;

  res = swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  free(cmd_SendDataWrittingList);
}

DEFINE_TEST(test_12_asset_receiveDataCommandList)
{
  rc_ReturnCode_t res = RC_OK;
  waiting_notification = 1;

  res = swi_av_Init();
  ASSERT_TESTCASE_IS_OK(res);

  swi_av_Asset_t* asset;

  res = swi_av_asset_Create(&asset, ASSET_ID);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_RegisterDataWrite(asset, dwcb_DataCommand, NULL );
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_asset_Start(asset);
  ASSERT_TESTCASE_IS_OK(res);

  const char* str = "'SendData', { path = 'av_test_asset_id.commands.avTestCommand', body = { param1=42, param2='bar' } ,  ticketid = %u, Type = 2,  __class = 'Message' })\n";
  char* cmd_SendDataCommandList = addTicketId(str);

  exec_lua_code(cmd_SendDataCommandList);
  SWI_LOG("AV_TEST", DEBUG, "exec_lua_code SendDataCommandList done\n");
  while (waiting_notification)
    ;

  res = swi_av_asset_Destroy(asset);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_av_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  free(cmd_SendDataCommandList);
}

int main(int argc, char *argv[])
{
  INIT_TEST("AV_TEST");

  test_1_Init_Destroy();
  test_2_TriggerPolicy();
  test_3_ConnectToServer();
  test_4_asset_Create_Start_Destroy();
  test_5_asset_pushData();
  test_6_Acknowledge();
  test_7_path_utils();
  test_8_UpdateNotification();
  test_9_TableManipulation();
  test_10_asset_receiveDataWriting();
  test_11_asset_receiveDataWritingList();
  test_12_asset_receiveDataCommandList();


//preventive clean if one previous test failed
//  swi_av_Destroy();

  return 0;
}
