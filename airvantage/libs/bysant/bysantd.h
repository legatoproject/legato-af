/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Julien Desgats for Sierra Wireless - initial API and implementation
 *     Fabien Fleutot for Sierra Wireless - initial API and implementation
 *******************************************************************************/
#ifndef __BYSANTD_H_INCLUDED__
#define __BYSANTD_H_INCLUDED__
#include "bysant.h"

#define BSD_MAX_CLASSES (16)
#define BSD_STACK_SIZE  (16)

/* If a buffer is smaller than this constant, and a deserialization
 * fails due to a too short buffer, then there is no guarantee that
 * it will return the buffer size it actually needs to read the
 * whole data.
 * Instead, it might return the buffer size needed to estimate the
 * data size, i.e. there might be two consecutive rounds of hsd_read()
 * failing due to a too short buffer.
 */
#define BSD_MINBUFFSIZE 3

/* Deserialization context. */
typedef struct bsd_ctx_t {
  size_t read; /* Number of bytes successfully read. */
  int broken; /* True when the ctx has been corrupted. */
  int stacksize; /* Number of active stack frames. */
  bs_classcoll_t classcoll; /* Known classes. */

  struct bsd_stackframe_t {
    bs_stackframekind_t kind;
    bs_ctxid_t ctxid;
    int missing; /* # of children expected before closing. */
    union bsd_stackframecontent_t {
      struct bsd_map_t {
        int even; /* whether an even # of children has been added. */
      } map;
      struct bsd_object_t { /* used for both class definition and instantiation */
        const struct bs_class_t *classdef; /* current class */
      } object;
    } content;
  } stack[BSD_STACK_SIZE];
} bsd_ctx_t;

typedef enum bsd_status_t {
  BSD_EOK = 0,
  BSD_ENOTIMPL = -1,    /* not implemented */
  BSD_EINVALID = -2,    /* invalid stream (syntax error, ...) */
  BSD_EBADCONTEXT = -3, /* Decoding context is unknown/invalid */
  BSD_EINVOPCODE = -4,  /* used an unknown opcode */
  BSD_EBADCLASSID = -5, /* used an unknown class identifier */
  BSD_ETOODEEP = -6,    /* too many nested conainers */
  BSD_EMEMORY = -7,     /* out of memory */
  BSD_EINTERNAL = -100, /* internal error, should not happen */
} bsd_status_t;

typedef struct bsd_data_t {
  enum bsd_data_type_t {
    BSD_ERROR, // 0
    BSD_CLOSE,
    BSD_NULL,
    BSD_INT,
    BSD_BOOL,
    BSD_DOUBLE, // 5
    BSD_STRING,
    BSD_CHUNKED_STRING,
    BSD_CHUNK,
    BSD_LIST,
    BSD_ZLIST, // 10
    BSD_MAP,
    BSD_ZMAP,
    BSD_OBJECT,
    BSD_CLASSDEF,
  } type;
  enum bsd_data_kind_t {
    BSD_KTOPLEVEL,
    BSD_KLISTITEM,
    BSD_KMAPKEY,
    BSD_KMAPVALUE,
    BSD_KOBJFIELD,
    BSD_KCHUNK,
    BSD_KNEWCONTAINER,
    /* FIXME: do we yield class definitions */
  } kind;
  const char *fieldname;
  union bsd_data_content_t {
    int64_t i;
    int bool;
    double d;
    size_t length;
    const bs_class_t *classdef;
    enum bsd_data_type_t cont_type;
    bsd_status_t error;
    struct {
      size_t length;
      const char *data;
    } string, chunk;
  } content;
} bsd_data_t;

/* Initialize (or reset) a deserialization context. */
void bsd_init( bsd_ctx_t *ctx);

/* Cleanup deserialization context. */
void bsd_reset( bsd_ctx_t *ctx);

/* Adds an internal class to the context. If class mode is BS_CLASS_MANAGED,
 * the the context takes the ownership of the class an it will be freed by
 * by bsd_close. Otherwise (static definition, ...), it will just add
 * pointer to known classes. See bs_classcoll_set for details.
 * Return 0 on success, non-zero on failure (most likely out of memory). */
int bsd_addClass( bsd_ctx_t *ctx, const bs_class_t *classdef);

/* Attempt to read an Bysant element from buffer into x.
 *
 * If the buffer length is negative, the caller guarantees that enough
 * data has been provided in the buffer; no length verification will be made.
 *
 * Upon success, return the number of bytes consumed from data. This number
 * can be 0 (when fixed containers are closed).
 *
 * Upon failure due to insufficient data, return the opposite of the
 * number of bytes required. For instance, a result of -32 means that
 * at least 32 bytes of data were required, and that the 'length'
 * parameter was positive and less than 32.
 * However, this estimation cannot be always reliable and more data could
 * be necessary (worst case is the class definition which need a possibly
 * large amount of bytes without producing any value).
 *
 * Upon failure to deserialize data for other reasons, 0 will be returned,
 * and the data type in 'x' will be set to BSD_ERROR.
 *
 * Structure of x; x->type describes the element type:
 *  - BSD_ERROR: cannot deserialize; x->content.error holds the reason.
 *  - BSD_NULL: null
 *  - BSD_INT: integer; x->content.i holds the value.
 *  - BSD_BOOL: boolean; x->content.bool holds the value.
 *  - BSD_DOUBLE: IEEE double; x->content.d holds the value.
 *  - BSD_STRING: string. The size is in x->content.string.length,
 *                the bytes are in x->content.string.data.
 *  - BSD_CHUNKED_STRING: start of a sequence of <BSD_CHUNK> closed by
 *                        a <BSD_CLOSE> token.
 *  - BSD_CHUNK: piece of string, x->content.chunk holds the value,
 *               fields are same as <BSD_STRING>
 *  - BSD_LIST: fixed-size list. The size is in x->content.length.
 *              The end of the list will be signaled by <BSD_CLOSE>..
 *  - BSD_ZLIST: unknown-size list. The end of the list will be marked by
 *               an BSD_CLOSE element.
 *  - BSD_MAP: map; The size is in x->content.length.
 *             Content will be streamed in order:
 *             key_1, value_1, ..., key_n, value_n, <BSD_CLOSE>.
 *             The end of the list will be marked by a <BSD_CLOSE> token.
 *  - BSD_ZMAP: unknown-size map. See <BSD_MAP> and <BSD_ZLIST>.
 *  - BSD_OBJECT: object; the object class is in x->content.classdef.
 *                Fields will be streamed in the predefined order.
 *                The end of the list will be signaled by <BSD_CLOSE>.
 *  - BSD_CLASSDEF: custom class definition: the class structure is in
 *                  x->content.classdef.
 *  - BSD_CLOSE: Mark the end of a container. x->content.cont_type
 *               contains the kind of container just closed.
 * x->kind describes what the element is (the content is undefined for
 * <BSD_ERROR> and <BSD_CLOSE> tokens:
 *  - BSD_KTOPLEVEL: element is defined on the top level context
 *  - BSD_KLISTITEM: element is a item inside a list
 *  - BSD_KMAPKEY: element is a key of a map
 *  - BSD_KMAPVALUE: element is a value of a map
 *  - BSD_KOBJFIELD: element is a field inside an object (in this case
 *                   fieldname may contain the field name or NULL)
 *  - BSD_KCHUNK: element is a string chunk.
 *  - BSD_KNEWCONTAINER: element is a newly opened container.
 * x->fieldname is defined only for objects, it contains the name of
 * the object field if it is known, NULL otherwise.
 */
int bsd_read( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);

//TODO: should be a compile option
#include <stdio.h>
/* Debug function: prints the opcodes and misc info about serialized chunk on given file.
 * Also prints errors on output, if any.
 */
void bsd_dump( bsd_ctx_t *ctx, FILE *output, const uint8_t *buffer, int length);

#endif /* __BYSANTD_H_INCLUDED__ */
