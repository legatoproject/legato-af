#include "legato.h"
#include "pos_server.h"
#include "pos_sample_server.h"
#include "le_pos_local.h"

COMPONENT_INIT
{

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");

    le_pos_Init();
    le_pos_StartServer("posDaemon.le_pos");
    le_pos_sample_StartServer("posDaemon.le_pos_sample");

    LE_INFO("Positioning Daemon is ready.");
}

