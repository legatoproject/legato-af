#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    LE_INFO("Asking server to print 'Hello, world!'");
    printer_Print("Hello, world!");
}
