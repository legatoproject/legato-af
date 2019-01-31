#include "legato.h"

void Greet(const char* object);

COMPONENT_INIT
{
    int numArgs = le_arg_NumArgs();
    int arg;
    for (arg = 0; arg < numArgs; ++arg)
    {
        Greet(le_arg_GetArg(arg));
    }

    le_thread_Exit(0);
}
