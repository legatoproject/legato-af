#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    uint64_t count64 = 1234567890;
    uint32_t count32 = 10;
    bool done = false;
    char response[51];
    size_t responseSize = 50;
    uint32_t id = 0;

    while (!done)
    {
        LE_INFO("Asking server to print 'Hello, world!'");
        printer_Print(count64, "Hello, world!", count32, response, responseSize, &id);
        LE_INFO("Server says, '%s', id [%" PRIu32 "]", response, id);
        count64++;
        count32++;
        LE_INFO("Asking server to print 'This is Pirate Radio'");
        printer_Print(count64, "This is Pirate Radio", count32, response, responseSize, &id);
        LE_INFO("Server says, '%s', id [%" PRIu32 "]", response, id);
        count64++;
        count32++;
        LE_INFO("Asking server to print 'Now, let's get this RPC show on the road.....'");
        printer_Print(count64, "Now, let's get this RPC show on the road.....", count32, response, responseSize, &id);
        LE_INFO("Server says, '%s', id [%" PRIu32 "]", response, id);
        count64++;
        count32++;
        LE_INFO("Asking server to print 'This next song goes way back to the golden age of punk rock.  Sit back and enjoy!'");
        printer_Print(count64, "This next song goes way back to the golden age of punk rock.  Sit back and enjoy!", count32, response, responseSize, &id);
        LE_INFO("Server says, '%s', id [%" PRIu32 "]", response, id);
        count64++;
        count32++;
    }
}
