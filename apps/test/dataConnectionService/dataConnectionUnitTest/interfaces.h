#include "le_data_interface.h"
#include "le_cellnet_interface.h"
#include "le_cfg_interface.h"
#include "le_mdc_interface.h"
#include "le_mrc_interface.h"
#include "le_cfg_interface.h"
#include "le_wifiClient_interface.h"
#include "dcs.h"

//--------------------------------------------------------------------------------------------------
/**
 * MDC interface name
 */
//--------------------------------------------------------------------------------------------------
#define MDC_INTERFACE_NAME  "rmnet0"

//--------------------------------------------------------------------------------------------------
/**
 * Wifi interface name
 */
//--------------------------------------------------------------------------------------------------
#define WIFI_INTERFACE_NAME "wlan0"

//--------------------------------------------------------------------------------------------------
/**
 * Simulated wifi config tree
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_SECPROTOCOL        "secProtocol"
#define CFG_NODE_PASSPHRASE         "passphrase"

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a DCS connection event
 */
//--------------------------------------------------------------------------------------------------
void le_dcsTest_SimulateConnEvent
(
    le_dcs_Event_t evt
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new Wifi event
 */
//--------------------------------------------------------------------------------------------------
void le_wifiClientTest_SimulateEvent
(
    le_wifiClient_Event_t event
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgTest_SetStringNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,
    const char* path,
    const char* value
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgTest_SetIntNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,
    const char* path,
    int32_t value
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_data_GetClientSessionRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_data_GetServiceRef
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t le_msg_AddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate RAT
 */
//--------------------------------------------------------------------------------------------------
void le_mrcTest_SetRatInUse
(
    le_mrc_Rat_t rat    ///< [IN] RAT in use
);
