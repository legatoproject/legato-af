//--------------------------------------------------------------------------------------------------
/**
 * @file permissions.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Stores file system permissions.
 */
//--------------------------------------------------------------------------------------------------
struct Permissions_t
{
    /// Constructor
    Permissions_t(): permissions(0) {}
    Permissions_t(const Permissions_t& original): permissions(original.permissions) {}
    Permissions_t(bool readable,
                  bool writable,
                  bool executable): permissions((readable?PERM_READABLE:0) |
                                                (writable?PERM_WRITEABLE:0) |
                                                (executable?PERM_EXECUTABLE:0)) {}

    /// Getters
    bool IsReadable() const { return (permissions & PERM_READABLE); }
    bool IsWriteable() const { return (permissions & PERM_WRITEABLE); }
    bool IsExecutable() const { return (permissions & PERM_EXECUTABLE); }

    /// Setters
    void SetReadable() { permissions |= PERM_READABLE; }
    void SetWriteable() { permissions |= PERM_WRITEABLE; }
    void SetExecutable() { permissions |= PERM_EXECUTABLE; }

    friend inline bool operator <(const Permissions_t& a, const Permissions_t& b);
    bool operator==(const Permissions_t& a) const
    {
        return this->permissions == a.permissions;
    }
    bool operator!=(const Permissions_t& a) const
    {
        return !(*this == a);
    }

private:

    typedef enum
    {
        PERM_READABLE   = 1,
        PERM_WRITEABLE  = 2,
        PERM_EXECUTABLE = 4,
    }
    PermissionFlags_t;

    int permissions;    ///< Bit-wide OR of permissions flags in effect.
};


inline bool operator <(const Permissions_t& a, const Permissions_t& b)
{
    return a.permissions < b.permissions;
}


#endif // LEGATO_DEFTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD
