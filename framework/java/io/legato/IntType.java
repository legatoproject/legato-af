//--------------------------------------------------------------------------------------------------
/** @file IntType.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.math.BigInteger;

public enum IntType {
	SIZE(4, true), UINT8(1, true), UINT16(2, true), UINT32(4, true), UINT64(8, true), INT8(1, false), INT16(2,
			false), INT32(4, false), INT64(8, false);

	private final int sizeOf;
	private final boolean unsigned;

	IntType(int size, boolean unsigned) {
		this.sizeOf = size;
		this.unsigned = unsigned;
	}

	/**
	 * Test if the given value is valid for {@link IntType}
	 *
	 * @param in
	 *            the value to test
	 * @return true if the value is supported by the current {@link IntType}, false
	 *         else
	 */
	public boolean isValidValue(BigInteger in) {
		if (unsigned && in.compareTo(BigInteger.ZERO) < 0) {
			return false;
		}
		return in.bitLength() <= ((sizeOf * 8) - (unsigned ? 0 : 1));
	}

	/**
	 * Retrieve {@link IntType} from API declared type
	 *
	 * @param apitType
	 *            the {@link IntType} of number as string as declared in API files
	 * @return the {@link IntType}
	 */
	public static IntType fromApiType(String apiType) {
		return IntType.valueOf(apiType.toUpperCase());
	}

	/**
	 * Retrieve {@link IntType} from size
	 *
	 * @param sizethe
	 *            size of type in bytes
	 * @param unsigned
	 *            true if the {@link IntType} is signed
	 * @return the {@link IntType}
	 */

	public static IntType fromSize(int size, boolean unsigned) {
		for (IntType out : values()) {
			if (out.sizeOf == size && out.unsigned == unsigned) {
				return out;
			}
		}
		throw new IllegalArgumentException("Unknown int type: size: " + size + ", unsigned: " + unsigned);
	}
}