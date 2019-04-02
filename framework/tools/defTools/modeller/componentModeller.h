//--------------------------------------------------------------------------------------------------
/**
 * @file componentModeller.h
 *
 * Functions offered by the component modeller to other modeller modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_COMPONENT_MODELLER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_COMPONENT_MODELLER_H_INCLUDE_GUARD

namespace modeller
{


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a directory specified by a given
 * FILE_PATH token.
 *
 * @return Pointer to the component object, or NULL if the token specifies an empty environment
 *         variable.
 *
 * @throw mk::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const parseTree::Token_t* tokenPtr,
    const mk::BuildParams_t& buildParams,
    const std::list<std::string>& preSearchDirs ///< Dirs to search before buildParams source dirs
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a directory specified by a given
 * FILE_PATH token.
 *
 * @return Pointer to the component object, or NULL if the token specifies an empty environment
 *         variable.
 *
 * @throw mk::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const parseTree::Token_t* tokenPtr,
    const mk::BuildParams_t& buildParams,
    const std::list<std::string>& preSearchDirs, ///< Dirs to search before buildParams source dirs
    bool isStandAloneComp
);


} // namespace modeller

#endif // LEGATO_DEFTOOLS_COMPONENT_MODELLER_H_INCLUDE_GUARD
