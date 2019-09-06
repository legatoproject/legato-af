/**
 * This module is for unit testing the le_fs module in the legato
 * runtime library (liblegato.so).
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  File path length
 */
// -------------------------------------------------------------------------------------------------
#define PATH_LENGTH         128

// -------------------------------------------------------------------------------------------------
/**
 *  Short data length to read/write, in bytes
 */
// -------------------------------------------------------------------------------------------------
#define SHORT_DATA_LENGTH   150

// -------------------------------------------------------------------------------------------------
/**
 *  Long data length to read/write, in bytes
 */
// -------------------------------------------------------------------------------------------------
#define LONG_DATA_LENGTH    5000


//--------------------------------------------------------------------------------------------------
// Test functions
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Test main function.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t res;

    LE_TEST_INFO("Starting FS test");

    LE_TEST_PLAN(89);

    le_fs_FileRef_t fileRef = NULL;

    // Create and open a new file
    static const char filePath[PATH_LENGTH] = "/foo/bar/test.txt";
    LE_TEST_INFO("Open file '%s'", filePath);
    res = le_fs_Open(filePath, LE_FS_CREAT | LE_FS_RDWR | LE_FS_TRUNC, &fileRef);
    LE_DEBUG("res = %d", res);
    LE_TEST_OK(LE_OK == res, "file '%s' opened", filePath);
    LE_TEST_INFO("File handler: %p", fileRef);
    LE_TEST_ASSERT(NULL != fileRef, "Check fileRef");

    // Write in file
    static const uint8_t dataToWrite[SHORT_DATA_LENGTH] = "Hello world!";
    LE_TEST_INFO("Writing '%s' in file", dataToWrite);
    size_t writeSize = strlen((char*)dataToWrite);
    LE_TEST_ASSERT(LE_OK == le_fs_Write(fileRef, dataToWrite, writeSize),
               "Write %u bytes in file '%s'", (unsigned int)writeSize, filePath);

    // Check the file exists
    LE_TEST_OK(le_fs_Exists(filePath), "File '%s' exists", filePath);

    // Get file size
    size_t fileSize = 0;
    LE_TEST_OK(LE_OK == le_fs_GetSize(filePath, &fileSize), "size of file '%s' read",
               filePath);
    LE_TEST_INFO("File size of '%s': %u", filePath, (unsigned int)fileSize);
    LE_TEST_OK(strlen((char*)dataToWrite) == fileSize, "read size: %u, expected size: %u",
                   (unsigned int)fileSize, (unsigned int)strlen((char*)dataToWrite));

    int32_t currentOffset;
    int32_t offset;
    size_t readLength;
    static uint8_t readData[SHORT_DATA_LENGTH];

    // Seek negative offset from the beginning
    offset = -5;
    currentOffset = 0;
    LE_TEST_INFO("Seek offset %d from the beginning", (int)offset);
    LE_TEST_OK(LE_FAULT == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");

    // Seek from the beginning
    offset = 5;
    currentOffset = 0;
    int32_t currentPosition = 0;
    LE_TEST_INFO("Seek offset %d from the beginning", (int)offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    currentPosition += offset;
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(currentPosition == currentOffset, "Check new position");

    // Read 3 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    size_t expectedReadLength = readLength;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength),
               "Read file '%s' from the current position", filePath);
    currentPosition += readLength;
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(expectedReadLength == readLength, "Check read length");
    LE_TEST_OK(0 == strncmp(" wo", (char*)readData, readLength), "Check read data");

    // Seek from the current position
    offset = 2;
    LE_TEST_INFO("Seek offset %"PRIi32" from the current position", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_CUR, &currentOffset), "Seek");
    currentPosition += offset;
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(currentPosition == currentOffset, "Check new position");

    // Read 3 bytes from the current position: EOF should be reached after 2 bytes
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = 2;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength),
               "Read file '%s' from the current position", filePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(expectedReadLength == readLength, "Check read lenght");
    LE_TEST_OK(0 == strncmp("d!", (char*)readData, readLength), "Check read data");

    // Read 3 bytes from the current position: EOF is already reached
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = 0;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength),
               "Read file '%s' from the current position", filePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(expectedReadLength == readLength, "Check read length");

    // Seek from the end
    offset = -5;
    LE_TEST_INFO("Seek offset %"PRIi32" from the end", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_END, &currentOffset), "Seek");
    currentPosition = strlen((char*)dataToWrite) - 5;
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(currentPosition == currentOffset, "Check new position");

    // Read 3 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = readLength;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength),
               "Read file '%s' from the current position", filePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(expectedReadLength == readLength, "Check read length");
    LE_TEST_OK(0 == strncmp("orl", (char*)readData, readLength), "Check read data");

    // Set current position to the beginning of the file
    offset = 0;
    LE_TEST_INFO("Seek offset %"PRIi32" from the beginning", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(offset == currentOffset, "Check new position");

    // Read 150 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = SHORT_DATA_LENGTH;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength), "file '%s' read", filePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(strlen((char*)dataToWrite) == readLength, "read size %u / expected size %u",
               (unsigned int)readLength, (unsigned int)strlen((char*)dataToWrite));
    LE_TEST_OK(0 == strncmp((char*)dataToWrite, (char*)readData, readLength),
               "data comparison");

    // Error cases with useless actions
    LE_TEST_INFO("Test error cases with useless actions");
    readLength = 0;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength), "read 0 length data");
    LE_TEST_OK(LE_OK == le_fs_Write(fileRef, dataToWrite, 0), "write 0 length data");

    // Close the opened file
    LE_TEST_INFO("Closing file handler: %p", fileRef);
    LE_TEST_OK(LE_OK == le_fs_Close(fileRef), "file '%s' closed", filePath);
    fileRef = NULL;

    // Open the file in read only
    LE_TEST_INFO("Open file '%s'", filePath);
    res = le_fs_Open(filePath, LE_FS_RDONLY, &fileRef);
    LE_DEBUG("res = %d", res);
    LE_TEST_OK(LE_OK == res, "file '%s' opened in read only", filePath);
    LE_TEST_INFO("File handler: %p", fileRef);
    LE_TEST_ASSERT(NULL != fileRef, "Check fileRef");

    // Set current position to the beginning of the file
    offset = 0;
    LE_TEST_INFO("Seek offset %"PRIi32" from the beginning", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(offset == currentOffset, "Check new position");

    // Read 3 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = readLength;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength),
               "Read file '%s' from the current position", filePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK(expectedReadLength == readLength, "Check read length");
    LE_TEST_OK(0 == strncmp("Hel", (char*)readData, readLength), "Check read data");

    // Close the opened file
    LE_TEST_INFO("Closing file handler: %p", fileRef);
    LE_TEST_OK(LE_OK == le_fs_Close(fileRef), "file '%s' closed", filePath);
    fileRef = NULL;

    // Move the file
    static const char newFilePath[PATH_LENGTH] = "/foo/bar/test2.txt";
    LE_TEST_INFO("Moving file from '%s' to '%s'", filePath, newFilePath);
    LE_TEST_OK(LE_OK == le_fs_Move(filePath, newFilePath), "move file");
    // Check that old file cannot be opened
    LE_TEST_ASSERT(LE_OK != le_fs_Open(filePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef),
                   "open the old file");

    // Open the file
    LE_TEST_INFO("Open file '%s'", newFilePath);
    LE_TEST_OK(LE_OK == le_fs_Open(newFilePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef),
               "open the new file in append mode");
    LE_TEST_INFO("File handler: %p", fileRef);
    LE_TEST_ASSERT(fileRef != NULL, "fileRef %p", fileRef);

    // Append text to the file
    LE_TEST_INFO("Writing '%s' in file", dataToWrite);
    writeSize = strlen((char*)dataToWrite);
    LE_TEST_OK(LE_OK == le_fs_Write(fileRef, dataToWrite, writeSize),
               "Append to file '%s'", newFilePath);

    // Set current position to the beginning of the file
    LE_TEST_INFO("Seek offset %"PRIi32" from the beginning", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(offset == currentOffset, "Check new position");

    // Read 150 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = SHORT_DATA_LENGTH;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readData, &readLength), "Read %u bytes from '%s'",
               (unsigned int)readLength, newFilePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readData);
    LE_TEST_OK((2 * strlen((char*)dataToWrite)) == readLength,
                   "read length check: expected=%u, read=%u",
                   (unsigned int)(2 * strlen((char*)dataToWrite)), (unsigned int)readLength);
    LE_TEST_OK(0 == strncmp("Hello world!Hello world!", (char*)readData, readLength),
                   "data comparison");

    // Close the opened file
    LE_TEST_INFO("Closing file handler: %p", fileRef);
    LE_TEST_OK(LE_OK == le_fs_Close(fileRef), "close file '%s'", newFilePath);
    fileRef = NULL;

    // Get file size
    fileSize = 0;
    LE_TEST_OK(LE_OK == le_fs_GetSize(newFilePath, &fileSize), "Get file size");
    LE_TEST_INFO("File size of '%s': %u", newFilePath, (unsigned int)fileSize);
    LE_TEST_OK((2 * strlen((char*)dataToWrite)) == fileSize, "File size check");

    // Create and open a new file
    static const char deleteFilePath[PATH_LENGTH] = "/foo/bar/delete.txt";
    LE_TEST_INFO("Open file '%s'", deleteFilePath);
    LE_TEST_OK(LE_OK == le_fs_Open(deleteFilePath, LE_FS_CREAT | LE_FS_RDWR, &fileRef),
               "Open file '%s'", deleteFilePath);
    LE_TEST_INFO("File handler: %p", fileRef);
    LE_TEST_OK(fileRef != NULL, "Check fileRef");

    // Close the new file
    LE_TEST_INFO("Closing file handler: %p", fileRef);
    LE_TEST_OK(LE_OK == le_fs_Close(fileRef), "close file '%s'", deleteFilePath);
    fileRef = NULL;

    // move a file to an existing file
    LE_TEST_INFO("Moving file from '%s' to '%s'", newFilePath, deleteFilePath);
    LE_TEST_OK(LE_OK == le_fs_Move(newFilePath, deleteFilePath), "move file");
    // Check that old file cannot be opened
    LE_TEST_ASSERT(LE_OK != le_fs_Open(newFilePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef),
                   "open the old file");

    // Delete the new file
    LE_TEST_INFO("Deleting file '%s'",deleteFilePath);
    LE_TEST_OK(LE_OK == le_fs_Delete(deleteFilePath), "Delete file '%s'", deleteFilePath);
    // Check that deleted file cannot be opened
    LE_TEST_OK(LE_OK != le_fs_Open(deleteFilePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef),
                   "Check file deletion");

    // Create and open a new file
    static const char loremFilePath[PATH_LENGTH] = "/bar/foo/lorem_ipsum.txt";
    LE_TEST_INFO("Open file '%s'", loremFilePath);
    LE_TEST_OK(LE_OK == le_fs_Open(loremFilePath, LE_FS_CREAT | LE_FS_RDWR | LE_FS_TRUNC, &fileRef),
               "Open file '%s'", loremFilePath);
    LE_TEST_INFO("File handler: %p", fileRef);
    LE_TEST_ASSERT(fileRef != NULL, "Check fileRef");

    // Write in file
    static const uint8_t loremIpsum[LONG_DATA_LENGTH] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla molestie metus ac ultricies \
ultricies. Mauris sollicitudin pulvinar lorem vitae vehicula. Vestibulum quam tellus, vehicula in \
consequat et, tincidunt vel ligula. In fringilla ex sit amet vehicula pharetra. Etiam porttitor \
nibh nisl, quis auctor est tincidunt id. Morbi at felis quis quam semper laoreet non ut lacus. \
Donec viverra gravida lacinia. Praesent mollis ut nisl quis consectetur. In ultrices, augue ut \
rhoncus blandit, metus orci euismod felis, scelerisque lacinia dolor est eu mauris.\
Vestibulum consectetur congue justo ut finibus. Donec vestibulum, ligula eget varius convallis, \
lorem enim maximus quam, a aliquam ligula est id ex. Donec quis mi neque. Ut elit sapien, interdum \
quis rhoncus tincidunt, lobortis ac arcu. Morbi lobortis eros nec magna pharetra molestie. \
Curabitur tristique vehicula metus non malesuada. Quisque auctor luctus arcu, eget semper quam \
malesuada at. Morbi pellentesque at nulla et ullamcorper. Etiam sollicitudin lacus urna, quis \
malesuada nisl varius quis. Mauris cursus accumsan ipsum quis consequat. Quisque blandit maximus \
arcu, vitae vulputate ex laoreet ac. Class aptent taciti sociosqu ad litora torquent per conubia \
nostra, per inceptos himenaeos. Integer luctus auctor erat, eget facilisis risus tristique nec. \
Quisque dui ligula, placerat ut arcu quis, vulputate mollis elit. Aliquam enim ex, lobortis eu \
sodales id, auctor sit amet turpis.\
Pellentesque pharetra at arcu nec porttitor. Nam semper purus vel mi egestas bibendum. Maecenas \
gravida sed turpis et euismod. Vestibulum consectetur turpis lorem, eget tincidunt augue tincidunt \
nec. Morbi cursus lacus quis velit bibendum lobortis. Maecenas auctor purus ac turpis laoreet \
efficitur. Morbi vehicula vestibulum turpis, at sodales lacus consectetur eu. Etiam faucibus \
mauris commodo eros mollis, in dignissim augue lobortis. Etiam consequat enim mi, ac interdum quam \
rutrum a. Phasellus porta porttitor dui, vitae ullamcorper mi tincidunt eu. Fusce ac purus ac \
libero iaculis imperdiet. Phasellus ultrices ac neque ut iaculis.\
Ut sit amet malesuada elit, nec vestibulum odio. Aliquam aliquet facilisis urna a congue. In \
ornare nisl sed interdum facilisis. Donec porttitor consequat convallis. Curabitur pharetra \
placerat erat, a aliquet nunc cursus eu. Pellentesque habitant morbi tristique senectus et netus \
et malesuada fames ac turpis egestas. In vitae semper arcu, ac ullamcorper ipsum. In sollicitudin \
pharetra ipsum non condimentum. Fusce congue velit vitae erat laoreet, quis pellentesque risus \
posuere. In hac habitasse platea dictumst. Suspendisse potenti. Nunc rhoncus metus ac libero \
efficitur semper. Sed viverra vulputate enim et rutrum. Quisque et nulla odio.\
Pellentesque rhoncus sodales nulla, molestie vestibulum elit semper nec. Interdum et malesuada \
fames ac ante ipsum primis in faucibus. Nulla suscipit massa ut lectus venenatis blandit. Ut \
mauris lorem, aliquet id mauris id, imperdiet maximus lectus. Curabitur in tincidunt libero. \
Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut \
vehicula dolor a mauris malesuada, at rhoncus massa ultricies. Donec elit leo, sollicitudin eu \
urna et, suscipit dictum nulla. Donec euismod quam porttitor leo sagittis dictum. Duis eleifend \
est sit amet imperdiet maximus. Phasellus aliquam molestie iaculis. Cras sed quam enim. Curabitur \
viverra sem vel nibh interdum, in sollicitudin nisi facilisis. Aliquam et sagittis quam, ut \
molestie libero. Sed dignissim tortor sit amet mi auctor pretium.\
Phasellus vel arcu eu dui laoreet tincidunt. Maecenas in pellentesque diam, a egestas sapien. \
Aenean vulputate, justo eget venenatis sagittis, dolor nunc tempus nisi, eu dapibus nunc nisi non \
mauris. Nulla lacinia vel metus eu maximus. Nullam posuere diam at condimentum sollicitudin. \
Nullam non ligula massa. Aenean pharetra suscipit libero, ut tincidunt felis sagittis vitae. \
Maecenas consectetur velit nec mauris lacinia, eu condimentum odio porta. Aliquam lobortis libero \
non lacinia maximus. Curabitur rhoncus commodo quam eget feugiat. Mauris in justo sem. Morbi \
ornare pulvinar sapien, vel elementum nunc rutrum maximus.\
Nunc dignissim vestibulum felis eget commodo. Integer a tincidunt dui, eu consequat sapien. \
Suspendisse aliquam est in cursus blandit. Aliquam erat volutpat. Mauris porta lacus eget nisi \
elementum, vel ultrices velit accumsan. Maecenas vehicula, orci vitae ultrices pharetra, purus \
nulla semper ex, sit amet condimentum lorem nisl sed est. Morbi quis ultricies libero. Nam \
efficitur volutpat ligula. Integer sit amet iaculis enim. Proin lobortis urna luctus semper \
feugiat. Cras suscipit quam sit amet urna tristique, nec rhoncus odio tincidunt. Proin vulputate \
facilisis erat, a imperdiet risus eleifend nec.";
    LE_TEST_INFO("Writing Lorem ipsum in file");
    writeSize = strlen((char*)loremIpsum);
    LE_TEST_OK(LE_OK == le_fs_Write(fileRef, loremIpsum, writeSize),
               "Write %u bytes in file '%s'", (unsigned int)writeSize, loremFilePath);

    // Set current position to the beginning of the file
    offset = 0;
    LE_TEST_INFO("Seek offset %"PRIi32" from the beginning", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(offset == currentOffset, "Check new position");

    // Read 5000 bytes from the current position
    static uint8_t readLoremIpsum[LONG_DATA_LENGTH];
    memset(readLoremIpsum, '\0', LONG_DATA_LENGTH);
    readLength = LONG_DATA_LENGTH;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readLoremIpsum, &readLength),
               "Read %u byte in file '%s'", (unsigned int)readLength, loremFilePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readLoremIpsum);
    LE_TEST_OK(strlen((char*)loremIpsum) == readLength, "Check read length");

    // Set current position to the beginning of the file
    offset = 0;
    LE_TEST_INFO("Seek offset %"PRIi32" from the beginning", offset);
    LE_TEST_OK(LE_OK == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset), "Seek");
    LE_TEST_INFO("New position in file: %"PRIi32"", currentOffset);
    LE_TEST_OK(offset == currentOffset, "Check new position");

    // Read 150 bytes from the current position
    memset(readLoremIpsum, '\0', LONG_DATA_LENGTH);
    readLength = SHORT_DATA_LENGTH;
    LE_TEST_OK(LE_OK == le_fs_Read(fileRef, readLoremIpsum, &readLength),
               "Read %u byte in file '%s'", (unsigned int)readLength, loremFilePath);
    LE_TEST_INFO("Read %d bytes: '%s'", (int)readLength, readLoremIpsum);
    LE_TEST_OK(SHORT_DATA_LENGTH == readLength, "Check read length");

    // Close the opened file
    LE_TEST_INFO("Closing file handler: %p", fileRef);
    LE_TEST_OK(LE_OK == le_fs_Close(fileRef), "Close file '%s'", loremFilePath);
    fileRef = NULL;

    // Remove all created files and directories
    LE_TEST_INFO("Remove all created files and directories");
    LE_TEST_OK(LE_OK == le_fs_RemoveDirRecursive("/foo"), "Remove directory '/foo'");
    LE_TEST_ASSERT(false == le_fs_Exists("/foo"), "Check if the directory '/foo' is deleted");
    LE_TEST_OK(LE_OK == le_fs_RemoveDirRecursive("/bar"), "Remove directory '/bar'");
    LE_TEST_ASSERT(false == le_fs_Exists("/bar"), "Check if the directory '/bar' is deleted");


    // Error cases with wrong file handler
    LE_TEST_INFO("Test error cases with file handler %p", fileRef);
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Close(fileRef), "Test le_fs_Close with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Read(fileRef, readLoremIpsum, &readLength),
               "Test le_fs_Read with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Write(fileRef, loremIpsum, strlen((char*)loremIpsum)),
               "Test le_fs_Write with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Seek(fileRef, 5, LE_FS_SEEK_SET, &currentOffset),
               "Test le_fs_Seek with bad ref");

    fileRef = (void*)-1;

    LE_TEST_INFO("Test error cases with file handler %p", fileRef);
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Close(fileRef), "Test le_fs_Close with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Read(fileRef, readLoremIpsum, &readLength),
               "Test le_fs_Read with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Write(fileRef, loremIpsum, strlen((char*)loremIpsum)),
               "Test le_fs_Write with bad ref");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Write(fileRef, loremIpsum, 0),
               "Test le_fs_Write with bad ref and a length to zero");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Seek(fileRef, 5, LE_FS_SEEK_SET, &currentOffset),
               "Test le_fs_Seek with bad ref");

    // Error cases with wrong file paths
    static const char wrongFilePath[PATH_LENGTH] = "foo/bar/";
    LE_TEST_INFO("Test error cases with file path '%s'", wrongFilePath);
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Open(wrongFilePath, LE_FS_RDWR, &fileRef),
               "Test le_fs_Open with wrong file name");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_GetSize(wrongFilePath, &fileSize),
               "Test le_fs_GetSize with wrong file name");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Delete(wrongFilePath),
               "Test le_fs_Delete with wrong file name");
    LE_TEST_OK(LE_BAD_PARAMETER == le_fs_Move(loremFilePath, loremFilePath),
               "Test le_fs_Move with wrong file name");

    LE_TEST_INFO("End of FS test");
    LE_TEST_EXIT;
}
