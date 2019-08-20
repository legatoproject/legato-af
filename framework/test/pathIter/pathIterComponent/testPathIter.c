 /**
  * This module is for unit testing the le_pathIter module in the legato
  * runtime library (liblegato.so).
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"

#define LARGE_BUFFER_SIZE       100


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

    LE_TEST_INFO("Iterating path %s.", fullPath);
    LE_TEST(strcmp(fullPath, originalPath) == 0);

    le_pathIter_GoToStart(iteratorRef);
    ssize_t index = 0;

    LE_TEST_INFO(">>>> Forward Iteration >>>>");

    do
    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };

        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) != LE_OVERFLOW);

        LE_TEST(index < nodesSize);

        LE_TEST_INFO("> Found: %s, Expect: %s", buffer, nodes[index]);
        LE_TEST(strcmp(buffer, nodes[index]) == 0);
        index++;
    }
    while (le_pathIter_GoToNext(iteratorRef) != LE_NOT_FOUND);

    --index;

    LE_TEST_INFO("<<<< Reverse Iteration <<<<");

    le_pathIter_GoToEnd(iteratorRef);

    do
    {
        char buffer[LARGE_BUFFER_SIZE] = { 0 };

        LE_TEST(le_pathIter_GetCurrentNode(iteratorRef,
                                            buffer,
                                            LARGE_BUFFER_SIZE) != LE_OVERFLOW);

        LE_TEST(index > -1);

        LE_TEST_INFO("< Found: %s, Expect: %s", buffer, nodes[index]);
        LE_TEST(strcmp(buffer, nodes[index]) == 0);
        index--;
    }
    while (le_pathIter_GoToPrev(iteratorRef) != LE_NOT_FOUND);

    LE_TEST(index == -1);
}


static void TestUnixStyleIterator(void)
{
    LE_TEST_INFO("======== Test Unix Style Iterator.");

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
    LE_TEST_INFO("Compare path, got: '%s', expected: '%s'", fullPath, pathStrPtr);

    return strcmp(fullPath, pathStrPtr) == 0;
}


static void TestUnixStyleAppends(void)
{
    LE_TEST_INFO("======== Test Unix Style Appends.");

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
    LE_TEST_INFO("======== le_pathIter Test Started ========");

    TestUnixStyleIterator();
    TestUnixStyleAppends();

    LE_TEST_INFO("======== le_pathIter Test Complete ========");
    LE_TEST_EXIT;
}
