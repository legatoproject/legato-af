//--------------------------------------------------------------------------------------------------
/**
 * @file defFile.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSE_TREE_DEF_FILE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSE_TREE_DEF_FILE_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a tokenized file.
 *
 * This includes both top-level files and included files.
 */
//--------------------------------------------------------------------------------------------------
struct DefFileFragment_t
{
    std::string path;   ///< The file system path to the file.

    std::string pathMd5;///< MD5 hash of the file system path to the file.

    size_t version;     ///< File format version number (0 = unknown, 1 = first version).

    Token_t* firstTokenPtr; ///< Ptr to the first token in the file.
    Token_t* lastTokenPtr;  ///< Ptr to the last token in the file.

    std::map<Token_t*, DefFileFragment_t*> includedFiles; ///< Ptr to list of included files.  Maps
                                                          /// tokens instead of paths in case the
                                                          /// same file is included multiple times
                                                          /// in different contexts.

    /// Constructor
    DefFileFragment_t(const std::string& filePath);
};

//--------------------------------------------------------------------------------------------------
/**
 * Represents a parsed top-level .Xdef file.
 */
//--------------------------------------------------------------------------------------------------
struct DefFile_t: public DefFileFragment_t
{
    enum Type_t
    {
        CDEF,
        ADEF,
        MDEF,
        SDEF
    };

    Type_t type;        ///< The type of file.

    std::list<CompoundItem_t*> sections; ///< List of top-level sections in the file.

    void ThrowException(const std::string& message) const __attribute__ ((noreturn));

protected:

    /// Constructor
    DefFile_t(Type_t type, const std::string& filePath);
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a parsed .cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct CdefFile_t : public DefFile_t
{
    CdefFile_t(const std::string& filePath): DefFile_t(CDEF, filePath) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a parsed .adef file.
 */
//--------------------------------------------------------------------------------------------------
struct AdefFile_t : public DefFile_t
{
    AdefFile_t(const std::string& filePath): DefFile_t(ADEF, filePath) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a parsed .mdef file.
 */
//--------------------------------------------------------------------------------------------------
struct MdefFile_t : public DefFile_t
{
    MdefFile_t(const std::string& filePath): DefFile_t(MDEF, filePath) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a parsed .sdef file.
 */
//--------------------------------------------------------------------------------------------------
struct SdefFile_t : public DefFile_t
{
    SdefFile_t(const std::string& filePath): DefFile_t(SDEF, filePath) {}
};



#endif // LEGATO_DEFTOOLS_PARSE_TREE_DEF_FILE_H_INCLUDE_GUARD
