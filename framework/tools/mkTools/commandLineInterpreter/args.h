/**
 * @file args.h Command-Line Argument Processing Support
 *
 * The command line processing consists of two phases.  In the first phase, all expected arguments
 * are registered with the args scanner.  Finally, the arguments are scanned by a call to
 * Scan().
 *
 * Example:
 *
 * @code
 *
 *     void MyLooseArgHandler(void* context, const char* argValueString)
 *     {
 *         // Add argument string to list of things to be processed.
 *         ...
 *     }
 *
 *     int main(int argc, char** argv)
 *     {
 *         bool isExtreme = false;
 *
 *         args::AddOptionalFlag(&isExtreme, "x", "extreme", "Take it to the limit?");
 *         args::SetLooseArgHandler(MyLooseArgHandler, NULL);
 *
 *         args::Scan();
 *
 *         if (isExtreme)
 *         {
 *             // Do something extreme!
 *             ...
 *         }
 *         // Process list of things received as arguments.
 *         ...
 *     }
 *
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MKTOOLS_CLI_ARGS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CLI_ARGS_H_INCLUDE_GUARD

namespace args
{


// TODO: Improve help print-outs.


//--------------------------------------------------------------------------------------------------
/**
 * Callback functions that are registered to receive string arguments must look like this.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*StringValueCallback_t)
(
    void* contextPtr,
    const char* valuePtr
);


//--------------------------------------------------------------------------------------------------
/**
 *  Scan the command line arguments.  All registered parameters will be updated.  If a zero
 *  value is returned, then processing failed to complete.
 */
//--------------------------------------------------------------------------------------------------
void Scan
(
    int argc,           ///< The number of parameters provided on the command line.
    const char** argv   ///< Pointer to an array of pointers to strings given on the command line.
);


//--------------------------------------------------------------------------------------------------
/**
 * Register a callback to be called whenever an argument appears without a preceeding argument
 * identifier.
 */
//--------------------------------------------------------------------------------------------------
void SetLooseArgHandler
(
    StringValueCallback_t callbackPtr,
    void* contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 *  Register a command line flag.  This flag will be optional and will be simply set to 0 if
 *  unspecified on the command line.
 */
//--------------------------------------------------------------------------------------------------
void AddOptionalFlag
(
    bool* flag,            /// Ptr to flag value.  Will be set to true if set, false otherwise.
    const char shortName,  /// Simple name for this flag.
    const char* longName,  /// Longer more readable name for this flag.
    const char* doc        /// Help documentation for this param.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Register a manditory argument that takes an integer parameter.  This argument will be manditory
 *  and it will be a runtime error if the flag is not specified.
 */
//--------------------------------------------------------------------------------------------------
void AddInt
(
    int* value,      ///< The pointed at value will be updated with the user specified int.
    char shortName,  ///< Simple name for this argument.
    char* longName,  ///< Longer more readable name for this argument.
    const char* doc  ///< Help documentation for this param.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Register an optional int value.
 */
//--------------------------------------------------------------------------------------------------
void AddOptionalInt
(
    int* value,              ///< Value updated with either the command line value or the default.
    const int defaultValue,  ///< Default value for this argument.
    const char shortName,    ///< Simple name for this argument.
    const char* longName,    ///< Longer more readable name for this argument.
    const char* doc          ///< Help documentation for this param.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Register a manditor string value.
 */
//--------------------------------------------------------------------------------------------------
void AddString
(
    std::string* valuePtr, ///< Value to be updated with the user specified string.
    char shortName,        ///< Simple name for this argument.
    const char* longName,  ///< Longer more readable name for this argument.
    const char* doc        ///< Help documentation for this param.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Register an optional string paramter.
 */
//--------------------------------------------------------------------------------------------------
void AddOptionalString
(
    std::string* valuePtr,     ///< Value updated with either the command line value or the default.
    const char* defaultValue,  ///< Default value for this argument.
    const char shortName,      ///< Simple name for this argument.
    const char* longName,      ///< Longer more readable name for this argument.
    const char* doc            ///< Help documentation for this param.
);


//--------------------------------------------------------------------------------------------------
/**
 *  Add an optional string parameter.  However instead of only updating a single parameter a value
 *  update callback is invoked instead.  This way the callee can update a list of values everytime
 *  the supplied callback is invoked.
 */
//--------------------------------------------------------------------------------------------------
void AddMultipleString
(
    const char shortName,       ///< Simple name for this argument.
    const char* longName,       ///< Longer more readable name for this argument.
    const char* doc,            ///< Help documentation for this param.
    StringValueCallback_t callbackPtr, ///< Function to be called with each instance.
    void* contextPtr            ///< Opaque value to pass to the callback function.
);


namespace internal
{
    //----------------------------------------------------------------------------------------------
    /**
     * Callback function that casts a function pointer into another type and calls it.
     **/
    //----------------------------------------------------------------------------------------------
    template <typename cbt, typename... Arguments>
    void InternalCallback
    (
        void* contextPtr,
        Arguments... params
    )
    {
        const cbt& functor = *reinterpret_cast<cbt*>(contextPtr);
        functor(std::forward<Arguments>(params)...);
    }
}



template <typename cbt>
void SetLooseArgHandler
(
    const cbt& callback
)
{
    SetLooseArgHandler(internal::InternalCallback<cbt, const char*>, const_cast<cbt*>(&callback));
}


template <typename cbt>
void AddMultipleString
(
    char shortName,
    const char* longName,
    const char* doc,
    const cbt& callback
)
{
    AddMultipleString(shortName,
                      longName,
                      doc,
                      internal::InternalCallback<cbt, const char*>,
                      const_cast<cbt*>(&callback));
}


//--------------------------------------------------------------------------------------------------
/**
 * Saves the command-line arguments (in a file in the build's working directory)
 * for later use by MatchesSaved().
 */
//--------------------------------------------------------------------------------------------------
void Save
(
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Compares the current command-line arguments with those stored in the build's working directory.
 *
 * @return true if the arguments are effectively the same or
 *         false if there's a significant difference.
 */
//--------------------------------------------------------------------------------------------------
bool MatchesSaved
(
    const mk::BuildParams_t& buildParams
);



} // namespace args

#endif // LEGATO_MKTOOLS_CLI_ARGS_H_INCLUDE_GUARD
