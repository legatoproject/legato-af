
#include "legato.h"

COMPONENT_INIT
{
    LE_INFO("======== BEGIN SAFE REFERENCES TEST ========");

    le_ref_MapRef_t mapRef1 = le_ref_CreateMap("Map 1", 4);

    LE_INFO("Created reference map %p.", mapRef1);

    LE_INFO("Creating references in map %p.", mapRef1);

    void* safeRef1 = le_ref_CreateRef(mapRef1, (void*)0x1001);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == ((void*)0x1001));
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef1, le_ref_Lookup(mapRef1, safeRef1));
    void* safeRef2 = le_ref_CreateRef(mapRef1, (void*)0x1002);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef2, le_ref_Lookup(mapRef1, safeRef2));
    void* safeRef3 = le_ref_CreateRef(mapRef1, (void*)0x1003);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef3, le_ref_Lookup(mapRef1, safeRef3));
    void* safeRef4 = le_ref_CreateRef(mapRef1, (void*)0x1004);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef4, le_ref_Lookup(mapRef1, safeRef4));

    LE_INFO("Deleting references...");

    le_ref_DeleteRef(mapRef1, safeRef1);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == NULL);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef2) == ((void*)0x1002));
    LE_INFO("  Successfully deleted reference %p.", safeRef1);
    le_ref_DeleteRef(mapRef1, safeRef2);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef2) == NULL);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef3) == ((void*)0x1003));
    LE_INFO("  Successfully deleted reference %p.", safeRef2);
    le_ref_DeleteRef(mapRef1, safeRef3);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef3) == NULL);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef4) == ((void*)0x1004));
    LE_INFO("  Successfully deleted reference %p.", safeRef3);
    le_ref_DeleteRef(mapRef1, safeRef4);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef4) == NULL);
    LE_INFO("  Successfully deleted reference %p.", safeRef4);

    LE_INFO("Creating references in map %p.", mapRef1);

    safeRef1 = le_ref_CreateRef(mapRef1, (void*)0x1001);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == ((void*)0x1001));
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef1, le_ref_Lookup(mapRef1, safeRef1));
    safeRef2 = le_ref_CreateRef(mapRef1, (void*)0x1002);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef2, le_ref_Lookup(mapRef1, safeRef2));
    safeRef3 = le_ref_CreateRef(mapRef1, (void*)0x1003);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef3, le_ref_Lookup(mapRef1, safeRef3));
    safeRef4 = le_ref_CreateRef(mapRef1, (void*)0x1004);
    LE_ASSERT(le_ref_Lookup(mapRef1, safeRef1) == (void*)0x1001);
    LE_INFO("  Successfully created reference %p mapping to %p.", safeRef4, le_ref_Lookup(mapRef1, safeRef4));

    LE_ASSERT(le_ref_Lookup(mapRef1, NULL) == NULL);
    LE_INFO("NULL lookup failed, as expected.");
    LE_INFO("Deleting NULL (expect ERROR)");
    le_ref_DeleteRef(mapRef1, NULL);
    LE_ASSERT(le_ref_Lookup(mapRef1, &mapRef1) == NULL);
    LE_INFO("Looking up a pointer value failed, as expected");


    LE_INFO("======== SAFE REFERENCES TEST COMPLETE (PASSED) ========");
    exit(EXIT_SUCCESS);
}

