 /*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "returncodes.h"
#include <string.h> // strcmp(),
#include <stdlib.h> // bsearch()


// Array containing  returncodes as string value so that returncode[code] = name
static const char *const returncode[] =
{
  // RCGEN - DO NOT REMOVE THIS LINE! Code will be generated here.
};


// Array containing returncode string and code value associated in a struct and sorted in string ascending order !
static const struct cn
{
    const rc_ReturnCode_t n;
    const char *name;
} const rc_names[] =
{
  // RCGEN - DO NOT REMOVE THIS LINE! Code will be generated here.
};



/* Converts a numeric status into a string, or returns NULL if not found. */
const char *rc_ReturnCodeToString( rc_ReturnCode_t n)
{
    n = -n;
    if (n >= 0 && n < sizeof(returncode)/sizeof(*returncode))
        return returncode[n];
    else
        return NULL;
}
static int compcn(const void *m1, const void *m2)
{
    struct cn *cn1 = (struct cn *) m1;
    struct cn *cn2 = (struct cn *) m2;
    return strcmp(cn1->name, cn2->name);
}


/* Converts a status string into a numeric code, or returns 1 if not found. */
rc_ReturnCode_t rc_StringToReturnCode( const char *name)
{
    struct cn key, *res;
    key.name = name;
    if (!name) return 1;
    res = bsearch(&key, rc_names, (sizeof(rc_names)/sizeof(*rc_names)), sizeof(*rc_names), compcn);
    if (!res)
        return 1;
    else
        return res->n;
}


 
 