 /**
  * This module is for unit testing of the modemServices component.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"
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
void Testle_sms_Send_Text();
void Testle_sms_AsyncSendText();
void Testle_sms_AsyncSendPdu();
// void Testle_sms_Send_Pdu();
void Testle_sms_Send_Binary();
void Testle_sms_SendAsync();
void Testle_sms_RxExt();
void Testle_sms_ReceivedList();
void Testle_sms_SetGetSmsCenterAddress();
