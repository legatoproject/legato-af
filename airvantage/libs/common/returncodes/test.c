#include "returncodes.h"
#include <stdio.h>



int main()
{
    int i;

    for (i=0; i>=RC_NOT_AVAILABLE; i--)
    {
        if (rc_string2returncode(rc_returncode2string(i)) != i)
        {
            printf("Error with code %d\n", i);
            return -1;
        }
    }

    if (rc_returncode2string(1) != NULL || rc_returncode2string(-14654) != NULL)
    {
        printf("Unknown error code tranlastion should return NULL\n");
        return -1;
    }

    if (rc_string2returncode(NULL) != 1 || rc_string2returncode("FOOBAR") != 1)
    {
        printf("Unknown string error translation should return 1\n");
        return -1;
    }


    printf("Error code mapping is OK\n");
    return 0;
}