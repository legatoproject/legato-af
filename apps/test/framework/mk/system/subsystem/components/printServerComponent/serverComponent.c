
#include "legato.h"
#include "interfaces.h"



void printer_Print(const char* message)
{
    LE_INFO("*** Message: %s ***", message);
}



COMPONENT_INIT
{
    LE_DEBUG("Component serverComponent started.");
}
