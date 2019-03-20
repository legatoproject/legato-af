#include "legato.h"
#include "interfaces.h"

uint32_t id = 0;

void printerExtended_Print(uint64_t numBytes, const char* message, uint32_t temperature, uint32_t* idPtr)
{
    id++;
    LE_INFO("******** Client says numBytes [%" PRIu64 "],'%s', temperature [%" PRIu32 "]", numBytes, message, temperature);
    *idPtr = id;
    sleep(2);
    LE_INFO("******** Sending response back to Client, id [%" PRIu32 "]", *idPtr);
}

COMPONENT_INIT
{
}
