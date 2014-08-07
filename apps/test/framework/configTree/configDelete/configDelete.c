
#include "legato.h"
#include "interfaces.h"




/// Maximum expected size of a config tree name.
#define TREE_NAME_MAX 65




COMPONENT_INIT
{
    LE_INFO("----  Deleting all trees.  ------------------------------");

    {
        le_cfgAdmin_IteratorRef_t iteratorRef = le_cfgAdmin_CreateTreeIterator();

        while (le_cfgAdmin_NextTree(iteratorRef) == LE_OK)
        {
            char treeName[TREE_NAME_MAX] = { 0 };

            if (le_cfgAdmin_GetTreeName(iteratorRef, treeName, sizeof(treeName)) == LE_OK)
            {
                printf("Deleting %s.\n", treeName);
                le_cfgAdmin_DeleteTree(treeName);
            }
        }

        le_cfgAdmin_ReleaseTreeIterator(iteratorRef);
    }


    LE_INFO("----  Expecting no trees left.  -------------------------");

    le_cfgAdmin_IteratorRef_t iteratorRef = le_cfgAdmin_CreateTreeIterator();

    le_result_t result = le_cfgAdmin_NextTree(iteratorRef);

    LE_FATAL_IF(result != LE_NOT_FOUND,
                "Expected LE_NOT_FOUND but got %s, (%d), instead.",
                LE_RESULT_TXT(result),
                result);

    LE_INFO("----  Done.  --------------------------------------------");

    exit(EXIT_SUCCESS);
}
