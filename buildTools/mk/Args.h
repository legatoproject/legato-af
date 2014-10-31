
//--------------------------------------------------------------------------------------------------
/**
 *  @page c_command_line_args Command-Line Processing Support
 *
 *  The command line processing consists of two phases.  In the first phase any components that care
 *  about command line arguments can call one of the register functions.
 *
 *  Then later, during application initialization, the supplied arguments will be processed.  Once
 *  successful the components processing callback will be called.
 *
 *  Example:
 *
 *  @code
 *
 *      int IsExtreme = 0;
 *
 *      void MyArgsCallback(void* context)
 *      {
 *          if (IsExtreme)
 *          {
 *              // Do something extreme here.
 *          }
 *      }
 *
 *      void MyComponentInit()
 *      {
 *          le_arg_AddOptionalFlag(&IsExtreme, "x", "extreme", "Take it to the limit?");
 *          le_arg_SetArgProcessedCallback(MyArgsCallback, NULL);
 *      }
 *
 *  @endcode
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
*/
//--------------------------------------------------------------------------------------------------

#ifndef ARGS_INCLUDE_GUARD
#define ARGS_INCLUDE_GUARD



// TODO: Perhaps move the help printing into a seperate function, void le_arg_PrintHelp(void).
// TODO: If so, change the return code handing to indicate that this was requested.
// TODO: Remove the default values...
// TODO: Add the help command as a standard command?
// TODO: Add more callback versions of the param setters?
// TODO: Add a group parameter to the param setters so that paramters can be logically grouped.
// TODO: void le_arg_SetExclusiveGroup(char* shortNames);
//       Used to give a list of options that are exclusive to each other.




//--------------------------------------------------------------------------------------------------
/**
 *  Callback type of the function that will be called on successful completion of the command line
 *  arguments.
*/
//--------------------------------------------------------------------------------------------------
typedef void (*le_arg_ProcessedCallback_t)
(
    void* contextPtr  /// Pointer to the user specified context.
);



//--------------------------------------------------------------------------------------------------
/**
 * Callback functions that are registered to receive string arguments must look like this.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_arg_StringValueCallback_t)
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
void le_arg_Scan
(
    int argc,           ///< The number of parameters provided on the command line.
    const char** argv   ///< Pointer to an array of pointers to strings given on the command line.
);



//--------------------------------------------------------------------------------------------------
/**
 *  Register a callback to be called once all command line processing has been completed.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetArgProcessedCallback
(
    le_arg_ProcessedCallback_t callbackPtr,  /// The function to call.
    void* contextPtr                         /// The context to call it in.
);



//--------------------------------------------------------------------------------------------------
/**
 * Register a callback to be called whenever an argument appears without a preceeding argument
 * identifier.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetLooseParamHandler
(
    le_arg_StringValueCallback_t callbackPtr,
    void* contextPtr
);




//--------------------------------------------------------------------------------------------------
/**
 *  Register a command line flag.  This flag will be optional and will be simply set to 0 if
 *  unspecified on the command line.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_AddOptionalFlag
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
void le_arg_AddInt
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
void le_arg_AddOptionalInt
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
void le_arg_AddString
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
void le_arg_AddOptionalString
(
    std::string* valuePtr,     ///< Value updated with either the command line value or the default.
    const char* defaultValue,  ///< Default value for this argument.
    const char shortName,      ///< Simple name for this argument.
    const char* longName,      ///< Longer more readable name for this argument.
    const char* doc            ///< Help documentation for this param.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Add an optional string parameter.  However instead of only updating a single paramteter a value
 *  update callback is invoked instead.  This way the callee can update a list of values everytime
 *  the supplied callback is invoked.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_AddMultipleString
(
    const char shortName,       ///< Simple name for this argument.
    const char* longName,       ///< Longer more readable name for this argument.
    const char* doc,            ///< Help documentation for this param.
    le_arg_StringValueCallback_t callbackPtr, ///< Function to be called with each instance.
    void* contextPtr            ///< Opaque value to pass to the callback function.
);




// C++11 lambda capable versions of some of the above functions.
#ifdef __cplusplus
    namespace internal
    {
        template <typename cbt, typename... Arguments>
        void InternalCallback(void* contextPtr, Arguments... params)
        {
            const cbt& functor = *reinterpret_cast<cbt*>(contextPtr);
            functor(std::forward<Arguments>(params)...);
        }
    }


    template <typename cbt>
    void le_arg_SetArgProcessedCallback(const cbt& callback)
    {
        le_arg_SetArgProcessedCallback(internal::InternalCallback<cbt>,
                                       const_cast<cbt*>(&callback));
    }


    template <typename cbt>
    void le_arg_SetLooseParamHandler(const cbt& callback)
    {
        le_arg_SetLooseParamHandler(internal::InternalCallback<cbt, const char*>,
                                    const_cast<cbt*>(&callback));
    }


    template <typename cbt>
    void le_arg_AddMultipleString(char shortName, const char* longName, const char* doc,
                                  const cbt& callback)
    {
        le_arg_AddMultipleString(shortName,
                                 longName,
                                 doc,
                                 internal::InternalCallback<cbt, const char*>,
                                 const_cast<cbt*>(&callback));
    }
#endif




#endif
