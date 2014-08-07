/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef YAJL_HELPERS
#define YAJL_HELPERS

#define SWI_YA_ERROR 1

#define YAJL_GEN_ALLOC(gen)                                                         \
gen = yajl_gen_alloc(NULL);                                                         \
if (gen == NULL)                                                                    \
{                                                                                   \
  SWI_LOG("YAJL_HLPS", ERROR, "%s: Failed to allocate serializer\n", __FUNCTION__); \
 return RC_NO_MEMORY;                                                               \
}

#define YAJL_GEN_ELEMENT(func, id)                                                          \
do {                                                                                        \
  yajl_gen_status yres = func;                                                              \
  if (yres != yajl_gen_status_ok)                                                           \
  {                                                                                         \
    SWI_LOG("YAJL_HLPS", ERROR, "%s: %s serialization failed, res %d\n", __FUNCTION__, id, yres); \
    return RC_BAD_PARAMETER;                                                                \
  }                                                                                         \
} while(0)

#define YAJL_GEN_STRING(str, id) \
  YAJL_GEN_ELEMENT(yajl_gen_string(gen, (const unsigned char *)str, str ? strlen(str) : 0), id)

#define YAJL_GEN_INTEGER(i, id) \
  YAJL_GEN_ELEMENT(yajl_gen_integer(gen, i), id)

#define YAJL_GEN_DOUBLE(d, id) \
  YAJL_GEN_ELEMENT(yajl_gen_double(gen, d), id)

#define YAJL_GEN_FLOAT(f, id) \
  YAJL_GEN_DOUBLE(f, id)

#define YAJL_GEN_GET_BUF(payload, payloadLen)                                                       \
do {                                                                                                \
  yajl_gen_status yres = yajl_gen_get_buf(gen, (const unsigned char **)&payload, &payloadLen);      \
  if (yres != yajl_gen_status_ok)                                                                   \
  {                                                                                                 \
    SWI_LOG("YAJL_HLPS", ERROR, "%s: Failed to generate payload, res = %d\n", __FUNCTION__, yres);  \
    return RC_BAD_FORMAT;                                                                           \
  }                                                                                                 \
} while(0)

#define YAJL_TREE_PARSE(yval, payload)                                                              \
do {                                                                                                \
  yval = yajl_tree_parse(payload, NULL, 0);                                                         \
  if (yval == NULL)                                                                                 \
  {                                                                                                 \
    SWI_LOG("YAJL_HLPS", ERROR, "%s: Failed to parse payload\n", __FUNCTION__);                     \
    return RC_BAD_FORMAT;                                                                           \
  }                                                                                                 \
} while(0)

#endif
