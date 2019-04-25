//--------------------------------------------------------------------------------------------------
/** @file ServerSession.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * This represents the server end of a client/server connection.
 */
// -------------------------------------------------------------------------------------------------
public class ServerSession extends Session {
	// ---------------------------------------------------------------------------------------------
	/**
	 * Internal method to create a new server session wrapper from a Legato
	 * reference.
	 *
	 * @param newRef
	 *            The reference to wrap.
	 */
	// ---------------------------------------------------------------------------------------------
	ServerSession(long newRef) {
		super(newRef);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Get the Posix user ID for the process on the other end of the connection.
	 *
	 * @return The OS level ID for the user that started the process on the other
	 *         end of the connection.
	 */
	// ---------------------------------------------------------------------------------------------
	public long getClientUserId() {
		return LegatoJni.GetClientUserId(getRef());
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Used by the server to read the OS ID of the process on the other end of this
	 * connection.
	 *
	 * @return The Posix process ID of the program on the other end of this
	 *         connection.
	 */
	// ---------------------------------------------------------------------------------------------
	public long getClientProcessId() {
		return LegatoJni.GetClientProcessId(getRef());
	}
}
