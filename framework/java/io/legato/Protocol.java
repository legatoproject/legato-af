//--------------------------------------------------------------------------------------------------
/** @file Protocol.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

//--------------------------------------------------------------------------------------------------
/**
 * Protocols are used to help validate if the two ends of a connection are
 * compatable with each other.
 *
 * The primary comparison is the protocol's ID, which is a computed hash based
 * off of an API. So, if the API changes, so does the hash. Thus the protocol is
 * no longer compatable. The other thing both sides need to agree on is the size
 * of the associated message buffers.
 */
// --------------------------------------------------------------------------------------------------
public class Protocol {
	/**
	 * Legato reference to the protocol object.
	 */
	private long protocolRef;

	// ----------------------------------------------------------------------------------------------
	/**
	 * Internal constructor, this will create a new wrapper object based off of a
	 * Legato reference.
	 *
	 * @param newRef
	 *            The Legato reference to wrap.
	 */
	// ----------------------------------------------------------------------------------------------
	Protocol(long newRef) {
		protocolRef = newRef;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Construct a protocol object with a protocol ID and size.
	 *
	 * @param Id
	 *            The protocol ID or hash that identifies the protocol.
	 * @param largestMessageSize
	 *            The packet size for the protocol.
	 */
	// ----------------------------------------------------------------------------------------------
	public Protocol(String Id, int largestMessageSize) {
		protocolRef = LegatoJni.GetProtocolRef(Id, largestMessageSize);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Get the ID associated with the protocol.
	 *
	 * @return The Id that the protocol was created with.
	 */
	// ----------------------------------------------------------------------------------------------
	public String getId() {
		return LegatoJni.GetProtocolIdStr(protocolRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Size of the messages expected with this protocol.
	 *
	 * @return Size in bytes of the message buffers associated with this protocol.
	 */
	// ----------------------------------------------------------------------------------------------
	public int getMaxMessageSize() {
		return LegatoJni.GetProtocolMaxMsgSize(protocolRef);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Internal method for getting the underlying Legato reference for this object.
	 *
	 * @return A Legato reference.
	 */
	// ----------------------------------------------------------------------------------------------
	long getRef() {
		return protocolRef;
	}
}
