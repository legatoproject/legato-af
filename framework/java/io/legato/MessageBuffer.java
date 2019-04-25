//--------------------------------------------------------------------------------------------------
/** @file MessageBuffer.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.io.FileDescriptor;
import java.math.BigInteger;

//--------------------------------------------------------------------------------------------------
/**
 * This class handles the raw data access to a message's data payload.
 *
 * The get and set methods act as a streaming operation. The buffer maintains an
 * internal pointer that is updated as values are read and written.
 */
// --------------------------------------------------------------------------------------------------
public class MessageBuffer implements AutoCloseable {
	/**
	 * The message object that owns this buffer.
	 */
	private Message hostMessage;

	/**
	 * The current buffer insertion location. Reads and writes start from and update
	 * this location.
	 */
	private int location;

	// ----------------------------------------------------------------------------------------------
	/**
	 * Package internal class used to keep track of the amount of bytes variable
	 * length data structures require when being written into the data buffer.
	 */
	// ----------------------------------------------------------------------------------------------
	class LocationValue {
		/**
		 * The message object that owns this buffer.
		 */
		public final int bytesUsed;

		/**
		 * The message object that owns this buffer.
		 */
		public final Object value;

		// ------------------------------------------------------------------------------------------
		/**
		 * Construct a new location value.
		 *
		 * @param newBytesUsed
		 *            The number of bytes needed to store the object.
		 * @param newValue
		 *            The object itself.
		 */
		// ------------------------------------------------------------------------------------------
		LocationValue(int newBytesUsed, Object newValue) {
			bytesUsed = newBytesUsed;
			value = newValue;
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Internal constructor used to create a new buffer object from a message
	 * object.
	 *
	 * @param message
	 *            The message object to construct from.
	 */
	// ----------------------------------------------------------------------------------------------
	MessageBuffer(Message message) {
		hostMessage = message;
		location = 0;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Close and free the message buffer.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void close() {
		hostMessage = null;
		location = 0;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Reset the buffer read/write location back to the beginning.
	 */
	// ----------------------------------------------------------------------------------------------
	public void resetPosition() {
		location = 0;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a boolean value from the message buffer.
	 *
	 * @return A boolean value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public boolean readBool() {
		boolean result = LegatoJni.GetMessageBool(hostMessage.getRef(), location);
		location += 1;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a boolean value to the current buffer location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeBool(boolean newValue) {
		LegatoJni.SetMessageBool(hostMessage.getRef(), location, newValue);
		location += 1;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a {@link FileDescriptor} from the buffer.
	 *
	 * @return The {@link FileDescriptor} read from the buffer.
	 */
	// ----------------------------------------------------------------------------------------------
	public FileDescriptor getFileDescriptor() {
		return LegatoJni.GetMessageFd(hostMessage.getRef());
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a {@link FileDescriptor} into the buffer.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void setFileDescriptor(FileDescriptor fd) {
		LegatoJni.SetMessageFd(hostMessage.getRef(), fd);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a byte from the buffer at it's current location.
	 *
	 * @return The byte read from the buffer.
	 */
	// ----------------------------------------------------------------------------------------------
	public byte readByte() {
		byte result = LegatoJni.GetMessageByte(hostMessage.getRef(), location);
		location += 1;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a byte into the buffer at it's current location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeByte(byte newValue) {
		LegatoJni.SetMessageByte(hostMessage.getRef(), location, newValue);
		location += 1;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a short integer from the buffer at it's current location.
	 *
	 * @return A short value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public short readShort() {
		short result = LegatoJni.GetMessageShort(hostMessage.getRef(), location);
		location += 2;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a short integer to the buffer at it's current location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeShort(Short newValue) {
		LegatoJni.SetMessageShort(hostMessage.getRef(), location, newValue);
		location += 2;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read an integer from the buffer at it's current location.
	 *
	 * @return An integer value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public int readInt() {
		int result = LegatoJni.GetMessageInt(hostMessage.getRef(), location);
		location += 4;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write an integer to the buffer at it's current location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeInt(int newValue) {
		LegatoJni.SetMessageInt(hostMessage.getRef(), location, newValue);
		location += 4;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a long integer from the buffer at it's current location.
	 *
	 * @return A long value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public long readLong() {
		long result = LegatoJni.GetMessageLong(hostMessage.getRef(), location);
		location += 8;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a long integer to the buffer at it's current location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeLong(long newValue) {
		LegatoJni.SetMessageLong(hostMessage.getRef(), location, newValue);
		location += 8;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a floating point value from the buffer at it's current location.
	 *
	 * @return A double value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public double readDouble() {
		double result = LegatoJni.GetMessageDouble(hostMessage.getRef(), location);
		location += 8;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a floating point value to the buffer at it's current location.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeDouble(double newValue) {
		LegatoJni.SetMessageDouble(hostMessage.getRef(), location, newValue);
		location += 8;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a NULL terminated utf-8 encoded string from the buffer at it's current
	 * location.
	 *
	 * @return A string value from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public String readString() {
		LocationValue result = LegatoJni.GetMessageString(hostMessage.getRef(), location);

		location += result.bytesUsed;
		return (String) result.value;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Write a string into the buffer as a utf-8 NULL terminated string.
	 *
	 * @param newValue
	 *            The value to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeString(String strValue, int maxSize) {
		location += LegatoJni.SetMessageString(hostMessage.getRef(), location, strValue, maxSize);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a Legato reference from the buffer at it's current location.
	 *
	 * @return A Legato reference from the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public long readLongRef() {
		long result = Integer.toUnsignedLong(LegatoJni.GetMessageInt(hostMessage.getRef(), location));
		location += 4;

		return result;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Read a Legato reference to the buffer at it's current location.
	 *
	 * @param longRef
	 *            The ref to write at the current buffer location.
	 */
	// ----------------------------------------------------------------------------------------------
	public void writeLongRef(long longRef) {
		if (longRef > Integer.MAX_VALUE || longRef < Integer.MIN_VALUE || (((longRef & 0x01) == 0) && longRef != 0)) {
			throw new IllegalArgumentException("Illegal reference");
		}

		LegatoJni.SetMessageInt(hostMessage.getRef(), location, (int) longRef);
		location += 4;
	}

	/**
	 * Read a number from the buffer
	 *
	 * @param type
	 *            The {@link IntType} to read
	 * @return the value
	 */
	public BigInteger readNumber(IntType type) {
		switch (type) {
		case INT8:
			return new BigInteger(Byte.toString(readByte()));
		case UINT8:
			return new BigInteger(Integer.toString(Byte.toUnsignedInt(readByte())));
		case INT16:
			return new BigInteger(Short.toString(readShort()));
		case UINT16:
			return new BigInteger(Integer.toString(Short.toUnsignedInt(readShort())));
		case INT32:
			return new BigInteger(Integer.toString(readInt()));
		case SIZE:
		case UINT32:
			return new BigInteger(Integer.toUnsignedString(readInt()));
		case INT64:
			return new BigInteger(Long.toString(readLong()));
		case UINT64:
			return new BigInteger(Long.toUnsignedString(readLong()));
		}
		throw new IllegalStateException();
	}

	/**
	 * Write a {@link IntNumber} to the buffer at it's current location.
	 *
	 * @param type
	 *            the {@link IntType} of the number
	 * @param number
	 *            The value to write at the current buffer location.
	 */
	public void writeNumber(IntType type, BigInteger number) {
		if (!type.isValidValue(number)) {
			throw new IllegalArgumentException("Invalid value for " + type.name() + ": " + number);
		}
		switch (type) {
		case INT8:
		case UINT8:
			writeByte(number.byteValue());
			break;
		case INT16:
		case UINT16:
			writeShort(number.shortValue());
			break;
		case SIZE:
		case INT32:
		case UINT32:
			writeInt(number.intValue());
			break;
		case INT64:
		case UINT64:
			writeLong(number.longValue());
			break;
		}
	}
}
