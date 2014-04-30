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
#include "swi_devicetree.h"
#include "swi_dset.h"
#include "swi_log.h"
#include "testutils.h"

static char waitingForNotification = 1;
static swi_dt_regId_t regId;
static void test_dt_Get();
static void test_dt_MultipleGet();

static rc_ReturnCode_t notificationCb(swi_dset_Iterator_t *data)
{
  swi_dset_Iterator_t *set = NULL;
  rc_ReturnCode_t res;

  waitingForNotification = 0;
  res = swi_dt_Get("config.server.serverId", &set, NULL);
  ASSERT_TESTCASE_IS_OK(res);
  return RC_OK;
}

DEFINE_TEST(test_dt_Init)
{
  rc_ReturnCode_t res;

  res = swi_dt_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_Init();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_Init();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_dt_Destroy)
{
  rc_ReturnCode_t res;
  
  res = swi_dt_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
  
  res = swi_dt_Destroy();
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_Destroy();
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_dt_Set)
{
  rc_ReturnCode_t res;

  res = swi_dt_SetString("config.toto", "toto");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetString("config.tata", "tataw");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetString("config.tata", "tata");
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetString(NULL, "tata");
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  // On the tree handler side, adding new leaf dynamically with an hw path as 
  // argument is not possible by default
  res = swi_dt_SetString("treehdlsample.666", "dummy_value");
  ASSERT_TESTCASE_EQUAL(RC_NOT_FOUND, res);

  // This is not permitted to change the type of a static value (declared inside the .map)
  res = swi_dt_SetString("treehdlsample.int_value", "dummy_value");
  ASSERT_TESTCASE_EQUAL(RC_NOT_PERMITTED, res);

  // On extvars side, adding new leaf dynamically with a logical path is not possible by default.
  // The treemgr does not find the corresponding hw path, which should be an interger for C handlers, 
  // this is why error "BAD_PARAMETER" is thrown.
  res = swi_dt_SetString("treehdlsample.dummy_leaf", "dummy_value");
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
}


union t_value{
  int64_t ival;
  double dval;
  char* sval;
  bool bval;
};

static void getLeafCheckTypeValue(const char* getpath, swi_dset_Type_t gettype, union t_value tval)
{
  swi_dset_Iterator_t *set = NULL;
  bool isLeaf = true;
  swi_dset_Type_t t;
  SWI_LOG("DT_TEST", DEBUG, "get on %s\n", getpath);
  rc_ReturnCode_t res = swi_dt_Get(getpath, &set, &isLeaf);
  ASSERT_TESTCASE_IS_OK(res);
  if (isLeaf == false)
    ABORT("Leaf was expected here");

  res = swi_dset_Next(set);
  ASSERT_TESTCASE_IS_OK(res);
  t = swi_dset_GetType(set);
  if(gettype != t)
    ABORT("getLeafCheckTypeValue: unexpected type [%d] for path [%s], expected [%d]", t, getpath, gettype);
  int64_t ival = 0;
  double dval = 0;
  bool bval = false;
  const char* sval;
  switch(t){
    case SWI_DSET_STRING:
      sval = swi_dset_ToString(set);
      if(strcmp(sval, tval.sval) != 0)
        ABORT("getLeafCheckTypeValue: unexpected value [%s] for path [%s], expected [%s]", sval, getpath, tval.sval );
      break;
    case SWI_DSET_INTEGER:
      ival = swi_dset_ToInteger(set);
      if(ival != tval.ival)
        ABORT("getLeafCheckTypeValue: unexpected value [%d] for path [%s], expected [%d]", ival, getpath, tval.ival );
      break;
    case SWI_DSET_FLOAT:
      dval = swi_dset_ToFloat(set);
      if(dval != tval.dval )
        ABORT("getLeafCheckTypeValue: unexpected value [%f] for path [%s], expected [%f]", dval, getpath, tval.dval );
      break;
    case SWI_DSET_BOOL:
      bval = swi_dset_ToBool(set);
      if(bval != tval.bval)
        ABORT("getLeafCheckTypeValue: unexpected value [%d] for path [%s], expected [%d]", bval, getpath, tval.bval );
      break;
    case SWI_DSET_NIL:
      if(tval.sval != NULL)//not sure how to have a variable of NIL type as a result of a Get request.
        ABORT("getLeafCheckTypeValue: received type SWI_DSET_NIL, with expected value was not NULL");
      break;
    default:
      ABORT("Unexpected swi_dset_Type_t [%d] for path [%s]", t, getpath);
  }

  swi_dset_Destroy(set);
}

DEFINE_TEST(test_dt_Get)
{
  rc_ReturnCode_t res;
  swi_dset_Iterator_t *set = NULL;
  bool isLeaf = true;

  res = swi_dt_Get("config.toto", &set, NULL);
  ASSERT_TESTCASE_IS_OK(res);

  swi_dset_Next(set);
  if (swi_dset_GetType(set) != SWI_DSET_STRING)
    ABORT("Incorrect variable type in data set");
  if (strcmp(swi_dset_ToString(set), "toto") != 0)
    ABORT("Incorrect variable value in data set: %s", swi_dset_ToString(set));
  swi_dset_Destroy(set);

  res = swi_dt_Get("config.tata", &set, NULL);
  ASSERT_TESTCASE_IS_OK(res);

  swi_dset_Next(set);
  if (swi_dset_GetType(set) != SWI_DSET_STRING)
    ABORT("Incorrect variable type in data set");
  if (strcmp(swi_dset_ToString(set), "tata") != 0)
    ABORT("Incorrect variable value in data set : %s", swi_dset_ToString(set));
  swi_dset_Destroy(set);

  res = swi_dt_Get("config", &set, &isLeaf);
  swi_dset_Destroy(set);
  ASSERT_TESTCASE_IS_OK(res);
  if (isLeaf == true)
    ABORT("Leaf was unexpected here");

  res = swi_dt_Get("config.agent.deviceId", &set, &isLeaf);
  ASSERT_TESTCASE_IS_OK(res);
  if(isLeaf == false)
    ABORT("Leaf was expected here");

  res = swi_dset_Next(set);
  swi_dset_Type_t t = swi_dset_GetType(set);
  swi_dset_Destroy(set);
  ASSERT_TESTCASE_IS_OK(res);
  if (t != SWI_DSET_STRING)
    ABORT("Incorrect variable type in data set");

  //Var types tests
  union t_value tval;
  //Int
  tval.ival = 9999;
  getLeafCheckTypeValue("config.agent.assetport", SWI_DSET_INTEGER, tval);
  //Float
  //no float in defaultconfig; set one
  tval.dval = 3.1416;
  const char* floatpath = "config.DT_test_float";
  res = swi_dt_SetFloat(floatpath, tval.dval);
  ASSERT_TESTCASE_IS_OK(res);
  getLeafCheckTypeValue(floatpath, SWI_DSET_FLOAT, tval);
  //Bool
  //no bool in defaultconfig; set one
  tval.bval = true;
  const char* boolpath = "config.DT_test_bool";
  res = swi_dt_SetBool(boolpath, tval.bval);
  ASSERT_TESTCASE_IS_OK(res);
  getLeafCheckTypeValue(boolpath, SWI_DSET_BOOL, tval);
  //String
  tval.sval = "AIRVANTAGE";
  getLeafCheckTypeValue("config.server.serverId", SWI_DSET_STRING, tval);

  //Error cases:
  res = swi_dt_Get("unexisting_node", &set, NULL);
  ASSERT_TESTCASE_EQUAL(RC_NOT_FOUND, res);
  swi_dset_Destroy(set);

  res = swi_dt_Get(NULL, &set, NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_dt_Get(NULL, NULL, NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
}

DEFINE_TEST(test_dt_MultipleGet)
{
  rc_ReturnCode_t res;
  swi_dset_Iterator_t *set;
  const char * pathPtr[] = {
    "config.toto",
    "config",
    "config.tata",
    NULL
  };

  res = swi_dt_MultipleGet(3, pathPtr, &set);
  ASSERT_TESTCASE_IS_OK(res);

  swi_dset_Next(set);
  if (swi_dset_GetType(set) != SWI_DSET_STRING)
    ABORT("Incorrect variable type in data set");
  if (strcmp(swi_dset_ToString(set), "toto") != 0)
    ABORT("Incorrect variable value in data set: %s", swi_dset_ToString(set));

  swi_dset_Next(set);
  if (swi_dset_GetType(set) != SWI_DSET_STRING)
    ABORT("Incorrect variable type in data set");
  if (strcmp(swi_dset_ToString(set), "tata") != 0)
    ABORT("Incorrect variable value in data set: %s", swi_dset_ToString(set));

  res = swi_dset_Next(set);
  ASSERT_TESTCASE_EQUAL(RC_NOT_FOUND, res);
  swi_dset_Destroy(set);

  res = swi_dt_MultipleGet(0, pathPtr, &set);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_dt_MultipleGet(3, NULL, &set);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_dt_MultipleGet(0, NULL, &set);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
}

DEFINE_TEST(test_dt_SetTypes)
{
  rc_ReturnCode_t res;

  res = swi_dt_SetInteger("config.toto", 0xdeadbeef);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetFloat("config.toto", 666.666);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetBool("config.toto", false);
  ASSERT_TESTCASE_IS_OK(res);

  res = swi_dt_SetNull("config.toto");
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_dt_Register)
{
  rc_ReturnCode_t res;
  const char * regVarsPtr[] = {
    "config.toto",
    "config.tata",
    NULL
  };

  res = swi_dt_Register(0, regVarsPtr, (swi_dt_NotifyCB_t) notificationCb, 0, NULL, &regId);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
  res = swi_dt_Register(2, NULL, (swi_dt_NotifyCB_t) notificationCb, 0, NULL, &regId);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
  res = swi_dt_Register(2, regVarsPtr, NULL, 0, NULL, &regId);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
  res = swi_dt_Register(2, regVarsPtr, (swi_dt_NotifyCB_t) notificationCb, 1, NULL, &regId);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);
  res = swi_dt_Register(2, regVarsPtr, (swi_dt_NotifyCB_t) notificationCb, 0, NULL, NULL);
  ASSERT_TESTCASE_EQUAL(RC_BAD_PARAMETER, res);

  res = swi_dt_Register(2, regVarsPtr, (swi_dt_NotifyCB_t)notificationCb, 0, NULL, &regId);
  ASSERT_TESTCASE_IS_OK(res);
}

DEFINE_TEST(test_dt_Unregister)
{
  rc_ReturnCode_t res;

  res = swi_dt_Unregister(regId);
  ASSERT_TESTCASE_IS_OK(res);
}

int main(void)
{
  swi_dt_regId_t regId = NULL;

  INIT_TEST("DT_TEST");

  test_dt_Init();

  test_dt_Register();
  test_dt_Set();

  while(waitingForNotification)
    ;

  test_dt_Get();
  test_dt_MultipleGet();
  test_dt_Unregister();
  test_dt_SetTypes();
  test_dt_Destroy();
  return 0;
}
