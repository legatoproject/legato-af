#include "le_ecall_interface.h"
#include "le_mcc_interface.h"
#include "le_cfg_interface.h"
#include "le_cfgAdmin_interface.h"
#include "le_pm_interface.h"
#include "le_mdmDefs_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT  LE_WARN
//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_mcc_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_mcc_GetClientSessionRef
(
    void
);
