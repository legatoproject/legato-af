/** @file logDaemon.h
 *
 * This file contains the log control protocol specification.
 *
 * Log control messages have the following format:
 *
 * @verbatim
 Command ProcessName '/' ComponentName '/' CommandData
@endverbatim
 *
 * - Command = one character indicating what type of command it is.
 * - ProcessName = variable-length name, up to LIMIT_MAX_PROCESS_NAME_LEN characters.
 * - ComponentName = variable-length name, up to LIMIT_MAX_COMPONENT_NAME_LEN characters.
 * - CommandData = optional additional command data, depending on the type of command.
 *
 * The Log Control Daemon advertises two services.  One is for log clients to connect to.
 * The other is for log control tools to connect to.
 *
 * Log clients connect and send in their log session identification information in a
 * "Register" message, which includes the process name, component name, and process ID.
 * If the Log Control Daemon has non-default session control data available for that
 * session, then it will send log control commands to the log client at that time.  Also,
 * the Log Control Daemon will use log control commands to update log clients when log
 * control settings are changed by log control tools.
 *
 * @todo Change to use shared memory to control log sessions instead.
 *
 * Log tools connect and send in a log control command.  The Log Control Daemon responds
 * by sending printable strings to the log control tool.  The log control tool simply prints
 * these strings to stdout when it receives them.  The Log Control Daemon closes the IPC
 * session with the log control tool when it finishes processing the command.
 * Response strings that contain error messages always start with a "*".
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LOG_DAEMON_INCLUDE_GUARD
#define LOG_DAEMON_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * The maximum packet size in bytes of log commands.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_MAX_CMD_PACKET_BYTES        300


//--------------------------------------------------------------------------------------------------
/**
 * The log control service's well known service instance name.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CONTROL_PROTOCOL_ID         "LogControlProtocol"


//--------------------------------------------------------------------------------------------------
/**
 * The log control service's well known service instance name.  This is the service that the
 * log control tool uses.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CONTROL_SERVICE_NAME        "LogControl"


//--------------------------------------------------------------------------------------------------
/**
 * The log client service's well known service instance name.  This is the service that all
 * components use to connect to the Log Control Daemon to start receiving log filter settings
 * updates on-the-fly.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CLIENT_SERVICE_NAME         "LogClient"


// =====================================
//  COMMANDS
// =====================================

//--------------------------------------------------------------------------------------------------
/**
 * Logging commands that can be sent from the log tool to the log daemon and from the log daemon to
 * the components.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CMD_SET_LEVEL               'l' // CommandData = level string (see below)
#define LOG_CMD_ENABLE_TRACE            'e' // CommandData = keyword string
#define LOG_CMD_DISABLE_TRACE           'd' // CommandData = keyword string


//--------------------------------------------------------------------------------------------------
/**
 * Logging commands that can be sent from the components to the log daemon only.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CMD_REG_COMPONENT           'r' // CommandData = string containing the process ID.


//--------------------------------------------------------------------------------------------------
/**
 * Logging commands that can be sent from the log tool to the log daemon only.
 */
//--------------------------------------------------------------------------------------------------
#define LOG_CMD_LIST_COMPONENTS         'c' // No ProcessName, ComponentName, or CommandData
#define LOG_CMD_FORGET_PROCESS          'x' // No ComponentName or CommandData


// =========================================================================
//  LOG OUTPUT LOCATION NAMES (CommandData part of SET_OUTPUT_LOC commands)
// =========================================================================

#define LOG_OUTPUT_LOC_STDERR_STR "stderr"
#define LOG_OUTPUT_LOC_SYSLOG_STR "syslog"


#endif // LOG_DAEMON_INCLUDE_GUARD
