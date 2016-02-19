#include "legato.h"
#include "interfaces.h"

uint32_t foo_f()
{
    printf("Returning 'foo' value 1234.\n");
    return 1234;
}

void bar_f(const char* s)
{
    printf("Received on 'bar' interface: '%s'\n", s);
}

COMPONENT_INIT
{
}
