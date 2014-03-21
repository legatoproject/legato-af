 /**
  * This module is for unit testing of the modemServices component.
  *
  * 
  * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>


void getTel(void);
void Testle_sms_msg_SetGetText();
void Testle_sms_msg_SetGetBinary();
void Testle_sms_msg_SetGetPDU();
void Testle_sms_msg_SendText();
// void Testle_sms_msg_SendPdu();
void Testle_sms_msg_SendBinary();
void Testle_sms_msg_SendAsync();
void Testle_sms_msg_RxExt();
void Testle_sms_msg_ReceivedList();


