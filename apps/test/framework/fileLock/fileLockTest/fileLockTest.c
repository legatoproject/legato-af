#include "legato.h"

#define TEST_FILE       "/tmp/lockTestCount"
#define TEST_FILE2      "/tmp/lockTestCount2"


static void ReadAndIncCount(bool useLocks)
{
    // Open the file for reading and writing.
    int fd;
    if (useLocks)
    {
        fd = le_flock_Open(TEST_FILE, LE_FLOCK_READ_AND_WRITE);
    }
    else
    {
        fd = open(TEST_FILE, O_RDWR | O_APPEND);
    }

    LE_ASSERT(fd > 0);

    // Read the all bytes in the file.
    char count = 0;
    ssize_t r;
    do
    {
        r = read(fd, &count, 1);
    }
    while (r == 1);

    LE_ASSERT(r == 0);

    // Simulate doing something else for a little while by sleeping.  This should cause a race if
    // the files aren't locked.
    sleep(1);

    // Increment the count and write it back to the file.
    count++;
    LE_ASSERT(write(fd, &count, 1) == 1);

    // Close the file.
    if (useLocks)
    {
        // And release the lock.
        le_flock_Close(fd);
    }
    else
    {
        LE_ASSERT(close(fd) == 0);
    }
}

static void ForkAndIncCount(bool useLocks)
{
    pid_t pid = fork();
    LE_ASSERT(pid >= 0);

    if (pid == 0)
    {
        ReadAndIncCount(useLocks);

        exit(EXIT_SUCCESS);
    }

    ReadAndIncCount(useLocks);

    // Wait till the child is done.
    LE_ASSERT(wait(NULL) != -1);
}

static bool CheckCounts(int expectedCount)
{
    // Open the file and see if all the counts are in order and the goes up to the expected count.
    int fd = le_flock_Open(TEST_FILE, LE_FLOCK_READ);
    LE_ASSERT(fd > 0);

    // Read the all bytes in the file.
    char count = 0;
    char prevCount = count;

    ssize_t r;
    while (1)
    {
        r = read(fd, &count, 1);

        if (r == 1)
        {
printf("%d\n", count);
            if (count == prevCount + 1)
            {
                // Count is correct.
                prevCount = count;
            }
            else
            {
                LE_INFO("Count is out of order.");

                le_flock_Close(fd);
                return false;
            }
        }
        else
        {
            break;
        }
    }

    LE_ASSERT(r == 0);

    if (count == expectedCount)
    {
        return true;
    }
    else
    {
        LE_INFO("Count is incorrect.");
        return false;
    }
}

// Checks that files opened/created with le_fileLock API have the right access modes
// and file status flags.
static void CheckFlags(void)
{
    // Test Create function.
    int fd = le_flock_Create(TEST_FILE2, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_flock_Close(fd);

    fd = le_flock_Create(TEST_FILE2, LE_FLOCK_WRITE, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_flock_Close(fd);

    fd = le_flock_Create(TEST_FILE2, LE_FLOCK_APPEND, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_flock_Close(fd);

    fd = le_flock_Create(TEST_FILE2, LE_FLOCK_READ_AND_WRITE, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_flock_Close(fd);

    fd = le_flock_Create(TEST_FILE2, LE_FLOCK_READ_AND_APPEND, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_flock_Close(fd);

    // Test Open function.
    fd = le_flock_Open(TEST_FILE2, LE_FLOCK_READ);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_flock_Close(fd);

    fd = le_flock_Open(TEST_FILE2, LE_FLOCK_WRITE);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_flock_Close(fd);

    fd = le_flock_Open(TEST_FILE2, LE_FLOCK_APPEND);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_flock_Close(fd);

    fd = le_flock_Open(TEST_FILE2, LE_FLOCK_READ_AND_WRITE);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_flock_Close(fd);

    fd = le_flock_Open(TEST_FILE2, LE_FLOCK_READ_AND_APPEND);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_flock_Close(fd);

    // Test Create Stream function.
    FILE* filePtr = le_flock_CreateStream(TEST_FILE2, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST,
                                          S_IRWXU, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_flock_Close(fd);

    filePtr = le_flock_CreateStream(TEST_FILE2, LE_FLOCK_WRITE, LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_flock_Close(fd);

    filePtr = le_flock_CreateStream(TEST_FILE2, LE_FLOCK_APPEND, LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_flock_Close(fd);

    filePtr = le_flock_CreateStream(TEST_FILE2, LE_FLOCK_READ_AND_WRITE, LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_flock_Close(fd);

    filePtr = le_flock_CreateStream(TEST_FILE2, LE_FLOCK_READ_AND_APPEND, LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_flock_Close(fd);

    // Test Open Stream function.
    filePtr = le_flock_OpenStream(TEST_FILE2, LE_FLOCK_READ, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_flock_Close(fd);

    filePtr = le_flock_OpenStream(TEST_FILE2, LE_FLOCK_WRITE, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_flock_Close(fd);

    filePtr = le_flock_OpenStream(TEST_FILE2, LE_FLOCK_APPEND, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_flock_Close(fd);

    filePtr = le_flock_OpenStream(TEST_FILE2, LE_FLOCK_READ_AND_WRITE, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_flock_Close(fd);

    filePtr = le_flock_OpenStream(TEST_FILE2, LE_FLOCK_READ_AND_APPEND, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_flock_Close(fd);
}


COMPONENT_INIT
{
    LE_INFO("======== Starting File Locking Test ========");

    // Test without locking first.
    int fd = creat(TEST_FILE, S_IRWXU);
    LE_ASSERT(fd != -1);
    LE_ASSERT(close(fd) == 0);

    ForkAndIncCount(false);

    // Check the counts in the file.  The expected count is the number of the processes writing into
    // the file.
    LE_FATAL_IF(CheckCounts(2), "The file check is correct but shouldn't be.");
    LE_INFO("The file check is incorrect without file locks as expected.");

    // Recreate the file and run the same test with file locking.
    fd = le_flock_Create(TEST_FILE, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
    LE_ASSERT(fd > 0);
    le_flock_Close(fd);

    ForkAndIncCount(true);

    // Check the counts in the file.  The expected count is the number of the processes writing into
    // the file.
    LE_FATAL_IF(!CheckCounts(2), "The file check is incorrect.");
    LE_INFO("The file check is correct with file locks.");

    // Check file descriptor flags.
    CheckFlags();

    LE_INFO("======== File Locking Test Completed Successfully ========");
    exit(EXIT_SUCCESS);
}
