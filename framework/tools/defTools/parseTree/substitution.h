//--------------------------------------------------------------------------------------------------
/**
 * @file substitution.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSE_TREE_SUBSTITUTION_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSE_TREE_SUBSTITUTION_H_INCLUDE_GUARD


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
    const parseTree::Content_t* contentPtr = NULL,
    std::set<std::string>* usedVarsPtr = NULL ///< If not null, returns a list of all
                                              ///  variable names used in this substitution.
);


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
    std::set<std::string>* usedVarsPtr = NULL ///< If not null, returns a list of all
                                              ///  variable names used in this substitution.
);


#endif // LEGATO_DEFTOOLS_PARSE_TREE_SUBSTITUTION_H_INCLUDE_GUARD
