/**
 * @page c_args Command Line Arguments API
 *
 * @subpage le_args.h "API Reference"
 *
 * <HR>
 *
 * When a program starts, arguments may be passed from the command line.
 *
 * @verbatim
$ foo bar baz
@endverbatim
 *
 * In a traditional C/C++ program, these arguments are received as parameters to @c main().
 * The Legato framework makes these available to component via function calls instead.
 *
 * @section c_args_by_index Argument Access By Index
 *
 * The arguments can be fetched by index using @c le_arg_GetArg().  The first argument has index 0,
 * the second argument has index 1, etc.  In the above example, <b>bar</b> has index 0 and
 * <b>baz</b> has index 1.
 *
 * The number of available arguments is obtained using @c le_arg_NumArgs().
 *
 * The name of the program is obtained using @c le_arg_GetProgramName().
 *
 * The program name and all arguments are assumed to be Null-terminated UTF-8 strings.  For more
 * information about UTF-8 strings see @ref c_utf8.
 *
 * @section c_args_options Options
 *
 * Options are arguments that start with a "-" or "--".
 *
 * To search for a specific option, the following functions are provided:
 *  - le_arg_GetFlagOption() - Searches for a given flag (flags don't have values).
 *  - le_arg_GetIntOption() - Searches for a given option with an integer value.
 *  - le_arg_GetStringOption() - Searches for a given option with a string value.
 *
 * @note A "-" or "--" by itself is not considered an option.  These are treated as positional
 *       arguments.
 *
 * @section c_args_positional Positional Arguments
 *
 * Positional arguments are arguments that <b>do not</b> start with a "-" or "--"; except for
 * "-" or "--" by itself (these are positional arguments).
 *
 * For example, the following command line has four positional arguments ("foo", "bar", "-",
 * and "--").  A flag option ("-x"), and two string options ("-f ./infile" and
 * "--output=/tmp/output file") are intermixed with the positional arguments.
 *
 * @code
 * $ myExe -x foo -f ./infile - "--output=/tmp/output file" bar --
 * @endcode
 *
 * In this example, "foo" is the first positional argument, "-" is the
 * second, "bar" is the third, and "--" is the fourth.
 *
 * Positional arguments are retrieved using the @ref c_args_scanner and
 * le_arg_AddPositionalCallback().
 *
 * @section c_args_scanner Argument Scanner
 *
 * If you're building a command-line application with a complex argument list, you may want to
 * use the Legato framework's argument scanner feature.  It supports many options commonly seen
 * in command-line tools and performs a lot of the error checking and reporting for you.
 *
 * For example, the @c commandLine sample application implements a tool called @c fileInfo that
 * prints information about files or directories.  It is flexible about the order of appearance
 * of options on the command-line.  For example, the following are equivalent:
 *
 * @verbatim
# fileInfo -x -mc 20 permissions *
@endverbatim
 *
 * @verbatim
# fileInfo permissions --max-count=20 * -x
@endverbatim
 *
 * Note that
 *  - "-mc 20" and "--max-count=20" are different ways of specifying the same option;
 *  - the order of appearance of the options can change;
 *  - options (which start with '-' or '--') and other arguments can be intermixed.
 *
 * @subsection c_args_scanner_usage Usage
 *
 * A program (typically inside a @c COMPONENT_INIT) can call functions to register variables
 * to be set or call-back functions to be called when certain arguments are passed to the program.
 *
 * After registering the variables and call-back functions, le_arg_Scan() is called to parse
 * the argument list.
 *
 * The following functions can be called before le_arg_Scan() is called to register variables
 * to be set or call-back functions to be called by le_arg_Scan():
 *
 * - le_arg_SetFlagVar()
 * - le_arg_SetIntVar()
 * - le_arg_SetStringVar()
 * - le_arg_SetFlagCallback()
 * - le_arg_SetIntCallback()
 * - le_arg_SetStringCallback()
 * - le_arg_AddPositionalCallback()
 *
 * There are essentially 3 forms of function:
 *
 * - le_arg_SetXxxVar() - Registers a variable to be set by le_arg_Scan() when it sees a
 *                         certain argument starting with '-' or '--'.
 *
 * - le_arg_SetXxxCallback() - Registers a call-back function to be called by le_arg_Scan() when
 *                              it sees a certain argument starting with '-' or '--'.
 *
 * - le_arg_AddPositionalCallback() - Registers a call-back function to be called by le_arg_Scan()
 *                                    when it sees an argument that does not start with either
 *                                    '-' or '--'.
 *
 * le_arg_AddPositionalCallback() can be called multiple times.  This constructs a list of
 * call-back functions, where the first function in that list will be called for the first
 * positional argument, the second function in the list will be called for the second positional
 * argument, etc.
 *
 * Normally, an error will be generated if there are not the same number of positional arguments
 * as there are positional callbacks in the list.  However, this behaviour can be changed:
 *
 *  - If le_arg_AllowMorePositionalArgsThanCallbacks() is called, then the last callback in the
 *    list will be called for each of the extra positional arguments on the command-line.
 *
 *  - If le_arg_AllowLessPositionalArgsThanCallbacks() will allow shorter argument lists, which
 *    will result in one or more of the last callbacks in the list not being called.
 *
 * le_utf8_ParseInt() can be used by a positional callback to convert the string value it receives
 * into an integer value, if needed.
 *
 * @subsection c_args_parser_example Example
 *
 * @code
 *
 *  // Set IsExtreme to true if the -x or --extreme appears on the command-line.
 *  le_arg_SetFlagVar(&IsExtreme, "x", "extreme");
 *
 *  // Set Count to the value N given by "-mc N" or "--max-count=N".
 *  le_arg_SetIntVar(&MaxCount, "mc", "max-count");
 *
 *  // Register a function to be called if -h or --help appears on the command-line.
 *  le_arg_SetFlagCallback(PrintHelp, "h", "help");
 *
 *  // The first argument that doesn't start with '-' or '--' should be a command.
 *  le_arg_AddPositionalCallback(SetCommand);
 *
 *  // All other arguments that don't start with '-' or '--' should be file paths.
 *  le_arg_AddPositionalCallback(SetFilePath);
 *  le_arg_AllowMorePositionalArgsThanCallbacks();
 *
 *  // Perform command-line argument processing.
 *  le_arg_Scan();
 *
 * @endcode
 *
 * @subsection c_args_parser_errorHandling Error Handling
 *
 * If a program wishes to try to recover from errors on the command-line or to generate its own
 * special form of error message, it can use le_arg_SetErrorHandler() to register a callback
 * function to be called to handle errors.
 *
 * If no error handler is set, the default handler will print an error message
 * to the standard error stream and terminate the process with an exit code
 * of EXIT_FAILURE.
 *
 * Error conditions that can be reported to the error handler are described in the documentation
 * for @ref le_arg_ErrorHandlerFunc_t.
 *
 * @code
 *
 *  // Set Count to the value N given by "-mc N" or "--max-count=N".
 *  le_arg_SetIntVar(&MaxCount, "mc", "max-count");
 *
 *  // Register my own error handler.
 *  le_arg_SetErrorHandler(HandleArgError);
 *
 *  // Perform command-line argument processing.
 *  le_arg_Scan();
 *
 * @endcode
 *
 * @section c_args_writingYourOwnMain Writing Your Own main()?
 *
 * If you are not using a main() function that is auto-generated by the Legato application
 * framework's build tools (@c mksys, @c mkapp, or @c mkexe ), then you must call le_arg_SetArgs()
 * to pass @c argc and @c argv to the argument parsing system before using any other @c le_arg
 * functions.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_args.h
 *
 * Legato @ref c_args include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_ARGS_INCLUDE_GUARD
#define LEGATO_ARGS_INCLUDE_GUARD


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the number of command line arguments available not including the program name.
 *
 * @return
 *      Number of command line arguments available.
 */
//--------------------------------------------------------------------------------------------------
size_t le_arg_NumArgs
(
    void
);


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
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Register a boolean variable to be set if a given flag option appears on the argument list.
 *
 * No value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetFlagVar
(
    bool* varPtr,           ///< Ptr to the variable to be set true if the flag option is found.
    const char* shortName,  ///< Short form of option name (e.g., "h" will match "-h").
    const char* longName    ///< Long form of option name (e.g., "help" will match "--help").
);


//--------------------------------------------------------------------------------------------------
/**
 * Register an integer variable to be set if a given option appears on the argument list.
 *
 * An integer value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetIntVar
(
    int* varPtr,            ///< Ptr to the variable to be set if the option is found.
    const char* shortName,  ///< Short form of option name (e.g., "n" will match "-n 1234").
    const char* longName    ///< Long form of name ("max-count" matches "--max-count=1234").
);


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
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetStringVar
(
    const char** varPtr,    ///< Ptr to the variable to be set if the option is found.
    const char* shortName,  ///< Short form of option name (e.g., "n" will match "-n foo").
    const char* longName    ///< Long form of name ("name" matches "--name=foo").
);


//--------------------------------------------------------------------------------------------------
/**
 * Flag argument callback functions registered using le_arg_SetFlagCallback() must conform to this
 * prototype.
 *
 * If the flag appears N times on the command line, the callback will be called N times.
 **/
//--------------------------------------------------------------------------------------------------
typedef void (*le_arg_FlagCallbackFunc_t)(void);

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given flag option appears on the argument list.
 *
 * No value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetFlagCallback
(
    le_arg_FlagCallbackFunc_t func, ///< The callback function address.
    const char* shortName,          ///< Short form of option name (e.g., "h" will match "-h").
    const char* longName           ///< Long form of option name (e.g., "help" will match "--help").
);


//--------------------------------------------------------------------------------------------------
/**
 * Integer argument callback functions registered using le_arg_SetIntCallback() must conform to
 * this prototype.
 *
 * If the option appears N times on the command line, the callback will be called N times.
 *
 * @param value The value of the integer option.
 **/
//--------------------------------------------------------------------------------------------------
typedef void (*le_arg_IntCallbackFunc_t)
(
    int value   ///< see above.
);

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given integer value option appears on the
 * argument list.
 *
 * An integer value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetIntCallback
(
    le_arg_IntCallbackFunc_t func,  ///< The callback function address.
    const char* shortName,          ///< Short form of option name (e.g., "n" will match "-n 1234").
    const char* longName            ///< Long form of name ("max-count" matches "--max-count=1234").
);


//--------------------------------------------------------------------------------------------------
/**
 * String argument callback functions registered using le_arg_SetStringCallback() or
 * le_arg_AddPositionalCallback() must conform to this prototype.
 *
 * If the option appears N times on the command line, the callback will be called N times.
 *
 * @param value Pointer to the value of the string option.
 **/
//--------------------------------------------------------------------------------------------------
typedef void (*le_arg_StringCallbackFunc_t)
(
    const char* value   ///< see above.
);

//--------------------------------------------------------------------------------------------------
/**
 * Register a callback function to be called if a given string option appears on the argument list.
 *
 * A string value is expected after the option name.
 *
 * One or the other of shortName or longName may be NULL.
 * If not NULL, these MUST be pointers to strings that are never deallocated or changed.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetStringCallback
(
    le_arg_StringCallbackFunc_t func,///< The callback function address.
    const char* shortName,           ///< Short form of option name (e.g., "n" will match "-n foo").
    const char* longName             ///< Long form of name ("name" matches "--name=foo").
);


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
);


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
);


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
);


//--------------------------------------------------------------------------------------------------
/**
 * Error handler function prototype.  All argument error handler functions (passed into
 * le_arg_SetErrorHandler() ) must conform to this prototype.
 *
 * Errors that can be reported to this function are:
 *  - LE_BAD_PARAMETER - The argument is not expected.
 *  - LE_NOT_FOUND - The argument should have a value, but no value was given.
 *  - LE_FORMAT_ERROR - The argument should have a numerical value, but was given something else.
 *  - LE_OUT_OF_RANGE - Magnitude of numerical argument too big to be stored in chosen data type.
 *  - LE_OVERFLOW - Too many positional arguments found on command line.
 *  - LE_UNDERFLOW - Too few positional arguments found on command line.
 *  - LE_UNSUPPORTED - The argument should not have a value but was given one.
 *
 * @return  The number of arguments to skip before resuming argument scanning.
 *          0 = resume scanning at argIndex + 1; 1 = resume at argIndex + 2; etc.
 *
 * @param argIndex Index of argument that is bad (0 = first arg after program name).
 *
 * @param errorCode Code indicating the type of error that was encountered.
 **/
//--------------------------------------------------------------------------------------------------
typedef size_t (*le_arg_ErrorHandlerFunc_t)
(
    size_t  argIndex,       ///< see above.
    le_result_t errorCode   ///< see above.
);


//--------------------------------------------------------------------------------------------------
/**
 * Register an error handler function to be called by le_arg_Scan() whenever an unexpected argument
 * is encountered or an option's value cannot be converted to the correct data type.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_SetErrorHandler
(
    le_arg_ErrorHandlerFunc_t errorHandlerFunc  ///< [IN] The error handler function.
);


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
);


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
);

//--------------------------------------------------------------------------------------------------
/**
 * Scans the argument list, setting variables and calling callbacks registered using the
 * le_arg_SetXxxVar(), le_arg_SetXxxCallback(), and le_arg_AddPositionalParameters() functions.
 **/
//--------------------------------------------------------------------------------------------------
void le_arg_Scan
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Passes argc and argv to the argument parser for later use by le_arg_Scan().
 *
 * @note    This function is normally called by main().  If the Legato application framework is
 *          automatically generating main() for you, then you can just ignore this function.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetArgs
(
    const size_t    argc,   ///< [IN] argc from main().
    const char**    argv    ///< [IN] argv from main().
);


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
);

#endif // LEGATO_ARGS_INCLUDE_GUARD
