
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


static void TestConcatenation(void)
{
    LE_INFO("======== Test Concatenations.");

    {
        char buf[100] = "hello/";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "there", "how/", "/are",
                                            "/you/", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "hello/there/how/are/you/") == 0));
    }

    {
        char buf[100] = "/hello//";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "there", "how/", "//are", "//you",
                               (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "/hello/there/how/are/you") == 0));
    }

    {
        char buf[100] = "";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "hello", "there", "how/", "/are",
                                            "/you/", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "hello/there/how/are/you/") == 0));
    }

    {
        char buf[100] = "/";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "/", "//", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "/") == 0));
    }

    {
        char buf[100] = "";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "/", "//", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "/") == 0));
    }

    {
        char buf[100] = "__";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "__", "____", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "__") == 0));
    }

    {
        char buf[100] = "";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "__", "____", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "__") == 0));
    }

    {
        char buf[100] = "__hello___";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "there", "how__", "_____are",
                                "____you", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "__hello___there__how___are__you") == 0));
    }

    {
        // Test overflow.
        char buf[35] = "";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "__hello___", "there", "how__", "_____are",
                                "____you_doing", (char*)NULL);
        LE_TEST((result == LE_OVERFLOW) && (strcmp(buf, "__hello___there__how___are__you_do") == 0));
    }

    {
        // Test overflow on the separator boundary.
        char buf[35] = "";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "__hello___", "there", "how__",
                                "_____are_you_do", "__ing", (char*)NULL);
        LE_TEST((result == LE_OVERFLOW) && (strcmp(buf, "__hello___there__how___are_you_do") == 0));
    }

    {
        // Test no segments.
        char buf[100] = "";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "") == 0));
    }

    {
        // Test single segment.
        char buf[100] = "";
        le_result_t result = le_path_Concat("__", buf, sizeof(buf), "__h___", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "__h___") == 0));
    }

    {
        char buf[100] = "/";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "h", "", "i/", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "/h/i/") == 0));
    }

    {
        char buf[100] = "////";
        le_result_t result = le_path_Concat("/", buf, sizeof(buf), "h", "/", "/i/", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "/h/i/") == 0));
    }

    {
        char buf[100] = "h***";
        le_result_t result = le_path_Concat("***", buf, sizeof(buf), "i", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "h***i") == 0));
    }

    {
        char buf[100] = "***h***";
        le_result_t result = le_path_Concat("***", buf, sizeof(buf), "***i", (char*)NULL);
        LE_TEST((result == LE_OK) && (strcmp(buf, "***h***i") == 0));
    }
}


static void TestSubPaths(void)
{
    {
        char path[] = "a/b/c";
        char subpath[] = "a/b/c/d";

        LE_TEST(le_path_IsSubpath(path, subpath, "/"));
        LE_TEST(!le_path_IsSubpath(subpath, path, "/"));
    }

    {
        char path[] = "a/b/c";
        char subpath[] = "a/b/c/";

        LE_TEST(!le_path_IsSubpath(path, subpath, "/"));
        LE_TEST(!le_path_IsSubpath(subpath, path, "/"));
    }

    {
        char path[] = "a/b/c/";
        char subpath[] = "a/b/c/d/";

        LE_TEST(le_path_IsSubpath(path, subpath, "/"));
        LE_TEST(!le_path_IsSubpath(subpath, path, "/"));
    }

    {
        char path[] = "a/b/c/";
        char subpath[] = "a/b/cd";

        LE_TEST(!le_path_IsSubpath(path, subpath, "/"));
        LE_TEST(!le_path_IsSubpath(subpath, path, "/"));
    }

    {
        char path[] = "/app/sec";
        char subpath[] = "/app/secStoreTest1";

        LE_TEST(!le_path_IsSubpath(path, subpath, "/"));
        LE_TEST(!le_path_IsSubpath(subpath, path, "/"));
    }
}


static void TestPathEquivalence(void)
{
    {
        char path1[] = "a/b/c";
        char path2[] = "a/b/c/d";

        LE_TEST(!le_path_IsEquivalent(path1, path2, "/"));
    }

    {
        char path1[] = "a/b/c///";
        char path2[] = "a/b/c/";

        LE_TEST(le_path_IsEquivalent(path1, path2, "/"));
    }

    {
        char path1[] = "a/b/c";
        char path2[] = "a/b/c//";

        LE_TEST(le_path_IsEquivalent(path1, path2, "/"));
    }

    {
        char path1[] = "/";
        char path2[] = "///";

        LE_TEST(le_path_IsEquivalent(path1, path2, "/"));
    }

    {
        char path1[] = "/a";
        char path2[] = "///";

        LE_TEST(!le_path_IsEquivalent(path1, path2, "/"));
    }
}


static void IteratePath
(
    le_pathIter_Ref_t iteratorRef,
    const char* originalPath,
    const char* nodes[],
    size_t nodesSize
)
{
    char fullPath[LARGE_BUFFER_SIZE] = { 0 };

    le_pathIter_GetPath(iteratorRef, fullPath, LARGE_BUFFER_SIZE);

    LE_INFO("Iterating path %s.", fullPath);
    LE_TEST(strcmp(fullPath, originalPath) == 0);

    le_pathIter_GoToStart(iteratorRef);
    ssize_t index = 0;

    LE_INFO(">>>> Forward Iteration >>>>");

    do
    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };

        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) != LE_OVERFLOW);

        LE_TEST(index < nodesSize);

        LE_INFO("> Found: %s, Expect: %s", buffer, nodes[index]);
        LE_TEST(strcmp(buffer, nodes[index]) == 0);
        index++;
    }
    while (le_pathIter_GoToNext(iteratorRef) != LE_NOT_FOUND);

    --index;

    LE_INFO("<<<< Reverse Iteration <<<<");

    le_pathIter_GoToEnd(iteratorRef);

    do
    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };

        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) != LE_OVERFLOW);

        LE_TEST(index > -1);

        LE_INFO("< Found: %s, Expect: %s", buffer, nodes[index]);
        LE_TEST(strcmp(buffer, nodes[index]) == 0);
        index--;
    }
    while (le_pathIter_GoToPrev(iteratorRef) != LE_NOT_FOUND);

    LE_TEST(index == -1);
}


static void TestUnixStyleIterator(void)
{
    LE_INFO("======== Test Unix Style Iterator.");

    static const char* nodes[] = { "a", "path", "to", "some", "end" };
    static const char* nodes2[] = { "a", "b", "c", "d", "e" };

    {
        static const char path[] = "/a/path/to/some/end";

        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix(path);
        IteratePath(iteratorRef, path, nodes, NUM_ARRAY_MEMBERS(nodes));
        le_pathIter_Delete(iteratorRef);
    }

    {
        static const char path[] = "::a::path::to::some::end";

        le_pathIter_Ref_t iteratorRef = le_pathIter_Create(path, "::", "..", ".");
        IteratePath(iteratorRef, path, nodes, NUM_ARRAY_MEMBERS(nodes));
        le_pathIter_Delete(iteratorRef);
    }

    {
        static const char path[] = "/a/b/c/d/e";

        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix(path);
        IteratePath(iteratorRef, path, nodes2, NUM_ARRAY_MEMBERS(nodes2));
        le_pathIter_Delete(iteratorRef);
    }

    {
        static const char path[] = "::a::b::c::d::e";

        le_pathIter_Ref_t iteratorRef = le_pathIter_Create(path, "::", "..", ".");
        IteratePath(iteratorRef, path, nodes2, NUM_ARRAY_MEMBERS(nodes2));
        le_pathIter_Delete(iteratorRef);
    }

    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");

        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) == LE_NOT_FOUND);
        LE_TEST(strcmp(buffer, "") == 0);
    }

    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/");

        le_pathIter_GoToStart(iteratorRef);
        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) == LE_NOT_FOUND);
        LE_TEST(strcmp(buffer, "") == 0);

        le_pathIter_GoToEnd(iteratorRef);
        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) == LE_NOT_FOUND);
        LE_TEST(strcmp(buffer, "") == 0);
    }

    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/some/path/somewhere");

        le_pathIter_GoToStart(iteratorRef);
        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) == LE_OK);
        LE_TEST(strcmp(buffer, "some") == 0);

        le_pathIter_GoToEnd(iteratorRef);
        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) != LE_NOT_FOUND);
        LE_TEST(strcmp(buffer, "somewhere") == 0);
    }
}


static bool TestPath
(
    le_pathIter_Ref_t iteratorRef,
    const char* pathStrPtr
)
{
    char fullPath[LARGE_BUFFER_SIZE] = { 0 };

    LE_ASSERT(le_pathIter_GetPath(iteratorRef, fullPath, LARGE_BUFFER_SIZE) == LE_OK);
    LE_INFO("Compare path, got: '%s', expected: '%s'", fullPath, pathStrPtr);

    return strcmp(fullPath, pathStrPtr) == 0;
}


static void TestUnixStyleAppends(void)
{
    LE_INFO("======== Test Unix Style Appends.");

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/b/c/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/b/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../../x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../../../x/y/z") == LE_UNDERFLOW);
        LE_TEST(TestPath(iteratorRef, "/"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../../x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../../../x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "../x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "/x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "/x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c");
        LE_TEST(le_pathIter_Append(iteratorRef, "./x/y/z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "a/b/c/x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");
        LE_TEST(le_pathIter_Append(iteratorRef, "./x/y/./z") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "./x/y/z"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");
        LE_TEST(le_pathIter_Append(iteratorRef, "/a//path/to/a///some/../place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/path/to/a/place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_Create("", "::", "^^", "__");
        LE_TEST(le_pathIter_Append(iteratorRef, "__::a::::path::to::__::a::some::^^::place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "__::a::path::to::a::place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_Create("::", "::", "^^", "__");
        LE_TEST(le_pathIter_Append(iteratorRef, "__::a::::path::to::__::a::some::^^::place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "::a::path::to::a::place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_Create("", "/", NULL, NULL);
        LE_TEST(le_pathIter_Append(iteratorRef, "/a//path/./to/a///some/../place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/path/./to/a/some/../place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");
        LE_TEST(le_pathIter_Append(iteratorRef, "../../../a//path/") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "../../../a/path"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");
        LE_TEST(le_pathIter_Append(iteratorRef, "/a//path/to/a///some/../place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/path/to/a/place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);

        LE_TEST(le_pathIter_GoToStart(iteratorRef) == LE_OK);

        LE_TEST(le_pathIter_GoToNext(iteratorRef) == LE_OK);
        LE_TEST(le_pathIter_GoToNext(iteratorRef) == LE_OK);
        LE_TEST(le_pathIter_GoToNext(iteratorRef) == LE_OK);

        le_pathIter_Truncate(iteratorRef);

        LE_TEST(le_pathIter_Append(iteratorRef, "nowhere") == LE_OK);

        LE_TEST(TestPath(iteratorRef, "/a/path/to/nowhere"));

        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("");
        LE_TEST(le_pathIter_Append(iteratorRef, "/a//path/to/a///some/../place") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/path/to/a/place"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);

        LE_TEST(le_pathIter_Append(iteratorRef, "../../nowhere") == LE_OK);

        LE_TEST(TestPath(iteratorRef, "/a/path/to/nowhere"));

        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c");
        LE_TEST(TestPath(iteratorRef, "/a/b/c"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/b"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/b/c/");
        LE_TEST(TestPath(iteratorRef, "/a/b/c"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/a/b"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c");
        LE_TEST(TestPath(iteratorRef, "a/b/c"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "a/b"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/b/c/");
        LE_TEST(TestPath(iteratorRef, "a/b/c"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "a/b"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a");
        LE_TEST(TestPath(iteratorRef, "/a"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/");
        LE_TEST(TestPath(iteratorRef, "/a"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, "/"));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == true);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a");
        LE_TEST(TestPath(iteratorRef, "a"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, ""));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }

    {
        le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("a/");
        LE_TEST(TestPath(iteratorRef, "a"));
        LE_TEST(le_pathIter_Append(iteratorRef, "..") == LE_OK);
        LE_TEST(TestPath(iteratorRef, ""));
        LE_TEST(le_pathIter_IsAbsolute(iteratorRef) == false);
        le_pathIter_Delete(iteratorRef);
    }
}


COMPONENT_INIT
{
    LE_TEST_INIT;

    LE_INFO("======== Begin Path API Test ========");
    TestGetBasenamePtr();
    TestGetDir();
    TestConcatenation();
    TestSubPaths();
    TestPathEquivalence();


    LE_INFO("======== Begin Path Iterator API Test ========");
    TestUnixStyleIterator();
    TestUnixStyleAppends();


    LE_INFO("======== Path API Test Complete ========");
    LE_TEST_SUMMARY;
}
