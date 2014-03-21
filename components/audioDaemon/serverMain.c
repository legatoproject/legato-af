#include "legato.h"
#include "audio_server.h"
#include "le_audio_local.h"

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

    le_audio_Init();
    le_audio_StartServer("audioDaemon.audio");

    LE_DEBUG("Audio Daemon is ready.");
}

