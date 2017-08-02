 /**
  * This module is for unit testing the le_mem module in the legato
  * runtime library (liblegato.so).
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include <math.h>
#include <sys/wait.h>

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

#define ID_POOL_SIZE        100
#define COLOUR_POOL_SIZE    51
#define REMOVE_THRESHOLD    RAND_MAX / 2
#define NUM_EXTRA_ID        4
#define FORCE_SIZE          3
#define NUM_EXPAND_SUB_POOL 2
#define NUM_ALLOC_SUPER_POOL    1

static unsigned int NumRelease = 0;
static unsigned int ReleaseId;

static void IdDestructor(void* objPtr)
{
    NumRelease++;
    ReleaseId = ((idObj_t*)objPtr)->id;

    // Test for deadlock on Memory Pool's internal mutex.
    // Also test that the ID Pool (from which this object was allocated) can be found.
    LE_ASSERT(le_mem_FindPool("ID Pool") != NULL);
}


COMPONENT_INIT
{
    le_mem_PoolRef_t idPool, colourPool;
    idObj_t* idsPtr[ID_POOL_SIZE + NUM_EXTRA_ID] = {NULL};
    colourObj_t* coloursPtr[COLOUR_POOL_SIZE] = {NULL};
    unsigned int i = 0;
    unsigned int numRelease = 0;

    printf("\n");
    printf("*** Unit Test for le_mem module. ***\n");

    //
    // Create multiple pools.
    //
    idPool = le_mem_CreatePool("ID Pool", sizeof(idObj_t));
    colourPool = le_mem_CreatePool("Colour Pool", sizeof(colourObj_t));

    printf("Created two memory pools.\n");

    //
    // Expand the pools.
    //
    idPool = le_mem_ExpandPool(idPool, ID_POOL_SIZE);
    colourPool = le_mem_ExpandPool(colourPool, COLOUR_POOL_SIZE);

    printf("Expanded all pools.\n");

    //
    // Set destructors.
    //
    le_mem_SetDestructor(idPool, IdDestructor);

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

        if ( WEXITSTATUS(status) == EXIT_FAILURE ) // Child terminated with an error.
        {
            printf("Assert allocation performed correctly.\n");
        }
        else
        {
            printf("Assert allocation incorrect: %d", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    //
    // Allocate all objects.
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        idsPtr[i] = le_mem_TryAlloc(idPool);

        if (NULL == idsPtr[i])
        {
            printf("Allocation error: %d", __LINE__);
            exit(EXIT_FAILURE);
        }

        idsPtr[i]->id = i;
    }

    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        coloursPtr[i] = le_mem_TryAlloc(colourPool);

        if (NULL == coloursPtr[i])
        {
            printf("Allocation error: %d", __LINE__);
            exit(EXIT_FAILURE);
        }

        coloursPtr[i]->r = i;
        coloursPtr[i]->g = i + 1;
        coloursPtr[i]->b = i + 2;
    }

    printf("Allocated all objects from all pools.\n");

    //
    // Check objects
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        if (idsPtr[i]->id != i)
        {
            printf("Object error: %d", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < COLOUR_POOL_SIZE; i++)
    {
        if ( (coloursPtr[i]->r != i) ||
             (coloursPtr[i]->g != i+1) ||
             (coloursPtr[i]->b != i+2) )
        {
            printf("Object error: %d", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    printf("Checked all objects in pools.\n");

    //
    // Randomly release some objects.
    //
    {
        //seed the random number generator with the clock
        srand((unsigned int)clock());

        idObj_t* objPtr;
        size_t numNotReleased = 0;
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

        if (NumRelease != ID_POOL_SIZE - numNotReleased)
        {
            printf("Released objects incorrectly: %d", __LINE__);
            exit(EXIT_FAILURE);
        }

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
        le_mem_PoolStats_t stats;
        le_mem_GetStats(idPool, &stats);

        if (stats.numFree != ID_POOL_SIZE)
        {
            printf("Released objects incorrectly: %d", __LINE__);
            exit(EXIT_FAILURE);
        }

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

            if ( WEXITSTATUS(status) == EXIT_FAILURE ) // Child terminated with an error.
            {
                printf("Ref count correct.\n");
            }
            else
            {
                printf("Ref count incorrect: %d", __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }

    printf("Released objects according to ref counts correctly.\n");
    printf("Checked that destructors were called correctly.\n");

    //
    // Try allocate until full.
    //
    for (i = 0; i < ID_POOL_SIZE; i++)
    {
        if (NULL == idsPtr[i])
        {
            idsPtr[i] = le_mem_TryAlloc(idPool);

            if (NULL == idsPtr[i])
            {
                printf("Allocation error: %d.", __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }

    // The pool should now be empty.
    if (le_mem_TryAlloc(idPool) != NULL)
    {
        printf("Allocation error: %d.", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Tried allocating from empty pool.\n");

    //
    // Force allocate.
    //
    le_mem_SetNumObjsToForce(idPool, FORCE_SIZE);
    for (i = ID_POOL_SIZE; i < ID_POOL_SIZE + NUM_EXTRA_ID; i++)
    {
        idsPtr[i] = le_mem_ForceAlloc(idPool);

        if (NULL == idsPtr[i])
        {
            printf("Allocation error: %d.", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    printf("Forced allocated objects.\n");

    //
    // Get stats.
    //
    le_mem_PoolStats_t stats;
    le_mem_GetStats(idPool, &stats);

    if ( (stats.numAllocs != ID_POOL_SIZE + NUM_EXTRA_ID + NumRelease) ||
         (stats.numOverflows != (unsigned int)ceil(((1.0*NUM_EXTRA_ID / FORCE_SIZE)))) ||
         (stats.numFree != (stats.numOverflows*FORCE_SIZE) % NUM_EXTRA_ID) )
    {
        printf("Stats are incorrect: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Stats are correct.\n");

    //
    // Get pool size.
    //
    if (le_mem_GetObjectCount(idPool) != ID_POOL_SIZE + (stats.numOverflows * FORCE_SIZE))
    {
        printf("Pool size incorrect: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Checked pool size.\n");

    //
    // Get object size.
    //
    if (le_mem_GetObjectSize(idPool) != sizeof(idObj_t))
    {
        printf("Object size incorrect: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Checked object size.\n");

    //
    // Reset stats.
    //
    {
        size_t numFree = stats.numFree;

        le_mem_ResetStats(idPool);
        le_mem_GetStats(idPool, &stats);

        if ( (stats.numAllocs != 0) ||
             (stats.numOverflows != 0) ||
             (stats.numFree != numFree) )
        {
            printf("Stats are incorrect: %d", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    printf("Reset stats correctly.\n");

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
    if ( (le_mem_GetObjectCount(colourSubPool1) != numRelease) ||
         (le_mem_GetObjectCount(colourPool) != COLOUR_POOL_SIZE) )
    {
        printf("Sub-pool incorrect: %d", __LINE__);
        exit(EXIT_FAILURE);
    }
    printf("Sub-pool created correctly.\n");

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
    le_mem_PoolRef_t colourSubPool2 = le_mem_CreateSubPool(colourPool, "Second sub-pool", COLOUR_POOL_SIZE - numRelease);
    printf("Created second sub-pool.\n");


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

        if (NULL == coloursPtr[i])
        {
            printf("Error allocating from sub-pool: %d", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    //
    // Check pools.
    //
    le_mem_GetStats(colourPool, &stats);
    if ( (le_mem_GetObjectCount(colourPool) != COLOUR_POOL_SIZE + NUM_EXPAND_SUB_POOL) || (stats.numFree != 0) )
    {
        printf("Error in super-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    le_mem_GetStats(colourSubPool1, &stats);
    if ( (le_mem_GetObjectCount(colourSubPool1) != numRelease) || (stats.numFree != numRelease) )
    {
        printf("Error in sub-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    le_mem_GetStats(colourSubPool2, &stats);
    if ( (le_mem_GetObjectCount(colourSubPool2) != COLOUR_POOL_SIZE - numRelease + NUM_EXPAND_SUB_POOL) ||
         (stats.numFree != NUM_EXPAND_SUB_POOL) )
    {
        printf("Error in sub-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Expanded sub-pool correctly.\n");
    printf("Allocated from sub-pools correctly.\n");

    // Try Allocating from empty super-pool.
    if (le_mem_TryAlloc(colourPool) != NULL)
    {
        printf("Error in super-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    //
    // Delete sub-pool.
    //
    le_mem_DeleteSubPool(colourSubPool1);

    // Allocate from the super-pool.
    for (i = 0; i < NUM_ALLOC_SUPER_POOL; i++)
    {
        if (COLOUR_POOL_SIZE > numRelease)
        {
            coloursPtr[numRelease] = le_mem_AssertAlloc(colourPool);
        }
    }

    //
    // Check pools.
    //
    le_mem_GetStats(colourPool, &stats);
    if ( (stats.numFree != numRelease - NUM_ALLOC_SUPER_POOL) )
    {
        printf("Error in super-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    le_mem_GetStats(colourSubPool2, &stats);
    if ( (le_mem_GetObjectCount(colourSubPool2) != COLOUR_POOL_SIZE - numRelease + NUM_EXPAND_SUB_POOL) ||
         (stats.numFree != NUM_EXPAND_SUB_POOL) )
    {
        printf("Error in sub-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Deleted sub-pool correctly.\n");

    //
    // Re-create sub-pool causing super pool to expand.
    //
    colourSubPool1 = le_mem_CreateSubPool(colourPool, "First sub-pool", numRelease + NUM_EXPAND_SUB_POOL);

    if ( (le_mem_GetObjectCount(colourSubPool1) != numRelease + NUM_EXPAND_SUB_POOL) ||
         (le_mem_GetObjectCount(colourPool) != COLOUR_POOL_SIZE + 2*NUM_EXPAND_SUB_POOL + NUM_ALLOC_SUPER_POOL) )
    {
        printf("Error re-creating sub-pool: %d", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Successfully recreated sub-pool.\n");

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
    printf("*** Unit Test for le_mem module passed. ***\n");
    printf("\n");
    exit(EXIT_SUCCESS);
}
