//--------------------------------------------------------------------------------------------------
/** @file args.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * On platforms with distinct processes, one instance of the argument information exists for each
 * process, however on an RTOS with a shared memory space for all tasks, the argument information
 * needs to be put in thread-local storage so that each task started can distinguish its arguments.
 **/
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_RTOS
#   define USE_THREAD_LOCAL_STORAGE 0
#   define MAX_ARG_INFOS            1
#else /* LE_CONFIG_RTOS */
#   define USE_THREAD_LOCAL_STORAGE 1
#   define MAX_ARG_INFOS            LE_CONFIG_MAX_THREAD_POOL_SIZE
#endif /* end LE_CONFIG_RTOS */


/// Default handler should exit on error.
#define DEFAULT_EXIT_ON_ERROR   0x1U
/// Any handler, default or otherwise, should exit on error.
#define EXPLICIT_EXIT_ON_ERROR  0x2U


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of the different types of options.
 **/
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_ARG_OPTION_FLAG_VAR,
    LE_ARG_OPTION_FLAG_CALLBACK,
    LE_ARG_OPTION_INT_VAR,
    LE_ARG_OPTION_INT_CALLBACK,
    LE_ARG_OPTION_STRING_VAR,
    LE_ARG_OPTION_STRING_CALLBACK
}
OptionType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Option record.  Used to store an option's details in the option list.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t   link;       ///< Used to link the object into the list.
    const char* shortNamePtr;   ///< Ptr to short name (or NULL).
    const char* longNamePtr;    ///< Ptr to long name (or NULL).
    OptionType_t    type;       ///< The type of option (determines how to treat the destPtr).
    void* destPtr;              ///< Ptr to the variable set or function to call when option found.
}
OptionRec_t;


//--------------------------------------------------------------------------------------------------
/**
 * Positional callback record.  Used to store a positional callback function in the positional
 * callback list.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t   link;               ///< Used to link the object into the list.
    le_arg_StringCallbackFunc_t func;   ///< Function address.
}
PositionalCallbackRec_t;

//--------------------------------------------------------------------------------------------------
/**
 * Argument handler info structure.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    /// Pointer to the function that is to be called when an error is encountered in le_arg_Scan().
    le_arg_ErrorHandlerFunc_t ErrorHandler;
    /// Our copy of argc.
    size_t Argc;
    /// Our pointer to argv.
    const char** Argv;
    /// Option list.
    le_sls_List_t OptionList;
    /// Positional callback list.
    le_sls_List_t PositionalCallbackList;
    /// true = More positional arguments are allowed than there are positional callbacks in the
    ///        positional callback list, in which case the last positional callback in the list will
    ///        be called multiple times.
    bool IsMorePositionalArgsThanCallbacksAllowed;
    /// true = Less positional arguments are allowed than there are positional callbacks in the
    ///        positional callback list, in which case the last positional callbacks (the ones for
    ///        which there are not args) will not be called.
    bool IsLessPositionalArgsThanCallbacksAllowed;
    /// true = All positional callbacks have been called at least once.
    ///
    /// @note    Initialized to true because there are initially no callbacks.  Will be set to false
    ///         when a callback is added to the list.
    bool AllPositionalCallbacksHaveBeenCalled;
    /// Result of scanning.
    le_result_t ScanError;
    /// Determines if exit(EXIT_FAILURE) is called on error.
    unsigned int ExitBehaviour;
} ArgInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Argument info memory pool.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ArgInfoPoolRef;
LE_MEM_DEFINE_STATIC_POOL(ArgInfoPool, MAX_ARG_INFOS, sizeof(ArgInfo_t));

//--------------------------------------------------------------------------------------------------
/**
 * Command line options memory pool.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t OptionRecPoolRef;
LE_MEM_DEFINE_STATIC_POOL(OptionRecPool, LE_CONFIG_MAX_ARG_OPTIONS, sizeof(OptionRec_t));

//--------------------------------------------------------------------------------------------------
/**
 * Positional parameter callback pool.
 **/
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PositionalCallbackRecPoolRef;
LE_MEM_DEFINE_STATIC_POOL(PositionalCallbackRecPool, LE_CONFIG_MAX_ARG_POSITIONAL_CALLBACKS,
    sizeof(PositionalCallbackRec_t));

//--------------------------------------------------------------------------------------------------
/**
 * Default error handler function.
 **/
//--------------------------------------------------------------------------------------------------
static size_t DefaultErrorHandler
(
    size_t  argIndex,       ///< Index of argument that is bad (0 = first arg after program name).
    le_result_t errorCode   ///< Code indicating the type of error that was encountered.
)
{
    const char* errorString;

    switch (errorCode)
    {
        case LE_BAD_PARAMETER:

            errorString = "Unexpected argument";
            break;

        case LE_NOT_FOUND:

            errorString = "Argument value missing";
            break;

        case LE_FORMAT_ERROR:

            errorString = "Numerical argument value expected";
            break;

        case LE_OUT_OF_RANGE:

            errorString = "Numerical argument magnitude too large";
            break;

        case LE_OVERFLOW:

            errorString = "Too many arguments";
            break;

        case LE_UNDERFLOW:

            errorString = "Too few arguments";
            break;

        case LE_UNSUPPORTED:

            errorString = "Argument should not have a value.";
            break;

        default:

            LE_CRIT("Unexpected error code %d at argument index %" PRIuS ".", errorCode, argIndex);
            errorString = "Internal fault: Unexpected error";
            break;
    }

    const char* programNamePtr = le_arg_GetProgramName();

    fprintf(stderr, "* %s: at argument %" PRIuS ": %s.\n", programNamePtr, argIndex + 1,
        errorString);
    fprintf(stderr, "Try '%s --help'.\n", programNamePtr);

    return 0;
}


#if !USE_THREAD_LOCAL_STORAGE
//--------------------------------------------------------------------------------------------------
/**
 * State of argument processing
 */
//--------------------------------------------------------------------------------------------------
static ArgInfo_t ArgInfo =
{
    .ErrorHandler = DefaultErrorHandler,
    .Argc = 0,
    .Argv = NULL,
    .OptionList = LE_SLS_LIST_INIT,
    .PositionalCallbackList = LE_SLS_LIST_INIT,
    .IsMorePositionalArgsThanCallbacksAllowed = false,
    .IsLessPositionalArgsThanCallbacksAllowed = false,
    .AllPositionalCallbacksHaveBeenCalled = true,
    .ScanError = LE_OK,
    .ExitBehaviour = DEFAULT_EXIT_ON_ERROR
};

#define ResetArgInfo(argInfo)   (void)(argInfo)

/**
 * le_args state accessor
 */
#define LE_ARG_INFO_PTR   (&ArgInfo)
#else
//--------------------------------------------------------------------------------------------------
/**
 * Key for thread local storage of argument info
 */
static pthread_key_t ArgInfoKey;

/**
 * le_args state accessor
 */
#define LE_ARG_INFO_PTR    (((ArgInfo_t*)pthread_getspecific(ArgInfoKey)))

//--------------------------------------------------------------------------------------------------
/**
 *  Reset an ArgInfo structure for this thread.  Used for CLI commands under RTOS, which can be run
 *  repeatedly in the same CLI thread.
 */
//--------------------------------------------------------------------------------------------------
static void ResetArgInfo
(
    ArgInfo_t* argInfoPtr
)
{
    le_sls_Link_t           *linkPtr;
    OptionRec_t             *optionPtr;
    PositionalCallbackRec_t *positionalPtr;

    argInfoPtr->ErrorHandler = &DefaultErrorHandler;
    argInfoPtr->Argc = 0;
    argInfoPtr->Argv = NULL;
    argInfoPtr->IsMorePositionalArgsThanCallbacksAllowed = false;
    argInfoPtr->IsLessPositionalArgsThanCallbacksAllowed = false;
    argInfoPtr->AllPositionalCallbacksHaveBeenCalled = true;
    argInfoPtr->ScanError = LE_OK;
    argInfoPtr->ExitBehaviour = DEFAULT_EXIT_ON_ERROR;

    while ((linkPtr = le_sls_Pop(&argInfoPtr->OptionList)) != NULL)
    {
        optionPtr = CONTAINER_OF(linkPtr, OptionRec_t, link);
        le_mem_Release(optionPtr);
    }
    while ((linkPtr = le_sls_Pop(&argInfoPtr->PositionalCallbackList)) != NULL)
    {
        positionalPtr = CONTAINER_OF(linkPtr, PositionalCallbackRec_t, link);
        le_mem_Release(positionalPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize a new ArgInfo_t structure
 */
//--------------------------------------------------------------------------------------------------
void InitArgInfo
(
    ArgInfo_t* argInfoPtr
)
{
    argInfoPtr->OptionList = LE_SLS_LIST_INIT;
    argInfoPtr->PositionalCallbackList = LE_SLS_LIST_INIT;
    ResetArgInfo(argInfoPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an ArgInfo structure for this thread.
 */
//--------------------------------------------------------------------------------------------------
static ArgInfo_t* CreateArgInfo
(
    void
)
{
    ArgInfo_t *argInfoPtr = le_mem_Alloc(ArgInfoPoolRef);

    InitArgInfo(argInfoPtr);

    pthread_setspecific(ArgInfoKey, argInfoPtr);

    return argInfoPtr;
}

#endif




//--------------------------------------------------------------------------------------------------
/**
 * Searches the Argv array for a given option.
 *
 * @return  The index into the Argv array of the option, or -1 if not found.
 **/
//--------------------------------------------------------------------------------------------------
static ssize_t FindOption
(
    const char* shortName,  ///< Pointer to the short version of the option name (can be NULL).
    const char* longName    ///< Pointer to the long version of the option name (can be NULL).
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;
    size_t longNameLen = 0;

    if (longName != NULL)
    {
        longNameLen = strlen(longName);
    }

    if (!argInfoPtr)
    {
        return -1;
    }

    size_t i;

    for (i = 0; i < argInfoPtr->Argc; i++)
    {
        const char* arg = argInfoPtr->Argv[i];

        if (arg[0] == '-')  // Could be an option.
        {
            if (arg[1] == '-')  // Could be a long name option.
            {
                if (arg[2] != '\0') // Definitely a long name option.
                {
                    // If a long name was provided, and the part after the "--" matches it,
                    // and the option name isn't longer than the long name given, then this is
                    // what we are looking for.
                    if (   (longName != NULL)
                        && (strncmp(arg + 2, longName, longNameLen) == 0)
                        && (   (arg[2 + longNameLen] == '\0')
                            || (arg[2 + longNameLen] == '=')   )   )
                    {
                        return i;
                    }
                }
            }
            else if (arg[1] != '\0') // Definitely a short name option.
            {
                // If a short name was provided, and the part after the "-" matches it,
                // then this is what we are looking for.
                if (   (shortName != NULL)
                    && (strcmp(arg + 1, shortName) == 0)    )
                {
                    return i;
                }
            }
        }
    }

    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the value of an option argument at a given index.
 *
 * @return  A pointer to the null-terminated string, or NULL on error.
 **/
//--------------------------------------------------------------------------------------------------
static const char* GetOptionValue
(
    size_t index    ///< Index into the Argc array of the option argument.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        return NULL;
    }

    if (argInfoPtr->Argv[index][1] == '-')  // long name - value is after '='.
    {
        const char* valuePtr = strchr(argInfoPtr->Argv[index], '=');

        if (valuePtr == NULL)
        {
            return NULL;
        }
        else
        {
            return valuePtr + 1;
        }
    }
    else // short name - value is in next arg.
    {
        if ((index + 1) >= argInfoPtr->Argc)
        {
            // The option was provided but the argument list ran out before a value was found.
            return NULL;
        }
        else
        {
            return argInfoPtr->Argv[index + 1];
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether  a positional argument at a given index in the Argv array is integer or floating
 * point number.
 *
 * @return  True if it is a valid number, false otherwise.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsArgNumber
(
    size_t i    ///< The index of the argument in the Argv array.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;
    char *endptr;
    // Need to store value to quiet GCC on Ubuntu 12.04
    __attribute__((unused)) long double value;

    if (!argInfoPtr)
    {
        return false;
    }

    errno = 0;
    value = strtold(argInfoPtr->Argv[i], &endptr);

    // This is a number if and only if:
    //   - A conversion is performed
    //   - Which consumes the entire string
    //   - And the result is not out of range of a long double.
    return  ((endptr != argInfoPtr->Argv[i])
             && (*endptr == '\0')
             && (errno != ERANGE));
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new option record and adds it to the option list.
 **/
//--------------------------------------------------------------------------------------------------
static void CreateOptionRec
(
    const char* shortNamePtr,   ///< Ptr to short name (or NULL).
    const char* longNamePtr,    ///< Ptr to long name (or NULL).
    OptionType_t    type,       ///< The type of option (determines how to treat the destPtr).
    void* destPtr               ///< Ptr to the variable set or function to call when option found.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;
    LE_FATAL_IF(!argInfoPtr, "No arguments available");
    OptionRec_t *recPtr = le_mem_Alloc(OptionRecPoolRef);
    LE_ASSERT(recPtr != NULL);

    recPtr->link = LE_SLS_LINK_INIT;
    recPtr->shortNamePtr = shortNamePtr;
    recPtr->longNamePtr = longNamePtr;

    recPtr->type = type;
    recPtr->destPtr = destPtr;

    le_sls_Queue(&argInfoPtr->OptionList, &recPtr->link);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the error flag while handling an error condition.
 **/
//--------------------------------------------------------------------------------------------------
static inline size_t HandleError
(
    ArgInfo_t   *argInfoPtr,    ///< Argument info structure.
    size_t       argIndex,      ///< Index of argument that is bad.
    le_result_t  errorCode      ///< Code indicating the type of error that was encountered.
)
{
    size_t result = argInfoPtr->ErrorHandler(argIndex, errorCode);
    argInfoPtr->ScanError = errorCode;
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handle a positional argument at a given index in the Argv array.
 *
 * @return  The number of arguments that le_arg_Scan() should skip over following this one.
 **/
//--------------------------------------------------------------------------------------------------
static size_t HandlePositionalArgument
(
    size_t i    ///< The index of the argument in the Argv array.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        // No arguments is always an underflow when trying to fetch the next argument
        return LE_UNDERFLOW;
    }

    // Pop the first positional callback from the list.
    le_sls_Link_t* linkPtr = le_sls_Pop(&argInfoPtr->PositionalCallbackList);

    // If wasn't anything on the list, then there are too many positional arguments.
    if (linkPtr == NULL)
    {
        return HandleError(argInfoPtr, i - 1, LE_OVERFLOW);
    }
    else
    {
        PositionalCallbackRec_t* recPtr = CONTAINER_OF(linkPtr, PositionalCallbackRec_t, link);

        // Call the positional callback.
        recPtr->func(argInfoPtr->Argv[i]);

        // If this was the last callback on the list,
        if (le_sls_IsEmpty(&argInfoPtr->PositionalCallbackList))
        {
            argInfoPtr->AllPositionalCallbacksHaveBeenCalled = true;

            // If there are allowed to be more positional arguments than positional callbacks,
            // then add this callback back onto the list in case we encounter more positional
            // arguments.
            if (argInfoPtr->IsMorePositionalArgsThanCallbacksAllowed)
            {
                le_sls_Queue(&argInfoPtr->PositionalCallbackList, linkPtr);
            }
        }
        else
        {
            le_mem_Release(recPtr);
        }
    }

    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle a flag option.
 **/
//--------------------------------------------------------------------------------------------------
static void HandleFlagOption
(
    OptionRec_t* recPtr ///< Ptr to the option record for this option.
)
{
    if (recPtr->type == LE_ARG_OPTION_FLAG_VAR)
    {
        bool* varPtr = recPtr->destPtr;
        *varPtr = true;
    }
    else // must be LE_ARG_OPTION_FLAG_CALLBACK:
    {
        le_arg_FlagCallbackFunc_t func = recPtr->destPtr;
        func();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle an option with a value (a non-flag option).
 *
 * @return
 *  - LE_OK if successful.
 **/
//--------------------------------------------------------------------------------------------------
static le_result_t HandleOptionWithValue
(
    OptionRec_t* recPtr,    ///< Ptr to the option record for this option.
    const char* valueStr    ///< Ptr to the value string (null-terminated).
)
{
    le_result_t result = LE_FAULT;

    if (recPtr->type == LE_ARG_OPTION_INT_VAR)
    {
        int* destPtr = recPtr->destPtr;

        result = le_utf8_ParseInt(destPtr, valueStr);
    }
    else if (recPtr->type == LE_ARG_OPTION_INT_CALLBACK)
    {
        int value;

        result = le_utf8_ParseInt(&value, valueStr);

        if (result == LE_OK)
        {
            le_arg_IntCallbackFunc_t func = recPtr->destPtr;
            func(value);
        }
    }
    else if (recPtr->type == LE_ARG_OPTION_STRING_VAR)
    {
        const char** destPtr = recPtr->destPtr;
        *destPtr = valueStr;

        result = LE_OK;
    }
    else if (recPtr->type == LE_ARG_OPTION_STRING_CALLBACK)
    {
        le_arg_StringCallbackFunc_t func = recPtr->destPtr;
        func(valueStr);

        result = LE_OK;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle a short name option at a given index in the Argv array.
 *
 * @return  The number of arguments that le_arg_Scan() should skip over following this one.
 **/
//--------------------------------------------------------------------------------------------------
static size_t HandleShortOption
(
    size_t i    ///< The index of the argument in the Argv array.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    LE_ASSERT(argInfoPtr);

    // The option name starts after the leading '-'.
    const char* optionNamePtr = (argInfoPtr->Argv[i]) + 1;

    // Traverse the option list, looking for one that matches.
    le_sls_Link_t* linkPtr = le_sls_Peek(&argInfoPtr->OptionList);
    while (linkPtr != NULL)
    {
        OptionRec_t* recPtr = CONTAINER_OF(linkPtr, OptionRec_t, link);

        if ((recPtr->shortNamePtr != NULL) && (strcmp(recPtr->shortNamePtr, optionNamePtr) == 0))
        {
            // The option name is a match.  If this is a flag option, we don't expect a value.
            if (   (recPtr->type == LE_ARG_OPTION_FLAG_VAR)
                || (recPtr->type == LE_ARG_OPTION_FLAG_CALLBACK)   )
            {
                HandleFlagOption(recPtr);
                return 0;
            }
            else
            {
                // This is not a flag option, so we expect a value argument.
                if (argInfoPtr->Argc > (i + 1))
                {
                    le_result_t result = HandleOptionWithValue(recPtr, argInfoPtr->Argv[i + 1]);
                    if (result != LE_OK)
                    {
                        return HandleError(argInfoPtr, i - 1, result);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    return HandleError(argInfoPtr, i - 1, LE_NOT_FOUND);
                }
            }
        }

        linkPtr = le_sls_PeekNext(&argInfoPtr->OptionList, linkPtr);
    }

    // It doesn't match with any short option. So it may be a negative number.
    if (IsArgNumber(i))
    {
        // Only it can be a positional argument.
        return HandlePositionalArgument(i);
    }

    // Doesn't match with anything. Report an unexpected argument.
    return HandleError(argInfoPtr, i - 1, LE_BAD_PARAMETER);
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle a long name option at a given index in the Argv array.
 *
 * @return  The number of arguments that le_arg_Scan() should skip over following this one.
 **/
//--------------------------------------------------------------------------------------------------
static size_t HandleLongOption
(
    size_t i    ///< The index of the argument in the Argv array.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    LE_ASSERT(argInfoPtr);

    // The option name starts after the leading "--".
    const char* optionNamePtr = (argInfoPtr->Argv[i]) + 2;

    // Traverse the option list, looking for one that matches.
    le_sls_Link_t* linkPtr = le_sls_Peek(&argInfoPtr->OptionList);
    while (linkPtr != NULL)
    {
        OptionRec_t* recPtr = CONTAINER_OF(linkPtr, OptionRec_t, link);

        if (recPtr->longNamePtr != NULL)
        {
            size_t len = strlen(recPtr->longNamePtr);

            if (strncmp(optionNamePtr, recPtr->longNamePtr, len) == 0)
            {
                if (optionNamePtr[len] == '=')
                {
                    // The option is a match and has a value.
                    // Make sure a value is expected.
                    if (   (recPtr->type == LE_ARG_OPTION_FLAG_VAR)
                        || (recPtr->type == LE_ARG_OPTION_FLAG_CALLBACK)    )
                    {
                        return HandleError(argInfoPtr, i - 1, LE_UNSUPPORTED);
                    }
                    else
                    {
                        le_result_t result = HandleOptionWithValue(recPtr, optionNamePtr + len + 1);
                        if (result != LE_OK)
                        {
                            return HandleError(argInfoPtr, i - 1, result);
                        }
                        else
                        {
                            return 0;
                        }
                    }
                }
                else if (optionNamePtr[len] == '\0')
                {
                    // The option is a match and does not have a value.
                    // Make sure no value is expected.
                    if (   (recPtr->type == LE_ARG_OPTION_FLAG_VAR)
                        || (recPtr->type == LE_ARG_OPTION_FLAG_CALLBACK)    )
                    {
                        HandleFlagOption(recPtr);
                        return 0;
                    }
                    else
                    {
                        return HandleError(argInfoPtr, i - 1, LE_NOT_FOUND);
                    }
                }
            }
        }

        linkPtr = le_sls_PeekNext(&argInfoPtr->OptionList, linkPtr);
    }

    // Report an unexpected argument.
    return HandleError(argInfoPtr, i - 1, LE_BAD_PARAMETER);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the le_arg module
 */
//--------------------------------------------------------------------------------------------------
void arg_Init
(
    void
)
{
#if USE_THREAD_LOCAL_STORAGE
    LE_FATAL_IF(pthread_key_create(&ArgInfoKey, NULL) != 0,
                "Failed to create thread local storage for argument info");
#endif

    ArgInfoPoolRef = le_mem_InitStaticPool(ArgInfoPool, MAX_ARG_INFOS, sizeof(ArgInfo_t));
    OptionRecPoolRef = le_mem_InitStaticPool(OptionRecPool, LE_CONFIG_MAX_ARG_OPTIONS,
        sizeof(OptionRec_t));
    PositionalCallbackRecPoolRef = le_mem_InitStaticPool(PositionalCallbackRecPool,
        LE_CONFIG_MAX_ARG_POSITIONAL_CALLBACKS, sizeof(PositionalCallbackRec_t));
}

//--------------------------------------------------------------------------------------------------
/**
 * Release the argument info (if any) for the current thread.
 */
//--------------------------------------------------------------------------------------------------
void arg_DestructThread
(
    void
)
{
#if USE_THREAD_LOCAL_STORAGE
    ArgInfo_t *argInfoPtr = ((ArgInfo_t *) pthread_getspecific(ArgInfoKey));
    if (argInfoPtr != NULL)
    {
        ResetArgInfo(argInfoPtr);
        le_mem_Release(argInfoPtr);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Gets the program name.
 *
 * @return  Pointer to the null-terminated program name string.
 */
//--------------------------------------------------------------------------------------------------
const char* le_arg_GetProgramName
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    // WARNING: This function gets called by the logging API, so don't use the logging API.
    //          Otherwise a stack overflow may occur.

    // This will be true if le_arg_SetArgs() wasn't called or if argc was really 0.
    if (!argInfoPtr || (argInfoPtr->Argc == 0))
    {
        return "_UNKNOWN_";
    }

    return le_path_GetBasenamePtr(argInfoPtr->Argv[0], "/");
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the number of command line arguments available not including the program name.
 *
 * @return
 *      The number of command line arguments available.
 */
//--------------------------------------------------------------------------------------------------
size_t le_arg_NumArgs
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr || (argInfoPtr->Argc == 0))
    {
        return 0;
    }

    return argInfoPtr->Argc - 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a command line argument by index.
 *
 * @return Pointer to the argument string (null-terminated), or NULL if the index is out of range.
 */
//--------------------------------------------------------------------------------------------------
const char* le_arg_GetArg
(
    size_t  argIndex    ///< [IN] Index of the argument (0 = first argument after the program name).
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if ( !argInfoPtr || (argIndex >= argInfoPtr->Argc - 1) || (argInfoPtr->Argc == 0) )
    {
        return NULL;
    }

    return argInfoPtr->Argv[argIndex + 1];
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the argument list for a flag option.  Can search for a short name (e.g., <c>-f</c>)
 * or a long name (e.g., <c>--flag</c>) for the same flag at the same time.
 *
 * @return
 *  - LE_OK if found,
 *  - LE_NOT_FOUND if not found,
 *  - LE_FORMAT_ERROR if found but has a value (e.g., <c>--flag=foo</c>).
 *
 * @note    If both shortName and longName are NULL, LE_NOT_FOUND will be returned.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetFlagOption
(
    const char* shortName,  ///< [IN] Name that appears after a single '-' (can be NULL).
    const char* longName    ///< [IN] Name that appears afer a "--" (can be NULL).
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        return LE_NOT_FOUND;
    }

    ssize_t index = FindOption(shortName, longName);

    if (index < 0)
    {
        return LE_NOT_FOUND;
    }
    else if (strchr(argInfoPtr->Argv[index], '=') != NULL)
    {
        return LE_FORMAT_ERROR;
    }
    else
    {
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the argument list for an option with an integer value.  Can search for a short name
 * (e.g., <c>-c 1234</c>) or a long name (e.g., <c>--count=1234</c>) form of the same option at the
 * same time.
 *
 * @return
 *  - LE_OK if found and successfully converted to an integer.
 *  - LE_NOT_FOUND if not found.
 *  - LE_FORMAT_ERROR if the option wasn't provided with an integer value.
 *  - LE_OUT_OF_RANGE - Magnitude of integer value too big to be stored in an int.
 *
 * @note    If both shortName and longName are NULL, LE_NOT_FOUND will be returned.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetIntOption
(
    int* varPtr,            ///< [OUT] Variable into which the value will be stored if found.
    const char* shortName,  ///< [IN] Name that appears after a single '-' (can be NULL).
    const char* longName    ///< [IN] Name that appears afer a "--" (can be NULL).
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        return LE_NOT_FOUND;
    }

    ssize_t index = FindOption(shortName, longName);

    if (index < 0)
    {
        return LE_NOT_FOUND;
    }

    const char* valuePtr = GetOptionValue(index);
    if (valuePtr == NULL)
    {
        return LE_FORMAT_ERROR;
    }

    return le_utf8_ParseInt(varPtr, valuePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches the argument list for an option with a string value.  Can search for a short name
 * (e.g., <c>-f foo</c>) or a long name (e.g., <c>--file=foo</c>) form of the same option at the
 * same time.
 *
 * @note <c>--file=</c> is a valid string option with an empty string ("") value.  The equivalent
 *       short name version of that option would be something like <c>-f ""</c>.
 *
 * @return
 *  - LE_OK if found.
 *  - LE_NOT_FOUND if not found.
 *  - LE_FORMAT_ERROR if the option wasn't provided with a value.
 *
 * @note    If both shortName and longName are NULL, LE_NOT_FOUND will be returned.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetStringOption
(
    const char** varPtr,    ///< [OUT] Variable into which to store a pointer to the value if found.
    const char* shortName,  ///< [IN] Name that appears after a single '-' (can be NULL).
    const char* longName    ///< [IN] Name that appears afer a "--" (can be NULL).
)
{
    ssize_t index = FindOption(shortName, longName);

    if (index < 0)
    {
        return LE_NOT_FOUND;
    }

    const char* valuePtr = GetOptionValue(index);

    if (valuePtr == NULL)
    {
        return LE_FORMAT_ERROR;
    }
    else
    {
        *varPtr = valuePtr;
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a boolean variable to be set if a given flag option appears on the argument list.
 *
 * No value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetFlagVar
(
    bool* varPtr,           ///< Ptr to the variable to be set true if the flag option is found.
    const char* shortName,  ///< Short form of option name (e.g., "h" will match "-h").
    const char* longName    ///< Long form of option name (e.g., "help" will match "--help").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_FLAG_VAR, varPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register an integer variable to be set if a given option appears on the argument list.
 *
 * An integer value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetIntVar
(
    int* varPtr,            ///< Ptr to the variable to be set if the option is found.
    const char* shortName,  ///< Short form of option name (e.g., "n" will match "-n 1234").
    const char* longName    ///< Long form of name ("max-count" matches "--max-count=1234").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_INT_VAR, varPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a string variable to be set if a given option appears on the argument list.
 *
 * A value is expected after the option name.
 *
 * @code
 * const char* namePtr = "default";
 * le_arg_SetStringVar(&namePtr, "n", "name");
 * @endcode
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetStringVar
(
    const char** varPtr,    ///< Ptr to the variable to be set if the option is found.
    const char* shortName,  ///< Short form of option name (e.g., "n" will match "-n foo").
    const char* longName    ///< Long form of name ("name" matches "--name=foo").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_STRING_VAR, varPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given flag option appears on the argument list.
 *
 * No value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetFlagCallback
(
    le_arg_FlagCallbackFunc_t func, ///< The callback function address.
    const char* shortName,          ///< Short form of option name (e.g., "h" will match "-h").
    const char* longName           ///< Long form of option name (e.g., "help" will match "--help").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_FLAG_CALLBACK, func);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given integer value option appears on the
 * argument list.
 *
 * An integer value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetIntCallback
(
    le_arg_IntCallbackFunc_t func,  ///< The callback function address.
    const char* shortName,          ///< Short form of option name (e.g., "n" will match "-n 1234").
    const char* longName            ///< Long form of name ("max-count" matches "--max-count=1234").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_INT_CALLBACK, func);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given string option appears on the argument list.
 *
 * A string value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetStringCallback
(
    le_arg_StringCallbackFunc_t func,   ///< The callback function address.
    const char* shortName,          ///< Short form of option name (e.g., "n" will match "-n foo").
    const char* longName            ///< Long form of name ("name" matches "--name=foo").
)
{
    CreateOptionRec(shortName, longName, LE_ARG_OPTION_STRING_CALLBACK, func);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if an argument appears outside of any options.
 * For example, in the following command-line, "foo" and "bar" are positional arguments (while "-l"
 * is a flag option and "ls" is the program name):
 *
 * @code
 * $ ls -l foo bar
 * @endcode
 *
 * Each callback function registered using this method is added to the positional callback list.
 * When the first positional argument is encountered, the first positional callback function is
 * called.  When the Nth positional argument is encountered, the Nth positional callback
 * is called.  If there are N positional arguments and M positional callbacks, and N > M, then
 * the last positional callback will be called once for each positional argument from M through N,
 * inclusive.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_AddPositionalCallback
(
    le_arg_StringCallbackFunc_t func    ///< The callback function address.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        // No arguments to process, so no need to add callback
        return;
    }

    PositionalCallbackRec_t* recPtr = NULL;

    if (argInfoPtr->AllPositionalCallbacksHaveBeenCalled)
    {
        argInfoPtr->AllPositionalCallbacksHaveBeenCalled = false;

        // If more args than callbacks are allowed, then we need to discard the callback that
        // is currently on the callback list (if any), since we were only saving it in case more
        // positional args were found.
        if (argInfoPtr->IsMorePositionalArgsThanCallbacksAllowed)
        {
            le_sls_Link_t* linkPtr = le_sls_Pop(&argInfoPtr->PositionalCallbackList);

            if (linkPtr != NULL)
            {
                recPtr = CONTAINER_OF(linkPtr, PositionalCallbackRec_t, link);
            }
        }
    }

    // If we popped a callback record from the list, reuse it.  Otherwise, allocate a new one.
    if (recPtr == NULL)
    {
        recPtr = le_mem_Alloc(PositionalCallbackRecPoolRef);
        LE_ASSERT(recPtr != NULL);
        recPtr->link = LE_SLS_LINK_INIT;
    }

    recPtr->func = func;

    le_sls_Queue(&argInfoPtr->PositionalCallbackList, &recPtr->link);
}


//--------------------------------------------------------------------------------------------------
/**
 * Tell the argument parser to allow more positional arguments than positional callbacks.
 *
 * If more positional arguments are encountered than the number of positional callbacks when this
 * is allowed, le_arg_Scan() will call the last positional callback again for each extra positional
 * argument it finds.  If this is not allowed, le_arg_Scan() will print an error message to the
 * standard error stream and exit the process with EXIT_FAILURE if there are more positional
 * arguments than there are positional callbacks.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_AllowMorePositionalArgsThanCallbacks
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    // Only set if there are actually arguments to process
    if (argInfoPtr)
    {
        argInfoPtr->IsMorePositionalArgsThanCallbacksAllowed = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Tell the argument parser to allow less positional arguments than positional callbacks.
 *
 * If less positional arguments are encountered than the number of positional callbacks when this
 * is allowed, any positional callbacks that don't have arguments won't be called.  If this is not
 * allowed, le_arg_Scan() will print an error message to the standard error stream and exit the
 * process with EXIT_FAILURE if there are less positional arguments than there are positional
 * callbacks.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_AllowLessPositionalArgsThanCallbacks
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    // Only set if there are arguments to process
    if (argInfoPtr)
    {
        argInfoPtr->IsLessPositionalArgsThanCallbacksAllowed = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Register an error handler function to be called by le_arg_Scan() whenever an unexpected argument
 * is encountered or an option's value cannot be converted to the correct data type.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetErrorHandler
(
    le_arg_ErrorHandlerFunc_t errorHandlerFunc  ///< [IN] The error handler function.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    // Only set if there are arguments to process
    if (argInfoPtr)
    {
        argInfoPtr->ErrorHandler = errorHandlerFunc;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Control whether an argument scanning error should cause the process to exit.
 *
 *  The default behaviour is to exit.  If this is set to false then le_arg_GetScanResult() should be
 *  called after le_arg_Scan() to determine if an error occured.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetExitOnError
(
    bool exitOnError    ///< [IN] Exit on scan error.
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    // Only set if there are arguments to process
    if (argInfoPtr)
    {
        argInfoPtr->ExitBehaviour = (exitOnError ? EXPLICIT_EXIT_ON_ERROR : 0);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Determine if argument scanning failed.
 *
 *  If process termination is disabled via le_arg_SetExitOnError(), then this function may be used
 *  to get the result of argument scanning (le_arg_Scan()).
 *
 *  @return The result of the last argument scan.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_GetScanResult
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        // Treat scanning as failed if there are no arguments
        return LE_NOT_FOUND;
    }
    else
    {
        return argInfoPtr->ScanError;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Scans the argument list, setting variables and calling callbacks registered using the
 * le_arg_SetXxxVar(), le_arg_SetXxxCallback(), and le_arg_AddPositionalParameters() functions.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_Scan
(
    void
)
{
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;

    if (!argInfoPtr)
    {
        // No argument sot scan; just return
        return;
    }

    LE_DEBUG("Argc = %" PRIuS, argInfoPtr->Argc);

    size_t i;
    for (i = 1; i < argInfoPtr->Argc; i++)  // Start at 1, because 0 is the program name.
    {
        LE_DEBUG("Argv[%" PRIuS "] = '%s'", i, argInfoPtr->Argv[i]);

        // If it doesn't start with a '-', then it must be a positional argument.
        if (argInfoPtr->Argv[i][0] != '-')
        {
            i += HandlePositionalArgument(i);
        }
        // If it does start with a '-', but not a "--",
        else if (argInfoPtr->Argv[i][1] != '-')
        {
            // It's either just a '-' or it is a short name option.
            if (argInfoPtr->Argv[i][1] == '\0')
            {
                // Just a '-' gets treated as a positional argument.
                i += HandlePositionalArgument(i);
            }
            else
            {
                i += HandleShortOption(i);
            }
        }
        // If it starts with "--",
        else
        {
            // If it just a "--", then handle it as a positional argument.
            if (argInfoPtr->Argv[i][2] == '\0')
            {
                i += HandlePositionalArgument(i);
            }
            else
            {
                i += HandleLongOption(i);
            }
        }

        if (argInfoPtr->ScanError != LE_OK)
        {
            goto end;
        }
    }

    // If we are not allowed to have less positional arguments than positional callbacks,
    // and not all callbacks have been called at least once, then it's an error.
    if (   (argInfoPtr->IsLessPositionalArgsThanCallbacksAllowed == false)
        && (argInfoPtr->AllPositionalCallbacksHaveBeenCalled == false)  )
    {
        // Note: Ignore return code. Can't skip args, because there are none left.
        (void)HandleError(argInfoPtr, i - 1, LE_UNDERFLOW);
    }

end:
    if (argInfoPtr->ScanError != LE_OK)
    {
        if ((argInfoPtr->ExitBehaviour & EXPLICIT_EXIT_ON_ERROR) ||
            ((argInfoPtr->ExitBehaviour & DEFAULT_EXIT_ON_ERROR) &&
                (argInfoPtr->ErrorHandler == &DefaultErrorHandler)))
        {
            exit(EXIT_FAILURE);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets argc and argv for later use.  This function must be called by Legato's generated main
 * function.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetArgs
(
    const size_t    argc,   ///< [IN] argc from main.
    const char**    argv    ///< [IN] argv from main.
)
{
#if USE_THREAD_LOCAL_STORAGE
    ArgInfo_t* argInfoPtr = ((ArgInfo_t*)pthread_getspecific(ArgInfoKey));
    if (!argInfoPtr)
    {
        argInfoPtr = CreateArgInfo();
    }
#else
    ArgInfo_t* argInfoPtr = LE_ARG_INFO_PTR;
#endif

    ResetArgInfo(argInfoPtr);
    argInfoPtr->Argc = argc;
    argInfoPtr->Argv = argv;
}

//--------------------------------------------------------------------------------------------------
/**
 * Collect one quoted argument from the character stream.
 *
 * @return Pointer to NUL terminator of captured argument.
 */
//--------------------------------------------------------------------------------------------------
static char *CollectQuotedArg
(
    char         *c,        ///< [IN]  Character stream.
    const char  **argPtr    ///< [OUT] Argument entry to point at the captured argument.
)
{
    char quote = *c;

    // Argument starts after quote
    ++c;
    *argPtr = c;

    // Proceed until there is a closing quote or the string ends
    while (*c != '\0' && *c != quote)
    {
        ++c;
    }

    // Terminate the argument.
    *c = '\0';
    return c;
}

//--------------------------------------------------------------------------------------------------
/**
 * Collect one whitespace delimited argument from the character stream.
 *
 * @return Pointer to NUL terminator of captured argument.
 */
//--------------------------------------------------------------------------------------------------
static char *CollectArg
(
    char         *c,        ///< [IN]  Character stream.
    const char  **argPtr    ///< [OUT] Argument entry to point at the captured argument.
)
{
    // Argument starts right away
    *argPtr = c;

    // Proceed until there is whitespace or the string ends
    while (*c != '\0' && !isspace(*c))
    {
        ++c;
    }

    // Terminate the argument.
    *c = '\0';
    return c;
}

//--------------------------------------------------------------------------------------------------
/**
 * Tokenizes a string into individual arguments.  Simple quoting is allowed using either ' or " to
 * enclose multi-word arguments.
 *
 * @return
 *      - LE_OK on success.
 *      - LE_BAD_PARAMETER if a parameter is invalid.
 *      - LE_OUT_OF_RANGE if more arguments are present than can be captured in the provided array
 *        (those that can be captured will be).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_arg_Split
(
    const char   *firstStr,     ///< [IN]     A separate string to treat as the first argument (for
                                ///<          example, the program name).  May be NULL.
    char         *cmdlinePtr,   ///< [IN,OUT] Command line argument string to split.  This string
                                ///<          will be modified in-place.
    int          *argc,         ///< [IN,OUT] As input, the size of the argv array.  As output, the
                                ///<          number of arguments obtained.
    const char  **argv          ///< [OUT]    The tokenized arguments.  These will be pointers into
                                ///<          the firstStr and modified cmdlinePtr buffers.
)
{
    char        *c;
    int          i = 0;
    le_result_t  result = LE_OK;

    if (cmdlinePtr == NULL || argc == NULL || *argc < 2 || argv == NULL)
    {
        return LE_BAD_PARAMETER;
    }
    memset(argv, 0, *argc * sizeof(*argv));

    if (firstStr != NULL)
    {
        argv[i] = firstStr;
        ++i;
    }

    for (c = cmdlinePtr; *c != '\0'; ++c)
    {
        if (!isspace(*c))
        {
            if (i >= *argc - 1)
            {
                result = LE_OUT_OF_RANGE;
                break;
            }

            if (*c == '\'' || *c == '"')
            {
                c = CollectQuotedArg(c, &argv[i]);
            }
            else
            {
                c = CollectArg(c, &argv[i]);
            }
            ++i;
        }
    }

    *argc = i;
    return result;
}
