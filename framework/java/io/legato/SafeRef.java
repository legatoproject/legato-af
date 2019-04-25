//--------------------------------------------------------------------------------------------------
/** @file SafeRef.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.security.SecureRandom;

// -------------------------------------------------------------------------------------------------
/**
 * The SafeRef class is used to translate between Java object pointers and
 * Legato style references that can safely be passed across process boundaries.
 *
 * @code MyDataObject obj = new MyDataObject(...); long newRef =
 *       myRefMap.newRef(obj);
 *
 *       PassRefToClient(newRef); @endcode
 *
 *       Later when the client returns the ref for further processing, the ref
 *       is found using the get method. If the ref is invalid then a null object
 *       is returned.
 */
// -------------------------------------------------------------------------------------------------
public class SafeRef<RefType> {
	/**
	 * Internal map used to map between IDs and Java objects.
	 */
	private Map<Long, RefType> refMap;

	/**
	 * We need to keep track of the next ID we will be handing out.
	 *
	 * Even though we hand out longs, reference is in range of integer, so use an
	 * integer internally.
	 */
	private int nextRef;

	// ---------------------------------------------------------------------------------------------
	/**
	 * Construct a new RefMap.
	 */
	// ---------------------------------------------------------------------------------------------
	public SafeRef() {
		// Create our hash map, and acquire a random starting point to make it harder to
		// accidentally use a ref from a different map.
		refMap = new HashMap<Long, RefType>();
		nextRef = new SecureRandom().nextInt() | 1;
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Take the given object and add it to our map, returning it's new ID.
	 *
	 * @param newObject
	 *            The object to map.
	 *
	 * @return The reference ID for the object.
	 */
	// ---------------------------------------------------------------------------------------------
	public long newRef(RefType newObject) {
		// Use the current ref ID and then advance by 2 in order to keep the refs on an
		// odd number
		// making it unlikely to mistake them for a pointer.
		long thisRef = nextRef;

		refMap.put(thisRef, newObject);
		nextRef += 2;

		return thisRef;
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Look up the reference in the map.
	 *
	 * @param ref
	 *            The safe reference to look up in the map.
	 *
	 * @return The original Java object is returned, or null if the ref is invalid.
	 */
	// ---------------------------------------------------------------------------------------------
	public RefType get(long ref) {
		return refMap.get(ref);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Remove a ref from the map, thereby invalidating it.
	 *
	 * @param ref
	 *            The safe reference to remove from the map.
	 */
	// ---------------------------------------------------------------------------------------------
	public void remove(long ref) {
		refMap.remove(ref);
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Clear out all of the objects protected by the ref map. Once done all current
	 * refs are invalid.
	 */
	// ---------------------------------------------------------------------------------------------
	public void clear() {
		refMap.clear();
	}

	// ---------------------------------------------------------------------------------------------
	/**
	 * Get the current collection of objects backed by the safe ref map.
	 *
	 * @return The collection of objects protected by SafeRefs.
	 */
	// ---------------------------------------------------------------------------------------------
	public Collection<RefType> getObjects() {
		return refMap.values();
	}
}
