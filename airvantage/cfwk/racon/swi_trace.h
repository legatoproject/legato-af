/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/


/**
 * @file
 * @brief Trace management
 *
 * This header provides a trace function, which implementation will depend on hardware device.
 * @ingroup common
 */





#ifndef SWI_TRACE_H_
#define SWI_TRACE_H_

#define SWI_ENABLE_TRACE

#ifndef SWI_ENABLE_TRACE
#define SWI_TRACE(lvl, fmt, ...)


#else //SWI_ENABLE_TRACE



/** \def SWI_TRACE(lvl, fmt, ...)
 *  \brief Trace function
 *  \param lvl level used to determine if the trace has to be done. Level must be an integer different of 0 to activate the trace.
 *  \param fmt the format string, like in stdio functions
 *  \param ... the variable arguments to build the string to trace.
 *
 * @note the level param name will be printed at the beginning of the trace line. SWI_TRACE call automatically creates a new line at the end.
 * \code
 * //For instance, using level created like that:
 * #define DEBUG_LEVEL_EXEMPLE 1
 * //used in a call like this one
 * SWI_TRACE(DEBUG_LEVEL_EXEMPLE, "my format string with a number: [%d]", 42);
 * //will produce:
 * DEBUG_LEVEL_EXEMPLE       my format string with a number: [42]
 * \endcode
 */

#define printer printf

#define SWI_TRACE(lvl, fmt, ...) do { if (lvl) printer(#lvl":\t"fmt"\n", ## __VA_ARGS__); } while(0)



#endif // not SWI_ENABLE_TRACE
#endif //SWI_TRACE_H_

