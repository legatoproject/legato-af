#include "legato.h"

COMPONENT_INIT
{
    while (1)
    {
        LE_INFO("Hello, world.");
        sleep(5);
    }
}
