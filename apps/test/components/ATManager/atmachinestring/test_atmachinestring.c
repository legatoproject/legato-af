/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "atMachineString.h"
#include "atCmd.h"

#define STRING_1    "STRING_1"
#define STRING_2    "STRING_2"

/* The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite(void)
{
    atmachinestring_Init();

    return 0;
}

/* The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite(void)
{
    return 0;
}

static le_dls_List_t   ListAddRemoveTest=LE_DLS_LIST_INIT;

// Test this function :
// void atmachinestring_AddInList(le_dls_List_t *list,const char **patternListPtr);
void testatmachinestring_AddInList()
{
    le_dls_Link_t* linkPtr;
    atmachinestring_t * currentPtr;
    const char* patternlist[] = {STRING_1,STRING_2,NULL};

    atmachinestring_AddInList(&ListAddRemoveTest,patternlist);

    CU_ASSERT_EQUAL(le_dls_NumLinks(&ListAddRemoveTest),2);
    linkPtr = le_dls_Peek(&ListAddRemoveTest);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_1),0);
    linkPtr = le_dls_PeekNext(&ListAddRemoveTest,&currentPtr->link);
    currentPtr = CONTAINER_OF(linkPtr,atmachinestring_t,link);
    CU_ASSERT_EQUAL(strcmp(currentPtr->line,STRING_2),0);

    CU_PASS("atmachinestring_AddInList");
}

// Test this function :
// void atmachinestring_ReleaseFromList(le_dls_List_t *pList);
void testatmachinestring_ReleaseFromList()
{
    atmachinestring_ReleaseFromList(&ListAddRemoveTest);

    CU_ASSERT_EQUAL(le_dls_NumLinks(&ListAddRemoveTest),0);

    CU_PASS("atmachinestring_ReleaseFromList");
}

#define LINE0   ""
#define LINE0_0 " "
#define LINE1   "+CMTI: \"SM\",10"
#define LINE2   "+VOILA"
#define LINE3   "+CREG: 10,13,64,35,01"
#define LINE4   "ERROR"
#define LINE5   "+CME ERROR: 10"
#define LINE6   "\0\0\0\0\0"
#define LINE7   "AT\0TEST\0"
#define LINE8   "+CMGL: 1,2\0+CMGL2: 3,4\0"
#define LINE9   "+CMGR: 1,,,10"

// Test these functions :
// char* atcmd_GetLineParameter(const char* linePtr,uint32_t pos);
// uint32_t atcmd_CountLineParameter(char *linePtr);
void testatcmd_CountLineParameter()
{
    uint32_t  num=0;
    char buffer[1024];
    char* plast=NULL;

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE0);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE0_0);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,1);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE1);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,3);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,1),"+CMTI:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,2),"\"SM\""),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,3),"10"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE2);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,1);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,1),"+VOILA"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE3);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,6);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,1),"+CREG:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,2),"10"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,3),"13"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,4),"64"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,5),"35"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,6),"01"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE4);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,1);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,1),"ERROR"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    strcpy(buffer,LINE5);
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,2);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,1),"+CME ERROR:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(buffer,2),"10"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE6,sizeof(LINE6));
    plast=atcmd_GetLineParameter(buffer,1);
    CU_ASSERT_PTR_EQUAL(plast,buffer);

    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE7,sizeof(LINE7));
    plast=atcmd_GetLineParameter(buffer,1);
    CU_ASSERT_PTR_EQUAL(plast,buffer);
    CU_ASSERT_EQUAL(strcmp(plast,"AT"),0);
    plast=atcmd_GetLineParameter(buffer,2);
    CU_ASSERT_PTR_EQUAL(plast,buffer+3);
    CU_ASSERT_EQUAL(strcmp(plast,"TEST"),0);

    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE8,sizeof(LINE8));
    plast=buffer;
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,3);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,1),"+CMGL:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,2),"1"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,3),"2"),0);
    plast=atcmd_GetLineParameter(plast,num+1);
    CU_ASSERT_PTR_EQUAL(plast,buffer+11);
    CU_ASSERT_EQUAL(strcmp(plast,"+CMGL2: 3,4"),0);
    num=atcmd_CountLineParameter(plast);
    CU_ASSERT_EQUAL(num,3);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,1),"+CMGL2:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,2),"3"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,3),"4"),0);


    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE9,sizeof(LINE9));
    plast=buffer;
    num=atcmd_CountLineParameter(buffer);
    CU_ASSERT_EQUAL(num,5);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,1),"+CMGR:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,2),"1"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,3),""),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,4),""),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,5),"10"),0);

    #define LINE10  "+CMGR: 0,,89,07913366003001F0040B913366719650F00000315030212152805031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560"
    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE10,sizeof(LINE10));
    plast=buffer;
    num=atcmd_CountLineParameter(buffer);
    fprintf(stdout,"line %d\n", num);
    CU_ASSERT_EQUAL(num,5);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,1),"+CMGR:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,2),"0"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,3),""),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,4),"89"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,5),"07913366003001F0040B913366719650F00000315030212152805031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560"),0);

    #define LINE11  "+CMGR: 0,,159,07913366003001F0040B913366719650F0000031503041534080A031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560"

    fprintf(stdout,"strlen %zd <%s>\n",strlen(LINE11),LINE11);
    num=0;
    memset(buffer,0,sizeof(buffer));
    memcpy(buffer,LINE11,sizeof(LINE11));
    plast=buffer;
    num=atcmd_CountLineParameter(buffer);
    fprintf(stdout,"line %d\n", num);
    CU_ASSERT_EQUAL(num,5);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,1),"+CMGR:"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,2),"0"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,3),""),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,4),"159"),0);
    CU_ASSERT_EQUAL(strcmp(atcmd_GetLineParameter(plast,5),"07913366003001F0040B913366719650F0000031503041534080A031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E560"),0);

}

#define COPYSTRING1 "\"0123456789\""
#define COPYSTRING2 "0123456789"
#define COPYSTRING3 "\"0123456789012345\""
#define COPYSTRING4 "0123456789012345"

// Test this function :
// uint32_t atcmd_CopyStringWithoutQuote(char* outBufferPtr,const char* inBufferPtr,uint32_t inBufferSize);
void testatcmd_CopyStringWithoutQuote()
{
    uint32_t numCopy;
    char outbuffer[16+1];

    numCopy = atcmd_CopyStringWithoutQuote(outbuffer,COPYSTRING1,strlen(COPYSTRING1));
    CU_ASSERT_EQUAL(numCopy,sizeof(COPYSTRING1)-2-1);
    CU_ASSERT_EQUAL(strcmp(outbuffer,COPYSTRING2),0);

    numCopy = atcmd_CopyStringWithoutQuote(outbuffer,COPYSTRING3,strlen(COPYSTRING3));
    CU_ASSERT_EQUAL(numCopy,sizeof(COPYSTRING3)-2-1);
    CU_ASSERT_EQUAL(strcmp(outbuffer,COPYSTRING4),0);
}

COMPONENT_INIT
{
    int result = EXIT_SUCCESS;

    // Init the test case / test suite data structures

    CU_TestInfo test[] =
    {
        { "Test atmachinestring_AddInList" , testatmachinestring_AddInList },
        { "Test atmachinestring_ReleaseFromList" , testatmachinestring_ReleaseFromList },
        { "Test atcmd_CountLineParameter" , testatcmd_CountLineParameter },
        { "Test atcmd_CopyStringWithoutQuote" , testatcmd_CopyStringWithoutQuote },
        CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
        { "atstring tests",           init_suite, clean_suite, test },
        CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");
        result = EXIT_FAILURE;
    }

    CU_cleanup_registry();
    exit(result);
}
