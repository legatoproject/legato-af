//--------------------------------------------------------------------------------------------------
/** @file LegatoJni.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.io.FileDescriptor;

//--------------------------------------------------------------------------------------------------
/**
 * This class serves as the internal native code wrapper for the C layer of the
 * Legato API.
 *
 * This class is not intended to be used directly. Instead you should use the
 * other classes of this package to access the Legato APIs.
 */
// --------------------------------------------------------------------------------------------------
class LegatoJni {
	static {
		System.loadLibrary("legatoJni");
		Init();
	}

	protected static native void Init();

	// Core runtime functions.
	public static native void ScheduleComponentInit(Component component);

	public static native void RunLoop();

	// Logging functions.
	public static native LogHandler.LogHandle RegComponent(String componentName);

	public static native void ConnectToLogControl();

	public static native void LogMessage(long sessionRef, long logLevelRef, int severity, String file, String method,
			int line, String message);

	// Protocol functions.
	public static native long GetProtocolRef(String id, int largestMessageSize);

	public static native String GetProtocolIdStr(long protocolRef);

	public static native int GetProtocolMaxMsgSize(long protocolRef);

	// Session functions.
	public static native long CreateSession(long protocolRef, String serviceName);

	public static native void CloseSession(long sessionRef);

	public static native void DeleteSessionRef(long sessionRef);

	public static native long CreateMessage(long sessionRef);

	public static native long GetSessionProtocol(long sessionRef);

	public static native void OpenSessionSync(long sessionRef);

	public static native void SetSessionCloseHandler(long sessionRef, SessionEvent<?> handler);

	public static native void SetSessionReceiveHandler(long sessionRef, MessageEvent handler);

	public static native int GetClientUserId(long sessionRef);

	public static native int GetClientProcessId(long sessionRef);

	// Service functions.
	public static native long CreateService(long protocolRef, String serviceName);

	public static native void DeleteService(long serviceRef);

	public static native void AddServiceOpenHandler(long serviceRef, SessionEvent<?> handler);

	public static native void AddServiceCloseHandler(long serviceRef, SessionEvent<?> handler);

	public static native void SetServiceReceiveHandler(long serviceRef, MessageEvent handler);

	public static native void AdvertiseService(long serviceRef);

	public static native void HideService(long serviceRef);

	// Message functions.
	public static native long CreateMsg(long sessionRef);

	public static native void AddMessageRef(long messageRef);

	public static native void ReleaseMessage(long messageRef);

	public static native void Send(long messageRef);

	public static native long RequestSyncResponse(long messageRef);

	public static native void Respond(long messageRef);

	public static native boolean NeedsResponse(long messageRef);

	public static native int GetMaxPayloadSize(long messageRef);

	public static native long GetSession(long messageRef);

	public static native FileDescriptor GetMessageFd(long messageRef);

	public static native void SetMessageFd(long messageRef, FileDescriptor fd);

	public static native boolean GetMessageBool(long messageRef, int location);

	public static native void SetMessageBool(long messageRef, int location, boolean newValue);

	public static native byte GetMessageByte(long messageRef, int location);

	public static native void SetMessageByte(long messageRef, int location, byte value);

	public static native short GetMessageShort(long messageRef, int location);

	public static native void SetMessageShort(long messageRef, int location, short value);

	public static native int GetMessageInt(long messageRef, int location);

	public static native void SetMessageInt(long messageRef, int location, int value);

	public static native long GetMessageLong(long messageRef, int location);

	public static native void SetMessageLong(long messageRef, int location, long value);

	public static native double GetMessageDouble(long messageRef, int location);

	public static native void SetMessageDouble(long messageRef, int location, double value);

	public static native MessageBuffer.LocationValue GetMessageString(long messageRef, int location);

	public static native int SetMessageString(long messageRef, int location, String value, int maxByteSize);

	public static native long GetMessageLongRef(long messageRef, int location);

	public static native void SetMessageLongRef(long messageRef, int location, long longRef);
}
