/**
 * @file le_dualsys.c
 *
 * Implementation of Dual System definition
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"


//==================================================================================================
//                                       Public API Functions
//==================================================================================================


//--------------------------------------------------------------------------------------------------
/**
 * Disable (true) or enable (false) the synchronisation check before performing an update.
 * The default behavior at startup is always to have the check enabled. It remains enabled
 * until this service is called with the value true. To re-enable the synchronization check
 * call this service with the value false.
 *
 * @note Upgrading some partitions without performing a sync before may let the whole system
 *       into a unworkable state. THIS IS THE RESPONSABILITY OF THE CALLER TO KNOW WHAT IMAGES
 *       ARE ALREADY FLASHED INTO THE UPDATE SYSTEM.
 *
 * @note Fuction must be called after each target reboot or updateDaemon restart
 *
 * @return
 *      - LE_OK              On success
 *      - LE_UNSUPPORTED     The feature is not supported
 *      - LE_FAULT           On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dualsys_DisableSyncBeforeUpdate
(
    bool isDisabled  ///< [IN] State of sync check : true (disable) or false (enable)
)
{
    return pa_fwupdate_DisableSyncBeforeUpdate(isDisabled);
}


//--------------------------------------------------------------------------------------------------
/**
 * Define a new "system" by setting the three sub-systems.  This system will become the current
 * system in use after the reset performed by this service, if no error are reported.
 *
 * @note On success, a device reboot is initiated without returning any value.
 *
 * @return
 *      - LE_FAULT           On failure
 *      - LE_UNSUPPORTED     The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dualsys_SetSystem
(
    le_dualsys_System_t systemMask  ///< [IN] Sub-system bitmask for "modem/lk/linux" partitions
)
{
    pa_fwupdate_System_t systemId[PA_FWUPDATE_SUBSYSID_MAX];

    LE_DEBUG("systemMask = 0x%x", systemMask);
    systemId[PA_FWUPDATE_SUBSYSID_MODEM] = (systemMask & LE_DUALSYS_MODEM_GROUP)
                                               ? PA_FWUPDATE_SYSTEM_2
                                               : PA_FWUPDATE_SYSTEM_1;
    systemId[PA_FWUPDATE_SUBSYSID_LK] = (systemMask & LE_DUALSYS_LK_GROUP)
                                            ? PA_FWUPDATE_SYSTEM_2
                                             : PA_FWUPDATE_SYSTEM_1;
    systemId[PA_FWUPDATE_SUBSYSID_LINUX] = (systemMask & LE_DUALSYS_LINUX_GROUP)
                                               ? PA_FWUPDATE_SYSTEM_2
                                               : PA_FWUPDATE_SYSTEM_1;
    LE_DEBUG("Setting system %d,%d,%d", systemId[PA_FWUPDATE_SUBSYSID_MODEM],
             systemId[PA_FWUPDATE_SUBSYSID_LK], systemId[PA_FWUPDATE_SUBSYSID_LINUX]);
    return pa_fwupdate_SetSystem(systemId);
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the current "system" in use.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *      - LE_UNSUPPORTED   The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dualsys_GetCurrentSystem
(
    le_dualsys_System_t* systemMaskPtr ///< [OUT] Sub-system bitmask for "modem/lk/linux" partitions
)
{
    pa_fwupdate_System_t systemId[PA_FWUPDATE_SUBSYSID_MAX];
    le_result_t res = pa_fwupdate_GetSystem(systemId);

    if (NULL == systemMaskPtr)
    {
        LE_ERROR("Null pointer provided!");
        return LE_FAULT;
    }

    if (LE_OK == res)
    {
       *systemMaskPtr = (PA_FWUPDATE_SYSTEM_2 == systemId[PA_FWUPDATE_SUBSYSID_MODEM])
                         ? LE_DUALSYS_MODEM_GROUP : 0;
       *systemMaskPtr |= (PA_FWUPDATE_SYSTEM_2 == systemId[PA_FWUPDATE_SUBSYSID_LK])
                          ? LE_DUALSYS_LK_GROUP : 0;
       *systemMaskPtr |= (PA_FWUPDATE_SYSTEM_2 == systemId[PA_FWUPDATE_SUBSYSID_LINUX])
                          ? LE_DUALSYS_LINUX_GROUP : 0;
       LE_DEBUG("systemMask = 0x%x", *systemMaskPtr);
    }
    return res;
}
