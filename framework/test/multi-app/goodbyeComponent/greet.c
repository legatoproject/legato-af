#include "legato.h"
#include "greet.h"

void Greet(const char* object)
{
    greet_SetGreeting("Goodbye, cruel %s!");
    greet_Greet(object);
}
