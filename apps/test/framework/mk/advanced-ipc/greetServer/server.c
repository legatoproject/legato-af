#include "legato.h"
#include "interfaces.h"

#define MAX_GREETING_BYTES  256

static char Greeting[MAX_GREETING_BYTES] = "Hello, World!";


COMPONENT_INIT
{
    LE_INFO("Greet Server started with %zu arguments.", le_arg_NumArgs());

    if (le_arg_NumArgs() > 0)
    {
        le_arg_GetArg(0, Greeting, sizeof(Greeting));

        if (le_arg_NumArgs() > 1)
        {
            LE_WARN("Ignoring %zu extra command-line arguments.", le_arg_NumArgs() - 1);
        }
    }

    LE_INFO("Using '%s' as the greeting.", Greeting);
}


void hello_Greet(void)
{
    LE_INFO("%s", Greeting);
}
