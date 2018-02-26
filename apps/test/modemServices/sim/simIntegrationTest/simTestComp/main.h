 /**
  * This module is for unit testing of the modemServices component.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"


void Print
(
    char* string
);

void simTest_State
(
    le_sim_Id_t cardNum,
    const char* pinPtr
);

void simTest_Authentication
(
    le_sim_Id_t cardNum,
    const char* pinPtr,
    const char* pukPtr
);

void simTest_Create
(
    le_sim_Id_t cardNum,
    const char* pinPtr
);

void simTest_Lock
(
    le_sim_Id_t cardNum,
    const char* pinPtr
);

void simTest_SimAbsent
(
    le_sim_Id_t cardNum
);

void simTest_SimSelect
(
    void
);

void simTest_SimGetIccid
(
    le_sim_Id_t simId
);

void simTest_SimGetEid
(
    le_sim_Id_t simId
);

void simTest_SimAccess
(
    le_sim_Id_t simId
);

void simTest_SimPowerUpDown
(
    void
);
