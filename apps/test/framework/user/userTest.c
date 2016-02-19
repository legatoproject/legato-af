/*
 * Test the creating and deleting users.
 */

#include "legato.h"
#include "user.h"

#define USER_NAME       "Sparticus"
#define APP_USER_NAME   "appAthens"
#define APP_NAME        "Athens"
#define GROUP_NAME      "testGroup"

uid_t Uid, AppUid;
gid_t Gid, AppGid;


static void TestUserCreation(void)
{
    LE_ASSERT(user_Create(USER_NAME, &Uid, &Gid) == LE_OK);

    uid_t myUid;
    gid_t myGid;

    LE_ASSERT(user_Create(USER_NAME, &myUid, &myGid) == LE_DUPLICATE);
    LE_ASSERT( (myUid == Uid) && (myGid == Gid) );

    LE_ASSERT(user_Create(APP_USER_NAME, &AppUid, &AppGid) == LE_OK);
}

static void TestUserNameAndId(void)
{
    // Test user id.
    uid_t uid;
    gid_t gid;

    LE_ASSERT(user_GetIDs(USER_NAME, &uid, &gid) == LE_OK);
    LE_ASSERT(uid == Uid);
    LE_ASSERT(gid == Gid);

    // Test the GetUid() function.
    LE_ASSERT(user_GetUid(USER_NAME, &uid) == LE_OK);
    LE_ASSERT(uid == Uid);

    // Test the GetGid() function.
    LE_ASSERT(user_GetGid(USER_NAME, &gid) == LE_OK);
    LE_ASSERT(gid == Gid);

    // Test user name.
    char buf[100];
    LE_ASSERT(user_GetName(Uid, buf, sizeof(buf)) == LE_OK);

    LE_ASSERT(strcmp(buf, USER_NAME) == 0);

    // Test group name.
    LE_ASSERT(user_GetGroupName(Gid, buf, sizeof(buf)) == LE_OK);

    LE_ASSERT(strcmp(buf, USER_NAME) == 0);

    // Test app name.
    LE_ASSERT(user_GetAppName(Uid, buf, sizeof(buf)) == LE_NOT_FOUND);

    LE_ASSERT(user_GetAppName(AppUid, buf, sizeof(buf)) == LE_OK);
    LE_ASSERT(strcmp(buf, APP_NAME) == 0);
}

static void TestUserDeletion(void)
{
    LE_ASSERT(user_Delete(USER_NAME) == LE_OK);
    LE_ASSERT(user_Delete(APP_USER_NAME) == LE_OK);

    LE_ASSERT(user_GetIDs(USER_NAME, NULL, NULL) == LE_NOT_FOUND);

    char buf[100];
    LE_ASSERT(user_GetName(Uid, buf, sizeof(buf)) == LE_NOT_FOUND);

    LE_ASSERT(user_GetAppName(AppUid, buf, sizeof(buf)) == LE_NOT_FOUND);
}


static void TestCovertToUserName(void)
{
    char userName[100];

    LE_ASSERT(user_AppNameToUserName(APP_NAME, userName, sizeof(userName)) == LE_OK);

    LE_ASSERT(strcmp(userName, APP_USER_NAME) == 0);
}


static void TestGroupCreation(void)
{
    LE_ASSERT(user_CreateGroup(GROUP_NAME, &Gid) == LE_OK);
    LE_INFO("Created group with gid %d", Gid);

    // The group already exists so this should fail.
    gid_t gid;
    LE_ASSERT(user_CreateGroup(GROUP_NAME, &gid) == LE_DUPLICATE);

    // Test the GetGid() function.
    LE_ASSERT(user_GetGid(GROUP_NAME, &gid) == LE_OK);
    LE_ASSERT(gid == Gid);
}


static void TestGroupDelete(void)
{
    LE_ASSERT(user_DeleteGroup(GROUP_NAME) == LE_OK);

    gid_t gid;
    LE_ASSERT(user_GetGid(GROUP_NAME, &gid) == LE_NOT_FOUND);

    LE_ASSERT(user_DeleteGroup(GROUP_NAME) == LE_NOT_FOUND);
}


COMPONENT_INIT
{
    LE_INFO("======== Starting Users Test ========");

    // These functions should be called together in this order.
    TestUserCreation();
    TestUserNameAndId();
    TestUserDeletion();

    // These functions should be called together in this order.
    TestCovertToUserName();

    // These functions should be called together in this order.
    TestGroupCreation();
    TestGroupDelete();

    LE_INFO("======== Users Test Completed Successfully ========");
    exit(EXIT_SUCCESS);
}
