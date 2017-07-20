//--------------------------------------------------------------------------------------------------
/**
 *  Legato @ref c_command_line_args implementation.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <stdexcept>
#include <iostream>
#include <set>
#include <list>
#include <string>
#include <cstring>

#include "mkTools.h"
#include "commandLineInterpreter.h"


namespace cli
{

namespace args
{


//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the programs registered command line arguments.
 */
//--------------------------------------------------------------------------------------------------
struct ParamInfo_t
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
        StringValueCallback_t stringUpdatePtr;
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
    ParamInfo_t
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
    ParamInfo_t
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


    ParamInfo_t
    (
        char newShortName,
        const char* newLongNamePtr,
        const char* doc,
        Type newType,
        StringValueCallback_t stringUpdatePtr,
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
    const ParamInfo_t& paramA,  /// Compare this param info...
    const ParamInfo_t& paramB   ///   to this one...
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
static std::set<ParamInfo_t> Params;




//--------------------------------------------------------------------------------------------------
/**
 *  Structure used to keep track of the user callbacks.
 */
//--------------------------------------------------------------------------------------------------
struct LooseCallbackInfo
{
    StringValueCallback_t functionPtr;
    void* contextPtr;
};



/// List of "loose" (unnamed) argument call-backs.
static std::list<LooseCallbackInfo> LooseArgCallbacks;




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
    ParamInfo_t& info,             /// The parameter info we're updating.
    const std::string& param       /// The string given on the command line.
)
{
    info.wasFound = true;

    if ((info.type != ParamInfo_t::FLAG) && (param.empty()))
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Value missing from argument --%s (-%s)."),
                       info.longName, info.shortName)
        );
    }

    switch (info.type)
    {
        case ParamInfo_t::FLAG:
            // Flag arguments do not take extra parameters.  They're either given or not.
            if (param.empty() == false)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Unexpected parameter, '%s' passed to flag argument"
                                       " --%s (-%s)."),
                               param, info.longName, info.shortName)
                );
            }
            else
            {
                *static_cast<bool*>(info.valuePtr) = true;
            }
            break;

        case ParamInfo_t::INT:
            // Attempt to parse the given string into an integer.
            *static_cast<int*>(info.valuePtr) = std::stoi(param);
            break;

        case ParamInfo_t::STRING:
            *(static_cast<std::string*>(info.valuePtr)) = param;
            break;

        case ParamInfo_t::MULTI_STRING:
            if (info.valueCallback.stringUpdatePtr)
            {
                info.valueCallback.stringUpdatePtr(info.valueCallback.contextPtr,
                                                   param.c_str());
            }
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Internal function called to extract a long param ("--foo=bar" style) from the
 *  argument list.
 */
//--------------------------------------------------------------------------------------------------
static void GetLongParam
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
        throw mk::Exception_t(
            mk::format(LE_I18N("Malformed argument '%s'."), arg)
        );
    }

    // Extract the argument value (if there is one).
    std::string argValue;
    if (pos != std::string::npos)
    {
        pos++;
        argValue = arg.substr(pos);
    }

    // Go through our list of expected params and match this argument to it.
    for (auto& paramIter: Params)
    {
        // If it matches,
        if (paramIter.longName == argName)
        {
            SetParamValue(const_cast<ParamInfo_t&>(paramIter), argValue);

            return;
        }
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Unexpected parameter: '%s'."), arg)
    );
}




//--------------------------------------------------------------------------------------------------
/**
 *  Internal function called to extract a short param ("-f bar" style) from the
 *  argument list.
 *
 *  @return
 *      A value of true if an arg was found, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool GetShortParam
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
        std::string param;

        if (paramIter.shortName == (argv[index] + 1))
        {
            if (paramIter.type == ParamInfo_t::FLAG)
            {
                SetParamValue(const_cast<ParamInfo_t&>(paramIter), "");

                return false;
            }
            else
            {
                if ((index + 1) >= argc)
                {
                    throw mk::Exception_t(
                        mk::format(LE_I18N("Missing value for argument '%s'."), argv[index])
                    );
                }

                SetParamValue(const_cast<ParamInfo_t&>(paramIter), argv[index + 1]);

                return true;
            }
        }
    }
    throw mk::Exception_t(
        mk::format(LE_I18N("Unexpected parameter: '%s'."), argv[index])
    );
}



//--------------------------------------------------------------------------------------------------
/**
 *  Given a param info struct, update the value it's pointing to with the default value
 *  specified.
 */
//--------------------------------------------------------------------------------------------------
static void SetParamDefault
(
    ParamInfo_t& info  /// The info struct to update.
)
{
    switch (info.type)
    {
        case ParamInfo_t::FLAG:
            *static_cast<bool*>(info.valuePtr) = false;
            break;

        case ParamInfo_t::INT:
            *static_cast<int*>(info.valuePtr) = info.defaultInt;
            break;

        case ParamInfo_t::STRING:
            *(static_cast<std::string*>(info.valuePtr)) = info.defaultString;
            break;

        case ParamInfo_t::MULTI_STRING:
            break;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 *  Display help info for the registered command line params.
 */
//--------------------------------------------------------------------------------------------------
static void DisplayHelp
(
)
{
    // TODO: Reformat into man page style and add functions to allow the client to set the
    //       NAME and introductory DESCRIPTION text.  Ideally, the SYNOPSIS should be auto-
    //       generated.

    std::cout << std::endl << LE_I18N("Command line parameters") << std::endl;

    for (auto param: Params)
    {
        std::cout << mk::format(LE_I18N("  -%s, --%s"), param.shortName, param.longName);

        switch (param.type)
        {
            case ParamInfo_t::FLAG:
                break;

            case ParamInfo_t::INT:
                std::cout << LE_I18N(", <integer>");
                break;

            case ParamInfo_t::STRING:
            case ParamInfo_t::MULTI_STRING:
                std::cout << LE_I18N(", <string>");
                break;
        }

        std::cout << std::endl << "        ";

        if (param.isOptional)
        {
            if (param.type == ParamInfo_t::MULTI_STRING)
            {
                std::cout << LE_I18N("(Multiple, optional) ");
            }
            else
            {
                std::cout << LE_I18N("(Optional) ");
            }
        }

        std::cout << param.docString << std::endl << std::endl;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Scan the command line arguments.  All registered params will be updated and matching argument
 *  callbacks will be called.
 *
 *  @throw On error.
 */
//--------------------------------------------------------------------------------------------------
void Scan
(
    int argc,           /// Count of entries in the argc list.
    const char** argv   /// Array of pointers to null-terminated arguments.
)
{
    for (int i = 1; i < argc; ++i)
    {
        const char* next = argv[i];
        size_t len = std::strlen(next);

        // If there's no leading '-',
        if (next[0] != '-')
        {
            // If there are registered call-backs for handling a "loose" argument, then
            // call those and pass this parameter to them.
            if (LooseArgCallbacks.size() > 0)
            {
                for (auto callbackIter: LooseArgCallbacks)
                {
                    callbackIter.functionPtr(callbackIter.contextPtr, next);
                }
            }
            else
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Argument without command flag, %s."), next)
                );
            }
        }
        // If there is only a single '-', character, that's an error.
        else if (len < 2)
        {
            throw mk::Exception_t(LE_I18N("No name given for parameter '-'"));
        }
        // If there are two leading '-' characters,
        else if (next[1] == '-')
        {
            // If there are only two '-' characters and nothing more, then that's an error.
            if (len < 3)
            {
                throw mk::Exception_t(LE_I18N("No name given for parameter '--'"));
            }


            // Make sure that the user isn't asking for help.
            if (std::strcmp(next, "--help") == 0)
            {
                DisplayHelp();
                exit(EXIT_SUCCESS);
            }


            // Looks like we were given a valid name for a "--xxxx=" arg.
            GetLongParam(argv[i]);
        }
        // If there is a leading '-' character and at least one additional character after that,
        else
        {
            // We only support a single character for short name params.
            if (len > 2)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Bad short parameter name, %s."), next)
                );
            }


            // Check if the user is asking for help.
            if (std::strcmp(next, "-h") == 0)
            {
                DisplayHelp();
                exit(EXIT_SUCCESS);
            }


            // Looks like we were given a valid name for a "-x" arg.
            if (GetShortParam(argc, argv, i))
            {
                // An argument was found, so consume it from the list so that we don't try to
                // process it again.
                ++i;
            }
        }
    }


    // Go through our list of expected arguments and make sure that all mandatory arguments
    // were found and set any optional arguments that were not found to their default values.
    for (auto& paramIter: Params)
    {
        if ((paramIter.isOptional == false) && (paramIter.wasFound == false))
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Missing required parameter: --%s (-%s)."),
                           paramIter.longName, paramIter.shortName)
            );
        }
        else if ((paramIter.isOptional == true) && (paramIter.wasFound == false))
        {
            SetParamDefault(const_cast<ParamInfo_t&>(paramIter));
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever a "loose" argument (not a named parameter)
 * is found on the command-line.
 **/
//--------------------------------------------------------------------------------------------------
void SetLooseArgHandler
(
    StringValueCallback_t callbackPtr,
    void* contextPtr
)
{
    if (callbackPtr == NULL)
    {
        throw mk::Exception_t(LE_I18N("Internal error: a NULL parameter processing callback "
                                      "has been given."));
    }

    LooseArgCallbacks.push_back({callbackPtr, contextPtr});
}




//--------------------------------------------------------------------------------------------------
/**
 *  Register a command line flag.  This flag will be optional and will be simply set to 0 if
 *  unspecified on the command line.
 */
//--------------------------------------------------------------------------------------------------
void AddOptionalFlag
(
    bool* flagPtr,         ///< Pointer to flag value. Will be set to true if set, false otherwise.
    const char shortName,  ///< Simple name for this flag.
    const char* longName,  ///< Longer more readable name for this flag.
    const char* doc        ///< Help documentation for this param.
)
{
    Params.insert(ParamInfo_t(shortName, longName, doc, ParamInfo_t::FLAG, true, flagPtr, 0));
}




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
)
{
    Params.insert(ParamInfo_t(shortName, longName, doc, ParamInfo_t::INT, false, value, 0));
}




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
)
{
    Params.insert(ParamInfo_t(shortName,
                              longName,
                              doc,
                              ParamInfo_t::INT,
                              true,
                              value,
                              defaultValue));
}




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
)
{
    Params.insert(ParamInfo_t(shortName,
                              longName,
                              doc,
                              ParamInfo_t::STRING,
                              false,
                              valuePtr,
                              0));
}




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
)
{
    Params.insert(ParamInfo_t(shortName,
                              longName,
                              doc,
                              ParamInfo_t::STRING,
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
void AddMultipleString
(
    const char shortName,       ///< Simple name for this argument.
    const char* longName,       ///< Longer more readable name for this argument.
    const char* doc,            ///< Help documentation for this param.
    StringValueCallback_t callbackPtr, ///< Function to be called with each instance.
    void* contextPtr            ///< Opaque value to pass to the callback function.
)
{
    Params.insert(ParamInfo_t(shortName,
                              longName,
                              doc,
                              ParamInfo_t::MULTI_STRING,
                              callbackPtr,
                              contextPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file system path to the file in which command-line arguments are saved.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetSaveFilePath
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    return path::Combine(buildParams.workingDir, "mktool_args");
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
)
//--------------------------------------------------------------------------------------------------
{
    auto filePath = GetSaveFilePath(buildParams);

    // Make sure the containing directory exists.
    file::MakeDir(buildParams.workingDir);

    // Open the file
    std::ofstream argsFile(filePath);
    if (!argsFile.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), filePath)
        );
    }

    // Write each arg as a line in the file.
    for (int i = 0; i < buildParams.argc; i++)
    {
        if (strcmp(buildParams.argv[i], "--dont-run-ninja") != 0) // But skip '--dont-run-ninja'.
        {
            argsFile << buildParams.argv[i] << '\n';
        }

        if (argsFile.fail())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Error writing to file '%s'."), filePath)
            );
        }
    }

    // Close the file.
    argsFile.close();
    if (argsFile.fail())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Error closing file '%s'."), filePath)
        );
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    auto filePath = GetSaveFilePath(buildParams);

    if (!file::FileExists(filePath))
    {
        if (buildParams.beVerbose)
        {
            std::cout << LE_I18N("Command-line arguments from previous run not found.")
                      << std::endl;
        }
        return false;
    }

    // Open the file
    std::ifstream argsFile(filePath);
    if (!argsFile.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for reading."), filePath)
        );
    }

    int i;
    char lineBuff[1024];

    for (i = 0; i < buildParams.argc; i++)
    {
        // Read a line from the file (discarding '\n') and check for EOF or error.
        argsFile.getline(lineBuff, sizeof(lineBuff));
        if (argsFile.eof())
        {
            goto different;
        }
        else if (!argsFile.good())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Error reading from file '%s'."), filePath)
            );
        }

        // Compare the line from the file with the argument.
        if (strcmp(buildParams.argv[i], lineBuff) != 0)
        {
            goto different;
        }
    }

    // Read one more line to make sure we get an end-of-file, otherwise there are less args
    // this time than last time.
    argsFile.getline(lineBuff, sizeof(lineBuff));
    if (argsFile.eof())
    {
        return true;
    }

different:

    if (buildParams.beVerbose)
    {
        std::cout << LE_I18N("Command-line arguments are different this time.") << std::endl;
        std::cout << mk::format(LE_I18N("-- Last time (%s) --"), filePath) << std::endl;
        argsFile.clear();
        argsFile.seekg(0);
        if (!argsFile.good())
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Error reading from file '%s'."), filePath)
            );
        }
        while (!argsFile.eof())
        {
            argsFile.getline(lineBuff, sizeof(lineBuff));
            std::cout << lineBuff << " ";
        }
        std::cout << std::endl << LE_I18N("-- This time --") << std::endl;
        for (i = 0; i < buildParams.argc; i++)
        {
            std::cout << buildParams.argv[i] << " ";
        }
        std::cout << std::endl;
    }
    return false;
}


} // namespace args

} // namespace cli


