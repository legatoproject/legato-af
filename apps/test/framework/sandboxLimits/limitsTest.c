/*
 * Test the sandbox limits.
 *
 * This test only ensures that the limits were set properly for our process it does not check if the
 * limits are being enforced correctly as the enforcement is done by the kernel.
 *
 * The expected limit values must be passed in on the command line in the proper order.
 */

#include "legato.h"
#include <mqueue.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/statvfs.h>


static int ActualFileSizeLimit = 0;

// Expected limits must be passed in on the command line in the order of this enum.
typedef enum
{
    LIM_MSG_QUEUE_SIZE = 0,
    LIM_NUM_PROCS,
    LIM_NUM_SIGS_QUEUED,
    LIM_FILE_SIZE,
    LIM_NUM_FDS,
    LIM_MEM_LOCK_SIZE,
    LIM_CORE_DUMP,
    LIM_FILE_SYS_SIZE,
    LIM_MAX_NUM_LIMITS  // Must be last in this list.
}
Limits_t;

static rlim_t ExpectedLimits[LIM_MAX_NUM_LIMITS];

// The list of rlimits.  Must correspond to the limits in Limits_t and be in the same order.
#define NUM_RLIMITS     LIM_MAX_NUM_LIMITS - 1
static int RLimit[NUM_RLIMITS] = {RLIMIT_MSGQUEUE, RLIMIT_NPROC, RLIMIT_SIGPENDING,
                                  RLIMIT_FSIZE, RLIMIT_NOFILE, RLIMIT_MEMLOCK, RLIMIT_CORE};

static char* RLimitStr[NUM_RLIMITS] = {"RLIMIT_MSGQUEUE", "RLIMIT_NPROC", "RLIMIT_SIGPENDING",
                                        "RLIMIT_FSIZE", "RLIMIT_NOFILE",
                                        "RLIMIT_MEMLOCK", "RLIMIT_CORE"};


static void TestFileSystemSize(void)
{
    // Calculate the actual file system size by rounding the limit up to the next page size.
    long pageSize = sysconf(_SC_PAGESIZE);
    size_t actualLimit = ExpectedLimits[LIM_FILE_SYS_SIZE] / pageSize;

    if (ExpectedLimits[LIM_FILE_SYS_SIZE] % pageSize != 0)
    {
        actualLimit++;
    }

    actualLimit = actualLimit * pageSize;

    // Find the actual file system size.
    struct statvfs fsStat;
    LE_ASSERT(statvfs("/", &fsStat) == 0);

    size_t fileSysSize = fsStat.f_blocks * fsStat.f_frsize;

    // This is the file system size.  Compare it to the limit.
    LE_FATAL_IF(actualLimit != fileSysSize,
                "File system size is %zd but expected size is %zd.",
                fileSysSize, actualLimit);
}


// Gets the expected limits from the command line arguments.
static void GetExpectedLimits(void)
{
    int i;
    for (i = 0; i < LIM_MAX_NUM_LIMITS; i++)
    {
        const char* argPtr = le_arg_GetArg(i);
        LE_ASSERT(argPtr != NULL);

        // Get the value.
        char* endPtr;
        errno = 0;
        int value = strtol(argPtr, &endPtr, 10);

        if ( (errno == 0) && (endPtr != argPtr) )
        {
            // This is a valid number.
            ExpectedLimits[i] = (rlim_t)value;
        }
        else
        {
            LE_FATAL("Arg %d is not valid", i);
        }
    }

    // Get the ActualFileSizeLimit here too.
    struct rlimit lim;
    LE_ASSERT(getrlimit(RLIMIT_FSIZE, &lim) != -1);

    ActualFileSizeLimit = lim.rlim_cur;
}


// Test rlimits by comparing the actual limits setting and the expected limits values.
static void TestRLimits(void)
{
    struct rlimit lim;

    int i;
    for (i = 0; i < NUM_RLIMITS; i++)
    {
        LE_ASSERT(getrlimit(RLimit[i], &lim) != -1);

        LE_FATAL_IF( (lim.rlim_cur != ExpectedLimits[i]) ||
                     (lim.rlim_max != ExpectedLimits[i]),
                    "%s: expected %lu, actually %lu", RLimitStr[i], ExpectedLimits[i], lim.rlim_cur);
    }
}



COMPONENT_INIT
{
    LE_INFO("======== Starting Resource Limits Test ========");

    GetExpectedLimits();
    TestRLimits();
    TestFileSystemSize();

    LE_INFO("======== Passed ========");
    exit(EXIT_SUCCESS);
}
