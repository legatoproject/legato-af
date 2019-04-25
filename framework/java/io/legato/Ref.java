//--------------------------------------------------------------------------------------------------
/** @file Ref.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * Simple reference type to help support API out parameters.
 */
// -------------------------------------------------------------------------------------------------
public class Ref<RefType> {
	private RefType value;

	public Ref() {
		this(null);
	}

	public Ref(RefType newValue) {
		value = newValue;
	}

	public RefType getValue() {
		return value;
	}

	public void setValue(RefType newValue) {
		value = newValue;
	}

	public boolean isSet() {
		return value != null;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((value == null) ? 0 : value.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		Ref<?> other = (Ref<?>) obj;
		if (value == null) {
			if (other.value != null)
				return false;
		} else if (!value.equals(other.value))
			return false;
		return true;
	}
}
