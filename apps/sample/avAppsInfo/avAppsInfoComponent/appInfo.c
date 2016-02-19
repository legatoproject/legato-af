//--------------------------------------------------------------------------------------------------
/** @file appInfo.c
 *
 * This app exposes assets over lwm2m that allow for inspection of resources used by applications.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "limit.h"
#include "fileDescriptor.h"
#include "cgroups.h"

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pools for created objects to track apps and their processes
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t AppObjPool;
static le_mem_PoolRef_t ProcObjPool;

//--------------------------------------------------------------------------------------------------
/**
 * Structure for reporting and saving cpu timing values
 */
//--------------------------------------------------------------------------------------------------
typedef struct TimeValues {
    uint32_t utime;
    uint32_t stime;
    uint32_t cpu_total;
} TimeValues_t;


//--------------------------------------------------------------------------------------------------
/**
 * Structure for storing proc related data (contains link for adding to a singly linked list)
 */
//--------------------------------------------------------------------------------------------------
typedef struct ProcObj {
    le_sls_Link_t link;
    pid_t pid;
    TimeValues_t old_time;
    bool stale;
} ProcObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure for storing app related data and the head of the list of proc objects for this app
 */
//--------------------------------------------------------------------------------------------------
typedef struct AppObj {
    le_sls_Link_t link;
    char name[LIMIT_MAX_APP_NAME_BYTES];
    le_avdata_AssetInstanceRef_t appAssetRef;
    le_sls_List_t procList;
} AppObj_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure for parsing the data from proc/stat
 */
//--------------------------------------------------------------------------------------------------
typedef struct cpu_time {
    uint32_t user;
    uint32_t nice;
    uint32_t system;
    uint32_t idle;
    uint32_t iowait;
    uint32_t irq;
    uint32_t softirq;
    uint32_t steal;
    uint32_t guest;
    uint32_t guest_nice;
} cpu_time_t;

//--------------------------------------------------------------------------------------------------
/**
 * Singly linked list of app objects for all installed apps.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t AppObjList = LE_SLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Static data used by AddNumToStr and ResetAddNumToStr
 */
//--------------------------------------------------------------------------------------------------
static int AddNumToStr_Offset = 0;
static char* AddNumToStr_Fmt = "%d";
static bool AddNumToStr_Full = false;

//--------------------------------------------------------------------------------------------------
/**
 * Resets the AddNumToStr machine - see AddNumToStr
 */
//--------------------------------------------------------------------------------------------------
void ResetAddNumToStr()
{
    // empty string - reset!
    AddNumToStr_Fmt="%d";
    AddNumToStr_Full = false;
    AddNumToStr_Offset = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * AddNumToStr is not safe for reuse and requires special care to use correctly. It is used only in
 * UpdateAppInfo to build a comma separated string of PIDs but UpdateAppInfo was already getting
 * long. AddNumToStr depends on static variables so that it can be called repeatedly to build a the
 * string. The values (if there are more than one) will be separated by commas and if the resulting
 * string is longer than the max length given the string will end ",..." indicating there are more
 * entries than could be fit. When adding values to a string the same string should be used for
 * each subsequent call until you are done. To start a new string call ResetAddNumToStr.
 * If this were to be generalized it might be good to create an object that contains the data
 * needed to make this work but that is beyond current need.
 */
//--------------------------------------------------------------------------------------------------
void AddNumToStr(char* str, size_t strSz, int num)
{
    if (!AddNumToStr_Full){
        int l = snprintf(str + AddNumToStr_Offset, strSz - AddNumToStr_Offset, AddNumToStr_Fmt, num);
        if (l > (strSz - AddNumToStr_Offset))
        {
            // Now you've gone too far!!
            while (AddNumToStr_Offset > 0)
            {
                if ((strSz - AddNumToStr_Offset) > strlen(",..."))
                {
                    // see if ... fits on the end
                    strncpy(str + AddNumToStr_Offset, ",...", strSz - AddNumToStr_Offset);
                    break;
                }
                else
                {
                    // back up one comma and try again
                    do
                    {
                        AddNumToStr_Offset--;
                    }
                    while (AddNumToStr_Offset >= 0 && str[AddNumToStr_Offset] != ',');
                }
            }
            AddNumToStr_Full = true;
        }
        AddNumToStr_Offset += l;
        AddNumToStr_Fmt = ",%d";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan the proc list of the given app object for the proc object representing the given PID
 * @return
 *      a pointer to the proc object found or NULL if not found.
 */
//--------------------------------------------------------------------------------------------------
static ProcObj_t* FindProcObj
(
    AppObj_t* appObjRef,
    pid_t pid
)
{
    le_sls_List_t* listPtr = &(appObjRef->procList);
    le_sls_Link_t* linkPtr = le_sls_Peek(listPtr);
    while (linkPtr && CONTAINER_OF(linkPtr, ProcObj_t, link)->pid != pid)
    {
        linkPtr = le_sls_PeekNext(listPtr, linkPtr);
    }
    return CONTAINER_OF(linkPtr, ProcObj_t, link);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a new proc object
 * All new objects are born stale so if they don't get freshened they will be pruned.
 * @return
 *      pointer to the new prob object
 */
//--------------------------------------------------------------------------------------------------
static ProcObj_t* CreateProcObj
(
    AppObj_t* appObjRef,
    pid_t pid
)
{
    le_sls_List_t* listPtr = &(appObjRef->procList);
    ProcObj_t* newProcObj = le_mem_ForceAlloc(ProcObjPool);
    newProcObj->link = LE_SLS_LINK_INIT;
    newProcObj->pid = pid;
    newProcObj->old_time.utime = 0;
    newProcObj->old_time.stime = 0;
    newProcObj->old_time.cpu_total = 0;
    newProcObj->stale = true;
    le_sls_Queue(listPtr, &(newProcObj->link));
    return newProcObj;
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan the proc list on the given app object and mark all the proc info as stale.
 */
//--------------------------------------------------------------------------------------------------
static void MarkProcsStale
(
    AppObj_t* appObjRef
)
{
    le_sls_List_t* listPtr = &(appObjRef->procList);
    le_sls_Link_t* linkPtr = le_sls_Peek(listPtr);
    while (linkPtr)
    {
        ProcObj_t* procObjRef = CONTAINER_OF(linkPtr, ProcObj_t, link);
        procObjRef->stale = true;
        linkPtr = le_sls_PeekNext(listPtr, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Scan the proc object list of the given app object removing and freeing any proc objects found
 * to be marked stale.
 *
 * @note
 *      In removing I can POP it if I find it at the head or I must use RemoveAfter
 *      and remember the link just before. Effectively if I start with previous link == NULL
 *      then I will know which way to go.
 *      After removing the link I resume stepping from the link before.
 */
//--------------------------------------------------------------------------------------------------
static void PruneStaleProcs
(
    AppObj_t* appObjRef
)
{
    le_sls_List_t* listPtr = &(appObjRef->procList);
    le_sls_Link_t* linkPtr = le_sls_Peek(listPtr);
    le_sls_Link_t* previousLinkPtr = NULL;
    while (linkPtr)
    {
        ProcObj_t* procObjRef = CONTAINER_OF(linkPtr, ProcObj_t, link);
        if(procObjRef->stale == true)
        {
            if (previousLinkPtr)
            {
                le_sls_RemoveAfter(listPtr, previousLinkPtr);
            }
            else
            {
                le_sls_Pop(listPtr);
            }
            le_mem_Release(procObjRef);
            linkPtr = previousLinkPtr;
        }
        previousLinkPtr = linkPtr;
        linkPtr = le_sls_PeekNext(listPtr, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the data read from /proc/stat into a cpu_time struct.
 *
 * @return
 *      LE_OK if all went well
 *      LE_NOT_FOUND if the line doesn't start with "cpu "
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseCPULine(const char* buffer, cpu_time_t* cpuTime)
{
    // First chars in the line are "cpu " followed by several integers
    char* nextChPtr;
    if (strncmp("cpu ", buffer, strlen("cpu ")) != 0)
    {
        return LE_NOT_FOUND;
    }
    // Since we used fd_ReadLine the buffer is NULL so none of these calls
    // will run off the end of the buffer. Worst we'll get is zeros in
    // fields.
    nextChPtr = strchr(buffer, ' ');
    cpuTime->user = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->nice = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->system = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->idle = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->iowait = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->irq = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->softirq = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->steal = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->guest = strtoul(nextChPtr, &nextChPtr, 10);
    cpuTime->guest_nice = strtoul(nextChPtr, &nextChPtr, 10);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read /proc/stat and total up the various times to get total time
 *
 * @return
 *      Total jiffies in stat (or zero if we fail - assume all errors are transitory and we'll
 *      have better luck next time while reporting unreliable numbers this time).
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GetTotalCPU()
{
    cpu_time_t cpuTime;
    int fd = 0;
    char buffer[1024];
    le_result_t result;
    uint32_t cpuTotalTime = 0;
    do
    {
        fd = open("/proc/stat", O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if ( (fd == -1) && (errno == ENOENT ) )
    {
        LE_WARN("Could not find /proc/stat.");
    }
    else
    {
        result = fd_ReadLine(fd, buffer, sizeof(buffer) - 1);
        if (result == LE_OVERFLOW)
        {
            LE_INFO("Read from /proc/stat possibly truncated. Reported CPU time may be inaccurate");
            // But we will try to get total time anyway.
        }
        else if (result == LE_OK)
        {
            // We only have one cpu and the cpu line should be the first line.
            if (ParseCPULine(buffer, &cpuTime) != LE_OK)
            {
                LE_WARN("Failed to read cpu line in /proc/stat.");
            }
            else
            {
                cpuTotalTime = cpuTime.user +
                                cpuTime.nice +
                                cpuTime.system +
                                cpuTime.idle +
                                cpuTime.iowait +
                                cpuTime.irq +
                                cpuTime.softirq +
                                cpuTime.steal +
                                cpuTime.guest +
                                cpuTime.guest_nice;
            }
        }
        else
        {
            LE_WARN("Problem while trying to read /proc/stat.");
        }
        fd_Close(fd);
    }
    return cpuTotalTime;
}

//--------------------------------------------------------------------------------------------------
/**
 * Parse the data from a buffer read from /proc/pid/stat.
 *
 * @note
 *      The "/proc/pid/stat" line is a line of space separated values except that the second item
 *      is the executable name in parentheses. The name may include spaces and parantheses. We
 *      start by looking for the last closing parenthesis.
 *
 * @return
 *      None. The output values are written to the passed in references to utime and stime
 */
//--------------------------------------------------------------------------------------------------
static void GetProcTimes
(
    const char* buffer,
    uint32_t* utime,
    uint32_t* stime
)
{
    char* bracket = strchr(buffer, ')');
    char* lastBracket = NULL;
    while (bracket)
    {
        lastBracket = bracket + 1;
        bracket = strchr(lastBracket, ')');
    }
    // We are at the end of item 2, 12 more to go (see man-page for proc, proc(5))
    char* space = lastBracket;
    int count = 1;
    while (count < 12 && space)
    {
        space++;
        space = strchr(space, ' ');
        count ++;
    }
    *utime = strtoul(space, &space, 10);
    *stime = strtoul(space, NULL, 10);
}

//--------------------------------------------------------------------------------------------------
/**
 * Open and read the first line from /proc/[pid]/stat into buffer (up to bufferSize bytes)
 *
 * @return
 *      LE_OK if we read the whole line into the buffer
 *      LE_NOT_FOUND if we couldn't open /proc/[pid]/stat
 *      LE_OVERFLOW if the buffer is too small to take the whole line
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadProcPidStat
(
    pid_t pid,
    char* buffer,
    int bufferSize
)
{
    int fd = 0;
    le_result_t result = LE_FAULT;
    char procFile[LIMIT_MAX_PATH_BYTES];
    LE_FATAL_IF(snprintf(procFile, sizeof(procFile), "/proc/%d/stat", pid) >= sizeof(procFile),
                    "File name '%s...' size is too long.", procFile);

    do
    {
        fd = open(procFile, O_RDONLY);
    }
    while ( (fd == -1) && (errno == EINTR) );

    if ( (fd == -1) && (errno == ENOENT ) )
    {
        return LE_NOT_FOUND;
    }
    else
    {
        LE_FATAL_IF(fd == -1, "Could not read file %s.  %m.", procFile);

        result = fd_ReadLine(fd, buffer, bufferSize);
        LE_FATAL_IF(result == LE_OVERFLOW, "Buffer to read PID is too small.");

        if (result != LE_OK)
        {
            LE_FATAL("Error reading the %s", procFile);
        }
        fd_Close(fd);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the proc/[pid]/stat to get the timing values for the process and also read the current CPU
 * time (so that we get a cpu time close to the same time we read the proc times). Calculate the
 * difference between the stored time and the current time then store the new timing data in the
 * proc object for the next time we need deltas.
 * Output delta values are written tot he TimeValues_t ref passed in.
 */
//--------------------------------------------------------------------------------------------------
static void GetCPUDeltaForProc
(
    AppObj_t* appObjRef,
    pid_t pid,
    TimeValues_t* returnValues
)
{
    ProcObj_t* pO = FindProcObj(appObjRef, pid);

    if (pO == NULL)
    {
        pO = CreateProcObj(appObjRef, pid);
    }

    uint32_t cpuTotalTime = GetTotalCPU();

    // Read the stat file - utime and stime are 14th and 15th in the list.
    char statBuffer[512];

    if (ReadProcPidStat(pid, statBuffer, sizeof(statBuffer)) == LE_OK)
    {

        uint32_t utime;
        uint32_t stime;
        GetProcTimes(statBuffer, &utime, &stime);
        returnValues->utime = (utime - pO->old_time.utime);
        returnValues->stime = (stime - pO->old_time.stime);
        returnValues->cpu_total = (cpuTotalTime - pO->old_time.cpu_total);
        pO->old_time.utime = utime;
        pO->old_time.stime = stime;
        pO->old_time.cpu_total = cpuTotalTime;
        pO->stale = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the memory useage for the app from cgroups
 */
//--------------------------------------------------------------------------------------------------
static void GetMemForProc
(
    AppObj_t* appObjRef,
    pid_t pid
)
{
    char* appName = appObjRef->name;
    ssize_t result = cgrp_GetMemUsed(appName);
    if (result >= LE_OK)
    {
        char str[32];
        snprintf(str, sizeof(str)-1, "%u bytes", (unsigned int)result);
        le_avdata_SetString(appObjRef->appAssetRef, "Memory_Used", str);
    }
    else
    {
        LE_WARN("Couldn't get used memory for %s", appName);
        le_avdata_SetString(appObjRef->appAssetRef, "Memory_Used", "Error");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This app has been found not to be running. Clean up the asset fields and
 * prune any processes from the AppObj proc list
 */
//--------------------------------------------------------------------------------------------------
static void ClearApp
(
    AppObj_t* appObjRef
)
{
    le_avdata_AssetInstanceRef_t assetRef = appObjRef->appAssetRef;

    le_avdata_SetString(assetRef, "Status", "Stopped");
    le_avdata_SetString(assetRef, "PID", "None");
    le_avdata_SetString(assetRef, "Memory_Used", "None");
    le_avdata_SetString(assetRef, "CPU", "None");

    // Clean up the processes for this app object
    MarkProcsStale(appObjRef);
    PruneStaleProcs(appObjRef);
}

// This value is the same as used in appInfo
#define MAX_NUM_THREADS_TO_DISPLAY 100
//--------------------------------------------------------------------------------------------------
/**
 * Given an app name find all the associated processes and get the CPU and memory usage for each.
 * Sum the parts and update the asset fields for this app.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateAppInfo
(
    AppObj_t* appObjRef                           ///< [IN] App Object
)
{
    // Get the list of thread IDs for this app.
    pid_t tidList[MAX_NUM_THREADS_TO_DISPLAY];
    char Str[LE_AVDATA_STRING_VALUE_LEN];

    char* appNamePtr = appObjRef->name;

    if (le_appInfo_GetState(appNamePtr) == LE_APPINFO_RUNNING)
    {
        ssize_t numAvailThreads = cgrp_GetProcessesList(CGRP_SUBSYS_FREEZE,
                                                        appNamePtr,
                                                        tidList,
                                                        MAX_NUM_THREADS_TO_DISPLAY);

        if (numAvailThreads <= 0)
        {
            // Despite having been reported running it now appears to not be
            ClearApp(appObjRef);
        }
        else
        {
            le_avdata_SetString(appObjRef->appAssetRef, "Status", "Running");
            // Calculate the number of threads to iterate over.
            size_t numThreads = numAvailThreads;
            if (numAvailThreads > MAX_NUM_THREADS_TO_DISPLAY)
            {
                LE_WARN("App has %d processes."
                "Only the first %d will be used to calculate avappinfo stats",
                (unsigned int)numAvailThreads, MAX_NUM_THREADS_TO_DISPLAY);

                numThreads = MAX_NUM_THREADS_TO_DISPLAY;
            }

            // Mark all procs as having old data
            MarkProcsStale(appObjRef);

            // Iterate over the list of threads and build the process objects.
            size_t i;
            TimeValues_t deltaTimeValues;
            float percentTime = 0.0;
            ResetAddNumToStr();
            for (i = 0; i < numThreads; i++)
            {
                // The following funcs will update the data in procs they find
                GetCPUDeltaForProc(appObjRef, tidList[i], &deltaTimeValues);
                GetMemForProc(appObjRef, tidList[i]);
                uint32_t deltaAppTime = (deltaTimeValues.utime + deltaTimeValues.stime);
                uint32_t deltaCpuTime = deltaTimeValues.cpu_total;
                // It might be "nice" to have a value for each pid but that could get unwieldy.
                // Either we'd have to make a bunch more asset instances or a very long string.
                // Because sampling interval across all procs may not be equal this sum may not
                // be perfect but it should be pretty close.
                if (deltaCpuTime != 0)
                {
                    percentTime += (deltaAppTime * 100.0) / deltaCpuTime;
                }
                else
                {
                    percentTime = 0.0;
                }
                AddNumToStr(Str, LE_AVDATA_STRING_VALUE_LEN, tidList[i]);

            }
            // Any proc that didn't get updated must have died.
            PruneStaleProcs(appObjRef);
            le_avdata_SetString(appObjRef->appAssetRef, "PID", Str);
            snprintf(Str, LE_AVDATA_STRING_VALUE_LEN, "%3.1f%%\n", percentTime);
            le_avdata_SetString(appObjRef->appAssetRef, "CPU", Str);
        }
    }
    else
    {
        ClearApp(appObjRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *
 * Determines the number of assets to create and sets up handlers
 *
 * @note
 *      For each installed app create an asset and attach the handlers.
 *
 * @return
 *      None
 */
//--------------------------------------------------------------------------------------------------
static void CreateAppAssets
(
    void
)
{
    le_cfg_IteratorRef_t cfgIter = le_cfg_CreateReadTxn("/apps");

    if (le_cfg_GoToFirstChild(cfgIter) == LE_NOT_FOUND)
    {
        LE_DEBUG("There are no installed apps.");
        exit(EXIT_SUCCESS);
    }

    // Iterate over the list of apps.
    do
    {
        AppObj_t* appObjRef = le_mem_ForceAlloc(AppObjPool);

        LE_FATAL_IF(le_cfg_GetNodeName(cfgIter, "", appObjRef->name, sizeof(appObjRef->name)) != LE_OK,
                        "Application name in config is too long.");


        appObjRef->procList = LE_SLS_LIST_INIT;
        appObjRef->appAssetRef = le_avdata_Create("Application_Info");
        le_sls_Queue(&AppObjList, &(appObjRef->link));
        le_avdata_SetString(appObjRef->appAssetRef, "Name", appObjRef->name);
        UpdateAppInfo(appObjRef);
    }
    while (le_cfg_GoToNextSibling(cfgIter) == LE_OK);
    // Release the read iterator
    le_cfg_CancelTxn(cfgIter);
}

//--------------------------------------------------------------------------------------------------
/**
 * Run through the list of installed apps that we built at the start and update all the app info
 * in all of the created assets.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateAllApps
(
    le_timer_Ref_t timerRef ///< [IN] The reference to the expired timer
)
{
    le_sls_Link_t* linkPtr = le_sls_Peek(&AppObjList);
    // Iterate over the list of apps.
    while (linkPtr)
    {
        AppObj_t* appObjRef = CONTAINER_OF(linkPtr, AppObj_t, link);
        UpdateAppInfo(appObjRef);
        linkPtr = le_sls_PeekNext(&AppObjList, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * It all starts here
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("======== Starting ======== ");

    cgrp_Init();
    AppObjPool = le_mem_CreatePool("Apps", sizeof(AppObj_t));
    ProcObjPool = le_mem_CreatePool("Apps", sizeof(ProcObj_t));

    CreateAppAssets();

    // Create a timer with 4 second timeout that calls the update function
    le_clk_Time_t interval;
    interval.sec = 4;
    interval.usec = 0;

    le_timer_Ref_t intervalTimer = le_timer_Create("avappinfo_timer");
    LE_ASSERT(LE_OK == le_timer_SetHandler(intervalTimer, UpdateAllApps));
    LE_ASSERT(LE_OK == le_timer_SetInterval(intervalTimer, interval));
    LE_ASSERT(LE_OK == le_timer_SetRepeat(intervalTimer, 0)); // repeating for ever
    le_timer_Start(intervalTimer);

    /* Test code */
#ifdef TEST
    CallEventHandlers();
    DiagOutput();
#endif
    LE_INFO("====== All set up and running =========");
}
