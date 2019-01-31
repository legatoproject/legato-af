#include "legato.h"
#include "greet.h"

LE_CDATA_DECLARE
({
    const char* greeting;
});

void greet_SetGreeting
(
    const char* greeting
)
{
    LE_CDATA_THIS->greeting = greeting;
}

void greet_Greet
(
    const char* object
)
{
    LE_INFO(LE_CDATA_THIS->greeting, object);
}

COMPONENT_INIT
{
}
