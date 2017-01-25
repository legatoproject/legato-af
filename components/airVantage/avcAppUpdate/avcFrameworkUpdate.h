
//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Legato framework objects.
 *
 *    legato/0/0  -  Legato framework object, this object allows framework introspection and reset
 *                   over lwm2m.
 *
 *    legato/1/0  -  Framework update object.  This object is used to handle framework bundle
 *                   updates over lwm2m.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_AVC_FRAMEWORK_UPDATE_INCLUDE_GUARD
#define LEGATO_AVC_FRAMEWORK_UPDATE_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  Create the framework object instances.
 */
//--------------------------------------------------------------------------------------------------
void InitLegatoObjects
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Update Legato object; specifically the version.
 */
//--------------------------------------------------------------------------------------------------
void UpdateLegatoObject
(
    void
);




#endif
