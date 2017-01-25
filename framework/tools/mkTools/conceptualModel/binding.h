//--------------------------------------------------------------------------------------------------
/**
 * @file binding.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_BINDING_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_BINDING_H_INCLUDE_GUARD


struct Binding_t
{
    enum EndPointType_t
    {
        INTERNAL, ///< Interface on a component in an executable in the current app (exe.comp.if).
        EXTERNAL_APP,  ///< External interface of a given app "agent" (app.if).
        EXTERNAL_USER, ///< External interface of a given non-app (user) "agent" (user.if).
    };

    EndPointType_t clientType;
    std::string clientAgentName;///< Name of the client agent. An app name or user name.
    std::string clientIfName;   ///< Client-side interface name to be passed to Service Directory.

    EndPointType_t serverType;
    std::string serverAgentName;///< Name of the server agent. An app name or user name.
    std::string serverIfName;   ///< Server-side interface name to be passed to Service Directory.

    const parseTree::Binding_t* parseTreePtr; ///< Ptr to parse tree object. NULL if auto-bound.

    Binding_t(const parseTree::Binding_t* p): parseTreePtr(p) {}
};



#endif // LEGATO_MKTOOLS_MODEL_BINDING_H_INCLUDE_GUARD
