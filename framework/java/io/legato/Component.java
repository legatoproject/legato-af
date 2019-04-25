//--------------------------------------------------------------------------------------------------
/** @file Component.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

//--------------------------------------------------------------------------------------------------
/**
 * An abstract implementation for Legato components.
 */
// --------------------------------------------------------------------------------------------------
public abstract class Component {
	private Logger logger;
	private final Map<Class<?>, Object> services = new HashMap<>();

	final public void setLogger(Logger logger) {
		this.logger = logger;
	}

	final public Logger getLogger() {
		if (logger == null) {
			throw new IllegalArgumentException("Logger is not initialized");
		}
		return logger;
	}

	final public <T> void registerService(Class<T> serviceInterface, T implementation) {
		services.put(serviceInterface, implementation);
	}

	@SuppressWarnings("unchecked")
	final public <T> T getService(Class<T> serviceInterface) {
		if (!services.containsKey(serviceInterface)) {
			throw new IllegalArgumentException("Component does not require api: " + serviceInterface.getName());
		}
		Object out = services.get(serviceInterface);
		if (!serviceInterface.isInstance(out)) {
			throw new IllegalStateException("Wrong implementation for service: " + serviceInterface.getName());
		}
		return (T) services.get(serviceInterface);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * This method is called once all auto init services have been connected and the
	 * Legato event loop has been started.
	 */
	// ----------------------------------------------------------------------------------------------
	public abstract void componentInit();
}
