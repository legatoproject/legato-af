 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"


void Print(char* string);
void simTest_State(le_sim_Type_t cardNum, const char* pinPtr);
void TestAutole_sim_Authentication();
void simTest_Authentication(le_sim_Type_t cardNum,const char* pinPtr,const char* pukPtr);
void simTest_Create(le_sim_Type_t cardNum, const char* pinPtr);
void simTest_Lock(le_sim_Type_t cardNum,const char* pinPtr);
void simTest_SimAbsent(le_sim_Type_t cardNum);
void simTest_SimSelect(void);
