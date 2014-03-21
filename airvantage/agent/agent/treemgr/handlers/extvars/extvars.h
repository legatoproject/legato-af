/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Fabien Fleutot     for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef EXTVARS_H_
#define EXTVARS_H_

#include "returncodes.h"

/* The data types which can be handled by the treemgr variables */
typedef enum ExtVars_type_t {
  EXTVARS_TYPE_STR,
  EXTVARS_TYPE_INT,
  EXTVARS_TYPE_DOUBLE,
  EXTVARS_TYPE_BOOL,
  EXTVARS_TYPE_NIL,
  EXTVARS_TYPE_END
} ExtVars_type_t;

/* Variable identifiers */
typedef int ExtVars_id_t;


/*** Handler lifecycle ***/

/* Initialize the handler
 * @return RC_OK, RC_UNSPECIFIED_ERROR */
rc_ReturnCode_t ExtVars_initialize (void);


/* Prototype of the notification function.
 * The notification function must be called every time one of the registered variables changes.
 * It is not provided directly, but passed to the handler during initialization, to ease the
 * implementation of handlers as dynamically loaded libraries (cf. ExtVars_set_notifier for details).
 *
 * It takes an opaque handler context, as well as a list of the registered variables whose values changed.
 * A reference to the notification function will be passed, when the handler is initialized, through a
 * call to the `set_notifier` API entry.
 *
 * The handler must call this notification function every time a registered variable value changes.
 *
 * @param ctx    an opaque pointer, given through ExtVars_set_notifier, to be passed to the notification callback at each call.
 * @param nvars  number of notified variables
 * @param vars   array of `nvars` variable ids whose change must be notified.
 * @param values array of `nvars` values of the notified variables 
 * @param types  array of `nvars` types of the notified variables
 *
 * @return RC_OK, RC_BAD_PARAMETER */
typedef rc_ReturnCode_t ExtVars_notify_t(void *ctx, int nvars, ExtVars_id_t* vars, void** values, ExtVars_type_t* types);

/* Pass the notification function to the handler.
 *
 * The handler must call a notification function every time a registered variable's value changes.
 * But if handlers had a direct dependency to a public ExtVars_notify() function, building them as DLL would
 * become difficult and/or non-portable.
 *
 * To side-step this issue, handlers must provide a `set_notifier` function, whose purpose is to receive a pointer to
 * the notifier when the handler is initialized. Up to the handler to keep this function pointer and call it when appropriate.
 *
 * @param ctx an opaque pointer, to be passed to ExtVars's notification callback at each call
 * @param notifier address of the notification function
 */
void ExtVars_set_notifier (void *ctx, ExtVars_notify_t *notifier);


/* Register or unregister for notification on one variable
 * @param var the variable id
 * @param enable register for notification if true, unregister if false
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND */
rc_ReturnCode_t ExtVars_register_variable(ExtVars_id_t var, int enable);

/* Register or unregister for notification on all variables
 * @param enable register for notification if true, unregister if false
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND */
rc_ReturnCode_t ExtVars_register_all(int enable);

/*
 * Retrieve the content of a variable.
 *
 * The resources necessary to store the value must be allocated, if necessary, by the callback.
 * They must remain available at least until the `get_variable_release` callback is called.
 *
 * It is guaranteed that when a second call to `get_variable` is performed, any resource
 * returned by the previous calls can be safely freed. It is therefore acceptable to clean up
 * resources at the beginning of a `get_variable` rather than in the `get_variable_release`
 * callback.
 *
 * @param var identifier of the variable to retrieve
 * @param value (output) value of the retrieved variable
 * @param type  (output) type of the retrieved variable
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_FOUND */
rc_ReturnCode_t ExtVars_get_variable(ExtVars_id_t var, void **value, ExtVars_type_t *type);


/* Called by ExtVars after it has stopped needing the results of a `get_variable` callback;
 *  Allows to clean up resources needed to maintain those results valid. */
rc_ReturnCode_t ExtVars_get_variable_release(ExtVars_id_t var, void *value, ExtVars_type_t type);

/* List all the variables identifiers handled by the handler.
 *
 * The resources necessary to store the `vars` table must be allocated by the callback,
 * and must remain available at least until the `list_release` callback is called.
 *
 * It is guaranteed that when a second call to `list` is performed, any resource
 * returned by the previous calls can be safely freed. It is therefore acceptable to clean up
 * resources at the beginning of a `list` rather than in the `list_release` callback.
 *
 * @param nvars where the number of variables must be written
 * @param vars must be made to point to an array of `nvars` variables.
 * @return RC_OK, RC_BAD_PARAMETER
 */
rc_ReturnCode_t ExtVars_list(int* nvars, ExtVars_id_t** vars);

/* If provided, called when the handler stopped needing a list of variables
 * passed to ExtVars_list(). Allows to clean up any dynamically allocated resource. */
void ExtVars_list_release(int nvars, ExtVars_id_t *vars);


/* Set the value of several variables.
 *
 * The content of tables will remain available until the set_variables callback returns;
 * the callback is not responsible for freeing any resource it didn't create.
 *
 * @param nvars  number of written variables
 * @param vars   array of the written  variables names
 * @param values values to write in the variables
 * @param types  types of the written variables
 * @return RC_OK, RC_BAD_PARAMETER, RC_NOT_PERMITTED, RC_NOT_FOUND
 */
rc_ReturnCode_t ExtVars_set_variables(int nvars, ExtVars_id_t *vars, void** values, ExtVars_type_t* types);

#endif /* EXTVARS_H_ */
