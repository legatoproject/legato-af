#include "le_cellnet_interface.h"
#include "le_mrc_interface.h"
#include "le_secStore_interface.h"

#undef LE_KILL_CLIENT
#define LE_KILL_CLIENT LE_ERROR

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new MRC state
 */
//--------------------------------------------------------------------------------------------------
void le_mrcTest_SimulateState
(
    le_mrc_NetRegState_t state
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate the SIM presence
 */
//--------------------------------------------------------------------------------------------------
void le_simTest_SetPresent
(
    le_sim_Id_t simId,
    bool presence
);

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new SIM state
 */
//--------------------------------------------------------------------------------------------------
void le_simTest_SimulateState
(
    le_sim_Id_t        simId,
    le_sim_States_t    simState
);
