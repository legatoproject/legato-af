/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef __MEM_DEFINE_H_INCLUDED__
#define __MEM_DEFINE_H_INCLUDED__

#include <stdlib.h>
#define MEM_ALLOC( x) malloc( x)
#define MEM_FREE( x)  free( x)
#define MEM_REALLOC( x, t) realloc( x, t)
#define MEM_CALLOC( x, y) calloc( x, y)

#endif
