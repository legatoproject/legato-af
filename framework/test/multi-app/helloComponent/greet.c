#include "legato.h"
#include "greet.h"

void Greet(const char* object)
{
    greet_SetGreeting("Hello, %s!");
    greet_Greet(object);
}
