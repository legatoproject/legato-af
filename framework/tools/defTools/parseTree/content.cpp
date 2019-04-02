//--------------------------------------------------------------------------------------------------
/**
 * @file content.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parseTree
{



//--------------------------------------------------------------------------------------------------
/**
 * Get a human-readable name of a given content item type.
 *
 * @return The name.
 */
//--------------------------------------------------------------------------------------------------
std::string Content_t::TypeName
(
    Content_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    switch (type)
    {
        case TOKEN:
            return "token";

        case SIMPLE_SECTION:
            return "simple section";

        case TOKEN_LIST_SECTION:
            return "token list section";

        case COMPLEX_SECTION:
            return "complex section";

        case BUNDLED_FILE:
            return "bundled file";

        case BUNDLED_DIR:
            return "bundled dir";

        case REQUIRED_FILE:
            return "required file";

        case REQUIRED_DIR:
            return "required dir";

        case REQUIRED_DEVICE:
            return "required device";

        case PROVIDED_API:
            return "provided API";

        case REQUIRED_API:
            return "required API";

        case REQUIRED_CONFIG_TREE:
            return "required configuration tree";

        case REQUIRED_MODULE:
            return "required module";

        case EXTERN_API_INTERFACE:
            return "external API interface";

        case BINDING:
            return "binding";

        case COMMAND:
            return "command";

        case EXECUTABLE:
            return "executable";

        case RUN_PROCESS:
            return "process to be run";

        case ENV_VAR:
            return "environment variable";

        case MODULE_PARAM:
            return "module parameter";

        case POOL:
            return "pool";

        case APP:
            return "app";

        case MODULE:
            return "module";

        case NET_LINK:
            return "net link";

        case REQUIRED_COMPONENT:
            return "required component";

        default:
        {
            std::stringstream typeName;
            typeName << "<out-of-range:" << type << ">";
            return typeName.str();
        }
    }
}



} // namespace parseTree
