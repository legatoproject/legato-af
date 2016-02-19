#include <sys/ioctl.h>
#include <unistd.h>

#include "pa_gnss.h"
#include "atMgr.h"
#include "atCmdSync.h"

#define AT_PORT     "/tmp/modem_gnss"

COMPONENT_INIT
{
    uint32_t cpt = 5;
    pa_Gnss_Position_t position;

    int sockfd;
    struct sockaddr_un that;
    memset(&that,0,sizeof(that)); /* en System V */
    that.sun_family = AF_UNIX;
    strcpy(that.sun_path,AT_PORT);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr* )&that, sizeof(that)) < 0) /* error */
    {
        perror("connect");
    }

    atmgr_Start();

    atcmdsync_Init();

    if ( atcmdsync_SetCustomConfig(sockfd,
                                     (le_da_DeviceReadFunc_t)read,
                                     (le_da_DeviceWriteFunc_t)write,
                                     (le_da_DeviceIoControlFunc_t)ioctl,
                                     (le_da_DeviceCloseFunc_t)close) != LE_OK )
    {
        perror("atcmdsender");
    }

    pa_gnss_Init();

    pa_gnss_SetAcquisitionRate(5000);

    pa_gnss_Start();

    while (cpt--)
    {
        sleep(5);

        if ( pa_gnss_GetLastPositionData(&position) == LE_OK )
        {
            fprintf(stdout," Position: \n");
            fprintf(stdout,"\t latitude %d\n",position.latitude);
            fprintf(stdout,"\t longitude %d\n",position.longitude);
            fprintf(stdout,"\t altitude %d\n",position.altitude);
            fprintf(stdout,"\t hSpeed %d\n",position.hSpeed);
            fprintf(stdout,"\t dimension %d\n",position.dimension);
            fprintf(stdout,"\t hdop %d\n",position.hdop);
            fprintf(stdout,"\t vdop %d\n",position.vdop);
            fprintf(stdout,"\t Time: \n");
            fprintf(stdout,"\t\t hours %d\n",position.timeInfo.hours);
            fprintf(stdout,"\t\t minutes %d\n",position.timeInfo.minutes);
            fprintf(stdout,"\t\t seconds %d\n",position.timeInfo.seconds);
            fprintf(stdout,"\t\t milliseconds %d\n",position.timeInfo.milliseconds);
            fprintf(stdout,"\t\t day %d\n",position.timeInfo.day);
            fprintf(stdout,"\t\t month %d\n",position.timeInfo.month);
            fprintf(stdout,"\t\t year %d\n",position.timeInfo.year);
        }
    }

    pa_gnss_Stop();

    pa_gnss_Release();

    exit(EXIT_SUCCESS);
}
