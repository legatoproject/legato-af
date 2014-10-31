 /**
  * This module implements a le_remoteMgmt test.
  *
  * This test sets the flag DoNotDisturb. After the exit of the main task, the le_remoteMgmt is
  * warned that the client is died and it removes the DoNotDisturb flag itself. This can be checked
  * by watching the logs (nothing automatic).
  *
  * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
  *
  */

#include "legato.h"
#include "interfaces.h"

#define SERVICE_BASE_BINDINGS_CFG  "/users/root/bindings"

typedef void (*LegatoServiceInit_t)(void);

typedef struct {
    const char * appNamePtr;
    const char * serviceNamePtr;
    LegatoServiceInit_t serviceInitPtr;
} ServiceInitEntry_t;

#define SERVICE_ENTRY(aPP, sVC) { aPP, #sVC, sVC##_ConnectService },

const ServiceInitEntry_t ServiceInitEntries[] = {
    SERVICE_ENTRY("modemService", le_remoteMgmt)
};

static void SetupBindings(void)
{
    int serviceIndex;
    char cfgPath[512];
    le_cfg_IteratorRef_t iteratorRef;

    /* Setup bindings */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        /* Update binding in config tree */
        LE_INFO("-> Bind %s", entryPtr->serviceNamePtr);

        snprintf(cfgPath, sizeof(cfgPath), SERVICE_BASE_BINDINGS_CFG "/%s", entryPtr->serviceNamePtr);

        iteratorRef = le_cfg_CreateWriteTxn(cfgPath);

        le_cfg_SetString(iteratorRef, "app", entryPtr->appNamePtr);
        le_cfg_SetString(iteratorRef, "interface", entryPtr->serviceNamePtr);

        le_cfg_CommitTxn(iteratorRef);
    }

    /* Tel legato to reload its bindings */
    system("sdir load");
}

static void ConnectServices(void)
{
    int serviceIndex;

    /* Init services */
    for(serviceIndex = 0; serviceIndex < NUM_ARRAY_MEMBERS(ServiceInitEntries); serviceIndex++ )
    {
        const ServiceInitEntry_t * entryPtr = &ServiceInitEntries[serviceIndex];

        LE_INFO("-> Init %s", entryPtr->serviceNamePtr);
        entryPtr->serviceInitPtr();
    }

    LE_INFO("All services bound!");
}


COMPONENT_INIT
{
    LE_DEBUG("remoteMgmtTest init");

    SetupBindings();
    ConnectServices();

    le_remoteMgmt_SetDoNotDisturbSign();
    le_remoteMgmt_SetDoNotDisturbSign();

    // Exit and check (manualy :( ) if le_remoteMgmt_ClearDoNotDisturbSign is correctly called
    exit(1);
}


