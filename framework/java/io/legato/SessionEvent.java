//--------------------------------------------------------------------------------------------------
/** @file SessionEvent.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * Object that handles events that come in for a session.
 */
// -------------------------------------------------------------------------------------------------
public interface SessionEvent<SessionType> {
	// ---------------------------------------------------------------------------------------------
	/**
	 * The handle event is called for events that happen on a session.
	 *
	 * @param session
	 *            The session that the event occurred on.
	 */
	// ---------------------------------------------------------------------------------------------
	public void handle(SessionType session);
}
