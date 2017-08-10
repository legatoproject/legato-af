//--------------------------------------------------------------------------------------------------
/** @file args.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Default error handler function.
 *
 * @return Never returns.
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

            LE_CRIT("Unexpected error code %d at argument index %zu.", errorCode, argIndex);
            errorString = "Internal fault: Unexpected error";
            break;
    }

    const char* programNamePtr = le_arg_GetProgramName();

    fprintf(stderr, "* %s: at argument %zu: %s.\n", programNamePtr, argIndex + 1, errorString);
    fprintf(stderr, "Try '%s --help'.\n", programNamePtr);

    exit(EXIT_FAILURE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the function that is to be called when an error is encountered in le_arg_Scan().
 **/
//--------------------------------------------------------------------------------------------------
static le_arg_ErrorHandlerFunc_t ErrorHandler = DefaultErrorHandler;



//--------------------------------------------------------------------------------------------------
/**
 * Our copy of argc.
 */
//--------------------------------------------------------------------------------------------------
static size_t Argc = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Our pointer to argv.
 */
//--------------------------------------------------------------------------------------------------
static char** Argv = NULL;


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
    size_t longNameLen = 0;

    if (longName != NULL)
    {
        longNameLen = strlen(longName);
    }

    size_t i;

    for (i = 0; i < Argc; i++)
    {
        const char* arg = Argv[i];

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
    if (Argv[index][1] == '-')  // long name - value is after '='.
    {
        const char* valuePtr = strchr(Argv[index], '=');

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
        if ((index + 1) >= Argc)
        {
            // The option was provided but the argument list ran out before a value was found.
            return NULL;
        }
        else
        {
            return Argv[index + 1];
        }
    }
}


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
 * Option list.
 **/
//--------------------------------------------------------------------------------------------------
static le_sls_List_t OptionList = LE_SLS_LIST_INIT;


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
 * Positional callback list.
 **/
//--------------------------------------------------------------------------------------------------
static le_sls_List_t PositionalCallbackList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * true = More positional arguments are allowed than there are positional callbacks in the
 *        positional callback list, in which case the last positional callback in the list will
 *        be called multiple times.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsMorePositionalArgsThanCallbacksAllowed = false;


//--------------------------------------------------------------------------------------------------
/**
 * true = Less positional arguments are allowed than there are positional callbacks in the
 *        positional callback list, in which case the last positional callbacks (the ones for which
 *        there are not args) will not be called.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsLessPositionalArgsThanCallbacksAllowed = false;


//--------------------------------------------------------------------------------------------------
/**
 * true = All positional callbacks have been called at least once.
 *
 * @note    Initialized to true because there are initially no callbacks.  Will be set to false
 *          when a callback is added to the list.
 **/
//--------------------------------------------------------------------------------------------------
static bool AllPositionalCallbacksHaveBeenCalled = true;


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
    char *endptr;
    // Need to store value to quiet GCC on Ubuntu 12.04
    __attribute__((unused)) long double value;

    errno = 0;
    value = strtold(Argv[i], &endptr);

    // This is a number if and only if:
    //   - A conversion is performed
    //   - Which consumes the entire string
    //   - And the result is not out of range of a long double.
    return  ((endptr != Argv[i])
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
    OptionRec_t* recPtr = malloc(sizeof(OptionRec_t));
    LE_ASSERT(recPtr != NULL);

    recPtr->link = LE_SLS_LINK_INIT;
    recPtr->shortNamePtr = shortNamePtr;
    recPtr->longNamePtr = longNamePtr;

    recPtr->type = type;
    recPtr->destPtr = destPtr;

    le_sls_Queue(&OptionList, &recPtr->link);
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
    // Pop the first positional callback from the list.
    le_sls_Link_t* linkPtr = le_sls_Pop(&PositionalCallbackList);

    // If wasn't anything on the list, then there are too many positional arguments.
    if (linkPtr == NULL)
    {
        return ErrorHandler(i - 1, LE_OVERFLOW);
    }
    else
    {
        PositionalCallbackRec_t* recPtr = CONTAINER_OF(linkPtr, PositionalCallbackRec_t, link);

        // Call the positional callback.
        recPtr->func(Argv[i]);

        // If this was the last callback on the list,
        if (le_sls_IsEmpty(&PositionalCallbackList))
        {
            AllPositionalCallbacksHaveBeenCalled = true;

            // If there are allowed to be more positional arguments than positional callbacks,
            // then add this callback back onto the list in case we encounter more positional
            // arguments.
            if (IsMorePositionalArgsThanCallbacksAllowed)
            {
                le_sls_Queue(&PositionalCallbackList, linkPtr);
            }
        }
        else
        {
            // Note: Free is used here only because we expect argument processing to
            // occur only at start-up, and therefore we can be certain that we won't be creating
            // a long-running fragmentation problem that slowly eats all memory.
            free(recPtr);
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
    // The option name starts after the leading '-'.
    const char* optionNamePtr = (Argv[i]) + 1;

    // Traverse the option list, looking for one that matches.
    le_sls_Link_t* linkPtr = le_sls_Peek(&OptionList);
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
                if (Argc > (i + 1))
                {
                    le_result_t result = HandleOptionWithValue(recPtr, Argv[i + 1]);
                    if (result != LE_OK)
                    {
                        return ErrorHandler(i - 1, result);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    return ErrorHandler(i - 1, LE_NOT_FOUND);
                }
            }
        }

        linkPtr = le_sls_PeekNext(&OptionList, linkPtr);
    }

    // It doesn't match with any short option. So it may be a negative number.
    if (IsArgNumber(i))
    {
        // Only it can be a positional argument.
        return HandlePositionalArgument(i);
    }

    // Doesn't match with anything. Report an unexpected argument.
    return ErrorHandler(i - 1, LE_BAD_PARAMETER);
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
    // The option name starts after the leading "--".
    const char* optionNamePtr = (Argv[i]) + 2;

    // Traverse the option list, looking for one that matches.
    le_sls_Link_t* linkPtr = le_sls_Peek(&OptionList);
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
                        return ErrorHandler(i - 1, LE_UNSUPPORTED);
                    }
                    else
                    {
                        le_result_t result = HandleOptionWithValue(recPtr, optionNamePtr + len + 1);
                        if (result != LE_OK)
                        {
                            return ErrorHandler(i - 1, result);
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
                        return ErrorHandler(i - 1, LE_NOT_FOUND);
                    }
                }
            }
        }

        linkPtr = le_sls_PeekNext(&OptionList, linkPtr);
    }

    // Report an unexpected argument.
    return ErrorHandler(i - 1, LE_BAD_PARAMETER);
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
    // WARNING: This function gets called by the logging API, so don't use the logging API.
    //          Otherwise a stack overflow may occur.

    if (Argc == 0) // This will be true if le_arg_SetArgs() wasn't called or if argc was really 0.
    {
        return "_UNKNOWN_";
    }

    return le_path_GetBasenamePtr(Argv[0], "/");
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
    if (Argc == 0)
    {
        return 0;
    }

    return Argc - 1;
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
    if ( (argIndex >= Argc - 1) || (Argc == 0) )
    {
        return NULL;
    }

    return Argv[argIndex + 1];
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
    ssize_t index = FindOption(shortName, longName);

    if (index < 0)
    {
        return LE_NOT_FOUND;
    }
    else if (strchr(Argv[index], '=') != NULL)
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

    return le_utf8_ParseInt(varPtr, Argv[index + 1]);
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
    PositionalCallbackRec_t* recPtr = NULL;

    if (AllPositionalCallbacksHaveBeenCalled)
    {
        AllPositionalCallbacksHaveBeenCalled = false;

        // If more args than callbacks are allowed, then we need to discard the callback that
        // is currently on the callback list (if any), since we were only saving it in case more
        // positional args were found.
        if (IsMorePositionalArgsThanCallbacksAllowed)
        {
            le_sls_Link_t* linkPtr = le_sls_Pop(&PositionalCallbackList);

            if (linkPtr != NULL)
            {
                recPtr = CONTAINER_OF(linkPtr, PositionalCallbackRec_t, link);
            }
        }
    }

    // If we popped a callback record from the list, reuse it.  Otherwise, allocate a new one.
    if (recPtr == NULL)
    {
        recPtr = malloc(sizeof(PositionalCallbackRec_t));
        LE_ASSERT(recPtr != NULL);
        recPtr->link = LE_SLS_LINK_INIT;
    }

    recPtr->func = func;

    le_sls_Queue(&PositionalCallbackList, &recPtr->link);
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
    IsMorePositionalArgsThanCallbacksAllowed = true;
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
    IsLessPositionalArgsThanCallbacksAllowed = true;
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
    ErrorHandler = errorHandlerFunc;
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
    LE_DEBUG("Argc = %zu", Argc);

    int i;
    for (i = 1; i < Argc; i++)  // Start at 1, because 0 is the program name.
    {
        LE_DEBUG("Argv[%d] = '%s'", i, Argv[i]);

        // If it doesn't start with a '-', then it must be a positional argument.
        if (Argv[i][0] != '-')
        {
            i += HandlePositionalArgument(i);
        }
        // If it does start with a '-', but not a "--",
        else if (Argv[i][1] != '-')
        {
            // It's either just a '-' or it is a short name option.
            if (Argv[i][1] == '\0')
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
            if (Argv[i][2] == '\0')
            {
                i += HandlePositionalArgument(i);
            }
            else
            {
                i += HandleLongOption(i);
            }
        }
    }

    // If we are not allowed to have less positional arguments than positional callbacks,
    // and not all callbacks have been called at least once, then it's an error.
    if (   (IsLessPositionalArgsThanCallbacksAllowed == false)
        && (AllPositionalCallbacksHaveBeenCalled == false)  )
    {
        // Note: Ignore return code. Can't skip args, because there are none left.
        (void)ErrorHandler(i - 1, LE_UNDERFLOW);
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
    char**          argv    ///< [IN] argv from main.
)
{
    Argc = argc;
    Argv = argv;
}
