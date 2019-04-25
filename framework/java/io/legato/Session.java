//--------------------------------------------------------------------------------------------------
/** @file Session.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * Base class for the Client and Server session objects. This class holds the
 * functionality that is common to both.
 */
// -------------------------------------------------------------------------------------------------
class Session implements AutoCloseable {
	/**
	 * The Legato reference to the session object in question.
	 */
	private long sessionRef;

	// ---------------------------------------------------------------------------------------------
	/**
	 * Create a new session object wrapper from the native Legato reference.
	 *
	 * @param newRef
	 *            A preexisting session reference to use.
	 */
	// ---------------------------------------------------------------------------------------------
	Session(long newRef) {
		sessionRef = newRef;
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Construct a new session using the protocol and name.
	 *
	 * @param protocol
	 *            Use this protocol to communicate with the clients of this service.
	 * @param serviceName
	 *            Advertise the service using this name.
	 */
	// ---------------------------------------------------------------------------------------------
	public Session(Protocol protocol, String serviceName) {
		sessionRef = LegatoJni.CreateSession(protocol.getRef(), serviceName);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Close the current session.
	 */
	// ---------------------------------------------------------------------------------------------
	@Override
	public void close() {
		if (sessionRef != 0) {
			LegatoJni.CloseSession(sessionRef);
		}
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Delete the session ref and free the underlying resources.
	 */
	// ---------------------------------------------------------------------------------------------
	protected void deleteRef() {
		LegatoJni.DeleteSessionRef(sessionRef);
		sessionRef = 0;
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Construct a new message for communicating on the service.
	 *
	 * @return The message ready to populate and send to the client.
	 */
	// ---------------------------------------------------------------------------------------------
	public Message createMessage() {
		return new Message(LegatoJni.CreateMessage(sessionRef));
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Get the protocol associated with this session.
	 *
	 * @return The object that represents the session's protocol.
	 */
	// ---------------------------------------------------------------------------------------------
	public Protocol getProtocol() {
		return new Protocol(LegatoJni.GetSessionProtocol(sessionRef));
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Get the service associated with this session.
	 *
	 * @return A service object that represents the server side of the connection.
	 */
	// ---------------------------------------------------------------------------------------------
	// public Service getService()
	// {
	// return new Service(LegatoJni.GetSessionService(sessionRef));
	// }

	// ---------------------------------------------------------------------------------------------
	/**
	 * Internal method to get the wrapper objects native Legato reference.
	 *
	 * @return The reference used by this object to work with Legato C runtime
	 *         library.
	 */
	// ---------------------------------------------------------------------------------------------
	long getRef() {
		return sessionRef;
	}
}
