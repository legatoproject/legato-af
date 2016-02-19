#include "legato.h"
#include "interfaces.h"

static const char MagicWord[] = "Abracadabra!";

COMPONENT_INIT
{
    LE_INFO("Magician started with %zu arguments.", le_arg_NumArgs());
}


void magic_DoTrick(void)
{
    LE_INFO("%s", MagicWord);
}
