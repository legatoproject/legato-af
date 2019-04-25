//--------------------------------------------------------------------------------------------------
/** @file Service.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * Legato servers create Service objects to handle advertising their services
 * and managing client connections.
 *
 * Typically a server will create a Service object, and then use it to advertise
 * or hide the service that the server plans to offer.
 */
// -------------------------------------------------------------------------------------------------
public class Service implements AutoCloseable {
	/**
	 * Native Legato reference to the underlying service object.
	 */
	private long serviceRef;

	// ---------------------------------------------------------------------------------------------
	/**
	 * Internal constructor to create a new wrapper object for the Legato reference.
	 *
	 * @param newRef
	 *            The Legato object to wrap.
	 */
	// ---------------------------------------------------------------------------------------------
	Service(long newRef) {
		serviceRef = newRef;
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Create a new service for the given protocol, when the service is advertised
	 * to the system. use the given serviceName.
	 *
	 * @param protocol
	 *            Use this protocol to communicate with the clients of this service.
	 * @param serviceName
	 *            Advertise the service using this name.
	 */
	// ---------------------------------------------------------------------------------------------
	public Service(Protocol protocol, String serviceName) {
		serviceRef = LegatoJni.CreateService(protocol.getRef(), serviceName);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Close and kill the service. Once done, this service object is no longer
	 * valid.
	 */
	// ---------------------------------------------------------------------------------------------
	@Override
	public void close() {
		if (serviceRef != 0) {
			LegatoJni.DeleteService(serviceRef);
			serviceRef = 0;
		}
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Close and kill the service. Once done, this service object is no longer
	 * valid.
	 */
	// ---------------------------------------------------------------------------------------------
	@Override
	protected void finalize() throws Throwable {
		close();
		super.finalize();
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Register an event handler to be notified when clients connect to this
	 * service.
	 *
	 * @param handler
	 *            The object that will receive the connection events.
	 */
	// ---------------------------------------------------------------------------------------------
	public void addOpenHandler(SessionEvent<ServerSession> handler) {
		LegatoJni.AddServiceOpenHandler(serviceRef, handler);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Register an event handler to be notified when clients disconnect from this
	 * service.
	 *
	 * @param handler
	 *            The object to be notified on disconnection events.
	 */
	// ---------------------------------------------------------------------------------------------
	public void addCloseHandler(SessionEvent<ServerSession> handler) {
		LegatoJni.AddServiceCloseHandler(serviceRef, handler);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Register an event handler to be notified whenever messages are received from
	 * connected clients of this service.
	 *
	 * @param handler
	 *            The object to be notified when messages arrive.
	 */
	// ---------------------------------------------------------------------------------------------
	public void setReceiveHandler(MessageEvent handler) {
		LegatoJni.SetServiceReceiveHandler(serviceRef, handler);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Make this service public and allow clients to connect to it.
	 */
	// ---------------------------------------------------------------------------------------------
	public void advertiseService() {
		LegatoJni.AdvertiseService(serviceRef);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Hide this service and no longer accept new connections. However existing
	 * connections are unaffected.
	 */
	// ---------------------------------------------------------------------------------------------
	public void hideService() {
		LegatoJni.HideService(serviceRef);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Internal method to get the wrapper objects native Legato reference.
	 *
	 * @return The reference used by this object to work with Legato C runtime
	 *         library.
	 */
	// ---------------------------------------------------------------------------------------------
	long getRef() {
		return serviceRef;
	}
}
