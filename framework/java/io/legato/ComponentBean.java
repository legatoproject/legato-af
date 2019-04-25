//--------------------------------------------------------------------------------------------------
/** @file ComponentBean.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.MethodDescriptor;
import java.beans.Statement;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

//--------------------------------------------------------------------------------------------------
/**
 * The ComponentBean class is used by the Legato generated startup code to
 * provide introspection into a users component class.
 */
// --------------------------------------------------------------------------------------------------
public final class ComponentBean {
	/**
	 * Reference to the component bean.
	 */
	private Component componentRef;

	/**
	 * Map of methods on the bean.
	 */
	private Map<String, MethodDescriptor> methodMap;

	// ----------------------------------------------------------------------------------------------
	/**
	 * Construct a new bean info object from an existing object.
	 *
	 * @param newComponentRef
	 *            Reference to the Java object to examine.
	 */
	// ----------------------------------------------------------------------------------------------
	public ComponentBean(Component newComponentRef) throws IntrospectionException {
		componentRef = newComponentRef;
		methodMap = new HashMap<String, MethodDescriptor>();

		// Get the class info and build a map of all of the methods in the app.
		Class<?> theClass = componentRef.getClass();
		MethodDescriptor[] methods = Introspector.getBeanInfo(theClass).getMethodDescriptors();

		for (MethodDescriptor method : methods) {
			methodMap.put(method.getName(), method);
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * .
	 */
	// ----------------------------------------------------------------------------------------------
	public Component getRef() {
		return componentRef;
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Set a client API session on the object. If the named setter method does not
	 * exist, throw an exception.
	 *
	 * @param methodName
	 *            Name of the method to call as a setter.
	 * @param clientSessionRef
	 *            The API client session to set.
	 */
	// ----------------------------------------------------------------------------------------------
	public <T extends ClientSession> void setClientSession(String methodName, T clientSessionRef) throws Exception {
		if (hasMethod(methodName)) {
			new Statement(componentRef, methodName, new Object[] { clientSessionRef }).execute();
		} else {
			throw new Exception("Component class, '" + componentRef.getClass().getName()
					+ "', missing setter for client API, '" + methodName + "'.");
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Set the component's logger instance.
	 *
	 * @param logger
	 *            Instance of the logging object to set.
	 */
	// ----------------------------------------------------------------------------------------------
	public void setLogger(Logger logger) throws Exception {
		String methodName = "setLogger";

		if (hasMethod(methodName)) {
			new Statement(componentRef, methodName, new Object[] { logger }).execute();
		} else {
			throw new Exception("Component class, '" + componentRef.getClass().getName()
					+ "', missing setter for logger instance.");
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Attempt to set a server session object. If the bean ref doesn't have the
	 * appropriate setter then do nothing.
	 *
	 * @param methodName
	 *            Name of the setter method to try to call.
	 * @param serverSessionRef
	 *            The server session to set.
	 */
	// ----------------------------------------------------------------------------------------------
	public <T extends ServerSession> void trySetServerSession(String methodName, T serverSessionRef) throws Exception {
		if (hasMethod(methodName)) {
			new Statement(componentRef, methodName, new Object[] { serverSessionRef }).execute();
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Check to see of the given method exists on the bean object.
	 *
	 * @param methodName
	 *            The method we're looking to call.
	 */
	// ----------------------------------------------------------------------------------------------
	public boolean hasMethod(String methodName) {
		return methodMap.containsKey(methodName);
	}
}
