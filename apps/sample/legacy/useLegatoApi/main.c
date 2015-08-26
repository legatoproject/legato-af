#include <stdio.h>
#include "legato.h"
#include "le_info_interface.h"

int main(int argc, char** argv)
{
    le_info_ConnectService();

    char deviceModelStr[256];

    le_result_t result = le_info_GetDeviceModel(deviceModelStr, sizeof(deviceModelStr));

    if (result == LE_OK)
    {
        printf("Hello world from %s.\n", deviceModelStr);
    }
    else
    {
        printf("Failed to get device model. Error = '%s'.\n", LE_RESULT_TXT(result));
    }

    return EXIT_SUCCESS;
}


