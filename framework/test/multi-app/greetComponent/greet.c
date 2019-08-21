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
    // LE_INFO on certain platforms (ie. ThreadX) can only accept constant literals
    char buffer[64] = {0};
    snprintf(buffer, sizeof(buffer) - 1, LE_CDATA_THIS->greeting, object);
    LE_TEST_OUTPUT("%s", buffer);
}

COMPONENT_INIT
{
}
