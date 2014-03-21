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
  "OK",                   //   0
  "NOT_FOUND",            //  -1
  "OUT_OF_RANGE",         //  -2
  "NO_MEMORY",            //  -3
  "NOT_PERMITTED",        //  -4
  "UNSPECIFIED_ERROR",    //  -5
  "COMMUNICATION_ERROR",  //  -6
  "TIMEOUT",              //  -7
  "WOULD_BLOCK",          //  -8
  "DEADLOCK",             //  -9
  "BAD_FORMAT",           // -10
  "DUPLICATE",            // -11
  "BAD_PARAMETER",        // -12
  "CLOSED",               // -13
  "IO_ERROR",             // -14
  "NOT_IMPLEMENTED",      // -15
  "BUSY",                 // -16
  "NOT_INITIALIZED",      // -17
  "END",                  // -18
  "NOT_AVAILABLE",        // -19
};


// Array containing returncode string and code value associated in a struct and sorted in string ascending order !
static const struct cn
{
    const rc_ReturnCode_t n;
    const char *name;
} const rc_names[] =
{
  {  -10, "BAD_FORMAT" },
  {  -12, "BAD_PARAMETER" },
  {  -16, "BUSY" },
  {  -13, "CLOSED" },
  {   -6, "COMMUNICATION_ERROR" },
  {   -9, "DEADLOCK" },
  {  -11, "DUPLICATE" },
  {  -18, "END" },
  {  -14, "IO_ERROR" },
  {  -19, "NOT_AVAILABLE" },
  {   -1, "NOT_FOUND" },
  {  -15, "NOT_IMPLEMENTED" },
  {  -17, "NOT_INITIALIZED" },
  {   -4, "NOT_PERMITTED" },
  {   -3, "NO_MEMORY" },
  {    0, "OK" },
  {   -2, "OUT_OF_RANGE" },
  {   -7, "TIMEOUT" },
  {   -5, "UNSPECIFIED_ERROR" },
  {   -8, "WOULD_BLOCK" },
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


 
 