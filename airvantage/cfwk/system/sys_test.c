/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include <stdlib.h>
#include "swi_system.h"
#include "swi_log.h"
#include "testutils.h"

static int test_sys_Init()
{
  rc_ReturnCode_t res;

  res = swi_sys_Init();
  if (res != RC_OK)
    return res;
  res = swi_sys_Init();
  if (res != RC_OK)
    return res;
  res = swi_sys_Init();
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static int test_sys_Destroy()
{
  rc_ReturnCode_t res;

  res = swi_sys_Destroy();
  if (res != RC_OK)
    return res;
  res = swi_sys_Destroy();
  if (res != RC_OK)
    return res;
  res = swi_sys_Destroy();
  if (res != RC_OK)
    return res;
  return RC_OK;
}

static int test_sys_Reboot()
{
  rc_ReturnCode_t res;

  res = swi_sys_Reboot("TEST1 FOR REBOOT");
  if (res !=  RC_OK)
    return res;
  res = swi_sys_Reboot("TEST2 FOR REBOOT");
  if (res !=  RC_OK)
    return res;
  res = swi_sys_Reboot("");
  if (res !=  RC_OK)
    return res;
  res = swi_sys_Reboot(NULL);
  if (res !=  RC_OK)
    return res;
  return RC_OK;
}

int main(void)
{
  INIT_TEST("DT_TEST");

  CHECK_TEST(test_sys_Init());
  CHECK_TEST(test_sys_Reboot());
  CHECK_TEST(test_sys_Destroy());
  return 0;
}
