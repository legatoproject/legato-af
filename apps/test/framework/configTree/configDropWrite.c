
#include "legato.h"
#include "configTypes.h"
#include "le_cfg_interface.h"




COMPONENT_INIT
{
    le_cfg_Initialize();

    LE_INFO("----  Creating and leaving open a WRITE transaction.  ------------------------------");
    le_cfg_CreateWriteTxn("/");

    exit(EXIT_SUCCESS);
}
