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

#include <libgen.h>
#include "swi_log.h"
#include "returncodes.h"

#define INIT_TEST(name) swi_log_setlevel(INFO, "C_TEST", NULL)

#define ASSERT_TESTCASE_EQUAL(expected, got)                                                                \
do {                                                                                                        \
  if (got != expected) {                                                                                    \
    SWI_LOG("C_TEST", ERROR, "%s...FAIL\n", __func__);                                                      \
    SWI_LOG("C_TEST", ERROR, "%s:%d: expected code %d (%s), got %d (%s)\n",                                 \
      basename(__FILE__), __LINE__, expected, rc_ReturnCodeToString(expected), got, rc_ReturnCodeToString(got)); \
    exit(1);                                                                                                \
  }                                                                                                         \
} while(0)

#define ASSERT_TESTCASE_IS_OK(got) ASSERT_TESTCASE_EQUAL(RC_OK, got)
#define ABORT(msg, ...)							                   \
do {                                                                                       \
  SWI_LOG("C_TEST", ERROR, "%s...FAIL\n", __func__);                                       \
  SWI_LOG("C_TEST", ERROR, "%s:%d: " msg "\n", basename(__FILE__), __LINE__, ##__VA_ARGS__);    \
  exit(1);                                                                                 \
} while(0)


#define DEFINE_TEST(functiontest)                      \
static void __##functiontest();                        \
static void functiontest()                             \
{                                                      \
  __##functiontest();                                  \
  SWI_LOG("C_TEST", INFO, "%s...OK\n", __func__);      \
}                                                      \
static void __##functiontest()

#define CHECK_TEST(call)                        \
do {                                            \
  rc_ReturnCode_t res;                          \
  res = call;                                   \
  while (res == RC_CLOSED) {                    \
    res = call;                                 \
    sleep(2);                                   \
  }                                             \
  SWI_LOG("C_TEST", (res == RC_OK) ? INFO : ERROR,  #call "...%s\n", (res == RC_OK) ? "OK" : "FAIL");           \
  if (res != RC_OK)                                                                                             \
  {                                                                                                             \
      SWI_LOG("C_TEST", ERROR, "Test failed with status code %d (%s)\n", res, rc_ReturnCodeToString(res));     \
      return 1;                                                                                                 \
  }                                                                                                             \
} while(0)
