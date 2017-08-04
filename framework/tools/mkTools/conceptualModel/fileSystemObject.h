//--------------------------------------------------------------------------------------------------
/**
 * @file fileSystemObject.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a file system object, such as a file or directory.
 */
//--------------------------------------------------------------------------------------------------
struct FileSystemObject_t
{
    /// Constructor
    FileSystemObject_t(const parseTree::TokenList_t* tokenListPtr): parseTreePtr(tokenListPtr) {}

    FileSystemObject_t
    (
        const std::string& newSrcPath,
        const std::string& destPath,
        const Permissions_t& permissions,
        const FileSystemObject_t* baseObject = NULL
    )
    :   srcPath(newSrcPath),
        destPath(destPath),
        permissions(permissions),
        parseTreePtr(baseObject?(baseObject->parseTreePtr):NULL)
    {
    }

    const parseTree::TokenList_t* parseTreePtr;   ///< Pointer to related object in the parse tree.

    std::string srcPath;        ///< File system path where the object is found.
    std::string destPath;       ///< Path to where the object will be put on target.
    Permissions_t permissions;  ///< Read, write, and/or execute permissions on the object.

    /// Two file sytem objects refer to the same file if the destination paths are the same.
    bool operator==(const FileSystemObject_t& a) const
    {
        return destPath == a.destPath;
    }

    bool operator!=(const FileSystemObject_t& a) const
    {
        return !(*this == a);
    }

    bool operator<(const FileSystemObject_t& a) const
    {
        return destPath < a.destPath;
    }

    bool operator<=(const FileSystemObject_t& a) const
    {
        return !(*this > a);
    }

    bool operator>(const FileSystemObject_t& a) const
    {
        return destPath > a.destPath;
    }

    bool operator>=(const FileSystemObject_t& a) const
    {
        return !(*this < a);
    }

    struct Hash_t
    {
        typedef model::FileSystemObject_t argument_type;
        typedef std::size_t result_type;

        // Hash a FileSystemObject.  Destination path should be unique.
        result_type operator()(const argument_type& s) const
        {
            return std::hash<std::string>()(s.destPath);
        }
    };
};

struct FileObjectPtrHash_t
{
    typedef FileSystemObject_t::Hash_t base_hash;
    typedef std::shared_ptr<base_hash::argument_type> argument_type;
    typedef base_hash::result_type result_type;

    private:
        base_hash baseHash;

    public:
        result_type operator()(const argument_type &arg) const
        {
            return baseHash(*arg);
        }
};

struct FileObjectPtrLess_t : std::binary_function<std::shared_ptr<const FileSystemObject_t>,
                                                  std::shared_ptr<const FileSystemObject_t>,
                                                  bool>
{
    inline bool operator()(first_argument_type a, second_argument_type b) const
    {
        return (*a) < (*b);
    }
};

/// Convenience typedef for constructing unordered sets of file system objects.
typedef std::set<model::FileSystemObject_t> FileSystemObjectSet_t;

/// Convenience typedef for constructing unordered sets of file system object pointers.
typedef std::set<std::shared_ptr<model::FileSystemObject_t>,
                 model::FileObjectPtrLess_t> FileObjectPtrSet_t;


#endif // LEGATO_MKTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
