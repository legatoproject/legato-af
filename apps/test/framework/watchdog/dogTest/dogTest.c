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
 * This watchdog test begins kicking at start up and waits an increasing amount of time between
 * kicks until it crosses the configured timeout and is killed.
 *
 * The test takes 2 arguments.
 *
 *      start_duration  How many milliseconds to sleep on the first iteration
 *      increment       How many milliseconds longer to sleep on each successive iterations
 *
 * An arbitrary maximum sleep of 60 seconds has been chosen for this test
 * so that it can end in a reasonable time - however, at a small enough increment it can still take
 * a long time to complete.
 *
 * No sanity checking is done on the arguments - it is the tester's responsibility to set this up
 * in a reasonable way.
 *
 * Nota Bene: This test has a 60 SECOND LIMIT. Trying to test timeouts longer than that will FAIL
 */

COMPONENT_INIT
{
    const int millisecondLimit = 60000; // one minute
    struct timeval t1={0,0};
    struct timeval t2={0,0};
    struct timeval t3={0,0};

    LE_INFO("Watchdog test starting");

    // Get the process name.
    char procName[100];
    LE_ASSERT(le_arg_GetProgramName(procName, sizeof(procName), NULL) == LE_OK);

    LE_INFO("======== Start '%s' Test ========", procName);

    // Get the sleep increment in milliseconds.
    char millisecondsStr[100] = {'\0'};
    int millisecondIncrement;
    int millisecondSleep;

    int numArgs = le_arg_NumArgs();
    LE_INFO("numArgs = %d", numArgs);
    if (numArgs < 1)
    {
        LE_INFO("Expected 2 arguments, got %d", numArgs);
    }

    if (le_arg_GetArg(0, millisecondsStr, sizeof(millisecondsStr)) == LE_OK)
    {
        millisecondSleep = atoi(millisecondsStr);
    }
    if (le_arg_GetArg(1, millisecondsStr, sizeof(millisecondsStr)) == LE_OK)
    {
        millisecondIncrement = atoi(millisecondsStr);
    }


    for ( ;
          millisecondSleep < millisecondLimit;
          millisecondSleep += millisecondIncrement)
    {
        gettimeofday(&t1, NULL);
        LE_INFO("le_wdog_Kick then sleep for %d usec", millisecondSleep * 1000);
        le_wdog_Kick();
        gettimeofday(&t2, NULL);
        LE_INFO("kick took %ld usec", timeval_to_us(timeval_sub(t2, t1)));
        usleep(millisecondSleep * 1000);
        gettimeofday(&t3, NULL);
        LE_INFO("slept for %ld usec", timeval_to_us(timeval_sub(t3, t2)));
    }

    // We should never get here
    LE_INFO("FAIL");
}
