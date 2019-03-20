#include "legato.h"
#include "interfaces.h"

uint32_t id = 0;

char responseStr[3][30] = {{"I love this music!'"}, {"What is the name of the band?"}, {"This rocks!!!!!!!!!!!!!"}};

void printer_Print(uint64_t numBytes, const char* message, uint32_t temperature, char* response, size_t responseSize, uint32_t* idPtr)
{
    id++;
    LE_INFO("******** Client says numBytes [%" PRIu64 "],'%s', temperature [%" PRIu32 "], responseSize [%" PRIuS "]",
            numBytes, message, temperature, responseSize);
    memcpy(response, responseStr[id % 3], strlen(responseStr[id % 3] + 1));
    *idPtr = id;
    LE_INFO("******** Sending response back to Client, '%s', id [%" PRIu32 "]", response, *idPtr);
}

COMPONENT_INIT
{
}
