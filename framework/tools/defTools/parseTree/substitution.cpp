//--------------------------------------------------------------------------------------------------
/**
 * @file substitution.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parseTree
{


//--------------------------------------------------------------------------------------------------
/**
 * Check if the given character is a invalid start of name character.
 *
 * @return A true if valid, false otherwise.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsValidFirstChar
(
    char check
)
//--------------------------------------------------------------------------------------------------
{
    if (isalpha(check) || (check == '_'))
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the given character is a valid name character, this function is slightly less
 * restrictive than IsValidFirstChar.
 *
 * @return A true if valid, false otherwise.
 **/
//--------------------------------------------------------------------------------------------------
static bool IsValidChar
(
    char check
)
//--------------------------------------------------------------------------------------------------
{
    if (IsValidFirstChar(check) || isdigit(check))
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search the string from the beggining location and stop when an invalid character is found.
 *
 * @return The position of the last valid character.
 **/
//--------------------------------------------------------------------------------------------------
static size_t FindFirstNotNameChar
(
    const std::string& original,
    size_t begin
)
//--------------------------------------------------------------------------------------------------
{
    for (size_t i = begin; i < original.size(); ++i)
    {
        if (!IsValidChar(original[i]))
        {
            return i;
        }
    }

    return original.size();
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy the name string out of the original string.  Through an exception if an illegal char is
 * found.
 *
 * @return The name string extracted from the original.
 **/
//--------------------------------------------------------------------------------------------------
static std::string ExtractVarName
(
    const std::string& original,  ///< Extract the variable name from this string.
    size_t begin,                 ///< Start copying here.
    size_t count                  ///< Only copy this many chars.
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;

    result.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        auto next = original[begin + i];

        if (!IsValidChar(next))
        {
            throw mk::Exception_t(LE_I18N("Invalid character inside bracketed environment variable"
                                          " name."));
        }

        result.append(1, next);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Given an environment variable name.  Read it from the environment, make a note of the name, and
 * append that var's value into the output string.
 *
 * Throw an exception if the name is empty.
 **/
//--------------------------------------------------------------------------------------------------
static void EvalVar
(
    std::string& processed,             ///< The string we will dump the var value into.
    const std::string& original,        ///< The original string we pulled the name from.
    const std::string& varName,         ///< The name of the variable we extracted.
    std::set<std::string>* usedVarsPtr  ///< Record the found name in this set, if not null.
)
//--------------------------------------------------------------------------------------------------
{
    if (varName.empty())
    {
        throw mk::Exception_t(mk::format(LE_I18N("Empty environment variable name in string '%s'"),
                                         original));
    }

    if (usedVarsPtr != nullptr)
    {
        usedVarsPtr->insert(varName);
    }

    processed.append(envVars::Get(varName));
}


//--------------------------------------------------------------------------------------------------
/**
 * The calling function has found a bracketed var name at the location specified by 'begin'.
 * Extract the name, stopping at a closing bracket.  Throw an exception if an illegal character is
 * found within the name.
 *
 * Once the name is read, look up it's value and place this value into the output string.
 *
 * @return Number of characters consumed in the original string.
 **/
//--------------------------------------------------------------------------------------------------
static size_t HandleBracketVar
(
    const std::string& original,        ///< The string to extract a var name from.
    std::string& processed,             ///< The string we will dump the var value into.
    size_t begin,                       ///< Start name extraction from here.
    std::set<std::string>* usedVarsPtr  ///< Record the found name in this set, if not null.
)
//--------------------------------------------------------------------------------------------------
{
    size_t end = original.find('}', begin);

    if (end == std::string::npos)
    {
        throw mk::Exception_t(LE_I18N("Closing brace, '}', missing from environment variable."));
    }

    auto varName = ExtractVarName(original, begin, end - begin);

    EvalVar(processed, original, varName, usedVarsPtr);

    return end + 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * The calling function has found a var name at the location specified by begin in the original
 * string.  Read the full var name, stopping at either the end of the string or a non-name
 * character.
 *
 * Once the name is read, look up it's value and place this value into the output string.
 *
 * @return Number of characters consumed in the original string.
 **/
//--------------------------------------------------------------------------------------------------
static size_t HandleVar
(
    const std::string& original,        ///< The string to extract a var name from.
    std::string& processed,             ///< The string we will dump the var value into.
    size_t begin,                       ///< Start name extraction from here.
    std::set<std::string>* usedVarsPtr  ///< Record the found name in this set, if not null.
)
//--------------------------------------------------------------------------------------------------
{
    size_t end = FindFirstNotNameChar(original, begin);
    auto varName = original.substr(begin, end - begin);

    EvalVar(processed, original, varName, usedVarsPtr);

    return end;
}


//--------------------------------------------------------------------------------------------------
/**
 * Look for environment variables (specified as "$VAR_NAME" or "${VAR_NAME}") in a given string
 * and replace with environment variable contents.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
static std::string DoSubstitution
(
    const std::string& original,        ///< Original string to subsitute variables in.
    std::set<std::string>* usedVarsPtr  ///< If not null, record any variables found in original.
)
//--------------------------------------------------------------------------------------------------
{
    std::string processed;

    size_t begin = 0;
    size_t found = original.find('$');

    while (found != std::string::npos)
    {
        processed.append(original, begin, found - begin);

        if ((found + 1) >= original.size())
        {
            throw mk::Exception_t(LE_I18N("Environment variable name missing after '$'."));
        }

        auto next = original[found + 1];

        if (next == '$')
        {
            processed.append(1, '$');
            begin = found + 2;
        }
        else if (next == '{')
        {
            begin = HandleBracketVar(original, processed, found + 2, usedVarsPtr);
        }
        else if (IsValidFirstChar(next))
        {
            begin = HandleVar(original, processed, found + 1, usedVarsPtr);
        }
        else
        {
            throw mk::Exception_t(LE_I18N("Invalid character inside environment variable name."));
        }

        found = original.find('$', begin);
    }

    processed.append(original, begin, original.size() - begin);

    return processed;
}


//--------------------------------------------------------------------------------------------------
/**
 * Look for environment variables (specified as "$VAR_NAME" or "${VAR_NAME}") in a given string
 * and replace with environment variable contents.
 *
 * This version of the function differs from DoSubstitution in that variables like CURDIR are
 * handled relative to the content the string came from.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
std::string DoSubstitution
(
    const std::string& originalString,
    const parseTree::Content_t* contentPtr,
    std::set<std::string>* usedVarsPtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string oldDir;

    // Check to see if we were given a context to work with...
    if (contentPtr != NULL)
    {
        // Currently we only populate CURDIR.  However in the future we may add other variables based on
        // where the fragment where the text came from.
        oldDir = envVars::Get("CURDIR");
        envVars::Set("CURDIR",
                     path::MakeAbsolute(path::GetContainingDir(contentPtr->filePtr->path)));
    }

    // Actually subsitute any variables in the string now.
    auto result = DoSubstitution(originalString, usedVarsPtr);

    // Restore the old value of CURDIR if we had changed it before.
    if (contentPtr != NULL)
    {
        envVars::Set("CURDIR", oldDir);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Exactly like the previous version of DoSubstitution, except the context and the text
 * are both automatically extracted from the token pointer.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
std::string DoSubstitution
(
    const Token_t* tokenPtr,
    std::set<std::string>* usedVarsPtr
)
//--------------------------------------------------------------------------------------------------
{
    return DoSubstitution(tokenPtr->text, tokenPtr, usedVarsPtr);
}


} // namespace parseTree
