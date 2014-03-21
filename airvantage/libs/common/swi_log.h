/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Romain Perier for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/**
* @file
* @brief This API provides logging and debugging facilities.
*
* <HR>
*/

#ifndef SWI_LOG_H
#define SWI_LOG_H

#ifndef SWI_LOG_ENABLED
#define SWI_LOG_ENABLED 1
#endif

/**
 * A Macro function which must be used in programs to log messages.
 * This one can be disabled if the macro constant SWI_LOG_ENABLED is set to 0
 **/
#if SWI_LOG_ENABLED
#define SWI_LOG(module, info, format, ...)        \
  swi_log_trace(module, info, format, ##__VA_ARGS__)
#else
#define SWI_LOG(module, info, format, ...)
#endif

/**
 * swi_log_level_t:
 * Level of verbosity for a message.
 **/
typedef enum
{
  /// There is no severity, this is least significant severity
  NONE = 0,
  /// This is an error, use this severity to log critical messages
  ERROR,
  /// This is a warning, use this severity to catch user's attention
  WARNING,
  /// This is an information, consider this severity as a normal message
  INFO,
  /// This is a detail, with a such severity, the message does not display important or critical informations
  DETAIL,
  // This is a debugging output, use this severity to deliver technical informations or implementations detailed about the program behavior
  DEBUG,
  /// This severity is mainly useful to change the level of verbosity for a module, when you want to logs all kind of messages.
  /// Do not use this severity in order to log a message, it does not make sense. This is the most important severity.
  ALL
} swi_log_level_t;

typedef void (*swi_log_displaylogger_t)(const char *module, swi_log_level_t severity, const char *message);
typedef void (*swi_log_storelogger_t)(const char *module, swi_log_level_t severity, const char *message);

/**
 * displaylogger:
 * The logging function used internally to display messages on screen.
 **/
extern swi_log_displaylogger_t displaylogger;

/**
 * storelogger:
 * The logging function used internally to store messages on a disk or in a database.
 **/
extern swi_log_storelogger_t storelogger;

/**
* Change the level of verbosity for a list of modules.
*
* This function changes the level of verbosity for a list of modules. The list must be
* terminated by NULL. If <B>module</B> is NULL, this functions changes the default level of verbosity (#WARNING as default),
* ie the level used by modules which have not changed their level yet. If you plan to change the verbosity dynamically,
* you can also set the environnement variable SWI_LOG_VERBOSITY, which must contain a list of modules names concatenated with
* the required verbosity for the module or "default" to change the default verbosity level.
* Typically, if you want to change the verbosity for the modules "foo" and "bar" to info and debug, you need to set the variable like this:
* @code
* export SWI_LOG_VERBOSITY="foo:info,bar:debug"
* @endcode
*
* @param[in]  level  The required level of verbosity for the modules given in parameters
* @param[in]  module The module for which the level must be changed
*/
void swi_log_setlevel(swi_log_level_t level, const char *module, ...);


/**
* Determine if a module with a given severity can logs messages.
*
* This function determines if <B>module</B> with the given severity can logs
* messages through the logging messages framework. Basically, <B>severity</B> is compared
* to the module's verbosity, this one can display messages only when severity <= verbosity.
*
* @param[in] module The module which plans to log messages.
* @param[in] severity The severity which is going to be used by the module
* @return 1 if the module can logs messages through the logging messages framework, 0 otherwise.
* @see swi_log_setlevel
*/
int  swi_log_musttrace(const char *module, swi_log_level_t severity);

/**
 * Submit a message the logging messages framework, to display it.
 *
 * This functions submits a message to be displayed to the logging framework only if the module with the given severity is
 * authorized to logs messages (the function swi_log_musttrace returns 1). The message is formated according to <B>format</B> and the list of
 * arguments (like printf). When the message is logged, it is displayed on the screen using the logging function #displaylogger and can be stored
 * on a disk or a remote device using the logging function #storelogger (NULL by default).
 *
 * @param[in] module The module which plans to log messages
 * @param[in] severity The severity for the message
 * @param[in] format The message itself with formatting characters if required
 * @see swi_log_musttrace
 */
void swi_log_trace(const char *module, swi_log_level_t severity, const char *format, ...);

/**
 * Change the default rule to format messages
 *
 * This functions changes the default rule used to format messages before sending them to #displaylogger and #storelogger.
 * Basically, it is used to let the user personnalize the output. With such a feature, the user can choose the order for displaying
 * the message itself, the module name, the severity, the current time, or don't display them at all.
 *
 * Special characters used to describe the format are the following:
 * - \%t: display the current time
 * - \%m: display the module name
 * - \%s: display the severity
 * - \%l: display the message itself
 *
 *
 * The default format used by default is "%t %m-%s: %l", so for example, if you call the following macro:
 * @code
 * SWI_LOG("MY_MODULE", ERROR, "CRITICAL ERROR\n")
 * @endcode
 *
 * the resulting output will be:
 * @code
 * 2012-10-17 17:44:56 MY_MODULE-ERROR: CRITICAL ERROR
 * @endcode
 *
 * or it could be:
 * @code
 * !! MY_MODULE: CRITICAL_ERROR !!
 * @endcode
 *
 * if the format was "!! %m: %l !!"
 *
 * @param[in] format The new format which will be used to personnalize the output before logging the message
 */
void swi_log_setformat(const char *format);

#endif
