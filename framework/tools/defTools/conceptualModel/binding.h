//--------------------------------------------------------------------------------------------------
/**
 * @file binding.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_BINDING_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_BINDING_H_INCLUDE_GUARD


struct Binding_t
{
    enum EndPointType_t
    {
        INTERNAL, ///< Interface on a component in an executable in the current app (exe.comp.if).
        EXTERNAL_APP,  ///< External interface of a given app "agent" (app.if).
        EXTERNAL_USER, ///< External interface of a given non-app (user) "agent" (user.if).
        LOCAL,    ///< Interface within the same executable
    };

    EndPointType_t clientType;
    std::string clientAgentName;///< Name of the client agent. An app name or user name.
    std::string clientIfName;   ///< Client-side interface name to be passed to Service Directory.

    EndPointType_t serverType;
    std::string serverAgentName;///< Name of the server agent. An app name or user name.
    std::string serverIfName;   ///< Server-side interface name to be passed to Service Directory.

    const parseTree::TokenList_t* parseTreePtr; ///< Ptr to parse tree object. NULL if auto-bound.

    Binding_t(const parseTree::TokenList_t* p): parseTreePtr(p) {}
};



#endif // LEGATO_DEFTOOLS_MODEL_BINDING_H_INCLUDE_GUARD
