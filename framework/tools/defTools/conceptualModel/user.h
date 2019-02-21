//--------------------------------------------------------------------------------------------------
/**
 * @file user.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_USER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_USER_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single non-app user.
 */
//--------------------------------------------------------------------------------------------------
struct User_t
{
    User_t(const std::string& userName): name(userName) {}

    std::string name;   ///< Name of the user account (e.g., "root").

    std::map<std::string, model::Binding_t*> bindings; ///< Map of bindings (key is interface name).
};


#endif // LEGATO_DEFTOOLS_MODEL_USER_H_INCLUDE_GUARD
