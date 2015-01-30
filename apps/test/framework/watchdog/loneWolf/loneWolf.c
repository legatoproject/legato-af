#include "legato.h"
#include "interfaces.h"
#include <time.h>

#define timeval_to_ms(x) ( (x.tv_sec * 1000) + (x.tv_usec / 1000) )
#define timeval_to_us(x) ( (x.tv_sec * 1000000) + (x.tv_usec) )
struct timeval timeval_sub(struct timeval x,struct timeval y)
{
    struct timeval r = x;
    r.tv_sec -= y.tv_sec;
    r.tv_usec -= y.tv_usec;
    if (r.tv_usec < 0)
    {
        r.tv_usec += 1000000;
        r.tv_sec -= 1;
    }
    return r;
}

/*
 * This watchdog test is a lone wolf. It is hungry for attention and it kicks frequently. Worse
 * yet, this test process will be duplicated many times concurrently to create a wolf pack.
 * This is a stress test to see how the watchdog behaves when many processes want its attention.
 */

COMPONENT_INIT
{
    struct timeval t1={0,0};
    struct timeval t2={0,0};
    struct timeval t3={0,0};

    LE_INFO("Watchdog test starting");

    // Get the process name.
    const char* procName = le_arg_GetProgramName();
    LE_ASSERT(procName != NULL);

    LE_INFO("======== Start '%s' Test ========", procName);


    // 100 times through the loop with 100ms sleeps gives 10 seconds of test
    int i;
    for ( i = 0; i < 100; i++)
    {
        gettimeofday(&t1, NULL);
        le_wdog_Timeout(100);
        gettimeofday(&t2, NULL);
        LE_INFO("kick took %ld usec", timeval_to_us(timeval_sub(t2, t1)));
        usleep(90 * 1000); // 10 ms margin - might make this configurable some time.
        gettimeofday(&t3, NULL);
        LE_INFO("slept for %ld usec", timeval_to_us(timeval_sub(t3, t2)));
    }

    // If the service was able to keep up then all the wolves should survive
    LE_INFO("PASS");
}
