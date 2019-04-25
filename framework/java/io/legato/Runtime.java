//--------------------------------------------------------------------------------------------------
/** @file Runtime.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

//--------------------------------------------------------------------------------------------------
/**
 * This class is used to directly work with the Legato runtime.
 */
// --------------------------------------------------------------------------------------------------
public class Runtime {
	// ----------------------------------------------------------------------------------------------
	/**
	 * .
	 */
	// ----------------------------------------------------------------------------------------------
	public static void connectToLogControl() {
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Used to queue a Legato component's init function to run once the event loop
	 * as been started.
	 *
	 * @param component
	 *            The component to run.
	 */
	// ----------------------------------------------------------------------------------------------
	public static void scheduleComponentInit(Component component) {
		LegatoJni.ScheduleComponentInit(component);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Kick off the Legato event loop in the current thread. This method will not
	 * return from the loop.
	 */
	// ----------------------------------------------------------------------------------------------
	public static void runEventLoop() {
		LegatoJni.RunLoop();
	}
}
