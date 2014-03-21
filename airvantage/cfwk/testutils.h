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

#include <unistd.h>
#include "swi_log.h"
#include "returncodes.h"

#define INIT_TEST(name)                         \
  static const char *__testname = name;         \
  swi_log_setlevel(INFO, __testname, NULL)

#define CHECK_TEST(call)                        \
do {                                            \
  rc_ReturnCode_t res;                          \
  res = call;                                   \
  while (res == RC_CLOSED) {                    \
    res = call;                                 \
    sleep(2);                                   \
  }                                             \
  SWI_LOG(__testname, (res == RC_OK) ? INFO : ERROR,  #call "...%s\n", (res == RC_OK) ? "OK" : "FAIL");         \
  if (res != RC_OK)                                                                                             \
  {                                                                                                             \
      SWI_LOG(__testname, ERROR, "Test failed with status code %d (%s)\n", res, rc_ReturnCodeToString(res));     \
      return 1;                                                                                                 \
  }                                                                                                             \
} while(0)
