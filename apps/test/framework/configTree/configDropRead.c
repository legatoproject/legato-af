
#include "legato.h"
#include "configTypes.h"
#include "le_cfg_interface.h"




COMPONENT_INIT
{
    le_cfg_Initialize();

    LE_INFO("----  Creating and leaving open a READ transaction.  -------------------------------");
    le_cfg_CreateReadTxn("/");

    exit(EXIT_SUCCESS);
}
