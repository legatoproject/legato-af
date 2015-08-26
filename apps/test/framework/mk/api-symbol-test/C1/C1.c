#include "legato.h"
#include "interfaces.h"

COMPONENT_INIT
{
    int result = foo_f();
    if (result == 1234)
    {
        printf("C1 result = %d\n", result);
    }
    else
    {
        fprintf(stderr, "Got incorrect result!  Expected 1234, got %d.\n", result);
    }
}
