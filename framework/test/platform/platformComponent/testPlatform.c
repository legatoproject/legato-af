//--------------------------------------------------------------------------------------------------
/**
 * @file platformTest.c
 *
 * This file includes tests of platform dependent functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include <libfs/libfs.h>

COMPONENT_INIT
{
    int rc, sockFd, ret;
    static struct timeval time;
    FILE  *fs;

    LE_TEST_PLAN(15);
    LE_TEST_INFO("\n");
    LE_TEST_INFO("==== Unit Tests for platform dependent FUNCTIONS 1 started! ====\n");

    // 1) gettimeofday() function test
    LE_TEST_INFO("Unit Test for platform dependent: gettimeofday()\n");
    rc = gettimeofday(&time, NULL);
    LE_TEST_OK(rc == 0,"gettimeofday() %u.%06u\n", (unsigned int) time.tv_sec, (unsigned int) time.tv_usec);

    // 2) fcntl() function tests
    // Create socket
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    LE_TEST_ASSERT(sockFd >= 0, "Socket created.");
    LE_TEST_INFO("Unit Test for platform dependent: fcntl()\n");
    // 2a) fcntl(fd,F_GETFD)
    ret = fcntl(sockFd,F_GETFD);
    LE_TEST_OK(ret >= 0,"fcntl(sockFd, F_GETFD) OK = %d\n", ret);
    // 2b) fcntl(sockFd,F_GETFL, 0)
    ret = fcntl(sockFd, F_GETFL, 0);
    LE_TEST_OK(ret >= 0,"fcntl(sockFd, F_GETFL,0) OK = %d\n", ret);
    // 2c) fcntl(sockFd,F_GETFL)
    ret = fcntl(sockFd, F_GETFL);
    LE_TEST_OK(ret >= 0,"fcntl(sockFd, F_GETFL) OK = %d\n", ret);
    // 2d) fcntl(sockFd, F_SETFL, O_NONBLOCK)
    ret = fcntl(sockFd, F_SETFL, O_NONBLOCK);
    LE_TEST_OK(ret >= 0,"fcntl(sockFd, F_SETFL, O_NONBLOCK) OK = %d\n", ret);
    close(sockFd);

    // 3) access() function test
    LE_TEST_INFO("Unit Test for platform dependent: access()\n");
    // 3a) d:/config/version file is present
    ret = access("d:/config/version", W_OK | R_OK);
    LE_TEST_OK(ret == 0,"access(""d:/config/version"",W_OK | R_OK): OK => File present\n");
    // 3b) d:/config/version file is present but with X_OK flag
    ret = access("d:/config/version", W_OK | R_OK | X_OK);
    LE_TEST_OK(ret < 0,"access(""d:/config/version"",W_OK | R_OK | X_OK): OK\n");
    // 3c) d:/dummy file is not present!
    ret = access("d:/dummy", R_OK);
    LE_TEST_OK(ret < 0,"access(""d:/dummy"",R_OK): OK (missing file detected)\n");
    // 3d) d:/config => Directory is present!
    ret = access("d:/config", R_OK);
    LE_TEST_OK(ret == 0,"access(""d:/config"",R_OK): OK (Directory detected)\n");
    // 3e) d:/config => Directory is present!
    ret = access("d:/config", R_OK);
    LE_TEST_OK(ret == 0,"access(""access(""d:/config"",R_OK): OK (Directory detected)\n");
    // 3f) d:/config/ => Directory is present!
    ret = access("d:/config/", R_OK);
    LE_TEST_OK(ret == 0,"access(""d:/config/"",R_OK): OK (Directory detected)\n");
    // 3g) d:/configuration => Directory is absent!
    ret = access("d:/configuration/", R_OK);
    LE_TEST_OK(ret < 0,"access(""d:/configuration/"",R_OK): OK (Directory not detected)\n");
    // 3h) d:/dummy0 => Empty file is present!
    libFS_task_init();
    fs = fopen("d:/dummy0","w+");
    LE_TEST_ASSERT(fs != NULL, "File created : ");
    fclose(fs);
    libFS_task_exit();
    ret = access("d:/dummy0", R_OK);
    LE_TEST_OK(ret == 0,"access(""d:/dummy0"",R_OK): OK (file detected)\n");

    LE_TEST_INFO("==== Unit Tests for platform dependent FUNCTIONS 1 passed ====\n");
    LE_TEST_INFO("\n");
    LE_TEST_EXIT;
}
