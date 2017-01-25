
// -------------------------------------------------------------------------------------------------
/**
 *  @file Component.java
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 *
 */
// -------------------------------------------------------------------------------------------------

package io.legato;

import java.util.logging.Logger;



//--------------------------------------------------------------------------------------------------
/**
 *  The interface that all Legato components must implement.
 */
//--------------------------------------------------------------------------------------------------
public interface Component
{
    //----------------------------------------------------------------------------------------------
    /**
     *  This method is called once all auto init services have been connected and the Legato event
     *  loop has been started.
     */
    //----------------------------------------------------------------------------------------------
    public void componentInit();


    //----------------------------------------------------------------------------------------------
    /**
     *  Called by the framework before ComponentInit to set the active logger object.
     */
    //----------------------------------------------------------------------------------------------
    public void setLogger(Logger logger);
}
