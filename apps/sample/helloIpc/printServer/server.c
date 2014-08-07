#include "legato.h"
#include "interfaces.h"

void printer_Print(const char* message)
{
    LE_INFO("******** Client says '%s'", message);
}

COMPONENT_INIT
{
}
