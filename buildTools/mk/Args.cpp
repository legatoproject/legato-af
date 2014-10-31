//--------------------------------------------------------------------------------------------------
/**
 *  Legato @ref c_command_line_args implementation.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <stdexcept>
#include <iostream>
#include <set>
#include <list>
#include <string>
#include <cstring>
#include "Args.h"




//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the programs registered command line arguments.
 */
//--------------------------------------------------------------------------------------------------
struct ParamInfo
{
    enum Type
    {
        FLAG,         ///< This param is a flag, so takes no additional parameters.  It is either
                      ///   present or not.
        INT,          ///< This param expects an integer value to go with it.
        STRING,       ///< This param expects a string value to go with it.
        MULTI_STRING  ///< This param expects a string and can appear several times.
    };

    const std::string shortName;  /// The single char name to go with this param.
    const std::string longName;   /// The long, or nice name to go with this param.
    const std::string docString;  /// Supplied help message to go with this param.
    const Type type;              /// What kind of extra arguments does this param expect.
    const bool isOptional;        /// Is this parameter optional?  Or is it an error if it's left
                                  ///   out.

    bool wasFound;                /// Records whether or not the parameter was found.

    void* valuePtr;               /// Pointer to the variable to be updated with the new specified
                                  ///   value.  If allowed and unspecified the default value is used
                                  ///   instead.

    struct ValueCallback
    {
        le_arg_StringValueCallback_t stringUpdatePtr;
        void* contextPtr;
    };

    union
    {
        const int defaultInt;       /// Default value if this param is an integer.
        const char* defaultString;  /// Default value if this param is a string.

        ValueCallback valueCallback;
    };


    //----------------------------------------------------------------------------------------------
    /**
     *  Create a new param object with a default int value.
     */
    //----------------------------------------------------------------------------------------------
    ParamInfo
    (
        char newShortName,
        const char* newLongNamePtr,
        const char* doc,
        Type newType,
        bool newIsOptional,
        void* newValuePtr,
        int newDefaultInt
    )
    :   shortName(1, newShortName),
        longName(newLongNamePtr),
        docString(doc),
        type(newType),
        isOptional(newIsOptional),
        wasFound(false),
        valuePtr(newValuePtr),
        defaultInt(newDefaultInt)
    {
    }


    //----------------------------------------------------------------------------------------------
    /**
     *  Create a new param object with a new default string value.
     */
    //----------------------------------------------------------------------------------------------
    ParamInfo
    (
        char newShortName,
        const char* newLongNamePtr,
        const char* doc,
        Type newType,
        bool newIsOptional,
        void* newValuePtr,
        const char* newDefaultString
    )
    :   shortName(1, newShortName),
        longName(newLongNamePtr),
        docString(doc),
        type(newType),
        isOptional(newIsOptional),
        wasFound(false),
        valuePtr(newValuePtr),
        defaultString(newDefaultString)
    {
    }


    ParamInfo
    (
        char newShortName,
        const char* newLongNamePtr,
        const char* doc,
        Type newType,
        le_arg_StringValueCallback_t stringUpdatePtr,
        void* contextPtr
    )
    :   shortName(1, newShortName),
        longName(newLongNamePtr),
        docString(doc),
        type(newType),
        isOptional(true),
        wasFound(false),
        valuePtr(NULL),
        valueCallback({ stringUpdatePtr, contextPtr })
    {
    }
};




//--------------------------------------------------------------------------------------------------
/**
 *  Used for insertion into the param set.  This funcion will sort param infos by their long and
 *  short names only.
 */
//--------------------------------------------------------------------------------------------------
static bool operator <
(
    const ParamInfo& paramA,  /// Compare this param info...
    const ParamInfo& paramB   ///   to this one...
)
{
    if (paramA.shortName < paramB.shortName)
    {
        return true;
    }
    else if (   (paramA.shortName == paramB.shortName)
             && (paramA.longName < paramB.longName))
    {
        return true;
    }

    return false;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Collection of registered command parameters.
 */
//--------------------------------------------------------------------------------------------------
static std::set<ParamInfo> Params;




//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the user callbacks.
 */
//--------------------------------------------------------------------------------------------------
struct CallbackInfo
{
    le_arg_ProcessedCallback_t functionPtr;
    void* contextPtr;
};




//--------------------------------------------------------------------------------------------------
/**
 *  List of user registered callback functions.  The callbacks in this list are called once the
 *  command line arguments have been successfuly processed.
 */
//--------------------------------------------------------------------------------------------------
static std::list<CallbackInfo> FinishCallbacks;




struct LooseCallbackInfo
{
    le_arg_StringValueCallback_t functionPtr;
    void* contextPtr;
};




static std::list<LooseCallbackInfo> LooseParamCallbacks;




//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the user supplied command line arguemnts and wether or not they
 *  were ever processed.
 */
//--------------------------------------------------------------------------------------------------
struct FoundParamInfo
{
    std::list<std::string> value;  /// The supplied value for this pram.
    bool wasUsed;       /// Was this pram ever looked up?
};




//--------------------------------------------------------------------------------------------------
/**
 *  Attempt to parse a string from the command line and update the parameter value variable based
 *  on the param's type.
 */
//--------------------------------------------------------------------------------------------------
static void SetParamValue
(
    ParamInfo& info,             /// The parameter info we're updating.
    const std::string& paramArg  /// The string given on the command line.
)
{
    info.wasFound = true;

    if ((info.type != ParamInfo::FLAG) && (paramArg.empty()))
    {
        throw std::runtime_error("Value missing from argument --" + info.longName +
                                 " (-" + info.shortName + ").");
    }

    switch (info.type)
    {
        case ParamInfo::FLAG:
            // Flag arguments do not take extra parameters.  They're either given or not.
            if (paramArg.empty() == false)
            {
                throw std::runtime_error("Unexpected parameter, '" + paramArg +
                                         "' passed to flag argument --" + info.longName +
                                         " (-" + info.shortName + ").");
            }
            else
            {
                *static_cast<bool*>(info.valuePtr) = true;
            }
            break;

        case ParamInfo::INT:
            // Attempt to parse the given string into an integer.
            *static_cast<int*>(info.valuePtr) = std::stoi(paramArg);
            break;

        case ParamInfo::STRING:
            *(static_cast<std::string*>(info.valuePtr)) = paramArg;
            break;

        case ParamInfo::MULTI_STRING:
            if (info.valueCallback.stringUpdatePtr)
            {
                info.valueCallback.stringUpdatePtr(info.valueCallback.contextPtr,
                                                   paramArg.c_str());
            }
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Internal function called to extract a long param argument ("--foo=bar" style) from the
 *  argument list.
 */
//--------------------------------------------------------------------------------------------------
static void GetLongParamArg
(
    const char* argPtr              /// The argument to process.
)
{
    // Figure out the argument name.
    std::string arg = argPtr;
    size_t pos = arg.find("=", 2 /* skip the "--" */);
    std::string argName = arg.substr(2, pos - 2);

    // Make sure it's not empty.
    if (argName.empty())
    {
        std::string msg = "Malformed argument '";
        msg += arg;
        msg += "'.";
        throw std::runtime_error(msg);
    }

    // Extract the argument value (if there is one).
    std::string argValue;
    if (pos != std::string::npos)
    {
        pos++;
        argValue = arg.substr(pos);
    }

    // Go through our list of expected parameters and match this argument to it.
    for (auto& paramIter: Params)
    {
        // If it matches,
        if (paramIter.longName == argName)
        {
            SetParamValue(const_cast<ParamInfo&>(paramIter), argValue);

            return;
        }
    }

    std::string msg = "Unexpected parameter '";
    msg += arg;
    msg += "'.";
    throw std::runtime_error(msg);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Internal function called to extract a short param argument ("-f bar" style) from the
 *  argument list.
 *
 *  @return
 *      A value of true if an arg was found, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool GetShortParamArg
(
    int argc,              /// Count of arguments that are avaiable.
    const char** argv,     /// The argument list.
    int index              /// Index into arg list of short argument.
)
{
    if (index >= argc)
    {
        return false;
    }

    for (auto& paramIter: Params)
    {
        std::string paramArg;

        if (paramIter.shortName == (argv[index] + 1))
        {
            if (paramIter.type == ParamInfo::FLAG)
            {
                SetParamValue(const_cast<ParamInfo&>(paramIter), "");

                return false;
            }
            else
            {
                if ((index + 1) >= argc)
                {
                    std::string msg = "Missing value for argument '";
                    msg += argv[index];
                    msg += "'.";
                    throw std::runtime_error(msg);
                }

                SetParamValue(const_cast<ParamInfo&>(paramIter), argv[index + 1]);

                return true;
            }
        }
    }

    std::string msg = "Unexpected parameter '";
    msg += argv[index];
    msg += "'.";
    throw std::runtime_error(msg);
}



//--------------------------------------------------------------------------------------------------
/**
 *  Given a parameter info struct, update the value it's pointing to with the default value
 *  specified.
 */
//--------------------------------------------------------------------------------------------------
static void SetParamDefault
(
    ParamInfo& info  /// The info struct to update.
)
{
    switch (info.type)
    {
        case ParamInfo::FLAG:
            *static_cast<bool*>(info.valuePtr) = false;
            break;

        case ParamInfo::INT:
            *static_cast<int*>(info.valuePtr) = info.defaultInt;
            break;

        case ParamInfo::STRING:
            *(static_cast<std::string*>(info.valuePtr)) = info.defaultString;
            break;

        case ParamInfo::MULTI_STRING:
            break;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 *  Display help info for the registered command line parameters.
 */
//--------------------------------------------------------------------------------------------------
static void DisplayHelp
(
)
{
    // TODO: Reformat into man page style and add functions to allow the client to set the
    //       NAME and introductory DESCRIPTION text.  Ideally, the SYNOPSIS should be auto-
    //       generated.

    std::cout << std::endl << "Command line parameters" << std::endl;

    for (auto param: Params)
    {
        std::cout << "  -" << param.shortName << ", --" << param.longName;

        switch (param.type)
        {
            case ParamInfo::FLAG:
                break;

            case ParamInfo::INT:
                std::cout << ", <integer>";
                break;

            case ParamInfo::STRING:
                std::cout << ", <string>";
                break;

            case ParamInfo::MULTI_STRING:
                std::cout << ", <string>";
                break;
        }

        std::cout << std::endl << "        ";

        if (param.isOptional)
        {
            if (param.type == ParamInfo::MULTI_STRING)
            {
                std::cout << "(Multiple, optional) ";
            }
            else
            {
                std::cout << "(Optional) ";
            }
        }

        std::cout << param.docString << std::endl << std::endl;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Scan the command line arguments.  All registered parameters will be updated.  If a zero
 *  value is returned, then processing failed to complete.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_Scan
(
    int argc,           /// The parameter info we're updating.
    const char** argv   /// The string given on the command line.
)
{
    for (int i = 1; i < argc; ++i)
    {
        const char* next = argv[i];
        size_t len = std::strlen(next);

        // If there's no leading '-',
        if (next[0] != '-')
        {
            // If there are registered call-backs for handling a "loose" parameter, then
            // call those and pass this parameter to them.
            if (LooseParamCallbacks.size() > 0)
            {
                for (auto callbackIter: LooseParamCallbacks)
                {
                    callbackIter.functionPtr(callbackIter.contextPtr, next);
                }
            }
            else
            {
                throw std::runtime_error(std::string("Argument without command flag, ") +
                                         next + ".");
            }
        }
        // If there is only a single '-', character, that's an error.
        else if (len < 2)
        {
            throw std::runtime_error("No name given for param, '-'");
        }
        // If there are two leading '-' characters,
        else if (next[1] == '-')
        {
            // If there are only two '-' characters and nothing more, then that's an error.
            if (len < 3)
            {
                throw std::runtime_error("No name given for param, '--'");
            }


            // Make sure that the user isn't asking for help.
            if (std::strcmp(next, "--help") == 0)
            {
                DisplayHelp();
                exit(EXIT_SUCCESS);
            }


            // Looks like we were given a valid name for a "--xxxx=" arg.
            GetLongParamArg(argv[i]);
        }
        // If there is a leading '-' character and at least one additional character after that,
        else
        {
            // TODO: Support single name flags, bunched together, '-xyz'.

            // We only support a single character for the short name parameters.
            if (len > 2)
            {
                throw std::runtime_error(std::string("Bad short name parameter flag, ") + next +
                                         ".");
            }


            // Check if the user is asking for help.
            if (std::strcmp(next, "-h") == 0)
            {
                DisplayHelp();
                exit(EXIT_SUCCESS);
            }


            // Looks like we were given a valid name for a "-x" arg.
            if (GetShortParamArg(argc, argv, i))
            {
                // An argument was found, so consume it from the list so that we don't try to
                // process it again.
                ++i;
            }
        }
    }


    // Go through our list of expected parameters and make sure that all mandatory arguments
    // were found and set any optional parameters that were not found to their default values.
    for (auto& paramIter: Params)
    {
        if ((paramIter.isOptional == false) && (paramIter.wasFound == false))
        {
            throw std::runtime_error("Missing required parameter: --" + paramIter.longName +
                                     " (-" + paramIter.shortName + ").");
        }
        else if ((paramIter.isOptional == true) && (paramIter.wasFound == false))
        {
            SetParamDefault(const_cast<ParamInfo&>(paramIter));
        }
    }

    // Now that the args have been processed call the registered callbacks.

    for (auto callbackIter: FinishCallbacks)
    {
        callbackIter.functionPtr(callbackIter.contextPtr);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Register a callback to be called once all command line processing has been completed.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_SetArgProcessedCallback
(
    le_arg_ProcessedCallback_t callbackPtr,  ///< The function to call.
    void* contextPtr                         ///< The context to call it in.
)
{
    if (callbackPtr == NULL)
    {
        throw std::runtime_error("Error, a NULL argument processing callback has been given.");
    }

    FinishCallbacks.push_back(CallbackInfo{callbackPtr, contextPtr});
}




void le_arg_SetLooseParamHandler
(
    le_arg_StringValueCallback_t callbackPtr,
    void* contextPtr
)
{
    if (callbackPtr == NULL)
    {
        throw std::runtime_error("Error, a NULL parameter processing callback has been given.");
    }

    LooseParamCallbacks.push_back({callbackPtr, contextPtr});
}




//--------------------------------------------------------------------------------------------------
/**
 *  Register a command line flag.  This flag will be optional and will be simply set to 0 if
 *  unspecified on the command line.
 */
//--------------------------------------------------------------------------------------------------
void le_arg_AddOptionalFlag
(
    bool* flagPtr,         ///< Pointer to flag value. Will be set to true if set, false otherwise.
    const char shortName,  ///< Simple name for this flag.
    const char* longName,  ///< Longer more readable name for this flag.
    const char* doc        ///< Help documentation for this param.
)
{
    Params.insert(ParamInfo(shortName, longName, doc, ParamInfo::FLAG, true, flagPtr, 0));
}




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
)
{
    Params.insert(ParamInfo(shortName, longName, doc, ParamInfo::INT, false, value, 0));
}




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
)
{
    Params.insert(ParamInfo(shortName,
                            longName,
                            doc,
                            ParamInfo::INT,
                            true,
                            value,
                            defaultValue));
}




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
)
{
    Params.insert(ParamInfo(shortName,
                            longName,
                            doc,
                            ParamInfo::STRING,
                            false,
                            valuePtr,
                            0));
}




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
)
{
    Params.insert(ParamInfo(shortName,
                            longName,
                            doc,
                            ParamInfo::STRING,
                            true,
                            valuePtr,
                            defaultValue));
}




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
)
{
    Params.insert(ParamInfo(shortName,
                            longName,
                            doc,
                            ParamInfo::MULTI_STRING,
                            callbackPtr,
                            contextPtr));
}
