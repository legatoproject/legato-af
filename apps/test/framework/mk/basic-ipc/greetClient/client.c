#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    LE_INFO("Greet Client started.");

    hello_StartClient("server.hi");

    hello_Greet();
}
