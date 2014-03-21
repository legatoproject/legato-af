#include "legato.h"
#include "info_interface.h"
#include "sms_interface.h"
#include "mrc_interface.h"
#include "sim_interface.h"
#include "mdc_interface.h"
#include "mcc_profile_interface.h"
#include "mcc_call_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Initializes the Modem Daemon Client Library
 *
 * The linker/loader will automatically run this function when the library is loaded into a
 * process's address space at runtime.
 *
 * Based on available documentation and testing, if multiple libraries have constructor
 * functions, then they will be executed in the order that the libraries are loaded. Thus,
 * the constructor for the legato framework library will always be executed before this
 * constructor, since this library is loaded after the legato framework library.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((constructor)) static void InitClient
(
    void
)
{
    le_info_StartClient("modemDaemon.le_info");
    le_sms_msg_StartClient("modemDaemon.le_sms_msg");
    le_mrc_StartClient("modemDaemon.le_mrc");
    le_sim_StartClient("modemDaemon.le_sim");
    le_mdc_StartClient("modemDaemon.le_mdc");
    le_mcc_profile_StartClient("modemDaemon.le_mcc_profile");
    le_mcc_call_StartClient("modemDaemon.le_mcc_call");

    LE_INFO("Modem Daemon Client is ready.");
}

