//--------------------------------------------------------------------------------------------------
/**
 * @file fileSystemObject.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration used by GetBundledPath to enable generically accesing the source and destination of
 * the FileSystem object.
 */
//--------------------------------------------------------------------------------------------------
enum class BundleAccess_t
{
    Source,
    Dest
};


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
    :   parseTreePtr(baseObject?(baseObject->parseTreePtr):NULL),
        srcPath(newSrcPath),
        destPath(destPath),
        permissions(permissions)
    {
    }

    const parseTree::TokenList_t* parseTreePtr;   ///< Pointer to related object in the parse tree.

    std::string srcPath;        ///< File system path where the object is found.
    std::string destPath;       ///< Path to where the object will be put on target.
    Permissions_t permissions;  ///< Read, write, and/or execute permissions on the object.

    // Return either the source or dest path based on the accessFlag.
    inline const std::string& GetBundledPath(BundleAccess_t accessFlag) const
    {
        switch (accessFlag)
        {
            case BundleAccess_t::Source:
                return srcPath;

            case BundleAccess_t::Dest:
                return destPath;
        }

        throw mk::Exception_t(LE_I18N("Unknown bundled file access type."));
    }

    /// Two file system objects refer to the same file if the source and destination paths are the same.
    bool operator==(const FileSystemObject_t& a) const
    {
        return (srcPath == a.srcPath) && (destPath == a.destPath);
    }

    bool operator!=(const FileSystemObject_t& a) const
    {
        return !(*this == a);
    }

    // Order set by destination path first followed by source path.
    bool operator<(const FileSystemObject_t& a) const
    {
        if (destPath == a.destPath)
        {
            return (srcPath < a.srcPath);
        }

        return (destPath < a.destPath);
    }

    bool operator<=(const FileSystemObject_t& a) const
    {
        return !(*this > a);
    }

    bool operator>(const FileSystemObject_t& a) const
    {
        if (destPath == a.destPath)
        {
            return (srcPath > a.srcPath);
        }

        return (destPath > a.destPath);
    }

    bool operator>=(const FileSystemObject_t& a) const
    {
        return !(*this < a);
    }

    struct Hash_t
    {
        typedef model::FileSystemObject_t argument_type;
        typedef std::size_t result_type;

        // Hash a FileSystemObject.
        result_type operator()(const argument_type& s) const
        {
            return std::hash<std::string>()(s.srcPath + s.destPath);
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


#endif // LEGATO_DEFTOOLS_MODEL_FILE_SYSTEM_OBJECT_H_INCLUDE_GUARD
