//--------------------------------------------------------------------------------------------------
/**
 * @file compoundItem.h
 *
 * Definitions of all compound item types.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSE_TREE_COMPOUND_ITEM_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSE_TREE_COMPOUND_ITEM_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Compound content item, such as a section or named item, made up of multiple tokens.
 */
//--------------------------------------------------------------------------------------------------
struct CompoundItem_t : public Content_t
{
    Token_t* firstTokenPtr; ///< Ptr to the first token in the item.
    Token_t* lastTokenPtr;  ///< Ptr to the last token in the item.

    void ThrowException(const std::string& msg) const __attribute__ ((noreturn))
    {
        firstTokenPtr->ThrowException(msg);
    }

    void PrintWarning(const std::string& msg) const
    {
        firstTokenPtr->PrintWarning(msg);
    }

    virtual ~CompoundItem_t() {}

protected:

    /// Constructor.
    CompoundItem_t(Type_t contentType, Token_t* firstTokenPtr);
};


//--------------------------------------------------------------------------------------------------
/**
 * Compound (non-leaf) content item that contains a list of tokens as content.
 * An example is the "cflags:" section of the .cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct TokenList_t : public CompoundItem_t
{
    /// Constructor.
    TokenList_t(Type_t contentType, Token_t* firstTokenPtr);

    // Content accessors.
    void AddContent(Token_t* contentPtr);
    const std::vector<Token_t*>& Contents() const { return contents; }

    virtual ~TokenList_t() {}

private:

    /// List of significant things in the item.
    /// Excludes whitespace, comments, separators and braces.
    std::vector<Token_t*> contents;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a simple section that contains a single token.
 */
//--------------------------------------------------------------------------------------------------
struct SimpleSection_t : TokenList_t
{
    SimpleSection_t(Token_t* firstTokenPtr): TokenList_t(SIMPLE_SECTION, firstTokenPtr) {}

    /// @return a reference to the section name.
    const std::string& Name() const { return firstTokenPtr->text; }

    /// @return a reference to the content token's text.
    const std::string& Text() const { return Contents()[0]->text; }
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a section whose content is a list of zero or more tokens enclosed in curly braces.
 */
//--------------------------------------------------------------------------------------------------
struct TokenListSection_t : TokenList_t
{
    TokenListSection_t(Token_t* firstTokenPtr): TokenList_t(TOKEN_LIST_SECTION, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "file:" subsection of a "bundles:" section.
 */
//--------------------------------------------------------------------------------------------------
struct BundledFile_t : TokenList_t
{
    BundledFile_t(Token_t* firstTokenPtr): TokenList_t(BUNDLED_FILE, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "dir:" subsection of a "bundles:" section.
 */
//--------------------------------------------------------------------------------------------------
struct BundledDir_t : TokenList_t
{
    BundledDir_t(Token_t* firstTokenPtr): TokenList_t(BUNDLED_DIR, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "file:" subsection of a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredFile_t : TokenList_t
{
    RequiredFile_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_FILE, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "dir:" subsection of a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredDir_t : TokenList_t
{
    RequiredDir_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_DIR, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "device:" subsection of a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredDevice_t : TokenList_t
{
    RequiredDevice_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_DEVICE, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in an "api:" subsection of a "provides:" section of a .cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct ProvidedApi_t : TokenList_t
{
    ProvidedApi_t(Token_t* firstTokenPtr): TokenList_t(PROVIDED_API, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in an "api:" subsection of a "requires:" section of a .cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredApi_t : TokenList_t
{
    RequiredApi_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_API, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "component:" subsection of a "requires:" section of a .cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredComponent_t : TokenList_t
{
    RequiredComponent_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_COMPONENT, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry inside a "configTree:" subsection in a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredConfigTree_t : TokenList_t
{
    RequiredConfigTree_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_CONFIG_TREE, firstTokenPtr){}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in an "extern:" section of a .adef or .sdef file.
 */
//--------------------------------------------------------------------------------------------------
struct ExternApiInterface_t : TokenList_t
{
    ExternApiInterface_t(Token_t* firstTokenPtr): TokenList_t(EXTERN_API_INTERFACE, firstTokenPtr){}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "bindings:" section.
 */
//--------------------------------------------------------------------------------------------------
struct Binding_t : TokenList_t
{
    Binding_t(Token_t* firstTokenPtr): TokenList_t(BINDING, firstTokenPtr)
                                                                    { AddContent(firstTokenPtr); }
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry in a "commands:" section.
 */
//--------------------------------------------------------------------------------------------------
struct Command_t : TokenList_t
{
    Command_t(Token_t* firstTokenPtr): TokenList_t(COMMAND, firstTokenPtr)
                                                                    { AddContent(firstTokenPtr); }
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single named entry inside an "executables:" section.
 */
//--------------------------------------------------------------------------------------------------
struct Executable_t : TokenList_t
{
    Executable_t(Token_t* firstTokenPtr): TokenList_t(EXECUTABLE, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single named entry inside a "run:" section.
 */
//--------------------------------------------------------------------------------------------------
struct RunProcess_t : TokenList_t
{
    RunProcess_t(Token_t* firstTokenPtr): TokenList_t(RUN_PROCESS, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry inside an "envVars:" section.
 */
//--------------------------------------------------------------------------------------------------
struct EnvVar_t : TokenList_t
{
    EnvVar_t(Token_t* firstTokenPtr): TokenList_t(ENV_VAR, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a module parameter inside a "params:" section.
 */
//--------------------------------------------------------------------------------------------------
struct ModuleParam_t : TokenList_t
{
    ModuleParam_t(Token_t* firstTokenPtr): TokenList_t(MODULE_PARAM, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single entry inside a "pools:" section.
 */
//--------------------------------------------------------------------------------------------------
struct Pool_t : TokenList_t
{
    Pool_t(Token_t* firstTokenPtr): TokenList_t(POOL, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Section or named item that contains a list of compound items as content.
 * An example is the "processes:" section of the .adef file.
 */
//--------------------------------------------------------------------------------------------------
struct CompoundItemList_t : public CompoundItem_t
{
    /// Constructor.
    CompoundItemList_t(Type_t contentType, Token_t* firstTokenPtr);

    // Content accessors.
    const std::string& Name() const { return firstTokenPtr->text; }
    void AddContent(CompoundItem_t* contentPtr);
    const std::vector<CompoundItem_t*>& Contents() const { return contents; }

private:

    /// List of significant things in the item.
    /// Excludes whitespace, comments, separators and braces.  In fact, all tokens are at least
    /// one level deeper than the items in this list.
    std::vector<CompoundItem_t*> contents;
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a compound section (whose content is enclosed in curly braces) that contains a
 * more complex structure than just a list of tokens.
 */
//--------------------------------------------------------------------------------------------------
struct ComplexSection_t : CompoundItemList_t
{
    ComplexSection_t(Token_t* firstTokenPtr): CompoundItemList_t(COMPLEX_SECTION, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a named entry for a given application in an "apps:" section of a .sdef file.
 */
//--------------------------------------------------------------------------------------------------
struct App_t : CompoundItemList_t
{
    App_t(Token_t* firstTokenPtr): CompoundItemList_t(APP, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a named entry for a given application in an "kernelModules:" section of a .sdef file.
 */
//--------------------------------------------------------------------------------------------------
struct Module_t : CompoundItemList_t
{
    Module_t(Token_t* firstTokenPtr): CompoundItemList_t(MODULE, firstTokenPtr) {}
};

//--------------------------------------------------------------------------------------------------
/**
 * Represents a named entry for a given application in "kernelModules:" section of a .sdef file,
 * "requires: kernelModules:" secion of a .adef/.mdef/.cdef file.
 */
//--------------------------------------------------------------------------------------------------
struct RequiredModule_t : TokenList_t
{
    RequiredModule_t(Token_t* firstTokenPtr): TokenList_t(REQUIRED_MODULE, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Represents a named entry for a given network link in a "networks:" section of a .ndef file.
 */
//--------------------------------------------------------------------------------------------------
struct NetLink_t : TokenList_t
{
    NetLink_t(Token_t* firstTokenPtr): TokenList_t(NET_LINK, firstTokenPtr) {}
};


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new TokenList_t object of a given type.
 *
 * @return a pointer to the new object.
 */
//--------------------------------------------------------------------------------------------------
TokenList_t* CreateTokenList
(
    Content_t::Type_t contentType, ///< The type of object to create.
    Token_t* firstTokenPtr  ///< Pointer to the first token in this part of the parse tree.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts from a pointer to a Content_t into a pointer to a SimpleSection_t.
 *
 * @return The same pointer, but with the type converted.
 *
 * @throw mk::Exception_t if the conversion is invalid.
 */
//--------------------------------------------------------------------------------------------------
const SimpleSection_t* ToSimpleSectionPtr
(
    const Content_t* contentItemPtr   ///< The pointer to convert.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts from a pointer to a Content_t into a pointer to a SimpleSection_t.
 *
 * @return The same pointer, but with the type converted.
 *
 * @throw mk::Exception_t if the conversion is invalid.
 */
//--------------------------------------------------------------------------------------------------
const TokenListSection_t* ToTokenListSectionPtr
(
    const Content_t* contentItemPtr   ///< The pointer to convert.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts from a pointer to a Content_t into a pointer to a ComplexSection_t.
 *
 * @return The same pointer, but with the type converted.
 *
 * @throw mk::Exception_t if the conversion is invalid.
 */
//--------------------------------------------------------------------------------------------------
const ComplexSection_t* ToComplexSectionPtr
(
    const Content_t* contentItemPtr   ///< The pointer to convert.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts from a pointer to a Content_t into a pointer to a TokenList_t.
 *
 * @return The same pointer, but with the type converted.
 *
 * @throw mk::Exception_t if the conversion is invalid.
 */
//--------------------------------------------------------------------------------------------------
const TokenList_t* ToTokenListPtr
(
    const Content_t* contentItemPtr   ///< The pointer to convert.
);


//--------------------------------------------------------------------------------------------------
/**
 * Converts from a pointer to a Content_t into a pointer to a CompoundItemList_t.
 *
 * @return The same pointer, but with the type converted.
 *
 * @throw mk::Exception_t if the conversion is invalid.
 */
//--------------------------------------------------------------------------------------------------
const CompoundItemList_t* ToCompoundItemListPtr
(
    const Content_t* contentItemPtr   ///< The pointer to convert.
);



#endif // LEGATO_DEFTOOLS_PARSE_TREE_COMPOUND_ITEM_H_INCLUDE_GUARD
