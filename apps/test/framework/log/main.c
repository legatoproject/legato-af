 /**
  * This is the unit test for the legato logging and log control.
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include "component1.h"
#include "component2.h"
#include "component1Helper.h"
#include "log.h"

//--------------------------------------------------------------------------------------------------
/**
 * Test log file where logs are written to.
 */
//--------------------------------------------------------------------------------------------------
static FILE* LogFile;


//--------------------------------------------------------------------------------------------------
/**
 * Logs the messages for all the components in this process.
 */
//--------------------------------------------------------------------------------------------------
static void LogMessages(void)
{
    // log messages from the different components.
    log_TestFrameworkMsgs();
    comp1_Foo();
    comp1_HelperFoo();
    comp2_Foo();
}


//--------------------------------------------------------------------------------------------------
/**
 * Forks a process and executes the log control tool to send a log command.
 */
//--------------------------------------------------------------------------------------------------
static void SendLogCmd
(
    char* cmdPtr,
    char* paramPtr,
    char* destPtr
)
{
    pid_t pID = fork();

    LE_FATAL_IF(pID < 0, "Failed to fork test program.");

    if (pID == 0)
    {
        // Redirect the log tools stderr to its stdout which is the terminal.
        dup2(1, 2);

        // Call log tool.
        execl(TESTLOG_LOGTOOL_PATH,
              TESTLOG_LOGTOOL_PATH, cmdPtr, paramPtr, destPtr,
              (char*)0);

        exit(EXIT_FAILURE); // Should never get here.
    }
    else
    {
        int result;
        int childExitCode;

        do
        {
            result = waitpid(pID, &childExitCode, 0);
        }
        while ((result == -1) && (errno == EINTR));

        if (result < 0)
        {
            LE_FATAL("waitpid() failed with errno %m.");
        }

        if (!WIFEXITED(childExitCode))
        {
            LE_FATAL("Log Control Tool didn't terminate normally.");
        }

        if (WEXITSTATUS(childExitCode) != EXIT_SUCCESS)
        {
            LE_FATAL("Log Control Tool terminated with a failure code (%d).",
                     WEXITSTATUS(childExitCode));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check settings that were set before this process was created.
 */
//--------------------------------------------------------------------------------------------------
void TestInitialSettings(void)
{
    // Log messages with initial settings.
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("framework, main, log.c, log_TestFrameworkMsgs, frame 5 msg, logTest, *EMR*",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_1, main, component1.c, comp1_Foo, comp1 5 msg, logTest, *EMR*",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_1, main, component1Helper.c, comp1_HelperFoo, comp1 helper 5 msg, logTest, *EMR*",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_2, main, component2.c, comp2_Foo, comp2 5 msg, logTest, *EMR*",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


#ifdef FORMAT_COMMANDS_SUPPORTED
//--------------------------------------------------------------------------------------------------
/**
 * Test setting format for all components.
 */
//--------------------------------------------------------------------------------------------------
void TestFormatAll(void)
{
    // Set format for all compoenents.
    SendLogCmd("format", "%P, %F, %f, %L Format All.", NULL);
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("framework, log.c, log_TestFrameworkMsgs, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_1, component1.c, comp1_Foo, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_1, component1Helper.c, comp1_HelperFoo, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_2, component2.c, comp2_Foo, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test setting format for just one component.
 */
//--------------------------------------------------------------------------------------------------
void TestFormatComp1(void)
{
    // Set the formatting for only component 1
    SendLogCmd("format", "%L, %P, %F, %f Format Comp1.", "*/Comp_1");
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("framework, log.c, log_TestFrameworkMsgs, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR*, Comp_1, component1.c, comp1_Foo Format Comp1.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR*, Comp_1, component1Helper.c, comp1_HelperFoo Format Comp1.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("Comp_2, component2.c, comp2_Foo, *EMR* Format All.",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}
#endif // FORMAT_COMMANDS_SUPPORTED


//--------------------------------------------------------------------------------------------------
/**
 * Test setting level for all components.
 */
//--------------------------------------------------------------------------------------------------
void TestLevelAll(void)
{
    // Reset the formatting for all components.
    SendLogCmd("format", "%L | %P | %F, %f", "*/*");

    // Set the level filter for all components.
    SendLogCmd("level", "WARNING", "*/*");
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test setting level for just one component.
 */
//--------------------------------------------------------------------------------------------------
void TestLevelComp2(void)
{
    // Set the level filter for just component 2.
    SendLogCmd("level", "DEBUG", "*/Comp_2");
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


     LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp(" DBUG | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp(" INFO | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("-WRN- | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("=ERR= | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*CRT* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test turning on traces for all componenents.
 */
//--------------------------------------------------------------------------------------------------
void TestTraceAll(void)
{
    // Reset the level back to just EMERGENCY
    SendLogCmd("level", "EMERGENCY", "*/*");

    // Turn on the trace for all components.
    SendLogCmd("trace", "key 1", NULL);
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);

    LE_ASSERT(strncmp("key 1 | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test turning on traces for just the framework.
 */
//--------------------------------------------------------------------------------------------------
void TestTraceFramework(void)
{
    // Turn on a trace for the framework only.
    SendLogCmd("trace", "key 2", "*/framework");
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 2 | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);


    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("key 1 | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test stopping all traces for all components.
 */
//--------------------------------------------------------------------------------------------------
void TestStopTraceAll(void)
{
    // Turn off traces for all components.
    SendLogCmd("stoptrace", "key 1", "*/*");
    SendLogCmd("stoptrace", "key 2", "*/*");
    LogMessages();

    // Get and compare the log files.  When doing the comparison skip the newline at the end.
    char logLine[300];

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | framework | log.c, log_TestFrameworkMsgs",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1.c, comp1_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_1 | component1Helper.c, comp1_HelperFoo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);

    LE_ASSERT(fgets(logLine, sizeof(logLine), LogFile) != NULL);
    LE_ASSERT(strncmp("*EMR* | Comp_2 | component2.c, comp2_Foo",
                      logLine, le_utf8_NumBytes(logLine) - 1) == 0);
}


le_log_SessionRef_t comp1_LogSession;
le_log_SessionRef_t comp2_LogSession;

le_log_Level_t* comp1_LogLevelFilterPtr;
le_log_Level_t* comp2_LogLevelFilterPtr;

COMPONENT_INIT
{
    // Normally the run system would take care of all the log session registrations and
    // component inits.
    comp1_LogSession = log_RegComponent("Comp_1", &comp1_LogLevelFilterPtr);
    comp2_LogSession = log_RegComponent("Comp_2", &comp2_LogLevelFilterPtr);
    comp1_Init();
    comp2_Init();

    // Open the test file where the log messages are being written to.
    LogFile = fopen(TESTLOG_STDERR_FILE_PATH, "r");
    LE_ASSERT(LogFile != NULL);

    // Read the test file to the end.
    char logLine[300];
    while (fgets(logLine, sizeof(logLine), LogFile) != NULL) {}

    // Run tests.  These tests must be called in this order.
    TestInitialSettings();

#ifdef FORMAT_COMMANDS_SUPPORTED
    TestFormatAll();
    TestFormatComp1();
#endif

    TestLevelAll();
    TestLevelComp2();
    TestTraceAll();
    TestTraceFramework();
    TestStopTraceAll();
}
