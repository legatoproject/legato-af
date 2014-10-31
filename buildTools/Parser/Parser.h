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
 * Parses a System Definition (.sdef) and populates a System object with the information
 * garnered.
 *
 * @note    Expects the System's definition (.sdef) file path to be set.
 */
//--------------------------------------------------------------------------------------------------
void ParseSystem
(
    System* systemPtr,                  ///< The object to be populated.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
);


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
    App* appPtr,                        ///< The object to be populated.
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
    Component* componentPtr,            ///< The Component object to populate.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add an instance of a given component to an executable.
 *
 * @return Pointer to the Component Instance object.
 */
//--------------------------------------------------------------------------------------------------
void AddComponentToExe
(
    App* appPtr,
    Executable* exePtr,
    const std::string& path,    ///< The path to the component.
    const BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a pointer to the API object for a given .api file.
 *
 * @return A pointer to the API object.
 *
 * @throw legato::Exception if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
Api_t* GetApiObject
(
    const std::string& filePath,        ///< [in] Absolute file system path to API file.
    const BuildParams_t& buildParams    ///< [in] Build parameters.
);


}   // namespace parser

}   // namespace legato

#endif // PARSER_H_INCLUDE_GUARD
