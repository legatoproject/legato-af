//--------------------------------------------------------------------------------------------------
/**
 * @file programmingLanguage.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of all the supported programming languages.
 */
//--------------------------------------------------------------------------------------------------
enum ProgramLang_t
{
    LANG_C,     ///< C
    LANG_CXX,   ///< C++
    LANG_BIN,   ///< Binary, language unknown/irrelevant
    LANG_JAVA,  ///< Java
    LANG_PYTHON ///< Python
};

#endif // LEGATO_DEFTOOLS_PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD
