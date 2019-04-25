//--------------------------------------------------------------------------------------------------
/** @file Result.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.util.HashMap;
import java.util.Map;

//--------------------------------------------------------------------------------------------------
/**
 * This enum is used to define the standard result codes.
 */
// --------------------------------------------------------------------------------------------------
public enum Result {
	OK(0), /// < Successful.
	NOT_FOUND(-1), /// < Referenced item does not exist or could not be found.
	NOT_POSSIBLE(-2), /// < @deprecated It is not possible to perform the requested action.
	OUT_OF_RANGE(-3), /// < An index or other value is out of range.
	NO_MEMORY(-4), /// < Insufficient memory is available.
	NOT_PERMITTED(-5), /// < Current user does not have permission to perform requested action.
	FAULT(-6), /// < Unspecified internal error.
	COMM_ERROR(-7), /// < Communications error.
	TIMEOUT(-8), /// < A time-out occurred.
	OVERFLOW(-9), /// < An overflow occurred or would have occurred.
	UNDERFLOW(-10), /// < An underflow occurred or would have occurred.
	WOULD_BLOCK(-11), /// < Would have blocked if non-blocking behaviour was not requested.
	DEADLOCK(-12), /// < Would have caused a deadlock.
	FORMAT_ERROR(-13), /// < Format error.
	DUPLICATE(-14), /// < Duplicate entry found or operation already performed.
	BAD_PARAMETER(-15), /// < Parameter is invalid.
	CLOSED(-16), /// < The resource is closed.
	BUSY(-17), /// < The resource is busy.
	UNSUPPORTED(-18), /// < The underlying resource does not support this operation.
	IO_ERROR(-19), /// < An IO operation failed.
	NOT_IMPLEMENTED(-20), /// < Unimplemented functionality.
	UNAVAILABLE(-21), /// < A transient or temporary loss of a service or resource.
	TERMINATED(-22); /// < The process, operation, data stream, session, etc. has stopped.

	int value;

	Result(int newValue) {
		value = newValue;
	}

	public int getValue() {
		return value;
	}

	private static final Map<Integer, Result> valueMap = new HashMap<Integer, Result>();

	static {
		for (Result item : Result.values()) {
			valueMap.put(item.value, item);
		}
	}

	public static Result fromInt(int newValue) {
		return valueMap.get(newValue);
	}
}
