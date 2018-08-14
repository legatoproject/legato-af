#include "legato.h"
#include "interfaces.h"


COMPONENT_INIT
{
    LE_INFO("Starting noPerm");
    int fd = open("/files/log.txt", O_RDONLY);

    if (fd < 0)
    {
        LE_ERROR("Unable to open file [%s]", strerror(errno));
        return;
    }
    else
    {
        LE_INFO("Success creating file");
    }

    errno = 0;
    if(write(fd,"abc", 3) == -1)
    {
        LE_INFO("Expected result unable to write to file [%s]", strerror(errno));
        return;
    }
    else
    {
        LE_ERROR("Write successful when it should not be allowed.");
    }

    close(fd);
}