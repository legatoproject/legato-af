 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>


void GetTel(void);
void Testle_sms_SetGetText();
void Testle_sms_SetGetBinary();
void Testle_sms_SetGetPDU();
void Testle_sms_SendText();
// void Testle_sms_SendPdu();
void Testle_sms_SendBinary();
void Testle_sms_SendAsync();
void Testle_sms_RxExt();
void Testle_sms_ReceivedList();
void Testle_sms_SetGetSmsCenterAddress();
