 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>


#ifndef AUTOMATIC
void GetTel(void);
#endif
void Testle_mcc_Profile();
void Testle_mcc_Call();
void Testle_mcc_HangUpAll();
