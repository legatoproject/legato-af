/** @file atunsolicited.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"
#include "atMachineUnsolicited.h"

#define DEFAULT_ATUNSOLICITED_POOL_SIZE      1
static le_mem_PoolRef_t    AtUnsolicitedPool;

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atunsolicited
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachineunsolicited_Init
(
    void
)
{
    AtUnsolicitedPool = le_mem_CreatePool("AtUnsolicitedPool",sizeof(atUnsolicited_t));
    le_mem_ExpandPool(AtUnsolicitedPool,DEFAULT_ATUNSOLICITED_POOL_SIZE);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an atUnsolicited_t struct
 *
 * @return pointer on the structure
 */
//--------------------------------------------------------------------------------------------------
atUnsolicited_t* atmachineunsolicited_Create
(
    void
)
{
    atUnsolicited_t* newUnsolicitedPtr = le_mem_ForceAlloc(AtUnsolicitedPool);

    memset(newUnsolicitedPtr->unsolRsp, 0 ,sizeof(newUnsolicitedPtr->unsolRsp));
    newUnsolicitedPtr->unsolicitedReportId = NULL,
    newUnsolicitedPtr->withExtraData    = false;
    newUnsolicitedPtr->waitForExtraData = false;
    newUnsolicitedPtr->link = LE_DLS_LINK_INIT;

    return newUnsolicitedPtr;
}
