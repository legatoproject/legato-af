//--------------------------------------------------------------------------------------------------
/**
 * @file content.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSE_TREE_CONTENT_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSE_TREE_CONTENT_H_INCLUDE_GUARD


struct DefFileFragment_t;


//--------------------------------------------------------------------------------------------------
/**
 * Content item base class.
 */
//--------------------------------------------------------------------------------------------------
struct Content_t
{
    enum Type_t
    {
        TOKEN,          ///< Basic lexical token (Token_t).
        SIMPLE_SECTION, ///< Section (TokenList_t), "name: token"
        TOKEN_LIST_SECTION,///< Section (TokenList_t), "name: { token-list }"
        COMPLEX_SECTION,///< Section (CompoundItemList_t), "name: { compound-item-list }"
        BUNDLED_FILE,   ///< Bundled file (TokenList_t), "[rw] local/path /target/path"
        BUNDLED_DIR,    ///< Bundled dir (TokenList_t), "[rw] local/path /target/path"
        REQUIRED_FILE,  ///< Required file (TokenList_t), "src/path /dest/path"
        REQUIRED_DIR,   ///< Required dir (TokenList_t), "src/path /dest/path"
        REQUIRED_DEVICE,///< Required device (TokenList_t), "[rw] src/path /dest/path"
        PROVIDED_API,   ///< .cdef (TokenList_t), "powerLed = gpioOut.api [async]"
        REQUIRED_API,   ///< .cdef (TokenList_t), "powerLed = gpioOut.api [types-only]"
        REQUIRED_CONFIG_TREE,   ///< .adef (TokenList_t), "[w] treeName" or just "treeName"
        REQUIRED_MODULE, ///< Required module (TokenList_t) "drivers/example.mdef [optional]"
        REQUIRED_COMPONENT, ///< Required component (TokenList_t) "component/path [provide-header]"
        EXTERN_API_INTERFACE,   ///< .adef (TokenList_t), "externName = exe.comp.interface"
        BINDING,        ///< Binding (TokenList_t), "exe.component.api -> app.service"
        COMMAND,        ///< Command (TokenList_t), "cmd = app:/path/to/exe"
        EXECUTABLE,     ///< Executable (TokenList_t), "exe = ( comp1 comp2 )"
        RUN_PROCESS,    ///< Process to run (TokenList_t), "proc = ( exe arg1 arg2 )"
        ENV_VAR,        ///< Environment variable (TokenList_t), "varName = value"
        MODULE_PARAM,   ///< Module parameter (TokenList_t), "name = value"
        POOL,           ///< Pool (TokenList_t), "poolName = 123"
        APP,            ///< Named item in .sdef 'apps:' section (CompoundItemList_t),
                        ///  "appPath", "appPath { }" or "appPath { overrides }".
        MODULE,         ///< Named item in .sdef 'kernelModules:' section (CompoundItemList_t)
        NET_LINK        ///< Named item in .ndef 'links:' section.
    };

    Type_t type;        ///< The type of content item.
    DefFileFragment_t* filePtr; ///< The file it was found in.

    std::string TypeName() const { return TypeName(type); }
    static std::string TypeName(Content_t::Type_t type);

    virtual ~Content_t() {}

protected:

    /// Constructor
    Content_t(Type_t contentType, DefFileFragment_t* defFilePtr): type(contentType), filePtr(defFilePtr) {}
};



#endif // LEGATO_DEFTOOLS_PARSE_TREE_CONTENT_H_INCLUDE_GUARD
