#include "legato.h"
#include "interfaces.h"


COMPONENT_INIT
{
    LE_INFO("Starting writeFile");
    int fd = open("/usr/myFiles/writeFile", O_WRONLY);

    if (fd < 0)
    {
        LE_ERROR("Unable to open file [%s]", LE_ERRNO_TXT(errno));
        return;
    }

    errno = 0;
    if(write(fd,"hello", 5) == 1)
    {
        LE_ERROR("Unable to write to file [%s]", LE_ERRNO_TXT(errno));
        return;
    }
    else
    {
        LE_INFO("Successful write.");
    }

    close(fd);
}
