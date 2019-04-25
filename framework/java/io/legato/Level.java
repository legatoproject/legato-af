//--------------------------------------------------------------------------------------------------
/** @file Level.java
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

package io.legato;

// -------------------------------------------------------------------------------------------------
/**
 * Custom log levels to better integrate with the Legato log.
 */
// -------------------------------------------------------------------------------------------------
public class Level extends java.util.logging.Level {
	private static final long serialVersionUID = 6691808493564990397L;

	public static Level EMERG = new Level("EMERG", 1000);
	public static Level CRIT = new Level("CRIT", 900);
	public static Level ERR = new Level("ERR", 800);
	public static Level WARN = new Level("WARN", 700);
	public static Level INFO = new Level("INFO", 600);
	public static Level DEBUG = new Level("DEBUG", 300);

	protected Level(String name, int level) {
		super(name, level);
	}
}
