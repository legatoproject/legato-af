//--------------------------------------------------------------------------------------------------
/** @file MessageEvent.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

//--------------------------------------------------------------------------------------------------
/**
 * The interface used by the server and client to publish message received
 * events. Implement this interface and pass it to a session object to start
 * receiving messages on the session.
 */
// --------------------------------------------------------------------------------------------------
public interface MessageEvent {
	// ----------------------------------------------------------------------------------------------
	/**
	 * This method is called with the message that was received from the other side
	 * of the connection.
	 *
	 * @param message
	 *            The message that was received.
	 */
	// ----------------------------------------------------------------------------------------------
	public void handle(Message message);
}
