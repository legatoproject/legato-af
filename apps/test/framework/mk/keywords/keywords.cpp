#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    // Call API, although this is not intended to run.
    testApi_Test(0);
}
