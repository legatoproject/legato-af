#include "legato.h"
#include "file.h"
#include "fileDescriptor.h"


const char* TestFileList[][3] =
        {
            {"lockTestCount", "testfile1", "testfile2"},
            {"./lockTestCount", "./testfile1", "./testfile2"},
            {"/tmp/lockTestCount", "/tmp/testfile1", "/tmp/testfile2"},
            {"/legato/lockTestCount", "/legato/testfile1", "/legato/testfile2"}
        };


#define PRINT_ERR_IF       LE_FATAL_IF
//#define PRINT_ERR_IF       LE_ERROR_IF

#define NUM_WRITE       1024/2

static const char WriteStr[] = "This string is for atomic writing";

char* AccessModeToString
(
    le_flock_AccessMode_t accessMode
)
{
    switch (accessMode)
    {
        case LE_FLOCK_READ:             return "FLOCK_READ";
        case LE_FLOCK_APPEND:           return "FLOCK_APPEND";
        case LE_FLOCK_READ_AND_APPEND:  return "FLOCK_READ_AND_APPEND";
        case LE_FLOCK_READ_AND_WRITE:   return "FLOCK_READ_AND_WRITE";
        case LE_FLOCK_WRITE:            return "FLOCK_WRITE";
    }

    return "FLOCK_NONE";
}


static int CountStringfd(int fd)
{
    int count = 0;
    ssize_t len = strlen(WriteStr);
    char readStr[len+1];
    while(true)
    {
        readStr[0] = 0;

        ssize_t readbytes= fd_ReadSize(fd, readStr, len);

        if (readbytes > 0)
        {
            readStr[readbytes] = 0;
        }

        if ( (readbytes == len) && (strcmp(WriteStr, readStr) == 0))
        {
            count++;
        }
        else if ((readbytes == LE_FAULT) || (readbytes == 0))
        {
            break;
        }
        else
        {
            LE_FATAL("Test failed. String mismatch. WriteStr: '%s', ReadStr: '%s', len: %zd,"
                     " readBytes: %zd",
                     WriteStr,
                     readStr,
                     len,
                     readbytes);
        }
    }

    return count;
}


static void WriteString(int fd, int count)
{
    int i = 0;
    int len = strlen(WriteStr);
    for(i = 0; i < count; i++)
    {
        LE_ASSERT(fd_WriteSize(fd, (void *)WriteStr, len) == len);
    }

    LE_DEBUG("Wrote '%s' %d times", WriteStr, count);
}


static void WriteStringStream(FILE* file, int count)
{
    int i = 0;

    for(i = 0; i < count; i++)
    {
        fprintf(file, "%s", WriteStr);
    }

    LE_DEBUG("Wrote '%s' %d times", WriteStr, count);
}


static void IfNumStringWritten
(
    int numStrWritten,
    const char* filePath
)
{
    int fd = open(filePath, O_RDONLY);
    int count = CountStringfd(fd);
    fd_Close(fd);

    PRINT_ERR_IF(count != numStrWritten, "Failed. Expected: %d, found: %d", numStrWritten, count);

    //Now check whether read all string back using both le_atomFile_Open and le_atomFile_Create() function
    fd = le_atomFile_Create(filePath,
                          LE_FLOCK_READ,
                          LE_FLOCK_OPEN_IF_EXIST,
                          S_IRWXU);
    LE_ASSERT(fd > 0);
    count = CountStringfd(fd);
    PRINT_ERR_IF(count != numStrWritten, "Failed. Expected: %d, found: %d", numStrWritten, count);
    le_atomFile_Close(fd);

    fd = le_atomFile_Open(filePath, LE_FLOCK_READ);
    LE_ASSERT(fd > 0);
    count = CountStringfd(fd);
    PRINT_ERR_IF(count != numStrWritten, "Failed. Expected: %d, found: %d", numStrWritten, count);
    le_atomFile_Close(fd);
}


static void TestAtomicWrite
(
    const char* filePath
)
{
    int totalStringWritten = 0;
    int len = sizeof(WriteStr);
    // Read test
    int fd = le_atomFile_Create(filePath,
                              LE_FLOCK_READ,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);

    LE_ASSERT(fd > 0);
    LE_ASSERT(fd_WriteSize(fd, (void *)WriteStr, len) == LE_FAULT);
    le_atomFile_Close(fd);

    le_flock_AccessMode_t accessMode = LE_FLOCK_WRITE;
    for (accessMode = LE_FLOCK_WRITE; accessMode <= LE_FLOCK_READ_AND_APPEND; accessMode++)
    {
        LE_INFO("Testing %s for file descriptor", AccessModeToString(accessMode));
        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE);
        le_atomFile_Close(fd);   // All writes should be committed by now.

        if ((accessMode == LE_FLOCK_APPEND) || (accessMode == LE_FLOCK_READ_AND_APPEND))
        {
            totalStringWritten += NUM_WRITE;
        }
        else if ((accessMode == LE_FLOCK_WRITE) || (accessMode == LE_FLOCK_READ_AND_WRITE))
        {
            totalStringWritten = NUM_WRITE;
        }

        IfNumStringWritten(totalStringWritten, filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE/2);  // File should have only NUM_WRIE/2 string entry
        le_atomFile_Close(fd);
        totalStringWritten = NUM_WRITE/2;
        IfNumStringWritten(totalStringWritten, filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd == LE_DUPLICATE);

        //Now delete the file and try again.
        file_Delete(filePath);
        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE);  // File should have only NUM_WRIE/2 string entry
        le_atomFile_Close(fd);
        totalStringWritten = NUM_WRITE;
        IfNumStringWritten(totalStringWritten, filePath);

        LE_INFO("%s Test Passed", AccessModeToString(accessMode));
    }


    for (accessMode = LE_FLOCK_WRITE; accessMode <= LE_FLOCK_READ_AND_APPEND; accessMode++)
    {
        LE_INFO("Testing Cancel %s for file descriptor", AccessModeToString(accessMode));

        // First write something to file
        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE);

        le_atomFile_Close(fd);   // All writes should be committed by now.

        if ((accessMode == LE_FLOCK_APPEND) || (accessMode == LE_FLOCK_READ_AND_APPEND))
        {
            totalStringWritten += NUM_WRITE;
        }
        else if ((accessMode == LE_FLOCK_WRITE) || (accessMode == LE_FLOCK_READ_AND_WRITE))
        {
            totalStringWritten = NUM_WRITE;
        }

        IfNumStringWritten(totalStringWritten, filePath);

        // Test the api when file exists (LE_FLOCK_OPEN_IF_EXIST).

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        // Now write something and cancel it.
        WriteString(fd, NUM_WRITE);

        le_atomFile_Cancel(fd);   // All writes should be cancelled by now.
        // Should be no change on file
        IfNumStringWritten(totalStringWritten, filePath);

        // Test the api when file doesn't exists(LE_FLOCK_OPEN_IF_EXIST).
        file_Delete(filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        // Now write something and cancel it.
        WriteString(fd, NUM_WRITE);

        le_atomFile_Cancel(fd);   // All writes should be cancelled by now.
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);



        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE);

        le_atomFile_Close(fd);
        totalStringWritten = NUM_WRITE;
        // Check whether write is ok
        IfNumStringWritten(totalStringWritten, filePath);


        // Test the api when file exists (LE_FLOCK_REPLACE_IF_EXIST).

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        // Now write something and cancel it.
        WriteString(fd, NUM_WRITE);

        le_atomFile_Cancel(fd);   // All writes should be cancelled by now.
        // Should be no change on file
        IfNumStringWritten(totalStringWritten, filePath);

        // Test the api when file doesn't exists(LE_FLOCK_REPLACE_IF_EXIST).
        file_Delete(filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        // Now write something and cancel it.
        WriteString(fd, NUM_WRITE);

        le_atomFile_Cancel(fd);   // All writes should be cancelled by now.
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);



        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);

        le_atomFile_Close(fd);

        // File already there. Now try to open.

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd == LE_DUPLICATE);

        // Now delete the file and try again.
        file_Delete(filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE/2);  // File should have only NUM_WRIE/2 string entry

        le_atomFile_Close(fd);
        totalStringWritten = NUM_WRITE/2;
        IfNumStringWritten(totalStringWritten, filePath);

        file_Delete(filePath);

        fd = le_atomFile_Create(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU);
        LE_ASSERT(fd > 0);
        WriteString(fd, NUM_WRITE);  // File should have only NUM_WRIE/2 string entry

        le_atomFile_Cancel(fd);
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);

        LE_INFO("%s Test Passed", AccessModeToString(accessMode));
    }


    FILE* file;
    totalStringWritten = 0;  // All strings should be erased due to cancel operation

    // Now test for file stream.
    for (accessMode = LE_FLOCK_WRITE; accessMode <= LE_FLOCK_READ_AND_APPEND; accessMode++)
    {
        LE_INFO("Testing %s for file Stream", AccessModeToString(accessMode));
        file = le_atomFile_CreateStream(filePath,
                                      accessMode,
                                      LE_FLOCK_OPEN_IF_EXIST,
                                      S_IRWXU,
                                      NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CloseStream(file);   // All writes should be committed by now.

        if ((accessMode == LE_FLOCK_APPEND) || (accessMode == LE_FLOCK_READ_AND_APPEND))
        {
            totalStringWritten += NUM_WRITE;
        }
        else if ((accessMode == LE_FLOCK_WRITE) || (accessMode == LE_FLOCK_READ_AND_WRITE))
        {
            totalStringWritten = NUM_WRITE;
        }

        IfNumStringWritten(totalStringWritten, filePath);

        file = le_atomFile_CreateStream(filePath,
                                      accessMode,
                                      LE_FLOCK_REPLACE_IF_EXIST,
                                      S_IRWXU,
                                      NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE/2);  // File should have only NUM_WRIE/2 string entry
        le_atomFile_CloseStream(file);
        totalStringWritten = NUM_WRITE/2;
        IfNumStringWritten(totalStringWritten, filePath);


        le_result_t result = LE_OK;
        file = le_atomFile_CreateStream(filePath,
                                      accessMode,
                                      LE_FLOCK_FAIL_IF_EXIST,
                                      S_IRWXU,
                                      &result);
        LE_ASSERT(result == LE_DUPLICATE);

        //Now delete the file and try again.
        file_Delete(filePath);
        file = le_atomFile_CreateStream(filePath,
                                      accessMode,
                                      LE_FLOCK_FAIL_IF_EXIST,
                                      S_IRWXU,
                                      &result);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CloseStream(file);
        totalStringWritten = NUM_WRITE;
        IfNumStringWritten(totalStringWritten, filePath);

        LE_INFO("%s Test Passed", AccessModeToString(accessMode));
    }


    for (accessMode = LE_FLOCK_WRITE; accessMode <= LE_FLOCK_READ_AND_APPEND; accessMode++)
    {
        LE_INFO("Testing Cancel %s for file Stream", AccessModeToString(accessMode));
        // First write something to file
        file = le_atomFile_CreateStream(filePath,
                                      accessMode,
                                      LE_FLOCK_OPEN_IF_EXIST,
                                      S_IRWXU,
                                      NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CloseStream(file);   // All writes should be committed by now.

        if ((accessMode == LE_FLOCK_APPEND) || (accessMode == LE_FLOCK_READ_AND_APPEND))
        {
            totalStringWritten += NUM_WRITE;
        }
        else if ((accessMode == LE_FLOCK_WRITE) || (accessMode == LE_FLOCK_READ_AND_WRITE))
        {
            totalStringWritten = NUM_WRITE;
        }

        IfNumStringWritten(totalStringWritten, filePath);


        // Test the api when file exists (LE_FLOCK_OPEN_IF_EXIST).
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        // Now write something and cancel it.
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CancelStream(file);   // All writes should be cancelled by now.
        // Should be no change on file
        IfNumStringWritten(totalStringWritten, filePath);

        // Test the api when file doesn't exists(LE_FLOCK_OPEN_IF_EXIST).
        file_Delete(filePath);
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_OPEN_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        // Now write something and cancel it.
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CancelStream(file);   // All writes should be cancelled by now.
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);


        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CloseStream(file);
        totalStringWritten = NUM_WRITE;
        // Check whether write is ok
        IfNumStringWritten(totalStringWritten, filePath);


        // Test the api when file exists (LE_FLOCK_REPLACE_IF_EXIST).
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        // Now write something and cancel it.
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CancelStream(file);   // All writes should be cancelled by now.
        // Should be no change on file
        IfNumStringWritten(totalStringWritten, filePath);

        // Test the api when file doesn't exists(LE_FLOCK_REPLACE_IF_EXIST).
        file_Delete(filePath);
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        // Now write something and cancel it.
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CancelStream(file);   // All writes should be cancelled by now.
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);


        le_result_t result = LE_OK;
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU,
                              NULL);
        LE_ASSERT(file != NULL);
        le_atomFile_CloseStream(file);

        // File already there. Now try to open.
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU,
                              &result);
        LE_ASSERT(result == LE_DUPLICATE);

        // Now delete the file and try again.
        file_Delete(filePath);
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU,
                              NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE/2);  // File should have only NUM_WRIE/2 string entry
        le_atomFile_CloseStream(file);
        totalStringWritten = NUM_WRITE/2;
        IfNumStringWritten(totalStringWritten, filePath);

        file_Delete(filePath);
        file = le_atomFile_CreateStream(filePath,
                              accessMode,
                              LE_FLOCK_FAIL_IF_EXIST,
                              S_IRWXU, NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, NUM_WRITE);
        le_atomFile_CancelStream(file);
        totalStringWritten = 0;
        // File should not exist there.
        LE_ASSERT(file_Exists(filePath) == false);

        LE_INFO("%s Test Passed", AccessModeToString(accessMode));
    }
}


// Test try APIs
static void TestTryApis
(
    const char* filePath
)
{
    le_flock_AccessMode_t outAccessMode = LE_FLOCK_READ;

    for (outAccessMode = LE_FLOCK_READ; outAccessMode <= LE_FLOCK_READ_AND_APPEND; outAccessMode++)
    {
        int fd = le_atomFile_TryCreate(filePath,
                                     outAccessMode,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     S_IRWXU);
        LE_ASSERT(fd > 0);
        le_flock_AccessMode_t inAccessMode = LE_FLOCK_READ;
        for (inAccessMode = LE_FLOCK_READ; inAccessMode <= LE_FLOCK_READ_AND_APPEND; inAccessMode++)
        {
            int infd;
            if (outAccessMode != inAccessMode)
            {
                infd = le_atomFile_TryCreate(filePath,
                                           inAccessMode,
                                           LE_FLOCK_REPLACE_IF_EXIST,
                                           S_IRWXU);
                LE_ASSERT(infd == LE_WOULD_BLOCK);

                infd = le_atomFile_TryCreate(filePath,
                                           inAccessMode,
                                           LE_FLOCK_OPEN_IF_EXIST,
                                           S_IRWXU);
                LE_ASSERT(infd == LE_WOULD_BLOCK);

                infd = le_atomFile_TryCreate(filePath,
                                           inAccessMode,
                                           LE_FLOCK_FAIL_IF_EXIST,
                                           S_IRWXU);

                LE_ASSERT(infd == LE_WOULD_BLOCK);

            }
        }
        le_atomFile_Close(fd);

        FILE* file = le_atomFile_TryCreateStream(filePath,
                                               outAccessMode,
                                               LE_FLOCK_REPLACE_IF_EXIST,
                                               S_IRWXU,
                                               NULL);
        LE_ASSERT(file != NULL);
        inAccessMode = LE_FLOCK_READ;
        for (inAccessMode = LE_FLOCK_READ; inAccessMode <= LE_FLOCK_READ_AND_APPEND; inAccessMode++)
        {
            FILE* infile;
            le_result_t result = LE_OK;
            if (outAccessMode != inAccessMode)
            {
                infile = le_atomFile_TryCreateStream(filePath,
                                                   inAccessMode,
                                                   LE_FLOCK_REPLACE_IF_EXIST,
                                                   S_IRWXU,
                                                   &result);
                LE_ASSERT(infile == NULL);
                LE_ASSERT(result == LE_WOULD_BLOCK);

                infile = le_atomFile_TryCreateStream(filePath,
                                                   inAccessMode,
                                                   LE_FLOCK_OPEN_IF_EXIST,
                                                   S_IRWXU,
                                                   &result);
                LE_ASSERT(infile == NULL);
                LE_ASSERT(result == LE_WOULD_BLOCK);

                infile = le_atomFile_TryCreateStream(filePath,
                                                   inAccessMode,
                                                   LE_FLOCK_FAIL_IF_EXIST,
                                                   S_IRWXU,
                                                   &result);
                LE_ASSERT(infile == NULL);

                LE_ASSERT(result == LE_WOULD_BLOCK);

            }
        }
        le_atomFile_CloseStream(file);
    }
}


// Checks that files opened/created with le_fileLock API have the right access modes and file status flags.
static void CheckFlags
(
    const char* filePath
)
{
    // Test Create function.

    int fd = le_atomFile_Create(filePath,
                              LE_FLOCK_READ,
                              LE_FLOCK_REPLACE_IF_EXIST,
                              S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_atomFile_Close(fd);



    fd = le_atomFile_Create(filePath,
                          LE_FLOCK_WRITE,
                          LE_FLOCK_REPLACE_IF_EXIST,
                          S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_atomFile_Close(fd);

    fd = le_atomFile_Create(filePath,
                          LE_FLOCK_APPEND,
                          LE_FLOCK_REPLACE_IF_EXIST,
                          S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_atomFile_Close(fd);

    fd = le_atomFile_Create(filePath,
                          LE_FLOCK_READ_AND_WRITE,
                          LE_FLOCK_REPLACE_IF_EXIST,
                          S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_atomFile_Close(fd);

    fd = le_atomFile_Create(filePath,
                          LE_FLOCK_READ_AND_APPEND,
                          LE_FLOCK_REPLACE_IF_EXIST,
                          S_IRWXU);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_atomFile_Close(fd);

    // Test Open function.
    fd = le_atomFile_Open(filePath, LE_FLOCK_READ);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_atomFile_Close(fd);

    fd = le_atomFile_Open(filePath, LE_FLOCK_WRITE);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_atomFile_Close(fd);

    fd = le_atomFile_Open(filePath, LE_FLOCK_APPEND);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_atomFile_Close(fd);

    fd = le_atomFile_Open(filePath, LE_FLOCK_READ_AND_WRITE);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_atomFile_Close(fd);

    fd = le_atomFile_Open(filePath, LE_FLOCK_READ_AND_APPEND);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_atomFile_Close(fd);


    // Test Create Stream function.
    FILE* filePtr = le_atomFile_CreateStream(filePath,
                                           LE_FLOCK_READ,
                                           LE_FLOCK_REPLACE_IF_EXIST,
                                           S_IRWXU,
                                           NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_atomFile_CloseStream(filePtr);


    filePtr = le_atomFile_CreateStream(filePath,
                                     LE_FLOCK_WRITE,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     S_IRWXU,
                                     NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_CreateStream(filePath,
                                     LE_FLOCK_APPEND,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     S_IRWXU,
                                     NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_CreateStream(filePath,
                                     LE_FLOCK_READ_AND_WRITE,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     S_IRWXU,
                                     NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_CreateStream(filePath,
                                     LE_FLOCK_READ_AND_APPEND,
                                     LE_FLOCK_REPLACE_IF_EXIST,
                                     S_IRWXU,
                                     NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_atomFile_CloseStream(filePtr);

    // Test Open Stream function.
    filePtr = le_atomFile_OpenStream(filePath, LE_FLOCK_READ, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDONLY);
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_OpenStream(filePath, LE_FLOCK_WRITE, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_WRONLY);
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_OpenStream(filePath, LE_FLOCK_APPEND, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_WRONLY|O_APPEND));
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_OpenStream(filePath, LE_FLOCK_READ_AND_WRITE, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == O_RDWR);
    le_atomFile_CloseStream(filePtr);

    filePtr = le_atomFile_OpenStream(filePath, LE_FLOCK_READ_AND_APPEND, NULL);
    LE_ASSERT(NULL != filePtr);
    fd = fileno(filePtr);
    LE_ASSERT(fd > 0);
    LE_ASSERT(fcntl(fd, F_GETFL) == (O_RDWR|O_APPEND));
    le_atomFile_CloseStream(filePtr);
}

void TestMultiProcessAccess
(
    const char* filePath
)
{

    // Do create test for non-existing file
    unlink(filePath);
    pid_t childPID = fork();

    // child
    if (childPID == 0)
    {
        // sleep for a while to make sure that parent process can put a write lock to the file
        sleep(5);
        int childFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_REPLACE_IF_EXIST,
                                         S_IRWXU);
        LE_INFO("the lock is released!!!!!!!++++++++++++++");
        LE_INFO("Child is writing");
        while (true)
        {
            write(childFD, "child process write", sizeof("child process write"));
        }
    }
    // parent
    else if (childPID > 0)
    {

        int parentFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_REPLACE_IF_EXIST,
                                          S_IRWXU);
        WriteString(parentFD, NUM_WRITE);
        LE_INFO("Parent Finished writing");
        sleep(5);
        le_atomFile_Close(parentFD);
        LE_INFO("Parent closed file");
        sleep(5);
        kill(childPID, SIGKILL);
        LE_INFO("Parent killed child");
        IfNumStringWritten(NUM_WRITE, filePath);
    }

    // Do create test(READ_ONLY for non-existing file)
    unlink(filePath);
    childPID = fork();

    // child
    if (childPID == 0)
    {
        LE_INFO("Performing create test for LE_FLOCK_READ, file: %s", filePath);
        int childFD1 = le_atomFile_Create(filePath, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST,
                                          S_IRWXU);
        int childFD2 = le_atomFile_Create(filePath, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST,
                                          S_IRWXU);
        LE_INFO("Child opened file: %s with read-only access", filePath);
        LE_INFO("Child is trying to write");
        LE_ASSERT(fd_WriteSize(childFD1, "Unsuccessful write",
                                          sizeof("Unsuccessful write")) == LE_FAULT);
        LE_ASSERT(fd_WriteSize(childFD2, "Unsuccessful write",
                                          sizeof("Unsuccessful write")) == LE_FAULT);
        LE_INFO("Child canceling file creation");
        le_atomFile_Cancel(childFD1);
        le_atomFile_Cancel(childFD2);
        LE_ASSERT(file_Exists(filePath) == false);

        childFD2 = le_atomFile_Create(filePath, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
        LE_INFO("Child opened file: %s with read-only access", filePath);
        LE_INFO("Child is trying to write");
        LE_ASSERT(fd_WriteSize(childFD2, "Unsuccessful write",
                                          sizeof("Unsuccessful write")) == LE_FAULT);
        //Sleep for very long time, and parent will kill it by kill signal
        sleep(2000);
    }
    // parent
    else if (childPID > 0)
    {
        sleep(5);
        LE_INFO("Parent killing child");
        kill(childPID, SIGKILL);
        LE_ASSERT(file_Exists(filePath) == false);

        int parentFD = le_atomFile_Create(filePath, LE_FLOCK_READ, LE_FLOCK_REPLACE_IF_EXIST, S_IRWXU);
        LE_INFO("Parent opened file: %s with read-only access", filePath);
        LE_INFO("Parent is trying to write");
        LE_ASSERT(fd_WriteSize(parentFD, "Unsuccessful write", sizeof("Unsuccessful write")) == LE_FAULT);
        le_atomFile_Close(parentFD);
        LE_ASSERT(file_Exists(filePath) == true);
        LE_INFO("----Passed create test for LE_FLOCK_READ, file: %s ----------", filePath);
    }

    // Do open test for existing file.

    childPID = fork();

    // child
    if (childPID == 0)
    {
        // sleep for a while to make sure that parent process can put a write lock to the file
        sleep(5);
        int childFD = le_atomFile_Open(filePath, LE_FLOCK_WRITE);
        LE_INFO("the lock is released!!!!!!!++++++++++++++");
        LE_INFO("Child is writing");
        while (true)
        {
            write(childFD, "child process write", sizeof("child process write"));
        }
    }
    // parent
    else if (childPID > 0)
    {

        int parentFD = le_atomFile_Open(filePath, LE_FLOCK_WRITE);
        WriteString(parentFD, NUM_WRITE);
        LE_INFO("Parent Finished writing");
        sleep(5);
        le_atomFile_Close(parentFD);
        LE_INFO("Parent closed file");
        sleep(5);
        kill(childPID, SIGKILL);
        LE_INFO("Parent killed child");
        IfNumStringWritten(NUM_WRITE, filePath);
    }

    // Do create test for existing file.
    childPID = fork();

    // child
    if (childPID == 0)
    {
        // sleep for a while to make sure that parent process can put a write lock to the file
        sleep(5);
        int childFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST, S_IRWXU);
        LE_INFO("the lock is released!!!!!!!++++++++++++++");
        LE_INFO("Child is writing");
        while (true)
        {
            write(childFD, "child process write", sizeof("child process write"));
        }
    }
    // parent
    else if (childPID > 0)
    {

        int parentFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST,
                                          S_IRWXU);
        WriteString(parentFD, NUM_WRITE);
        LE_INFO("Parent Finished writing");
        sleep(5);
        le_atomFile_Close(parentFD);
        LE_INFO("Parent closed file");
        sleep(5);
        kill(childPID, SIGKILL);
        LE_INFO("Parent killed child");
        IfNumStringWritten(NUM_WRITE, filePath);
    }

    // Now check file permission retention

    // first unlink file
    unlink(filePath);
    childPID = fork();

    // child
    if (childPID == 0)
    {
        // sleep for a while to make sure that parent process can put a write lock to the file
        mode_t old_mode = umask(0);
        int childFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST,
                                         S_IRWXU | S_IRWXG | S_IRWXO);
        WriteString(childFD, NUM_WRITE);
        LE_INFO("Child is write finished, now exiting without committing file changes");
        umask(old_mode);
        exit(EXIT_SUCCESS);
    }
    // parent
    else if (childPID > 0)
    {
        sleep(5);
        mode_t old_mode = umask(0);
        int parentFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST,
                                          S_IRWXU);
        LE_INFO("Parent process unblocked and started writing");
        WriteString(parentFD, NUM_WRITE/2);
        le_atomFile_Close(parentFD);
        LE_INFO("Parent closed file");
        IfNumStringWritten(NUM_WRITE/2, filePath);
        umask(old_mode);

        struct stat fileStatus;
        stat(filePath, &fileStatus);
        mode_t origMode = fileStatus.st_mode;
        LE_ASSERT((origMode & (S_IRWXU|S_IRWXG|S_IRWXO)) == S_IRWXU);
    }


    // Now check the file deletion.

    LE_INFO("Doing file deletion API test");
    childPID = fork();
    // child
    if (childPID == 0)
    {
        // sleep for a while to make sure that parent process can put a write lock to the file
        sleep(5);
        LE_INFO("Child is trying to delete file: %s", filePath);
        le_result_t result = le_atomFile_TryDelete(filePath);
        LE_ASSERT(result == LE_WOULD_BLOCK);
        result = le_atomFile_Delete(filePath);
        LE_ASSERT(result == LE_OK);
        LE_INFO("the lock is released!! and file deleted !!!");
        exit(EXIT_SUCCESS);
    }
    // parent
    else if (childPID > 0)
    {

        int parentFD = le_atomFile_Create(filePath, LE_FLOCK_WRITE, LE_FLOCK_OPEN_IF_EXIST, S_IRWXU);
        LE_INFO("Parent writing in file: %s", filePath);
        WriteString(parentFD, NUM_WRITE);
        sleep(10);
        le_atomFile_Close(parentFD);
        LE_INFO("Parent Closed file: %s", filePath);
        sleep(5);
        le_result_t result = le_atomFile_Delete(filePath);
        LE_ASSERT(result == LE_NOT_FOUND);
    }
}


void TestAccessMode
(
    const char* filePath
)
{

    mode_t old_mode = umask((mode_t)0);

    le_flock_AccessMode_t outAccessMode = LE_FLOCK_WRITE;

    for (outAccessMode = LE_FLOCK_WRITE; outAccessMode <= LE_FLOCK_READ_AND_APPEND; outAccessMode++)
    {
        if (file_Exists(filePath))
        {
            file_Delete(filePath);
        }

        int fd = le_atomFile_Create(filePath,
                                    outAccessMode,
                                    LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU|S_IRWXG|S_IRWXO);
        LE_ASSERT(fd > 0);
        WriteString(fd, 10);
        le_atomFile_Close(fd);

        struct stat fileStatus;
        stat(filePath, &fileStatus);
        mode_t origMode = fileStatus.st_mode;
        LE_ASSERT((origMode & (S_IRWXU|S_IRWXG|S_IRWXO)) == (S_IRWXU|S_IRWXG|S_IRWXO));

        fd = le_atomFile_Open(filePath, outAccessMode);
        LE_ASSERT(fd > 0);
        WriteString(fd, 10);
        le_atomFile_Close(fd);
        stat(filePath, &fileStatus);
        mode_t updateMode = fileStatus.st_mode;
        LE_ASSERT(origMode == updateMode);

        fd = le_atomFile_Create(filePath,
                                outAccessMode,
                                LE_FLOCK_REPLACE_IF_EXIST,
                                S_IRWXU|S_IRWXG); // Request to change mode, but it shouldn't change
        LE_ASSERT(fd > 0);
        WriteString(fd, 10);
        le_atomFile_Close(fd);
        stat(filePath, &fileStatus);
        updateMode = fileStatus.st_mode;
        LE_ASSERT(origMode == updateMode);


        // Do test for stream.

        if (file_Exists(filePath))
        {
            file_Delete(filePath);
        }

        FILE* file = le_atomFile_CreateStream(filePath,
                                    outAccessMode,
                                    LE_FLOCK_REPLACE_IF_EXIST,
                                    S_IRWXU|S_IRWXG|S_IRWXO,
                                    NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, 10);
        le_atomFile_CloseStream(file);

        stat(filePath, &fileStatus);
        origMode = fileStatus.st_mode;
        LE_ASSERT((origMode & (S_IRWXU|S_IRWXG|S_IRWXO)) == (S_IRWXU|S_IRWXG|S_IRWXO));

        file = le_atomFile_OpenStream(filePath, outAccessMode, NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, 10);
        le_atomFile_CloseStream(file);
        stat(filePath, &fileStatus);
        updateMode = fileStatus.st_mode;
        LE_ASSERT(origMode == updateMode);

        file = le_atomFile_CreateStream(filePath,
                                outAccessMode,
                                LE_FLOCK_REPLACE_IF_EXIST,
                                S_IRWXU|S_IRWXG,  // Request to change mode, but it shouldn't change
                                NULL);
        LE_ASSERT(file != NULL);
        WriteStringStream(file, 10);
        le_atomFile_CloseStream(file);
        stat(filePath, &fileStatus);
        updateMode = fileStatus.st_mode;
        LE_ASSERT(origMode == updateMode);
    }

    umask(old_mode);
}


COMPONENT_INIT
{

    LE_INFO("======== Starting Atomic File Access API Test ========");

    int i = 0;

    for (i = 0; i < 4; i++)
    {
        LE_INFO("======== Starting Checking flag test for file: %s ========", TestFileList[i][0]);
        // Check file descriptor flags.
        CheckFlags(TestFileList[i][0]);
        LE_INFO("======== Checking flag test Done ========");

        LE_INFO("======== Starting atomic write test for file: %s ========", TestFileList[i][1]);
        TestAtomicWrite(TestFileList[i][1]);
        LE_INFO("======== Atomic write test done ========");

        LE_INFO("======== Starting try api test for file: %s ========", TestFileList[i][2]);
        TestTryApis(TestFileList[i][2]);
        LE_INFO("======== Try api test done ========");

        LE_INFO("======== Starting permission mode test for file: %s ========", TestFileList[i][2]);
        TestAccessMode(TestFileList[i][2]);
        LE_INFO("======== Permission test done ========");

        LE_INFO("======== Starting multi process test for file: %s ========", TestFileList[i][2]);
        TestMultiProcessAccess(TestFileList[i][2]);
        LE_INFO("======== Multi process test done ========");

        int j = 0;
        for (j = 0; j < 3; j++)
        {
            if (file_Exists(TestFileList[i][j]))
            {
                file_Delete(TestFileList[i][j]);
            }
        }
    }

    LE_INFO("======== Atomic File Access API test Completed Successfully ========");
    exit(EXIT_SUCCESS);
}
