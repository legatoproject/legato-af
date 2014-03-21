#include "legato.h"
#include "info_server.h"
#include "sms_server.h"
#include "mrc_server.h"
#include "sim_server.h"
#include "mdc_server.h"
#include "mcc_profile_server.h"
#include "mcc_call_server.h"
#include "le_ms.h"

COMPONENT_INIT
{
    le_ms_Init();
    le_info_StartServer("modemDaemon.le_info");
    le_sms_msg_StartServer("modemDaemon.le_sms_msg");
    le_mrc_StartServer("modemDaemon.le_mrc");
    le_sim_StartServer("modemDaemon.le_sim");
    le_mdc_StartServer("modemDaemon.le_mdc");
    le_mcc_profile_StartServer("modemDaemon.le_mcc_profile");
    le_mcc_call_StartServer("modemDaemon.le_mcc_call");

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.
    // todo: This should call fd_Close() but that function is not available outside the
    //       Legato framework
    close(STDIN_FILENO);

    LE_INFO("Modem Daemon is ready.");
}

