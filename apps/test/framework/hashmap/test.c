 /**
  * This module is for unit testing the le_hashmap module in the legato
  * runtime library
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"


void TestIntHashMap(le_hashmap_Ref_t map);
void TestStringHashMap(le_hashmap_Ref_t map);
void TestCustomHashMap(le_hashmap_Ref_t map);
void TestHashFns(void);
void TestNewIter();
void TestTinyMap(le_hashmap_Ref_t map);
void TestPointerMap(le_hashmap_Ref_t map);
void TestLongIntHashMap(le_hashmap_Ref_t map);
void* insertRetrieve(le_hashmap_Ref_t map, const void* key, const void* val);
size_t le_hashmap_HashCustom(const void* keyPtr);
bool le_hashmap_EqualsCustom(const void* firstPtr, const void* secondPtr);
bool itHandler(const void* keyPtr, const void* valuePtr, void* contextPtr);
void TestIterRemove(le_hashmap_Ref_t map);

typedef struct Key Key_t;
struct Key {
    int i;
    const char* str;
};


COMPONENT_INIT
{
    LE_TEST_INIT;

    LE_INFO("\n");
    LE_INFO("====  Unit test for  le_hashmap module. ====");

    LE_INFO("***  Creating hash maps required for tests. ***");
    LE_INFO("Creating int/int map");
    le_hashmap_Ref_t map1 = le_hashmap_Create("Map1", 200, &le_hashmap_HashUInt32, &le_hashmap_EqualsUInt32);

    LE_INFO("Creating string/string map");
    le_hashmap_Ref_t map2 = le_hashmap_Create("Map2", 200, &le_hashmap_HashString, &le_hashmap_EqualsString);

    LE_INFO("Creating custom map");
    le_hashmap_Ref_t map3 = le_hashmap_Create("Map3", 200, &le_hashmap_HashCustom, &le_hashmap_EqualsCustom);

    LE_INFO("Creating tiny map");
    le_hashmap_Ref_t map4 = le_hashmap_Create("Map4", 1, &le_hashmap_HashUInt32, &le_hashmap_EqualsUInt32);

    LE_INFO("Creating pointer map");
    le_hashmap_Ref_t map5 = le_hashmap_Create("Map5", 100, &le_hashmap_HashVoidPointer, &le_hashmap_EqualsVoidPointer);

    LE_INFO("Creating long int/long int map");
    le_hashmap_Ref_t map6 = le_hashmap_Create("Map6", 200, &le_hashmap_HashUInt64, &le_hashmap_EqualsUInt64);

    LE_TEST(map1 && map2 && map3 && map4 && map5 && map6);

    TestHashFns();
    TestIntHashMap(map1);
    TestStringHashMap(map2);
    TestCustomHashMap(map3);
    TestTinyMap(map4);
    TestPointerMap(map5);
    TestLongIntHashMap(map6);
    TestNewIter();
    TestIterRemove(map1);

    LE_INFO("==== Hashmap Tests PASSED ====\n");

    LE_TEST_SUMMARY;
}


void TestIntHashMap(le_hashmap_Ref_t map)
{
    uint32_t ikey1 = 100;
    uint32_t ival1 = 100;
    uint32_t ival2 = 350;
    uint32_t cCount1, cCount2 = 0;

    LE_INFO("*** Running int/int hashmap tests ***");

    void* rval = insertRetrieve(map, &ikey1, &ival1);
    LE_ASSERT(NULL != rval);
    LE_TEST (*((uint32_t*) rval) == ival1);

    rval = insertRetrieve(map, &ikey1, &ival2);
    LE_ASSERT(NULL != rval);
    LE_TEST((*((uint32_t*) rval) == ival2) && (le_hashmap_Size(map) == 1));

    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_isEmpty(map));

    // Time to store 1000 pairs
    uint32_t iKeys[1000];
    uint32_t iVals[1000];
    int j = 0;
    for (j=0; j<1000; j++) {
        iKeys[j] = j * 2;
        iVals[j] = j * 4;
        le_hashmap_Put(map, &iKeys[j], &iVals[j]);
    }
    LE_TEST(le_hashmap_Size(map) == 1000);

    cCount1 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %u", cCount1);
    for (j=0; j<1000; j+=2) {
        le_hashmap_Remove(map, &j);
    }
    LE_TEST(le_hashmap_Size(map) == 500);

    cCount2 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %zu", le_hashmap_CountCollisions(map));
    LE_TEST(cCount1 > cCount2);

    // Iterate over the map
    le_hashmap_It_Ref_t mapIt = le_hashmap_GetIterator(map);
    LE_TEST(le_hashmap_GetKey(mapIt) == NULL);
    int itercnt = 0;
    while (le_hashmap_NextNode(mapIt) == LE_OK)
    {
        itercnt++;
        le_hashmap_GetKey(mapIt);
        le_hashmap_GetValue(mapIt);
    }
    LE_INFO("Iterator count = %d", itercnt);
    LE_TEST(itercnt == 500);
    // Now back again.
    while (le_hashmap_PrevNode(mapIt) == LE_OK)
    {
        itercnt--;
        le_hashmap_GetKey(mapIt);
        le_hashmap_GetValue(mapIt);
    }
    LE_INFO("Iterator count = %d", itercnt);
    LE_TEST(itercnt == -1);

    // Cleanup the map again to allow it to be reused
    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_Size(map) == 0);

    // Check iterator on an empty map
    mapIt = le_hashmap_GetIterator(map);
    LE_TEST(le_hashmap_NextNode(mapIt) == LE_NOT_FOUND);
}

void* insertRetrieve(le_hashmap_Ref_t map, const void* key, const void* val)
{
    le_hashmap_Put(map, key, val);
    void *rval = le_hashmap_Get(map, key);
    return rval;
}

void TestHashFns(void)
{
    LE_INFO("*** Running hash and equality function tests ***");

    /* Test int hash function */
    uint32_t ikey1 = 100;
    uint32_t ikey2 = -250;
    uint32_t ikey3 = 256789;
    uint32_t ikey4 = 256789;
    const char* skey1 = "skey1";
    const char* skey2 = "skey2";

    // Need an equivalent string (but not the same one!)
    char* skey3 = malloc(sizeof(char) * 6);
    LE_ASSERT(NULL != skey3);
    skey3[0]='s';
    skey3[1]='k';
    skey3[2]='e';
    skey3[3]='y';
    skey3[4]='1';
    skey3[5]='\0';

    LE_INFO("Int hash function test");
    LE_TEST((ikey1 == le_hashmap_HashUInt32(&ikey1)) && (ikey2 == le_hashmap_HashUInt32(&ikey2)));

    LE_INFO("Int equality function test");
    LE_TEST(le_hashmap_EqualsUInt32(&ikey3, &ikey4) && !le_hashmap_EqualsUInt32(&ikey2, &ikey4));

    LE_INFO("String hash function test");
    LE_TEST(le_hashmap_HashString(skey1) == le_hashmap_HashString(skey3));

    LE_INFO("String equality function test");
    LE_TEST(le_hashmap_EqualsString(skey1, skey3) && !le_hashmap_EqualsString(skey1, skey2));

    free(skey3);
}

void TestStringHashMap(le_hashmap_Ref_t map)
{
    LE_INFO("*** Running string/string hashmap tests ***");
    const char* key1 = "key1";
    const char* key2 = "key2";
    const char* val1 = "val1";
    const char* val2 = "val2";

    void* rval = insertRetrieve(map, key1, val1);
    LE_ASSERT(NULL != rval);
    LE_TEST (((const char*) rval) == val1);

    rval = insertRetrieve(map, key2, val2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == val2) && (le_hashmap_Size(map) == 2));

    rval = insertRetrieve(map, key1, val2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == val2) && (le_hashmap_Size(map) == 2));

    int i = 0;
    char* keys[1000];
    char* vals[1000];
    for (i=0; i<1000; i++)
    {
        keys[i] = calloc(8, sizeof(char));
        vals[i] = calloc(8, sizeof(char));
        sprintf(keys[i], "key%.4d", i);
        sprintf(vals[i], "val%.4d", i);
        le_hashmap_Put(map, keys[i], vals[i]);
    }
    LE_TEST(le_hashmap_Size(map) == 1002);

    int cCount1 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %u", cCount1);

    // Test the foreach functionality (lots of output!)
    int maxCount = 100;
    le_hashmap_ForEach(map, &itHandler, &maxCount);
    LE_INFO("Iterate test PASSED");

    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_isEmpty(map));
}

void TestCustomHashMap(le_hashmap_Ref_t map)
{
    LE_INFO("*** Running custom hashmap tests ***");

    const char* skey1 = "key1";
    const char* skey2 = "key2";
    const char* sval1 = "val1";
    const char* sval2 = "val2";
    Key_t key1 = { 1, skey1 };
    Key_t key2 = { 2, skey2 };
    Key_t key3 = { 1, skey1 };

    // Store with key1 and retrieve with key3. They should resolve as equals
    // but the pointers are different so our callback will be called
    le_hashmap_Put(map, &key1, sval1);
    void *rval = le_hashmap_Get(map, &key3);
    LE_ASSERT(NULL != rval);
    LE_INFO("rval came back as %s", ((const char*) rval));
    LE_TEST (((const char*) rval) == sval1);

    rval = insertRetrieve(map, &key2, sval2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == sval2) && (le_hashmap_Size(map) == 2));

    rval = insertRetrieve(map, &key1, sval2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == sval2) && (le_hashmap_Size(map) == 2));

    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_isEmpty(map));
}

bool itHandler
(
    const void* keyPtr,
    const void* valuePtr,
    void* contextPtr
)
{
    static int count=0;
    int max = (*(int*)contextPtr);
    //LE_DEBUG("Handler called with %p and %p and %d", keyPtr, valuePtr, max);

    return ++count < max;
}

size_t le_hashmap_HashCustom(const void* keyPtr)
{
    //LE_INFO("Custom hash function called");
    return ((Key_t*)(keyPtr))->i;
}

bool le_hashmap_EqualsCustom(const void* firstPtr, const void* secondPtr)
{
    //LE_INFO("Custom equals function called");
    Key_t* k1 = ((Key_t*) firstPtr);
    Key_t* k2 = ((Key_t*) secondPtr);

    return ((k1->i == k2->i) && (le_hashmap_EqualsString(firstPtr, secondPtr)));
}

void TestNewIter()
{
    LE_INFO("Creating int/int map for iter tests");
    le_hashmap_Ref_t map10 = le_hashmap_Create("Map10", 13, &le_hashmap_HashUInt32,
                                               &le_hashmap_EqualsUInt32);

    uint32_t index = 0;
    uint32_t *iPtr = &index;
    uint32_t value = 0;
    uint32_t *vPtr = &value;
    uint32_t iterKey = 0;
    uint32_t *iterKeyPtr = &iterKey;

    uint32_t okKey = 3;
    uint32_t badKey = 50;

    // Try getting the first node when it's empty
    LE_TEST(le_hashmap_GetFirstNode(map10, (void **)&iPtr, (void **)&vPtr) == LE_NOT_FOUND);

    // Populate the map
    // Time to store 10 pairs
    uint32_t iKeys[10];
    uint32_t iVals[10];
    uint32_t j = 0;
    for (j=1; j<11; j++) {
        iKeys[j-1] = j;
        iVals[j-1] = j * 3;
        le_hashmap_Put(map10, &iKeys[j-1], &iVals[j-1]);
    }

    // Try getting the first node when now
    LE_TEST(le_hashmap_GetFirstNode(map10, (void **)&iPtr, (void **)&vPtr) == LE_OK);
    LE_INFO("Key = %d, value = %d", *iPtr, *vPtr);

    // try a NULL as the key
    LE_TEST(le_hashmap_GetFirstNode(map10, NULL, (void **)&vPtr) == LE_BAD_PARAMETER);

    // Get the node after a non-existent one
    LE_TEST(le_hashmap_GetNodeAfter(map10, (void *)&badKey, (void **)&iPtr,
                                    (void **)&vPtr) == LE_BAD_PARAMETER);

    // Get the node after a good one
    LE_TEST(le_hashmap_GetNodeAfter(map10, (void *)&okKey, (void **)&iPtr,
                                    (void **)&vPtr) != LE_BAD_PARAMETER);
    LE_INFO("Key is %d, value is %d", *iPtr, *vPtr);

    // Try and iterate over the whole map
    LE_TEST(le_hashmap_GetFirstNode(map10, (void **)&iterKeyPtr, (void **)&vPtr) == LE_OK);
    LE_INFO("First key is %d", *iterKeyPtr);

    for (j=0; j<9; j++)
    {
        // Get the node after a good one
        LE_TEST(le_hashmap_GetNodeAfter(map10, (void *)iterKeyPtr,
                                        (void **)&iterKeyPtr, (void **)&vPtr) == LE_OK);
        LE_INFO("Next key is %d", *iterKeyPtr);
    }
    // Run over the end
    LE_TEST(le_hashmap_GetNodeAfter(map10, (void *)iterKeyPtr, (void **)&iterKeyPtr,
                                    (void **)&vPtr) == LE_NOT_FOUND);
}

void TestPointerMap(le_hashmap_Ref_t map){
    LE_INFO("*** Running pointer hashmap tests ***");
    const char* key1 = "key1";
    const char* key2 = "key2";
    const char* val1 = "val1";
    const char* val2 = "val2";

    void* rval = insertRetrieve(map, key1, val1);
    LE_ASSERT(NULL != rval);
    LE_TEST (((const char*) rval) == val1);

    rval = insertRetrieve(map, key2, val2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == val2) && (le_hashmap_Size(map) == 2));

    rval = insertRetrieve(map, key1, val2);
    LE_ASSERT(NULL != rval);
    LE_TEST ((((const char*) rval) == val2) && (le_hashmap_Size(map) == 2));

    int i = 0;
    char* keys[1000];
    char* vals[1000];
    for (i=0; i<1000; i++)
    {
        keys[i] = calloc(8, sizeof(char));
        vals[i] = calloc(8, sizeof(char));
        sprintf(keys[i], "key%.4d", i);
        sprintf(vals[i], "val%.4d", i);
        le_hashmap_Put(map, keys[i], vals[i]);
    }
    LE_TEST(le_hashmap_Size(map) == 1002);

    int cCount1 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %u", cCount1);

    // Test the foreach functionality (lots of output!)
    int maxCount = 100;
    le_hashmap_ForEach(map, &itHandler, &maxCount);
    LE_INFO("Iterate test PASSED");

    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_isEmpty(map));
}

void TestTinyMap(le_hashmap_Ref_t map)
{
    uint32_t ikey1 = 100;
    uint32_t ival1 = 100;
    uint32_t ikey2 = 200;
    uint32_t ival2 = 200;

    LE_INFO("*** Running tiny hashmap tests ***");

    void* rval = insertRetrieve(map, &ikey1, &ival1);
    LE_ASSERT(NULL != rval);
    LE_TEST (*((uint32_t*) rval) == ival1);

    rval = insertRetrieve(map, &ikey2, &ival2);
    LE_ASSERT(NULL != rval);
    LE_TEST (*((uint32_t*) rval) == ival2);
}

void TestIterRemove(le_hashmap_Ref_t map)
{
    uint32_t iKeys[1000];
    uint32_t iVals[1000];
    int itercnt = 0;

    int j = 0;
    for (j=0; j<1000; j++) {
        iKeys[j] = j * 2;
        iVals[j] = j * 4;
        le_hashmap_Put(map, &iKeys[j], &iVals[j]);
    }
    LE_TEST(le_hashmap_Size(map) == 1000);

    le_hashmap_It_Ref_t mapIt = le_hashmap_GetIterator(map);
    LE_TEST(le_hashmap_GetKey(mapIt) == NULL);
    while (le_hashmap_NextNode(mapIt) == LE_OK)
    {
        itercnt++;
        const uint32_t* keyPtr = le_hashmap_GetKey(mapIt);
        LE_ASSERT(NULL != keyPtr);

        const uint32_t* valuePtr = le_hashmap_GetValue(mapIt);

        LE_ASSERT(*valuePtr == (*keyPtr * 2));

        if (itercnt % 2 != 0)
        {
            le_hashmap_Remove(map, keyPtr);
        }
    }
    LE_TEST(itercnt == 1000);
    LE_TEST(le_hashmap_Size(map) == 500);
}

void TestLongIntHashMap(le_hashmap_Ref_t map)
{
    uint64_t ikey1 = 1412320402000;
    uint64_t ival1 = 100;
    uint64_t ival2 = 350;
    uint32_t cCount1, cCount2 = 0;

    LE_INFO("*** Running long int/int hashmap tests ***");

    void* rval = insertRetrieve(map, &ikey1, &ival1);
    LE_ASSERT(NULL != rval);
    LE_TEST (*((uint64_t*) rval) == ival1);

    rval = insertRetrieve(map, &ikey1, &ival2);
    LE_ASSERT(NULL != rval);
    LE_TEST((*((uint64_t*) rval) == ival2) && (le_hashmap_Size(map) == 1));

    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_isEmpty(map));

    // Time to store 1000 pairs
    uint64_t iKeys[1000];
    uint64_t iVals[1000];
    int j = 0;
    for (j=0; j<1000; j++) {
        iKeys[j] = ikey1 + j;
        iVals[j] = j * 4;
        le_hashmap_Put(map, &iKeys[j], &iVals[j]);
    }
    LE_TEST(le_hashmap_Size(map) == 1000);

    cCount1 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %u", cCount1);
    for (j=0; j<1000; j+=2) {
        uint64_t iKey = ikey1 + j;
        le_hashmap_Remove(map, &iKey);
    }
    LE_TEST(le_hashmap_Size(map) == 500);

    cCount2 = le_hashmap_CountCollisions(map);
    LE_INFO("Collision count = %zu", le_hashmap_CountCollisions(map));
    LE_TEST(cCount1 > cCount2);

    // Iterate over the map
    le_hashmap_It_Ref_t mapIt = le_hashmap_GetIterator(map);
    LE_TEST(le_hashmap_GetKey(mapIt) == NULL);
    int itercnt = 0;
    while (le_hashmap_NextNode(mapIt) == LE_OK)
    {
        itercnt++;
        le_hashmap_GetKey(mapIt);
        le_hashmap_GetValue(mapIt);
    }
    LE_INFO("Iterator count = %d", itercnt);
    LE_TEST(itercnt == 500);
    // Now back again.
    while (le_hashmap_PrevNode(mapIt) == LE_OK)
    {
        itercnt--;
        le_hashmap_GetKey(mapIt);
        le_hashmap_GetValue(mapIt);
    }
    LE_INFO("Iterator count = %d", itercnt);
    LE_TEST(itercnt == 0);

    // Cleanup the map again to allow it to be reused
    le_hashmap_RemoveAll(map);
    LE_TEST(le_hashmap_Size(map) == 0);

    // Check iterator on an empty map
    mapIt = le_hashmap_GetIterator(map);
    LE_TEST(le_hashmap_NextNode(mapIt) == LE_NOT_FOUND);
}
