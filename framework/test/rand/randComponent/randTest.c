/*
 * Test the le_rand API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#if LE_CONFIG_REDUCE_FOOTPRINT
#   define BUF_SAMPLES  (8 * 1024)
#else
#   define BUF_SAMPLES  (1024 * 1024)
#endif

#define BUF_SIZE        (BUF_SAMPLES / sizeof(uint8_t))
#define MAX_INTERVAL    100

static const double Chi2Dist[MAX_INTERVAL] =
{
    3.841,
    5.991,
    7.815,
    9.488,
    11.070,
    12.592,
    14.067,
    15.507,
    16.919,
    18.307,
    19.675,
    21.026,
    22.362,
    23.685,
    24.996,
    26.296,
    27.587,
    28.869,
    30.144,
    31.410,
    32.671,
    33.924,
    35.172,
    36.415,
    37.652,
    38.885,
    40.113,
    41.337,
    42.557,
    43.773,
    44.985,
    46.194,
    47.400,
    48.602,
    49.802,
    50.998,
    52.192,
    53.384,
    54.572,
    55.758,
    56.942,
    58.124,
    59.304,
    60.481,
    61.656,
    62.830,
    64.001,
    65.171,
    66.339,
    67.505,
    68.669,
    69.832,
    70.993,
    72.153,
    73.311,
    74.468,
    75.624,
    76.778,
    77.931,
    79.082,
    80.232,
    81.381,
    82.529,
    83.675,
    84.821,
    85.965,
    87.108,
    88.250,
    89.391,
    90.531,
    91.670,
    92.808,
    93.945,
    95.081,
    96.217,
    97.351,
    98.484,
    99.617,
    100.749,
    101.879,
    103.010,
    104.139,
    105.267,
    106.395,
    107.522,
    108.648,
    109.773,
    110.898,
    112.022,
    113.145,
    114.268,
    115.390,
    116.511,
    117.632,
    118.752,
    119.871,
    120.990,
    122.108,
    123.225,
    124.342
};

static uint8_t SampleBuffer[BUF_SIZE];

static double Chi2Dist95(size_t degreesOfFreedom)
{
    if ( (degreesOfFreedom < 1) || (degreesOfFreedom > MAX_INTERVAL) )
    {
        LE_TEST_FATAL("Test error: Degrees of freedom out of range.");
    }

    return Chi2Dist[degreesOfFreedom - 1];
}

//--------------------------------------------------------------------------------------------------
/**
 * Chi-squared test.
 */
//--------------------------------------------------------------------------------------------------
static void Chi2Test(uint64_t* bucketPtr, uint32_t numBuckets, uint64_t numSamples)
{
    // Calculate the chi-square value.  Assume buckets have equal expected values.
    double expectedBucketVal = (double)numSamples / numBuckets;
    double chi2Val = 0;

    uint32_t j = 0;
    for (j = 0; j < numBuckets; j++)
    {
        double c = (bucketPtr[j] - expectedBucketVal);

        c = (c*c) / expectedBucketVal;

        chi2Val += c;

        LE_TEST_INFO("Bucket %" PRIu32 " has count of %" PRIu64, j, bucketPtr[j]);
    }

    LE_TEST_INFO("The chi-squared test statistic is %f for the current sample.", chi2Val);

    // Compare against chi-square distribution at 95% significance level.
    double chi95 = Chi2Dist95(numBuckets-1);
    LE_TEST_OK(chi2Val <= chi95, "sample bias at 95%% significance level");
}

/*
 * The values min and max must satisfy the following criteria:
 *
 * 2 < (max - min + 1) < 100 or
 * (max - min) % 100 == 0
 */
static void TestRange(uint32_t min, uint32_t max, uint64_t numSamples)
{
    LE_TEST_INFO("Test random numbers in range %" PRIu32 " to %" PRIu32 " inclusive.", min, max);

    // Create the buckets for a histogram.
    uint32_t numBuckets = max - min + 1;
    uint32_t bucketSize = 1;

    LE_TEST_ASSERT(numBuckets >= 2,
                   "test interval size: min: %" PRIu32 ", max: %" PRIu32 ", buckets: %" PRIu32,
                   max, min, numBuckets);

    if (numBuckets > MAX_INTERVAL)
    {
        LE_TEST_ASSERT((numBuckets % MAX_INTERVAL) == 0, "valid test interval");

        bucketSize = numBuckets / MAX_INTERVAL;

        numBuckets = MAX_INTERVAL;
    }

    uint64_t buckets[numBuckets];

    // Initialize the buckets.
    uint32_t j = 0;
    for (j = 0; j < numBuckets; j++)
    {
        buckets[j] = 0;
    }

    // Fill the buckets with random numbers.
    size_t i = 0;
    le_clk_Time_t startTime = le_clk_GetRelativeTime();
    for (i = 0; i < numSamples; i++)
    {
        uint32_t r = le_rand_GetNumBetween(min, max);

        // Interval check.
        if ( (r < min) || (r > max) )
        {
            LE_TEST_FATAL(
                "Random number %" PRIu32 " falls outside of range %" PRIu32 " to %" PRIu32,
                r, min, max);
        }

        // Add to bucket.
        buckets[(r - min) / bucketSize]++;

        if (0 == (i % 1000000))
        {
            le_clk_Time_t elapsedTime = le_clk_Sub(le_clk_GetRelativeTime(), startTime);
            double sampleSpeed = 0;
            if (elapsedTime.sec != 0)
            {
                sampleSpeed = ((double)(i))/elapsedTime.sec;
            }
            LE_TEST_INFO("[%.1f%%] Collecting ... %" PRIuS " samples [%.1f ksamples/s]",
                         ((float)(i)*100)/numSamples,
                         i,
                         sampleSpeed/1000);
        }
    }

    // We use a simple chi-square test here because we are only trying to detect simple biases.
    Chi2Test(buckets, numBuckets, numSamples);
}

static void TestSmallRange(void)
{
    TestRange(3, 7, 100000);
}

static void TestLargeRange(void)
{
    TestRange(9, 10000008, 40000000);
}

static void TestBuffer(void)
{
    LE_TEST_INFO("Test buffer (%" PRIuS ")", sizeof(SampleBuffer));
    memset(SampleBuffer, 0, sizeof(SampleBuffer));

    // Collect samples
    le_rand_GetBuffer(SampleBuffer, sizeof(SampleBuffer));

    const uint32_t numBuckets = 256 >> 2;
    uint64_t buckets[numBuckets];

    // Initialize the buckets.
    uint32_t i = 0;
    for (i = 0; i < numBuckets; i++)
    {
        buckets[i] = 0;
    }

    // Fill the buckets
    for (i = 0; i < BUF_SAMPLES; i++)
    {
        buckets[ SampleBuffer[i] >> 2 ] += 1;
    }

    Chi2Test(buckets, numBuckets, BUF_SAMPLES);
}

COMPONENT_INIT
{
    LE_TEST_PLAN(6);

    LE_TEST_INFO("======== Begin Random Number Tests ========");

    TestBuffer();
    TestSmallRange();
    TestLargeRange();

    LE_TEST_INFO("======== Completed Random Number Tests (Passed) ========");

    // Exit with the number of failed tests as the exit code.
    LE_TEST_EXIT;
}
