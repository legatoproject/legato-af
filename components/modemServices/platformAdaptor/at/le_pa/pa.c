/** @file pa.c
 *
 * AT implementation of c_pa API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 */

#include <pthread.h>
#include <semaphore.h>

#include "pa.h"
#include "atManager/inc/atMgr.h"
#include "atManager/inc/atCmdSync.h"
#include "atManager/inc/atPorts.h"

#include "pa_mrc.h"
#include "pa_mrc_local.h"
#include "pa_sms.h"
#include "pa_sms_local.h"
#include "pa_sim.h"
#include "pa_sim_local.h"
#include "pa_mdc.h"
#include "pa_mdc_local.h"
#include "pa_mcc_local.h"
#include "pa_ecall_local.h"
#include "pa_fwupdate_local.h"
#include "pa_common_local.h"

static le_thread_Ref_t  PaThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Enable CMEE
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t  EnableCMEE()
{

    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                "at+cmee=1",
                                                NULL,
                                                NULL,
                                                30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable echo
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t  DisableEcho()
{
    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                "ate0",
                                                NULL,
                                                NULL,
                                                30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Save settings
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t  SaveSettings()
{
    return atcmdsync_SendStandard(atports_GetInterface(ATPORT_COMMAND),
                                                "at&W",
                                                NULL,
                                                NULL,
                                                30000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set new message indication
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t  SetNewSmsIndication()
{
    pa_sms_NmiMode_t mode;
    pa_sms_NmiMt_t   mt;
    pa_sms_NmiBm_t   bm;
    pa_sms_NmiDs_t   ds;
    pa_sms_NmiBfr_t  bfr;

    // Get & Set the configuration to enable message reception
    LE_DEBUG("Get New SMS message indication");
    if (pa_sms_GetNewMsgIndic(&mode, &mt, &bm,  &ds, &bfr)  != LE_OK)
    {
        LE_WARN("Get New SMS message indication failed, set default configuration");
        if (pa_sms_SetNewMsgIndic(PA_SMS_NMI_MODE_0,
                                    PA_SMS_MT_1,
                                    PA_SMS_BM_0,
                                    PA_SMS_DS_0,
                                    PA_SMS_BFR_0)  != LE_OK)
        {
            //              LE_FATAL("Set New SMS message indication failed");
            LE_ERROR("Set New SMS message indication failed");
            return LE_NOT_POSSIBLE;
        }
    }

    LE_DEBUG("Set New SMS message indication");
    if (pa_sms_SetNewMsgIndic(mode,
                                PA_SMS_MT_1,
                                bm,
                                ds,
                                bfr)  != LE_OK)
    {
        //          LE_FATAL("Set New SMS message indication failed");
        LE_ERROR("Set New SMS message indication failed");
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set Default configuration
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DefaultConfig()
{
    if (DisableEcho()!=LE_OK)
    {
        LE_WARN("modem is not well configured");
        return LE_NOT_POSSIBLE;
    }

    if (pa_sms_SetMsgFormat(LE_SMS_FORMAT_PDU) != LE_OK)
    {
        LE_WARN("modem failed to switch to PDU format");
        return LE_NOT_POSSIBLE;
    }

    if (SetNewSmsIndication() != LE_OK)
    {
        LE_WARN("modem failed to set New SMS indication");
        return LE_NOT_POSSIBLE;
    }

    if (EnableCMEE()!=LE_OK)
    {
        LE_WARN("Failed to enable CMEE error");
        return LE_NOT_POSSIBLE;
    }

    if (SaveSettings()!=LE_OK)
    {
        LE_WARN("Failed to Save Modem Settings");
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

/**
 * PA Thread main
 *
 */

static void* PAThreadInit(void* context)
{
    le_sem_Ref_t semPtr = context;
    LE_INFO("Start PA");

    pa_common_Init();
    pa_mrc_Init();
    pa_sms_Init();
    pa_sim_Init();
    pa_mdc_Init();
    pa_mcc_Init();
    pa_ecall_Init();
    pa_fwupdate_Init();

    le_sem_Post(semPtr);
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the platform adapter layer for modem services.
 *
 * @note This does NOT initialize positioning services access via QMI.
 *
 * @todo Clarify the separation of positioning services and modem services in the PA layer
 *       interface.
 */
//--------------------------------------------------------------------------------------------------
void le_pa_Init()
{
    if (atports_GetInterface(ATPORT_COMMAND)==NULL) {
        LE_WARN("PA cannot be initialized");
    }

        atmgr_StartInterface(atports_GetInterface(ATPORT_COMMAND));

    le_sem_Ref_t semPtr = le_sem_Create("PAStartSem",0);

    if (PaThreadRef == NULL) {

        PaThreadRef = le_thread_Create("PA",PAThreadInit,semPtr);

    le_thread_Start(PaThreadRef);

    le_sem_Wait(semPtr);
    LE_INFO("PA is started");
    le_sem_Delete(semPtr);

    } else {
        LE_WARN("PA is already initialized");
    }

    if ( DefaultConfig() != LE_OK )
    {
        LE_WARN("PA is not configured as expected");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Component initializer automatically called by the application framework when the process starts.
 *
 * This is not used because the PA component is shared by two different processes (the Modem Daemon
 * and the Positioning Daemon), and each needs different QMI services initialized.
 **/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}
