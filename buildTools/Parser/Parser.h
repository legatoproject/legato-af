//--------------------------------------------------------------------------------------------------
/**
 * Definition of parsing routines exported to other parts of the build tools.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PARSER_H_INCLUDE_GUARD
#define PARSER_H_INCLUDE_GUARD

namespace legato
{

namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Parses an Application Definition (.adef) and populates an App object with the information
 * garnered.
 *
 * @note    Expects the Application's Path to be set.
 */
//--------------------------------------------------------------------------------------------------
void ParseApp
(
    App& app,                           ///< The object to be populated.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
);


//--------------------------------------------------------------------------------------------------
/**
 * Parses a Component Definition (Component.cdef) and populates a Component object with the
 * information garnered.
 *
 * @note    Expects the component's name to be set.  Will search for the Component.cdef based
 *          on the component search path in the buildParams.
 */
//--------------------------------------------------------------------------------------------------
void ParseComponent
(
    Component& component,               ///< The Component object to populate.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
);


}

}

#endif // PARSER_H_INCLUDE_GUARD
