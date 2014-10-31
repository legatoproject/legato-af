#include "interfaces.h"


void bar_Bar(void)
{
    LE_INFO("--------------------------------------------------------");
}


COMPONENT_INIT
{
    LE_INFO("Server started.");
}
