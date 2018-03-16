
#include "legato.h"
#include "interfaces.h"



COMPONENT_INIT
{
    char buffer[PRINTER_MSG_LEN_BYTES] = "This is a message from a client component.";

    LE_INFO("Component clientComponent started.");
    printer_Print(buffer);
}
