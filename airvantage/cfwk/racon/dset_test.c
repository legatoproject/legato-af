/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "swi_dset.h"
#include "dset_internal.h"
#include "testutils.h"
#include <string.h>
#include <inttypes.h>

#define DSET_TEST_DBG 1

int test_1_Init_Destroy()
{
  rc_ReturnCode_t res;
  swi_dset_Iterator_t* set = NULL;
  res = swi_dset_Create(&set);
  if (res != RC_OK)
    return 1;

  res = swi_dset_Destroy(set);
  if (res != RC_OK)
    return 1;

  return 0;
}

int test_2_Adding_Elements()
{
  rc_ReturnCode_t res;
  swi_dset_Iterator_t* set = NULL;
  res = swi_dset_Create(&set);
  if (res != RC_OK)
    return 1;

  //valid value 1
  res = swi_dset_PushInteger(set, "int", strlen("int"), 42);
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushInteger(set, NULL, 10, 32);
  if (res != RC_BAD_PARAMETER)
    return 1;

  res = swi_dset_PushInteger(set, "", 0, 32);
  if (res != RC_BAD_PARAMETER)
    return 1;

  res = swi_dset_PushInteger(NULL, "toto", strlen("toto"), 32);
  if (res != RC_BAD_FORMAT)
    return 1;

  //valid value 2
  res = swi_dset_PushFloat(set, "float", strlen("float"), 0.42);
  if (res != RC_OK)
    return 1;
  //valid value 3
  res = swi_dset_PushString(set, "str", strlen("str"), "funfunfun", strlen("funfunfun"));
  if (res != RC_OK)
    return 1;
  //valid value 4
  res = swi_dset_PushUnsupported(set, "unsupported stuff", strlen("unsupported stuff"));
  if (res != RC_OK)
    return 1;
  //valid value 5
  res = swi_dset_PushNull(set, "nuuuull", strlen("nuuuull"));
  if (res != RC_OK)
    return 1;

  //read values

  //valid value 1
  res = swi_dset_Next(set);
  if (res != RC_OK)
    return 1;
  if (strcmp(swi_dset_GetName(set), "int"))
    return 1;
  if (swi_dset_GetType(set) != SWI_DSET_INTEGER)
    return 1;
  if (swi_dset_ToInteger(set) != 42)
    return 1;
  //valid value 2
  res = swi_dset_Next(set);
  if (res != RC_OK)
    return 1;
  if (strcmp(swi_dset_GetName(set), "float"))
    return 1;
  if (swi_dset_GetType(set) != SWI_DSET_FLOAT)
    return 1;
  if (swi_dset_ToFloat(set) != 0.42)
    return 1;
  //valid value 3
  res = swi_dset_Next(set);
  if (res != RC_OK)
    return 1;
  if (strcmp(swi_dset_GetName(set), "str"))
    return 1;
  if (swi_dset_GetType(set) != SWI_DSET_STRING)
    return 1;
  if (strcmp(swi_dset_ToString(set), "funfunfun"))
    return 1;
  //valid value 4
  res = swi_dset_Next(set);
  if (res != RC_OK)
    return 1;
  if (strcmp(swi_dset_GetName(set), "unsupported stuff"))
    return 1;
  if (swi_dset_GetType(set) != SWI_DSET_UNSUPPORTED)
    return 1;
  //valid value 5
  res = swi_dset_Next(set);
  if (res != RC_OK)
    return 1;
  if (strcmp(swi_dset_GetName(set), "nuuuull"))
    return 1;
  if (swi_dset_GetType(set) != SWI_DSET_NIL)
    return 1;


  res = swi_dset_Destroy(set);
  if (res != RC_OK)
    return 1;

  return 0;
}
int test_3_Find_Elements()
{
  rc_ReturnCode_t res;
  swi_dset_Iterator_t* set = NULL;
  res = swi_dset_Create(&set);
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushInteger(set, "plop", strlen("plop"), 42);
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushString(set, "foo", strlen("foo"), "bar", strlen("bar"));
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushString(set, "dead", strlen("dead"), "beef", strlen("beef"));
  if (res != RC_OK)
    return 1;

  const char* s = NULL;
  res = swi_dset_GetStringByName(set, "foo", &s);
  if (res != RC_OK || NULL == s || strcmp(s, "bar"))
    return 1;

  int64_t i = 0;
  res = swi_dset_GetIntegerByName(set, "plop", &i);
  if (res != RC_OK || 42 != i)
    return 1;

  res = swi_dset_GetIntegerByName(set, "titi", &i);
  if (res != RC_NOT_FOUND)
    return 1;

  res = swi_dset_GetIntegerByName(NULL, "titi", &i);
  if (res != RC_BAD_FORMAT)
    return 1;

  res = swi_dset_GetIntegerByName(set, "titi", NULL );
  if (res != RC_BAD_PARAMETER)
    return 1;

  res = swi_dset_Destroy(set);
  if (res != RC_OK)
    return 1;

  return 0;
}

int test_4_Iterate_Elements()
{
  rc_ReturnCode_t res = RC_OK;
  swi_dset_Iterator_t* set = NULL;
  res = swi_dset_Create(&set);
  if (res != RC_OK)
    return 1;

  const char* names[] = { "plop1", "plop2", "plop3" };

  res = swi_dset_PushInteger(set, names[0], strlen(names[0]), 42);
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushInteger(set, names[1], strlen(names[1]), 43);
  if (res != RC_OK)
    return 1;

  res = swi_dset_PushInteger(set, names[2], strlen(names[2]), 44);
  if (res != RC_OK)
    return 1;

  res = swi_dset_Next(set);
  int i = 0;
  while (RC_OK == res)
  {
    SWI_LOG("DSET_TEST", DEBUG, "%s: name: %s, %d, %" PRId64 "\n", __FUNCTION__, swi_dset_GetName(set), swi_dset_GetType(set), swi_dset_ToInteger(set));

    if (SWI_DSET_INTEGER != swi_dset_GetType(set))
      return 1;
    if (strcmp(swi_dset_GetName(set), names[i]))
      return 1;
    if (swi_dset_ToInteger(set) != (42 + i))
      return 1;

    res = swi_dset_Next(set);
    i++;
  }

  res = swi_dset_Destroy(set);
  if (res != RC_OK)
    return 1;

  return 0;
}

int main(int argc, char ** argv)
{
  INIT_TEST("DSET_TEST");

  CHECK_TEST(test_1_Init_Destroy());
  CHECK_TEST(test_2_Adding_Elements());
  CHECK_TEST(test_3_Find_Elements());
  CHECK_TEST(test_4_Iterate_Elements());

  return 0;
}
