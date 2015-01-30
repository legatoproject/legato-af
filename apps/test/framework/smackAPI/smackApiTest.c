//--------------------------------------------------------------------------------------------------
/** @file smackApiTest.c
 *
 * Unit test for the SMACK API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "smack.h"
#include "fileDescriptor.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Get a process's SMACK label.
 */
//--------------------------------------------------------------------------------------------------
static const char* GetProcLabel
(
    pid_t pid
)
{
    static char label[24];

    // Get the process's smack file name.
    char smackFile[256];
    LE_ASSERT(snprintf(smackFile, sizeof(smackFile), "/proc/%d/attr/current", pid) < sizeof(smackFile));

    // Open the process's smack file.
    int fd;

    do
    {
        fd = open(smackFile, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    LE_FATAL_IF(fd == -1, "Could not open %s.  %m.\n", smackFile);

    LE_FATAL_IF(read(fd, label, sizeof(label)) < 0,
                "Could not read from %s.  %m.\n", smackFile);

    fd_Close(fd);

    return label;
}


COMPONENT_INIT
{
    LE_TEST_INIT;

    LE_INFO("******** Starting SMACK API Test. *******");

    // Test setting of file system object labels.
    LE_TEST(smack_SetLabel("/fileThatDoesntExist", "testLabel1") == LE_FAULT);

    LE_TEST(smack_SetLabel("/dev/null", "testLabel1") == LE_OK);

    char label[LIMIT_MAX_SMACK_LABEL_BYTES];
    LE_TEST( (getxattr("/dev/null", "security.SMACK64", label, sizeof(label)) >= 0) &&
             (strcmp(label, "testLabel1") == 0) );

    // Test set my process label.
    smack_SetMyLabel("smackTest");

    LE_TEST(strcmp(GetProcLabel(getpid()), "smackTest") == 0);

    // Test setting a rule.
    smack_SetRule("testLabel1", "-", "testLabel2");
    smack_SetRule("testLabel1", "-", "testLabel3");

    LE_TEST(!smack_HasAccess("testLabel1", "rw", "testLabel2"));

    smack_SetRule("testLabel1", "rw", "testLabel2");

    LE_TEST(smack_HasAccess("testLabel1", "rw", "testLabel2"));
    LE_TEST(smack_HasAccess("testLabel1", "r", "testLabel2"));
    LE_TEST(smack_HasAccess("testLabel1", "w", "testLabel2"));

    LE_TEST(!smack_HasAccess("testLabel1", "x", "testLabel2"));
    LE_TEST(!smack_HasAccess("testLabel1", "rx", "testLabel2"));

    smack_SetRule("testLabel1", "r", "testLabel3");

    LE_TEST(smack_HasAccess("testLabel1", "r", "testLabel3"));

    // Revoke subject.
    smack_RevokeSubject("testLabel1");
    LE_TEST(!smack_HasAccess("testLabel1", "rw", "testLabel2"));
    LE_TEST(!smack_HasAccess("testLabel1", "r", "testLabel3"));

    // Cleanup.
    LE_ASSERT(smack_SetLabel("/dev/null", "_") == LE_OK);
    LE_ASSERT(smack_SetLabel("/dev/zero", "_") == LE_OK);

    LE_INFO("******** SMACK API Test PASSED! *******");

    LE_TEST_EXIT;
}
