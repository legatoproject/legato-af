//--------------------------------------------------------------------------------------------------
/** @file ClientSession.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

//--------------------------------------------------------------------------------------------------
/**
 * Used by service clients, the client session holds the functionality needed to
 * connect and disconnect from Legato services.
 */
// --------------------------------------------------------------------------------------------------
public class ClientSession extends Session implements AutoCloseable {
	// ----------------------------------------------------------------------------------------------
	/**
	 * Construct a new session from a native reference. This method is private to
	 * the package and is only intended to be used by other classes within this
	 * package.
	 *
	 * @param newRef
	 *            The Legato reference to wrap.
	 */
	// ----------------------------------------------------------------------------------------------
	ClientSession(long newRef) {
		super(newRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Construct a new session, given a protocol and a service name to connect to.
	 *
	 * @param protocol
	 *            The protocol to use in this session.
	 * @param serviceName
	 *            The name of the service to connect to.
	 */
	// ----------------------------------------------------------------------------------------------
	public ClientSession(Protocol protocol, String serviceName) {
		super(protocol, serviceName);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Open the session and wait for the service to connect.
	 */
	// ----------------------------------------------------------------------------------------------
	public void open() {
		LegatoJni.OpenSessionSync(getRef());
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Close the session, and free the session's internal data.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void close() {
		long serviceRef = getRef();

		if (serviceRef != 0) {
			super.close();
			deleteRef();
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Close the session, and free the session's internal data.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	protected void finalize() throws Throwable {
	    close();
	    super.finalize();
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Attach an event handler to the session's close event.
	 *
	 * @param handler
	 *            The handler object that will be called when the event is received.
	 */
	// ----------------------------------------------------------------------------------------------
	public void setCloseHandler(SessionEvent<ClientSession> handler) {
		LegatoJni.SetSessionCloseHandler(getRef(), handler);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Attach an event handler for the sessions message received event.
	 *
	 * @param handler
	 *            The handler object that will be called when the event is received.
	 */
	// ----------------------------------------------------------------------------------------------
	public void setReceiveHandler(MessageEvent handler) {
		LegatoJni.SetSessionReceiveHandler(getRef(), handler);
	}
}
