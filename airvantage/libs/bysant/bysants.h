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
#ifndef __BYSANTS_H_INCLUDED__
#define __BYSANTS_H_INCLUDED__

/* Bysant serialization library. */

#include "bysant.h"
#include <stdint.h>

/* Error codes. */
typedef enum bss_status_t {
  BSS_EOK = 0, /* Success. */
  BSS_EAGAIN = 1, /* Writer overflow, retry later. */
  BSS_ETOODEEP = (-2), /* More than BSS_STACK_SIZE nested containers. */
  BSS_EINVALID = (-3), /* Invalid operation. */
  BSS_EMEM = (-4), /* Not enough memory. (deprecated?). */
  BSS_EBROKEN = (-5), /* Stream corrupted by some previous error. */
  BSS_ENOCONTAINER = (-6), /* Trying to close when no container is open. */
  BSS_EBADMAP = (-7), /* Odd number of children in a map, must be even. */
  BSS_ESIZE = (-8), /* # of declared children doesn't match # of children actually inserted. */
  BSS_EBADCTXID = (-9), /* Unknown context id (when declaring a container with a decoding context) */
  BSS_EBADCONTEXT = (-10), /* Cannot serialize object in current context. */
  BSS_EBADCLASSID = (-11), /* Unknown class ID */
  BSS_EBADFIELD = (-12), /* Wrong context id or field name not set. */
  BSS_EOUTOFBOUNDS = (-13), /* Too large or too small number. */
  BSS_EINTERNAL = (-100)
/* Internal bug, should never happen. */
} bss_status_t;

/* Callback which consumes the outgoing Hessian stream.
 * Such a callback must be given to each streaming context
 * at its creation.
 *
 * data:       Chunk of data to be processed.
 * len:        Number of bytes in 'data'.
 * writerctx:  A custom context, given at the streaming context
 *             creation, which can be used freely by the callback.
 * returns:    The number of bytes successfully processed.
 *             If this number is less than 'len', then the stream
 *             processor is considered in overflow.
 *
 * If the writer gets in overflow, it is the user's responsibility
 * to retry the serialization operation that failed due to overflow
 * when there are reasons to believe that the writer will accept more
 * data.
 *
 * When a serialization operation fails due to overflow, it might still
 * have been partially written. Therefore, once the overflow is over,
 * the same serialization operation must be retried. If an attempt is
 * made to serialize something else than what caused the overflow,
 * the serialization output stream might produce arbitrary garbage.
 */
typedef int bss_writer_t( unsigned const char *data, int len, void *writerctx);

#define BSS_STACK_SIZE  16 /* Max number of nested containers. */
#define BSS_MAX_CLASSES 16 /* Max number of classes. */
#define BS_MAX_CLASS_FIELDS 32 /* Max number of class field. */

/* Serialization context. */
typedef struct bss_ctx_t {
  bss_writer_t *writer; /* Custom stream consuming function. */
  void *writerctx; /* Extra argument to the writer.  */
  int written; /* Number of bytes successfully written. */
  int acknowledged;
  int skipped;
  int broken; /* True when the ctx has been corrupted. */
  int stacksize; /* Number of active stack frames. */
  int acknowledged_stacksize; /* # of stack frames at last completed transaction. */
  bs_classcoll_t classcoll; /* known classes */

  struct bss_stackframe_t {
    bs_stackframekind_t kind;
    bs_ctxid_t ctxid;
    int missing; /* # of children expected before closing for sized containers. */
    /* Extra data for some containers. */
    union bss_stackframecontent_t {
      struct bss_map_t {
        int even; /* whether an even # of children has been added. */
      } map;
      struct bss_object_t { /* used for instanciation */
        const bs_class_t *class; /* current class */
      } object;
    } content;
  } stack[BSS_STACK_SIZE];
} bss_ctx_t;

/* Initialize a streaming context.
 * ctx: address of the structure to be initialized;
 * writer: streaming callback;
 * writerctx: context of the streaming callback;
 */
void bss_init( bss_ctx_t *ctx, bss_writer_t *writer, void *writerctx);

/* Reset or close an already initialized context. */
void bss_reset( bss_ctx_t *ctx);

/* Start an object.
 * Class must be declared beforehand (except for predefined class).
 * All field contents must then be serialized in sequence after this call;
 * After the last element has been serialized, bss_close() must be called
 * to mark the object's end.
 *
 * classid: the object type identifier;
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_object( bss_ctx_t *ctx, bs_classid_t classid);

/* Adds an internal class to the context. If class mode is BS_CLASS_MANAGED,
 * the the context takes the ownership of the class an it will be freed by
 * by bss_close. Otherwise (static definition, ...), it will just add
 * pointer to known classes. See bs_classcoll_set for details.
 * If internal is non-zero, the class will *not* be sent on stream therefore
 * the class must be known by the deserializer too for decoding.
 * In case of error, the context will not take the ownership of managed classes.
 * Return 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_class( bss_ctx_t *ctx, const bs_class_t *classdef, int internal);

/* Start a map container.
 * Keys and values must be serialized in sequence after this call:
 * key1, value1, key2, value2, ..., key_n, value_n. Note that keys
 * can only be unsigned integers or strings (any other value will
 * result in a BSS_EBADCONTEXT error).
 * After the last value has been serialized, bss_close() must be called
 * to mark the map's end (even for fixed length maps).
 * 'ctxid' is the context in which values are encoded (keys are always
 * encoded in BS_CTXID_UNSIGNED_OR_STRING). Can be set to BS_CTXID_GLOBAL
 * if unspecified.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_map( bss_ctx_t *ctx, int len, bs_ctxid_t type);

/* Start a list container.
 * List elements must be serialized in sequence after this call;
 * After the last element has been serialized, bss_close() must be called
 * to mark the list's end (even for fixed length lists).
 * 'len' is the number of elements to be put in the list, or can be
 * negative is the number isn't known in advance.
 * 'ctxid' is the context in which elements are encoded. Can be set to
 * BS_CTXID_GLOBAL if unspecified.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_list( bss_ctx_t *ctx, int len, bs_ctxid_t ctxid);

/* Start a long string serialization.
 * This call must be followed by calls to bss_chunk(), which will pass
 * the string piece by piece. Pieces are of arbitrary sizes.
 * The end of the string must be marked with a call to bss_close().
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_chunked( bss_ctx_t *ctx);

/* Add data to serialize in a chunked string or a chunked binary.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_chunk( bss_ctx_t *ctx, void *data, size_t len);

/* Mark the end of structures: lists, maps and chunked data.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_close( bss_ctx_t *ctx);

/* Serialize an integer.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_int( bss_ctx_t *ctx, int64_t x);

/* Serialize an Boolean.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_bool( bss_ctx_t *ctx, int x);

/* Serialize a floating point number.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_double( bss_ctx_t *ctx, double x);

/* Serialize a string of length 'len'.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_lstring( bss_ctx_t *ctx, const char *str, size_t len);

/* Serialize a '\0' terminated string.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error, or a writer-returned negative error code. */
bss_status_t bss_string( bss_ctx_t *ctx, const char *str);

/* Serialize a NULL.
 * returns: 0 on success, BSS_EAGAIN in case of writer overflow,
 * another BSS_EXXX error (in particular, it will result in BSS_EINVALID
 * if it is used inside a variable length list/map or as map key), or a
 * writer-returned negative error code. */
bss_status_t bss_null( bss_ctx_t *ctx);

/* Inject literal data in the output stream.
 * This allows e.g. to put a value already serialized in a container. */
bss_status_t bss_raw( bss_ctx_t *ctx, const unsigned char *data, size_t len);

#endif /* __BYSANTS_H_INCLUDED__ */
