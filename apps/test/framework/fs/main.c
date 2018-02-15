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
    printf("Starting FS test\n");

    le_fs_FileRef_t fileRef = NULL;

    // Create and open a new file
    const char filePath[PATH_LENGTH] = "/foo/bar/test.txt";
    printf("Open file '%s'\n", filePath);
    LE_ASSERT_OK(le_fs_Open(filePath, LE_FS_CREAT | LE_FS_RDWR | LE_FS_TRUNC, &fileRef));
    printf("File handler: %p\n", fileRef);
    LE_ASSERT(0 <= fileRef);

    // Write in file
    const uint8_t dataToWrite[SHORT_DATA_LENGTH] = "Hello world!";
    printf("Writing '%s' in file\n", dataToWrite);
    LE_ASSERT_OK(le_fs_Write(fileRef, dataToWrite, strlen((char*)dataToWrite)));

    // Get file size
    size_t fileSize = 0;
    LE_ASSERT_OK(le_fs_GetSize(filePath, &fileSize));
    printf("File size of '%s': %zu\n", filePath, fileSize);
    LE_ASSERT(strlen((char*)dataToWrite) == fileSize);

    // Seek negative offset from the beginning
    int32_t offset = -5;
    int32_t currentOffset = 0;
    printf("Seek offset %d from the beginning\n", (int)offset);
    LE_ASSERT(LE_FAULT == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));

    // Seek from the beginning
    offset = 5;
    currentOffset = 0;
    int32_t currentPosition = 0;
    printf("Seek offset %d from the beginning\n", (int)offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));
    currentPosition += offset;
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(currentPosition == currentOffset);

    // Read 3 bytes from the current position
    uint8_t readData[SHORT_DATA_LENGTH+1];
    memset(readData, '\0', sizeof(readData));
    size_t readLength = 3;
    size_t expectedReadLength = readLength;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    currentPosition += readLength;
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT(expectedReadLength == readLength);
    LE_ASSERT(0 == strncmp(" wo", (char*)readData, readLength))

    // Seek from the current position
    offset = 2;
    printf("Seek offset %d from the current position\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_CUR, &currentOffset));
    currentPosition += offset;
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(currentPosition == currentOffset);

    // Read 3 bytes from the current position: EOF should be reached after 2 bytes
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = 2;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT(expectedReadLength == readLength);
    LE_ASSERT(0 == strncmp("d!", (char*)readData, readLength))

    // Read 3 bytes from the current position: EOF is already reached
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = 0;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT(expectedReadLength == readLength);

    // Seek from the end
    offset = -5;
    printf("Seek offset %d from the end\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_END, &currentOffset));
    currentPosition = strlen((char*)dataToWrite) - 5;
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(currentPosition == currentOffset);

    // Read 3 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = 3;
    expectedReadLength = readLength;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT(expectedReadLength == readLength);
    LE_ASSERT(0 == strncmp("orl", (char*)readData, readLength))

    // Set current position to the beginning of the file
    offset = 0;
    printf("Seek offset %d from the beginning\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(offset == currentOffset);

    // Read 150 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = SHORT_DATA_LENGTH;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT(strlen((char*)dataToWrite) == readLength);
    LE_ASSERT(0 == strncmp((char*)dataToWrite, (char*)readData, readLength))

    // Error cases with useless actions
    printf("Test error cases with useless actions\n");
    readLength = 0;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    LE_ASSERT_OK(le_fs_Write(fileRef, dataToWrite, 0));

    // Close the opened file
    printf("Closing file handler: %p\n", fileRef);
    LE_ASSERT_OK(le_fs_Close(fileRef));
    fileRef = NULL;

    // Move the file
    const char newFilePath[PATH_LENGTH] = "/foo/bar/test2.txt";
    printf("Moving file from '%s' to '%s'\n", filePath, newFilePath);
    LE_ASSERT_OK(le_fs_Move(filePath, newFilePath));
    // Check that old file cannot be opened
    LE_ASSERT(LE_OK != le_fs_Open(filePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef));

    // Open the file
    printf("Open file '%s'\n", newFilePath);
    LE_ASSERT_OK(le_fs_Open(newFilePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef));
    printf("File handler: %p\n", fileRef);
    LE_ASSERT(0 <= fileRef);

    // Append text to the file
    printf("Writing '%s' in file\n", dataToWrite);
    LE_ASSERT_OK(le_fs_Write(fileRef, dataToWrite, strlen((char*)dataToWrite)));

    // Set current position to the beginning of the file
    offset = 0;
    printf("Seek offset %d from the beginning\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(offset == currentOffset);

    // Read 150 bytes from the current position
    memset(readData, '\0', sizeof(readData));
    readLength = SHORT_DATA_LENGTH;
    LE_ASSERT_OK(le_fs_Read(fileRef, readData, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readData);
    LE_ASSERT((2 * strlen((char*)dataToWrite)) == readLength);
    LE_ASSERT(0 == strncmp("Hello world!Hello world!", (char*)readData, readLength))

    // Close the opened file
    printf("Closing file handler: %p\n", fileRef);
    LE_ASSERT_OK(le_fs_Close(fileRef));
    fileRef = NULL;

    // Get file size
    fileSize = 0;
    LE_ASSERT_OK(le_fs_GetSize(newFilePath, &fileSize));
    printf("File size of '%s': %zu\n", newFilePath, fileSize);
    LE_ASSERT((2 * strlen((char*)dataToWrite)) == fileSize);

    // Create and open a new file
    const char deleteFilePath[PATH_LENGTH] = "/foo/bar/delete.txt";
    printf("Open file '%s'\n", deleteFilePath);
    LE_ASSERT_OK(le_fs_Open(deleteFilePath, LE_FS_CREAT | LE_FS_RDWR, &fileRef));
    printf("File handler: %p\n", fileRef);
    LE_ASSERT(0 <= fileRef);

    // Close the new file
    printf("Closing file handler: %p\n", fileRef);
    LE_ASSERT_OK(le_fs_Close(fileRef));
    fileRef = NULL;

    // Delete the new file
    printf("Deleting file '%s'\n",deleteFilePath);
    LE_ASSERT_OK(le_fs_Delete(deleteFilePath));
    // Check that deleted file cannot be opened
    LE_ASSERT(LE_OK != le_fs_Open(deleteFilePath, LE_FS_RDWR | LE_FS_APPEND, &fileRef));

    // Create and open a new file
    const char loremFilePath[PATH_LENGTH] = "/bar/foo/lorem_ipsum.txt";
    printf("Open file '%s'\n", loremFilePath);
    LE_ASSERT_OK(le_fs_Open(loremFilePath, LE_FS_CREAT | LE_FS_RDWR | LE_FS_TRUNC, &fileRef));
    printf("File handler: %p\n", fileRef);
    LE_ASSERT(0 <= fileRef);

    // Write in file
    const uint8_t loremIpsum[LONG_DATA_LENGTH] =
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
    printf("Writing Lorem ipsum in file\n");
    LE_ASSERT_OK(le_fs_Write(fileRef, loremIpsum, strlen((char*)loremIpsum)));

    // Set current position to the beginning of the file
    offset = 0;
    printf("Seek offset %d from the beginning\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(offset == currentOffset);

    // Read 5000 bytes from the current position
    uint8_t readLoremIpsum[LONG_DATA_LENGTH+1];
    memset(readLoremIpsum, '\0', sizeof(readLoremIpsum));
    readLength = LONG_DATA_LENGTH;
    LE_ASSERT_OK(le_fs_Read(fileRef, readLoremIpsum, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readLoremIpsum);
    LE_ASSERT(strlen((char*)loremIpsum) == readLength);

    // Set current position to the beginning of the file
    offset = 0;
    printf("Seek offset %d from the beginning\n", offset);
    LE_ASSERT_OK(le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));
    printf("New position in file: %d\n", currentOffset);
    LE_ASSERT(offset == currentOffset);

    // Read 150 bytes from the current position
    memset(readLoremIpsum, '\0', sizeof(readLoremIpsum));
    readLength = SHORT_DATA_LENGTH;
    LE_ASSERT_OK(le_fs_Read(fileRef, readLoremIpsum, &readLength));
    printf("Read %d bytes: '%s'\n", (int)readLength, readLoremIpsum);
    LE_ASSERT(SHORT_DATA_LENGTH == readLength);

    // Close the opened file
    printf("Closing file handler: %p\n", fileRef);
    LE_ASSERT_OK(le_fs_Close(fileRef));
    fileRef = (le_fs_FileRef_t)-1;

    // Error cases with wrong file handler
    printf("Test error cases with file handler %p\n", fileRef);
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Close(fileRef));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Read(fileRef, readLoremIpsum, &readLength));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Write(fileRef, loremIpsum, strlen((char*)loremIpsum)));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Seek(fileRef, offset, LE_FS_SEEK_SET, &currentOffset));

    // Error cases with wrong file paths
    const char wrongFilePath[PATH_LENGTH] = "foo/bar/";
    printf("Test error cases with file path '%s'\n", wrongFilePath);
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Open(wrongFilePath, LE_FS_RDWR, &fileRef));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_GetSize(wrongFilePath, &fileSize));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Delete(wrongFilePath));
    LE_ASSERT(LE_BAD_PARAMETER == le_fs_Move(loremFilePath, loremFilePath));

    printf("Successful FS test\n");
    exit(EXIT_SUCCESS);
}

