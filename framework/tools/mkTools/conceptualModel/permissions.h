//--------------------------------------------------------------------------------------------------
/**
 * @file permissions.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Stores file system permissions.
 */
//--------------------------------------------------------------------------------------------------
struct Permissions_t
{
    /// Constructor
    Permissions_t(): permissions(0) {}

    /// Getters
    bool IsReadable() const { return (permissions & PERM_READABLE); }
    bool IsWriteable() const { return (permissions & PERM_WRITEABLE); }
    bool IsExecutable() const { return (permissions & PERM_EXECUTABLE); }

    /// Setters
    void SetReadable() { permissions |= PERM_READABLE; }
    void SetWriteable() { permissions |= PERM_WRITEABLE; }
    void SetExecutable() { permissions |= PERM_EXECUTABLE; }

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


#endif // LEGATO_MKTOOLS_MODEL_PERMISSIONS_H_INCLUDE_GUARD
