/**
 * This module is for unit testing the le_mem module in the legato
 * runtime library (liblegato.so).
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

// Define TEST_MEM_VALGRIND to 0 if not running under valgrind.
// Valgrind makes stats reporting unreliable.
#if LE_CONFIG_MEM_POOLS
#  define TEST_MEM_VALGRIND 0
#else
#  define TEST_MEM_VALGRIND 1
#endif

typedef struct
{
    uint32_t id;
}
idObj_t;


typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
}
colourObj_t;

#define ID_POOL_SIZE           100
#define COLOUR_POOL_SIZE        51
#define STRING_POOL_SIZE        11
#define STRING_POOL_BYTES      266
#define STRING_POOL_MED_BYTES   90
#define STRING_POOL_SMALL_BYTES  4
#define REMOVE_THRESHOLD    RAND_MAX / 2
#define NUM_EXTRA_ID        4
#define FORCE_SIZE          3
#define NUM_EXPAND_SUB_POOL 2
#define NUM_ALLOC_SUPER_POOL    1

static unsigned int NumRelease = 0;
static unsigned int ReleaseId;

LE_MEM_DEFINE_STATIC_POOL(StaticIdPool, ID_POOL_SIZE, sizeof(idObj_t));
LE_MEM_DEFINE_STATIC_POOL(StaticColourPool, COLOUR_POOL_SIZE, sizeof(colourObj_t));
LE_MEM_DEFINE_STATIC_POOL(StaticStringsPool, STRING_POOL_SIZE, STRING_POOL_BYTES);

static void IdDestructor(void* objPtr)
{
    NumRelease++;
    ReleaseId = ((idObj_t*)objPtr)->id;

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_NAMES_ENABLED), 1);
    // Test for deadlock on Memory Pool's internal mutex.
    // Also test that the ID Pool (from which this object was allocated) can be found.
    LE_TEST_OK(le_mem_FindPool("ID Pool") != NULL, "Check pool");
    LE_TEST_END_SKIP();
}

static void StaticIdDestructor(void *objPtr)
{
    NumRelease++;
    ReleaseId = ((idObj_t*)objPtr)->id;

    // Test for deadlock on Memory Pool's internal mutex.
    // Also test that the ID Pool (from which this object was allocated) can be found.
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_NAMES_ENABLED), 1);
    LE_TEST_OK(le_mem_FindPool("StaticIdPool") != NULL, "Check static pool");
    LE_TEST_END_SKIP();
}

static void TestPools
(
    le_mem_PoolRef_t idPool,            ///< [IN] ID pool
    le_mem_PoolRef_t colourPool,        ///< [IN] Colour pool
    le_mem_PoolRef_t stringPool         ///< [IN] String pool
)
{
    idObj_t* idsPtr[ID_POOL_SIZE + NUM_EXTRA_ID] = {NULL};
    colourObj_t* coloursPtr[COLOUR_POOL_SIZE] = {NULL};
    char* stringsPtr[4*STRING_POOL_SIZE] = {NULL};
    unsigned int i = 0;
    unsigned int numRelease = 0;

    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_LINUX), 1);
#if LE_CONFIG_LINUX
    //
    // Spawn child process and perform Assert allocation until failure.
    //
    pid_t pID = fork();
    if (pID == 0)
    {
        // This is the child.

        // Allocate more than the available objects so the assert will kill the process.
        for (i = 0; i < ID_POOL_SIZE + 1; i++)
        {
            idsPtr[i] = le_mem_AssertAlloc(idPool);
        }

        exit(EXIT_SUCCESS);
    }
    else
    {
        int status;

        // wait for the child to terminate.
        wait(&status);

        LE_TEST_OK(WEXITSTATUS(status) == EXIT_FAILURE, "Assert allocation");
    }
#endif /* end LE_CONFIG_LINUX */
    LE_TEST_END_SKIP();

    //
    // Allocate all objects.
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        idsPtr[i] = le_mem_TryAlloc(idPool);

        LE_TEST_OK(NULL != idsPtr[i], "Allocate id %d", i);
        if (NULL != idsPtr[i])
        {
            idsPtr[i]->id = i;
        }
    }

    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        coloursPtr[i] = le_mem_TryAlloc(colourPool);

        LE_TEST_OK(NULL != coloursPtr[i], "Allocate color %d", i);

        if (NULL != idsPtr[i])
        {
            coloursPtr[i]->r = i;
            coloursPtr[i]->g = i + 1;
            coloursPtr[i]->b = i + 2;
        }
    }

    //
    // Check objects
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        LE_TEST_OK(idsPtr[i] && (idsPtr[i]->id == i), "Check id %d", i);
    }

    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        LE_TEST_OK(coloursPtr[i] &&
                   ((coloursPtr[i]->r == i) &&
                    (coloursPtr[i]->g == i+1) &&
                    (coloursPtr[i]->b == i+2)), "Check color %d", i);
    }

    //
    // Randomly release some objects.
    //
    {
        struct timespec ts;

        clock_gettime(CLOCK_MONOTONIC, &ts);
        //seed the random number generator with the clock
        srand((unsigned int)(ts.tv_nsec/1000)^ts.tv_sec);

        idObj_t* objPtr;
        unsigned int numNotReleased = 0;
        NumRelease = 0;
        for (i = 0; i < ID_POOL_SIZE; i++)
        {
            objPtr = idsPtr[i];

            if (rand() < REMOVE_THRESHOLD)
            {
                // Increase the ref number for these objects.
                le_mem_AddRef(objPtr);

                // These objects should not get freed.
                numNotReleased++;
            }
            else
            {
                idsPtr[i] = NULL;
            }

            // Release all objects but only objects that did not have their ref count increased will be released.
            le_mem_Release(objPtr);
        }

        LE_TEST_OK((NumRelease == ID_POOL_SIZE - numNotReleased),
                   "Released %u/%d objects (%u remaining)",
                   NumRelease, ID_POOL_SIZE, numNotReleased);

        // Release the rest of the objects.
        for (i = 0; i < ID_POOL_SIZE; i++)
        {
            if (idsPtr[i] != NULL)
            {
                le_mem_Release(idsPtr[i]);
                idsPtr[i] = NULL;
            }
        }

        // Check the number of free objects.
        LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND || !LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_STATS), 1);
        le_mem_PoolStats_t stats;
        le_mem_GetStats(idPool, &stats);

        LE_TEST_OK((stats.numFree == ID_POOL_SIZE),
                   "Released all objects correctly");
        LE_TEST_END_SKIP();

        LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_LINUX), 2);
#if LE_CONFIG_LINUX
        // Spawn child process and try to release an object that is already released.
        pid_t pID = fork();
        if (pID == 0)
        {
            // This is the child.
            // This allocation should fail and kill the process.
            le_mem_Release(objPtr);

            exit(EXIT_SUCCESS);
        }
        else
        {
            int status;

            // wait for the child to terminate.
            wait(&status);

            LE_TEST_OK(( WEXITSTATUS(status) == EXIT_FAILURE ),
                       "Double free terminates process"); // Child terminated with an error.
        }
#endif /* end LE_CONFIG_LINUX */
        LE_TEST_END_SKIP();
    }

    //
    // Try allocate until full.
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        if (NULL == idsPtr[i])
        {
            idsPtr[i] = le_mem_TryAlloc(idPool);

            LE_TEST_OK(NULL != idsPtr[i], "Allocate id %d", i);
        }
    }

    // The pool should now be empty.
    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND, 1);
    LE_TEST_OK (le_mem_TryAlloc(idPool) == NULL,
                "Allocate from empty pool");
    LE_TEST_END_SKIP();

    //
    // Force allocate.
    //
    le_mem_SetNumObjsToForce(idPool, FORCE_SIZE);
    for (i = ID_POOL_SIZE; i < ID_POOL_SIZE + NUM_EXTRA_ID; i++)
    {
        idsPtr[i] = le_mem_ForceAlloc(idPool);

        LE_TEST_OK((NULL != idsPtr[i]), "Force alloc id %d", i);
    }

    //
    // Get stats.
    //
    le_mem_PoolStats_t stats;

    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND || !LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_STATS), 2);
    le_mem_GetStats(idPool, &stats);

    LE_TEST_OK(((stats.numAllocs == ID_POOL_SIZE + NUM_EXTRA_ID + NumRelease) &&
                (stats.numOverflows == (unsigned int)ceil(((1.0*NUM_EXTRA_ID / FORCE_SIZE)))) &&
                (stats.numFree == (stats.numOverflows*FORCE_SIZE) % NUM_EXTRA_ID)),
               "Check stats");

    //
    // Get pool size.
    //
    LE_TEST_OK((le_mem_GetObjectCount(idPool) == ID_POOL_SIZE + (stats.numOverflows * FORCE_SIZE)),
               "Check pool size");
    LE_TEST_END_SKIP();

    //
    // Get object size.
    //
    LE_TEST_OK((le_mem_GetObjectSize(idPool) == sizeof(idObj_t)),
               "Check object size");

    //
    // Reset stats.
    //
    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND || !LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_STATS), 1);
    {
        size_t numFree = stats.numFree;

        le_mem_ResetStats(idPool);
        le_mem_GetStats(idPool, &stats);

        LE_TEST_OK(((stats.numAllocs == 0) &&
                    (stats.numOverflows == 0) &&
                    (stats.numFree == numFree) ),
                   "Check reset stats");
    }
    LE_TEST_END_SKIP();

    //
    // Create sub-pool.
    //

    // Release some objs from the super-pool in a random manner.
    numRelease = 0;
    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        if (rand() < REMOVE_THRESHOLD)
        {
            le_mem_Release(coloursPtr[i]);
            coloursPtr[i] = NULL;
            numRelease++;
        }
    }

    // Create the sub-pool.
    le_mem_PoolRef_t colourSubPool1 = le_mem_CreateSubPool(colourPool, "Colour sub-pool", numRelease);

    //
    // Check sub-pools and super-pool.
    //
    LE_TEST_OK(NULL != colourSubPool1, "Create sub-pool");
    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND, 1);
    LE_TEST_OK(( (le_mem_GetObjectCount(colourSubPool1) == numRelease) &&
                 (le_mem_GetObjectCount(colourPool) == COLOUR_POOL_SIZE) ),
               "Check sub-pool size");
    LE_TEST_END_SKIP();

    //
    // Create second sub-pool.
    //

    // Release the rest of the objects from the super-pool.
    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        if (coloursPtr[i] != NULL)
        {
            le_mem_Release(coloursPtr[i]);
            coloursPtr[i] = NULL;
        }
    }

    // Create another sub-pool.
    le_mem_PoolRef_t colourSubPool2 =
        le_mem_CreateSubPool(colourPool, "Second sub-pool", COLOUR_POOL_SIZE - numRelease);
    LE_TEST_OK(NULL != colourSubPool2, "Create second sub-pool");

    //
    // Expand the sub-pool causing the super-pool to expand.
    //
    le_mem_ExpandPool(colourSubPool2, NUM_EXPAND_SUB_POOL);

    //
    // Allocate from sub-pool.
    //
    for (i = 0; i < COLOUR_POOL_SIZE - numRelease; i++)
    {
        coloursPtr[i] = le_mem_TryAlloc(colourSubPool2);

        LE_TEST_OK(NULL != coloursPtr[i], "Allocate color %d from sub-pool", i);
    }

    //
    // Check pools.
    //
    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND || !LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_STATS), 4);
    le_mem_GetStats(colourPool, &stats);
    LE_TEST_OK(( (le_mem_GetObjectCount(colourPool) == COLOUR_POOL_SIZE + NUM_EXPAND_SUB_POOL) &&
                 (stats.numFree == 0) ),
               "Check super-pool stats");

    le_mem_GetStats(colourSubPool1, &stats);
    LE_TEST_OK(( (le_mem_GetObjectCount(colourSubPool1) == numRelease) &&
                 (stats.numFree == numRelease) ),
               "Check sub-pool stats");

    le_mem_GetStats(colourSubPool2, &stats);
    LE_TEST_OK(( (le_mem_GetObjectCount(colourSubPool2) ==
                  COLOUR_POOL_SIZE - numRelease + NUM_EXPAND_SUB_POOL) &&
                 (stats.numFree == NUM_EXPAND_SUB_POOL) ),
               "Check second sub-pool stats");

    // Try Allocating from empty super-pool.
    LE_TEST_OK(le_mem_TryAlloc(colourPool) == NULL,
               "Allocate from empty super-pool");
    LE_TEST_END_SKIP();

    //
    // Delete sub-pool.
    //
    le_mem_DeleteSubPool(colourSubPool1);

    // Allocate from the super-pool.
    for (i = 0; i < NUM_ALLOC_SUPER_POOL; i++)
    {
        if (COLOUR_POOL_SIZE > numRelease)
        {
            coloursPtr[numRelease] = le_mem_TryAlloc(colourPool);
            LE_TEST_OK(NULL != coloursPtr[numRelease],
                       "Allocate item %d from super-pool", numRelease);
        }
    }

    //
    // Check pools.
    //
    LE_TEST_BEGIN_SKIP(TEST_MEM_VALGRIND || !LE_CONFIG_IS_ENABLED(LE_CONFIG_MEM_POOL_STATS), 3);
    le_mem_GetStats(colourPool, &stats);
    LE_TEST_OK((stats.numFree == numRelease - NUM_ALLOC_SUPER_POOL),
               "checking super-pool stats after releasing sub-pool");

    le_mem_GetStats(colourSubPool2, &stats);
    LE_TEST_OK(( (le_mem_GetObjectCount(colourSubPool2) ==
                  COLOUR_POOL_SIZE - numRelease + NUM_EXPAND_SUB_POOL) &&
                 (stats.numFree == NUM_EXPAND_SUB_POOL) ),
               "checking second sub-pool stats after releasing sub-pool");

    //
    // Re-create sub-pool causing super pool to expand.
    //
    colourSubPool1 = le_mem_CreateSubPool(colourPool, "First sub-pool", numRelease + NUM_EXPAND_SUB_POOL);

    LE_TEST_OK(((le_mem_GetObjectCount(colourSubPool1) == numRelease + NUM_EXPAND_SUB_POOL) &&
                (le_mem_GetObjectCount(colourPool) ==
                 COLOUR_POOL_SIZE + 2*NUM_EXPAND_SUB_POOL + NUM_ALLOC_SUPER_POOL)),
               "recreated sub-pool");
    LE_TEST_END_SKIP();

    // Create some reduced sub-pools
    le_mem_PoolRef_t tieredStrPoolMed = le_mem_CreateReducedPool(stringPool,
                                                                 "stringPoolMed",
                                                                 0, STRING_POOL_MED_BYTES);
    le_mem_PoolRef_t tieredStrPoolSmall = le_mem_CreateReducedPool(tieredStrPoolMed,
                                                                   "stringPoolSmall",
                                                                   4, STRING_POOL_SMALL_BYTES);

    size_t medObjectSize   = le_mem_GetObjectSize(tieredStrPoolMed);
    size_t smallObjectSize = le_mem_GetObjectSize(tieredStrPoolSmall);

    LE_TEST_OK(STRING_POOL_MED_BYTES <= medObjectSize && medObjectSize < STRING_POOL_BYTES/2,
               "Check medium pool size (%"PRIuS") is reasonable", medObjectSize);
    LE_TEST_OK(STRING_POOL_SMALL_BYTES <= smallObjectSize && smallObjectSize < medObjectSize/2,
               "Check small pool size (%"PRIuS") is reasonable", smallObjectSize);

    // Try allocating random sized strings
    int points = STRING_POOL_SIZE*4;
    for (i = 0; i < 4*STRING_POOL_SIZE && points >= 4; i++)
    {
        // Always allocate at least one byte
        size_t allocSize = (rand() % (STRING_POOL_BYTES-1)) + 1;
        stringsPtr[i] = le_mem_ForceVarAlloc(tieredStrPoolSmall, allocSize);
        LE_TEST_OK(stringsPtr[i], "allocate buffer %d (size %"PRIuS")", i, allocSize);
        memset(stringsPtr[i], 'a', allocSize);
        if (allocSize <= smallObjectSize)
        {
            points -= 1;
            LE_TEST_OK(le_mem_GetBlockSize(stringsPtr[i]) == smallObjectSize,
                       "got a small object");
        }
        else if (allocSize <= medObjectSize)
        {
            points -= 2;
            LE_TEST_OK(le_mem_GetBlockSize(stringsPtr[i]) == medObjectSize,
                       "got a medium object");
        }
        else
        {
            points -= 4;
            LE_TEST_OK(le_mem_GetBlockSize(stringsPtr[i]) == STRING_POOL_BYTES,
                       "got a large object");
        }
    }

    // Now try hibernating -- first disable interrupts
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_RTOS), 3);
#if LE_CONFIG_RTOS
    taskENTER_CRITICAL();
    void *beginFree, *endFree;
    le_mem_Hibernate(&beginFree, &endFree);
    LE_TEST_OK(endFree - beginFree > 0,
               "Free %" PRIuS " bytes of memory by hibernating", endFree - beginFree);
    le_mem_Resume();
    taskEXIT_CRITICAL();
#endif /* end LE_CONFIG_RTOS */
    LE_TEST_END_SKIP();

    // Now finish up by allocating some small strings
    for(; i < 4*STRING_POOL_SIZE && points >= 0; ++i)
    {
        stringsPtr[i] = le_mem_ForceVarAlloc(tieredStrPoolSmall, 1);
        LE_TEST_OK(stringsPtr[i], "allocate buffer %d (size 1)", i);
        memset(stringsPtr[i], 'b', 1);
        LE_TEST_OK(le_mem_GetBlockSize(stringsPtr[i]) == smallObjectSize,
                   "got a small object");
        points -= 1;
    }

    LE_TEST_INFO("Releasing some buffers");
    for (i = 0; i < 4*STRING_POOL_SIZE && stringsPtr[i]; i++)
    {
        if (rand() < REMOVE_THRESHOLD)
        {
            le_mem_Release(stringsPtr[i]);
            stringsPtr[i] = NULL;
            numRelease++;
        }
    }

    LE_TEST_INFO("Re-allocate as small buffers");
    for (; i-- > 0; )
    {
        if (!stringsPtr[i])
        {
            stringsPtr[i] = le_mem_ForceVarAlloc(tieredStrPoolSmall, 1);
            LE_TEST_OK(stringsPtr[i], "allocated a small buffer");
            memset(stringsPtr[i], 'c', 1);
        }
    }

    LE_TEST_INFO("Free everything");
    for (i = 0; i < 4*STRING_POOL_SIZE && stringsPtr[i]; ++i)
    {
        le_mem_Release(stringsPtr[i]);
        stringsPtr[i] = NULL;
    }

    // And delete the sub-pools
    LE_TEST_INFO("Delete sub-pools");
    le_mem_DeleteSubPool(tieredStrPoolSmall);
    le_mem_DeleteSubPool(tieredStrPoolMed);
}



COMPONENT_INIT
{
    le_mem_PoolRef_t idPool, colourPool, stringsPool;
    le_mem_PoolRef_t staticIdPool, staticColourPool, staticStringsPool;

    LE_TEST_INFO("Unit Test for le_mem module.");
    LE_TEST_PLAN(LE_TEST_NO_PLAN);

    LE_TEST_INFO("Creating two dynamic memory pools.");

    //
    // Create multiple pools.
    //
    idPool = le_mem_CreatePool("ID Pool", sizeof(idObj_t));
    colourPool = le_mem_CreatePool("Colour Pool", sizeof(colourObj_t));
    stringsPool = le_mem_CreatePool("Strings Pool", STRING_POOL_BYTES);

    LE_TEST_INFO("Expanding dynamic pools.");

    //
    // Expand the pools.
    //
    idPool = le_mem_ExpandPool(idPool, ID_POOL_SIZE);
    colourPool = le_mem_ExpandPool(colourPool, COLOUR_POOL_SIZE);
    stringsPool = le_mem_ExpandPool(stringsPool, STRING_POOL_SIZE);

    //
    // Set destructors.
    //
    le_mem_SetDestructor(idPool, IdDestructor);

    LE_TEST_INFO("Testing dynamic pools.");
    TestPools(idPool, colourPool, stringsPool);

    //
    // Create pools for static pool tests.
    //
    LE_TEST_INFO("Creating static pools.");
    staticIdPool = le_mem_InitStaticPool(StaticIdPool,
                                         ID_POOL_SIZE,
                                         sizeof(idObj_t));
    staticColourPool = le_mem_InitStaticPool(StaticColourPool,
                                             COLOUR_POOL_SIZE,
                                             sizeof(colourObj_t));
    staticStringsPool = le_mem_InitStaticPool(StaticStringsPool,
                                              STRING_POOL_SIZE,
                                              STRING_POOL_BYTES);

    //
    // Set destructors.
    //
    le_mem_SetDestructor(staticIdPool, StaticIdDestructor);

    LE_TEST_INFO("Testing static pools");
    TestPools(staticIdPool, staticColourPool, staticStringsPool);


    // FIXME: Find pool by name is currently suffering from issues
    // Failure is tracked by ticket LE-5909
#if 0
    //
    // Search for pools by name.
    //
    if ( (idPool != le_mem_FindPool("ID Pool")) ||
         (colourSubPool1 != le_mem_FindPool("First sub-pool")) )
    {
        printf("Error finding pools by name: %d", __LINE__);
        exit(EXIT_FAILURE);
    }
    printf("Successfully searched for pools by name.\n");
#endif
    LE_TEST_INFO("Done le_mem unit test");
    LE_TEST_EXIT;
}
