#include "legato.h"
#include "interfaces.h"


COMPONENT_INIT
{
    LE_INFO("Starting writeFile");
    int fd = open("/myTestDir/newfile", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR);

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
        LE_ERROR("Unable to write to file [%s]", strerror(errno));
        return;
    }
    else
    {
        LE_INFO("Successful write.");
    }

    close(fd);
}