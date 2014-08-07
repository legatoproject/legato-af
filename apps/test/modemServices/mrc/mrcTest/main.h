 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014.  All rights reserved.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>



void Testle_mrc_Power();
void Testle_mrc_GetStateAndQual();
void Testle_mrc_GetRat();
void Testle_mrc_GetNeighboringCellsInfo();
void Testle_mrc_NetRegHdlr();
void Testle_mrc_RatHdlr();
void Testle_mrc_ManageBands();
