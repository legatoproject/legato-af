#include "interfaces.h"


COMPONENT_INIT
{
    LE_INFO("Client started.");

    foo_Bar();

    exit(EXIT_SUCCESS);
}
