//--------------------------------------------------------------------------------------------------
/**
 * @file compoundItem.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parseTree
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
CompoundItem_t::CompoundItem_t
(
    Type_t contentType,
    Token_t* firstPtr
)
//--------------------------------------------------------------------------------------------------
:   Content_t(contentType, firstPtr->filePtr),
    firstTokenPtr(firstPtr),
    lastTokenPtr(firstPtr)
//--------------------------------------------------------------------------------------------------
{
};


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
TokenList_t::TokenList_t
(
    Type_t contentType,
    Token_t* firstPtr
)
//--------------------------------------------------------------------------------------------------
:   CompoundItem_t(contentType, firstPtr)
//--------------------------------------------------------------------------------------------------
{
};


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
CompoundItemList_t::CompoundItemList_t
(
    Type_t contentType,
    Token_t* firstPtr
)
//--------------------------------------------------------------------------------------------------
:   CompoundItem_t(contentType, firstPtr)
//--------------------------------------------------------------------------------------------------
{
};


//--------------------------------------------------------------------------------------------------
/**
 * Adds a content item to a token list.
 */
//--------------------------------------------------------------------------------------------------
void TokenList_t::AddContent
(
    Token_t* contentPtr
)
//--------------------------------------------------------------------------------------------------
{
    contents.push_back(contentPtr);

    lastTokenPtr = contentPtr;
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a content item to a compound item list.
 */
//--------------------------------------------------------------------------------------------------
void CompoundItemList_t::AddContent
(
    CompoundItem_t* contentPtr
)
//--------------------------------------------------------------------------------------------------
{
    contents.push_back(contentPtr);

    lastTokenPtr = contentPtr->lastTokenPtr;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    switch (contentType)
    {
        case Content_t::TOKEN:
            throw mk::Exception_t(LE_I18N("Internal error: TOKEN is not a TokenList_t type."));

        case Content_t::SIMPLE_SECTION:
            return new SimpleSection_t(firstTokenPtr);

        case Content_t::TOKEN_LIST_SECTION:
            return new TokenListSection_t(firstTokenPtr);

        case Content_t::COMPLEX_SECTION:
            throw mk::Exception_t(LE_I18N("Internal error: COMPLEX_SECTION is not a "
                                          "TokenList_t type."));

        case Content_t::BUNDLED_FILE:
            return new BundledFile_t(firstTokenPtr);

        case Content_t::BUNDLED_DIR:
            return new BundledDir_t(firstTokenPtr);

        case Content_t::REQUIRED_FILE:
            return new RequiredFile_t(firstTokenPtr);

        case Content_t::REQUIRED_DIR:
            return new RequiredDir_t(firstTokenPtr);

        case Content_t::REQUIRED_DEVICE:
            return new RequiredDevice_t(firstTokenPtr);

        case Content_t::PROVIDED_API:
            return new ProvidedApi_t(firstTokenPtr);

        case Content_t::REQUIRED_API:
            return new RequiredApi_t(firstTokenPtr);

        case Content_t::REQUIRED_COMPONENT:
            return new RequiredComponent_t(firstTokenPtr);

        case Content_t::REQUIRED_CONFIG_TREE:
            return new RequiredConfigTree_t(firstTokenPtr);

        case Content_t::REQUIRED_MODULE:
            return new RequiredModule_t(firstTokenPtr);

        case Content_t::EXTERN_API_INTERFACE:
            return new ExternApiInterface_t(firstTokenPtr);

        case Content_t::BINDING:
            return new Binding_t(firstTokenPtr);

        case Content_t::COMMAND:
            return new Command_t(firstTokenPtr);

        case Content_t::EXECUTABLE:
            return new Executable_t(firstTokenPtr);

        case Content_t::RUN_PROCESS:
            return new RunProcess_t(firstTokenPtr);

        case Content_t::ENV_VAR:
            return new EnvVar_t(firstTokenPtr);

        case Content_t::MODULE_PARAM:
            return new ModuleParam_t(firstTokenPtr);

        case Content_t::POOL:
            return new Pool_t(firstTokenPtr);

        case Content_t::APP:
            throw mk::Exception_t(LE_I18N("Internal error: APP is not a TokenList_t type."));

        case Content_t::MODULE:
            throw mk::Exception_t(LE_I18N("Internal error: MODULE is not a TokenList_t type."));

        case Content_t::NET_LINK:
            return new NetLink_t(firstTokenPtr);

    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: Invalid content type: %d"), contentType)
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (contentItemPtr == NULL)
    {
        return NULL;
    }

    if (contentItemPtr->type == Content_t::SIMPLE_SECTION)
    {
        return static_cast<const SimpleSection_t*>(contentItemPtr);
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: %s is not a SimpleSection_t."),
                   contentItemPtr->TypeName())
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (contentItemPtr == NULL)
    {
        return NULL;
    }

    if (contentItemPtr->type == Content_t::TOKEN_LIST_SECTION)
    {
        return static_cast<const TokenListSection_t*>(contentItemPtr);
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: %s is not a TokenListSection_t."),
                   contentItemPtr->TypeName())
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (contentItemPtr == NULL)
    {
        return NULL;
    }

    if (contentItemPtr->type == Content_t::COMPLEX_SECTION)
    {
        return static_cast<const ComplexSection_t*>(contentItemPtr);
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: %s is not a ComplexSection_t."),
                   contentItemPtr->TypeName())
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (contentItemPtr == NULL)
    {
        return NULL;
    }

    switch (contentItemPtr->type)
    {
        case Content_t::SIMPLE_SECTION:
        case Content_t::TOKEN_LIST_SECTION:
        case Content_t::BUNDLED_FILE:
        case Content_t::BUNDLED_DIR:
        case Content_t::REQUIRED_FILE:
        case Content_t::REQUIRED_DIR:
        case Content_t::REQUIRED_DEVICE:
        case Content_t::PROVIDED_API:
        case Content_t::REQUIRED_API:
        case Content_t::REQUIRED_COMPONENT:
        case Content_t::REQUIRED_CONFIG_TREE:
        case Content_t::REQUIRED_MODULE:
        case Content_t::EXTERN_API_INTERFACE:
        case Content_t::BINDING:
        case Content_t::COMMAND:
        case Content_t::EXECUTABLE:
        case Content_t::RUN_PROCESS:
        case Content_t::ENV_VAR:
        case Content_t::MODULE_PARAM:
        case Content_t::POOL:
        case Content_t::NET_LINK:
            return static_cast<const TokenList_t*>(contentItemPtr);

        case Content_t::TOKEN:
        case Content_t::COMPLEX_SECTION:
        case Content_t::APP:
        case Content_t::MODULE:
            throw mk::Exception_t(
                mk::format(LE_I18N("Internal error: %s is not a TokenList_t."),
                           contentItemPtr->TypeName())
            );
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: Invalid content type: %d"), contentItemPtr->type)
    );
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (contentItemPtr == NULL)
    {
        return NULL;
    }

    switch (contentItemPtr->type)
    {
        case Content_t::COMPLEX_SECTION:
        case Content_t::APP:
        case Content_t::MODULE:
            return static_cast<const CompoundItemList_t*>(contentItemPtr);

        case Content_t::TOKEN:
        {
            auto tokenPtr = static_cast<const Token_t*>(contentItemPtr);
            tokenPtr->ThrowException(LE_I18N("Internal error: TOKEN is not a CompoundItemList_t."));
        }

        case Content_t::SIMPLE_SECTION:
        case Content_t::TOKEN_LIST_SECTION:
        case Content_t::BUNDLED_FILE:
        case Content_t::BUNDLED_DIR:
        case Content_t::REQUIRED_FILE:
        case Content_t::REQUIRED_DIR:
        case Content_t::REQUIRED_DEVICE:
        case Content_t::PROVIDED_API:
        case Content_t::REQUIRED_API:
        case Content_t::REQUIRED_COMPONENT:
        case Content_t::REQUIRED_CONFIG_TREE:
        case Content_t::REQUIRED_MODULE:
        case Content_t::EXTERN_API_INTERFACE:
        case Content_t::BINDING:
        case Content_t::COMMAND:
        case Content_t::EXECUTABLE:
        case Content_t::RUN_PROCESS:
        case Content_t::ENV_VAR:
        case Content_t::MODULE_PARAM:
        case Content_t::POOL:
        case Content_t::NET_LINK:
        {
            auto tokenListPtr = static_cast<const TokenList_t*>(contentItemPtr);
            tokenListPtr->ThrowException(
                mk::format(LE_I18N("Internal error: %s is not a CompoundItemList_t."),
                           contentItemPtr->TypeName())
            );
        }
    }

    throw mk::Exception_t(
        mk::format(LE_I18N("Internal error: Invalid content type: %d"), contentItemPtr->type)
    );
}



} // namespace parseTree
