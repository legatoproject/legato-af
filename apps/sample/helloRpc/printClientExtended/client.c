#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    uint64_t count64 = 1;
    uint32_t count32 = 10000;
    bool done = false;
    uint32_t id = 0;

    while (!done)
    {
        LE_INFO("Asking server to print 'Sierra Wireless multi-client test!'");
        printerExtended_Print(count64, "Sierra Wireless multi-client test!", count32, &id);
        LE_INFO("Server says, id [%" PRIu32 "]", id);
        count64++;
        count32++;
    }
}
