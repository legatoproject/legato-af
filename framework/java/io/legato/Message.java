//--------------------------------------------------------------------------------------------------
/** @file Message.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

//--------------------------------------------------------------------------------------------------
/**
 * This class represents a Legato message.
 */
// --------------------------------------------------------------------------------------------------
public class Message implements AutoCloseable {
	/**
	 * Internal native message reference. This is used by the C code to refer to the
	 * actual message object.
	 */
	private long messageRef;

	// ----------------------------------------------------------------------------------------------
	/**
	 * Package private way to construct a Message from a native reference.
	 *
	 * @param newRef
	 *            The native reference to the underlying message object.
	 */
	// ----------------------------------------------------------------------------------------------
	Message(long newRef) {
		messageRef = newRef;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Close and free the message object and its data.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void close() {
		if (messageRef != 0) {
			LegatoJni.ReleaseMessage(messageRef);
			messageRef = 0;
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Close and free the message object and its data.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	protected void finalize() throws Throwable {
		close();
		super.finalize();
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Send this message out across the attached session. Do not wait for a
	 * response.
	 */
	// ----------------------------------------------------------------------------------------------
	public void send() {
		LegatoJni.Send(messageRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Send this message out across the attached session. This will not return until
	 * a response has been received from the other side.
	 *
	 * @return A message object that holds the other side's response.
	 */
	// ----------------------------------------------------------------------------------------------
	public Message requestResponse() {
		return new Message(LegatoJni.RequestSyncResponse(messageRef));
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Reuse this message object to respond to the original query.
	 */
	// ----------------------------------------------------------------------------------------------
	public void respond() {
		LegatoJni.Respond(messageRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Check the message to see if a response was requested.
	 *
	 * @return A true if a response was needed, false if not.
	 */
	// ----------------------------------------------------------------------------------------------
	public boolean needsResponse() {
		return LegatoJni.NeedsResponse(messageRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Get the size of the message payload buffer.
	 *
	 * @return The number of bytes that this message object can transmit.
	 */
	// ----------------------------------------------------------------------------------------------
	public int getMaxPayloadSize() {
		return LegatoJni.GetMaxPayloadSize(messageRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Get the client session attached to this message.
	 *
	 * @return The session object that the message was created on.
	 */
	// ----------------------------------------------------------------------------------------------
	public ClientSession getClientSession() {
		return new ClientSession(LegatoJni.GetSession(messageRef));
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Get a server session for this message.
	 *
	 * @return The server session object that this message was created on.
	 */
	// ----------------------------------------------------------------------------------------------
	public ServerSession getServerSession() {
		return new ServerSession(LegatoJni.GetSession(messageRef));
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Get the data buffer attached to this message.
	 *
	 * @return A buffer that holds the data for this message.
	 */
	// ----------------------------------------------------------------------------------------------
	public MessageBuffer getBuffer() {
		return new MessageBuffer(this);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Internal method, this is used to get the native reference for the underlying
	 * Legato object.
	 *
	 * @return A reference to the Legato object.
	 */
	// ----------------------------------------------------------------------------------------------
	public long getRef() {
		return messageRef;
	}
}
