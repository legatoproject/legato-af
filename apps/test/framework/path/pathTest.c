
#include "legato.h"

#define NUM_TEST_STRS           8
#define LARGE_BUFFER_SIZE       100
#define SMALL_BUFFER_SIZE       5

static char* PathNames[] = {"long/path/with/file/name",
                            "/long/path/with/trailing/slashes///",
                            "",
                            "/",
                            ".",
                            "..",
                            "/file/",
                            "fileonly"};

static char* SepPathNames[] = {"long**path**with**file**name",
                            "**long**path**with**trailing**slashes******",
                            "",
                            "**",
                            ".",
                            "..",
                            "**file**",
                            "fileonly"};

static const char* BaseNames[] = {"name",
                                  "",
                                  "",
                                  "",
                                  ".",
                                  "..",
                                  "",
                                  "fileonly"};

static const char* DirNames[] = {"long/path/with/file/",
                                  "/long/path/with/trailing/slashes///",
                                  "",
                                  "/",
                                  ".",
                                  "..",
                                  "/file/",
                                  "fileonly"};

static char* SepDirNames[] = {"long**path**with**file**",
                            "**long**path**with**trailing**slashes******",
                            "",
                            "**",
                            ".",
                            "..",
                            "**file**",
                            "fileonly"};

static char* NodeNames[] = {"long", "path", "with", "trailing", "slashes"};

static char Path2[] = "//another//path/to/test";
static char* Path2Nodes[] = {"another", "path", "to", "test"};

static char Path3[] = "//another//path/to/test/";
static char* Path3Nodes[] = {"another", "path", "to", "test"};

static char* OverflowNodeNames[] = {"long", "path", "with", "trai", "slas"};
static char* OverflowPath2Nodes[] = {"anot", "path", "to", "test"};
static char* OverflowPath3Nodes[] = {"anot", "path", "to", "test"};

static bool AbsPathNames[] = {false, true, false, true, false, false, true, false};

static void TestGetBasenamePtr(void)
{
    int i;
    char* basenamePtr;

    // Test the standard strings.
    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        basenamePtr = le_path_GetBasenamePtr(PathNames[i], "/");
        LE_DEBUG("Basename: '%s'", basenamePtr);
        LE_TEST(strcmp(basenamePtr, BaseNames[i]) == 0);
    }

    // Test with multibyte separators.
    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        basenamePtr = le_path_GetBasenamePtr(SepPathNames[i], "**");
        LE_DEBUG("Basename: '%s'", basenamePtr);
        LE_TEST(strcmp(basenamePtr, BaseNames[i]) == 0);
    }
}


static void TestGetDir(void)
{
    int i;
    char dirname[100];

    // Test the standard strings.
    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        LE_TEST( (le_path_GetDir(PathNames[i], "/", dirname, 100) == LE_OK) &&
                  (strcmp(dirname, DirNames[i]) == 0) );
        LE_DEBUG("Dir: '%s'", dirname);
    }

    // Test with multibyte separators.
    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        LE_TEST( (le_path_GetDir(SepPathNames[i], "**", dirname, 100) == LE_OK) &&
                  (strcmp(dirname, SepDirNames[i]) == 0) );
        LE_DEBUG("Dir: '%s'", dirname);
    }

    // Test an overflow condition.
    LE_TEST( (le_path_GetDir(PathNames[1], "/", dirname, 21) == LE_OVERFLOW) &&
             (strcmp(dirname, "/long/path/with/trai") == 0) );
    LE_DEBUG("Dir: '%s'", dirname);
}


static void TestIterator
(
    char* pathPtr,      // The path string.
    char* sepPtr,       // The separator string.
    char** nodesPtr,    // The list of expected node strings.
    uint numNodes,      // The number of expected nodes.
    uint nodeBufSize    // The size of the temporary node buffer (used for testing overflows).
)
{
    le_path_IteratorRef_t IteratorRef;
    char nodeBuf[nodeBufSize];

    // Create iterator.
    LE_TEST((IteratorRef = le_path_iter_Create(pathPtr, sepPtr)) != NULL);

    // Get next nodes
    int i;
    for (i = 0; i < numNodes; i++)
    {
        if (nodeBufSize == LARGE_BUFFER_SIZE)
        {
            LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_OK);
        }
        else
        {
            le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf));
        }
        LE_DEBUG("node: '%s'", nodeBuf);
        LE_TEST(strcmp(nodeBuf, nodesPtr[i]) == 0);
    }
    LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);
    LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);

    // Get first node.
    if (nodeBufSize == LARGE_BUFFER_SIZE)
    {
        LE_TEST(le_path_iter_GetFirstNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_OK);
    }
    else
    {
        le_path_iter_GetFirstNode(IteratorRef, nodeBuf, sizeof(nodeBuf));
    }
    LE_DEBUG("node: '%s'", nodeBuf);
    LE_TEST(strcmp(nodeBuf, nodesPtr[0]) == 0);

    // Get next nodes
    for (i = 1; i < numNodes; i++)
    {
        if (nodeBufSize == LARGE_BUFFER_SIZE)
        {
            LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_OK);
        }
        else
        {
            le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf));
        }
        LE_DEBUG("node: '%s'", nodeBuf);
        LE_TEST(strcmp(nodeBuf, nodesPtr[i]) == 0);
    }
    LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);

    // Delete iterator
    le_path_iter_Delete(IteratorRef);
}

static void TestEmptyStrInIterator(void)
{
    le_path_IteratorRef_t IteratorRef;
    char nodeBuf[100];

    // Test empty path.
    LE_TEST((IteratorRef = le_path_iter_Create("", "/")) != NULL);

    LE_TEST(le_path_iter_GetFirstNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);
    LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);

    le_path_iter_Delete(IteratorRef);

    // Test path with no nodes.
    LE_TEST((IteratorRef = le_path_iter_Create("///", "/")) != NULL);

    LE_TEST(le_path_iter_GetFirstNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);
    LE_TEST(le_path_iter_GetNextNode(IteratorRef, nodeBuf, sizeof(nodeBuf)) == LE_NOT_FOUND);

    le_path_iter_Delete(IteratorRef);
}

static void TestAbsolutePaths(void)
{
    le_path_IteratorRef_t IteratorRef;
    int i;

    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        LE_TEST((IteratorRef = le_path_iter_Create(PathNames[i], "/")) != NULL);
        LE_TEST(le_path_iter_IsAbsolute(IteratorRef) == AbsPathNames[i]);
        le_path_iter_Delete(IteratorRef);
    }

    for (i = 0; i < NUM_TEST_STRS; i++)
    {
        LE_TEST((IteratorRef = le_path_iter_Create(SepPathNames[i], "**")) != NULL);
        LE_TEST(le_path_iter_IsAbsolute(IteratorRef) == AbsPathNames[i]);
        le_path_iter_Delete(IteratorRef);
    }
}

static void TestConcatenation(void)
{
    LE_INFO("======== Test Concatenations.");

    char buf[35];

    le_result_t result = le_path_Concat("/", buf, sizeof(buf), "hello/", "there", "how/", "/are",
                                        "/you/", (char*)NULL);
    LE_TEST((result == LE_OK) && (strcmp(buf, "hello/there/how/are/you") == 0));

    result = le_path_Concat("/", buf, sizeof(buf), "//hello//", "there", "how/", "//are", "//you",
                           (char*)NULL);
    LE_TEST((result == LE_OK) && (strcmp(buf, "/hello/there/how/are/you") == 0));

    result = le_path_Concat("__", buf, sizeof(buf), "__hello___", "there", "how__", "_____are",
                            "____you", (char*)NULL);
    LE_TEST((result == LE_OK) && (strcmp(buf, "__hello___there__how___are__you") == 0));

    // Test overflow.
    result = le_path_Concat("__", buf, sizeof(buf), "__hello___", "there", "how__", "_____are",
                            "____you_doing", (char*)NULL);
    LE_TEST((result == LE_OVERFLOW) && (strcmp(buf, "__hello___there__how___are__you_do") == 0));

    // Test overflow on the separator boundary.
    result = le_path_Concat("__", buf, sizeof(buf), "__hello___", "there", "how__",
                            "_____are_you_do", "__ing", (char*)NULL);
    LE_TEST((result == LE_OVERFLOW) && (strcmp(buf, "__hello___there__how___are_you_do") == 0));

    // Test no segments.
    result = le_path_Concat("__", buf, sizeof(buf), (char*)NULL);
    LE_TEST((result == LE_OK) && (strcmp(buf, "") == 0));

    // Test single segment.
    result = le_path_Concat("__", buf, sizeof(buf), "__hello___", (char*)NULL);
    LE_TEST((result == LE_OK) && (strcmp(buf, "__hello_") == 0));

    // Test too few params segment.
    result = le_path_Concat("__", buf, sizeof(buf));
    LE_TEST((result == LE_OK) && (strcmp(buf, "") == 0));
}


LE_EVENT_INIT_HANDLER
{
    LE_TEST_INIT;
    LE_INFO("======== Begin Path API Test ========");

    TestGetBasenamePtr();
    TestGetDir();
    TestConcatenation();

    LE_INFO("======== Test iterator without overflows.");
    TestIterator(PathNames[1], "/", NodeNames, 5, LARGE_BUFFER_SIZE);
    TestIterator(SepPathNames[1], "**", NodeNames, 5, LARGE_BUFFER_SIZE);
    LE_INFO("======== Test iterator without overflows - Path2.");
    TestIterator(Path2, "/", Path2Nodes, 4, LARGE_BUFFER_SIZE);
    LE_INFO("======== Test iterator without overflows - Path3.");
    TestIterator(Path3, "/", Path3Nodes, 4, LARGE_BUFFER_SIZE);

    LE_INFO("======== Test iterator with overflows.");
    TestIterator(PathNames[1], "/", OverflowNodeNames, 5, SMALL_BUFFER_SIZE);
    TestIterator(SepPathNames[1], "**", OverflowNodeNames, 5, SMALL_BUFFER_SIZE);
    LE_INFO("======== Test iterator with overflows - Path2.");
    TestIterator(Path2, "/", OverflowPath2Nodes, 4, SMALL_BUFFER_SIZE);
    LE_INFO("======== Test iterator with overflows - Path3.");
    TestIterator(Path3, "/", OverflowPath3Nodes, 4, SMALL_BUFFER_SIZE);

    LE_INFO("======== Misc Iterator tests");
    TestEmptyStrInIterator();
    TestAbsolutePaths();

    LE_INFO("======== Path API Test Complete ========");
    LE_TEST_SUMMARY;
}
