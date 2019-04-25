//--------------------------------------------------------------------------------------------------
/** @file LogHandler.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

import java.util.logging.Formatter;
import java.util.logging.Handler;
import java.util.logging.LogRecord;

//--------------------------------------------------------------------------------------------------
/**
 * Custom log handler that is able to send log events to the Legato event log.
 */
// --------------------------------------------------------------------------------------------------
public class LogHandler extends Handler {
	/**
	 * Registered log session.
	 */
	private LogHandle session;

	// ----------------------------------------------------------------------------------------------
	/**
	 * Used to keep track of the log session and level handle for the C code.
	 */
	// ----------------------------------------------------------------------------------------------
	public class LogHandle {
		public final long sessionRef;
		public final long logLevelRef;

		LogHandle(long newSessionRef, long newLogLevelRef) {
			sessionRef = newSessionRef;
			logLevelRef = newLogLevelRef;
		}
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Create a new logging session with Legato.
	 */
	// ----------------------------------------------------------------------------------------------
	public LogHandler(String componentName) {
		session = LegatoJni.RegComponent(componentName);
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Handles the close method, we don't really support this so there's nothing to
	 * do here.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void close() throws SecurityException {
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Flush out any pending logs. The other layers handle this already, so there's
	 * nothing to do here.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void flush() {
	}

	// ----------------------------------------------------------------------------------------------
	/**
	 * Actually log an event.
	 */
	// ----------------------------------------------------------------------------------------------
	@Override
	public void publish(LogRecord record) {
		StackTraceElement[] stack = Thread.currentThread().getStackTrace();
		Formatter formatter = getFormatter();

		int recordLevel = record.getLevel().intValue();
		int legatoLevel;

		if (recordLevel <= Level.DEBUG.intValue()) {
			// LE_LOG_DEBUG
			legatoLevel = 0;
		} else if (recordLevel <= Level.INFO.intValue()) {
			// LE_LOG_INFO
			legatoLevel = 1;
		} else if (recordLevel <= Level.WARN.intValue()) {
			// LE_LOG_WARN
			legatoLevel = 2;
		} else if (recordLevel <= Level.ERR.intValue()) {
			// LE_LOG_ERR
			legatoLevel = 3;
		} else if (recordLevel <= Level.CRIT.intValue()) {
			// LE_LOG_CRIT
			legatoLevel = 4;
		} else {
			// LE_LOG_EMERG
			legatoLevel = 5;
		}

		LegatoJni.LogMessage(session.sessionRef, session.logLevelRef, legatoLevel, stack[5].getClassName(),
				stack[5].getMethodName(), stack[5].getLineNumber(), formatter.formatMessage(record));
	}
}
