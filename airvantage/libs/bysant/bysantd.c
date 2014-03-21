
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
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h> // for memcpy
#include <stdlib.h>
#include <rc_endian.h>
#include "bysantd.h"

static SEndian sendian;

static struct bsd_stackframe_t *topframe( bsd_ctx_t *ctx) {
  return ctx->stack + ctx->stacksize;
}

/* returns the context for the next value to write to the frame. In particular
 * it returns correct context for map keys. */
static bs_ctxid_t getctxid( struct bsd_stackframe_t *f) {
  if( (BS_FMAP == f->kind || BS_FZMAP == f->kind) && f->content.map.even) {
    return BS_CTXID_UNSIGNED_OR_STRING; /* map key */
  }
  if( BS_FOBJECT == f->kind) {
    const bs_class_t *class = f->content.object.classdef;
    return class->fields[class->nfields - f->missing].ctxid;
  }
  return f->ctxid;
}

static enum bsd_data_kind_t bsd_getFrameDataKind( struct bsd_stackframe_t *f) {
  switch( f->kind) {
  case BS_FCLASSDEF:
  case BS_FTOP:     return BSD_KTOPLEVEL;
  case BS_FMAP:
  case BS_FZMAP:    return f->content.map.even ? BSD_KMAPVALUE : BSD_KMAPKEY;
  case BS_FOBJECT:  return BSD_KOBJFIELD;
  case BS_FLIST:
  case BS_FZLIST:   return BSD_KLISTITEM;
  case BS_FCHUNKED: return BSD_KCHUNK;
  }
  return BSD_KTOPLEVEL; /* should never happen */
}

static enum bsd_data_type_t bsd_typeFromFrameKind( bs_stackframekind_t k) {
  switch( k) {
  //case BS_FTOP:
  case BS_FMAP:     return BSD_MAP;
  case BS_FZMAP:    return BSD_ZMAP;
  case BS_FOBJECT:  return BSD_OBJECT;
  case BS_FLIST:    return BSD_LIST;
  case BS_FZLIST:   return BSD_ZLIST;
  case BS_FCHUNKED: return BSD_CHUNKED_STRING;
  //case BS_FCLASSDEF:
  default:          return BSD_ERROR;
  }
}

static void bsd_string( bsd_data_t *x, size_t len, const char *data) {
  x->type = BSD_STRING;
  x->content.string.length = len;
  x->content.string.data = data;
}

static int bsd_error( bsd_data_t *x, enum bsd_status_t cause) {
  x->type = BSD_ERROR;
  x->content.error = cause;
  return 0;
}

/* Adds a frame to the decoding stack. */
static bsd_status_t openContainer(
    bsd_ctx_t *ctx,
    bs_stackframekind_t kind,
    bs_ctxid_t ctxid,
    int missing,
    union bsd_stackframecontent_t **c) {

  struct bsd_stackframe_t *f;
  if( (BSD_STACK_SIZE-1) == ctx->stacksize) return BSD_ETOODEEP;
  ctx->stacksize++;
  f = topframe( ctx);
  f->kind = kind;
  f->ctxid = ctxid;
  f->missing = missing;
  // both key and value count for maps
  if( BS_FMAP == kind || BS_FZMAP == kind) {
    f->missing *= 2;
    f->content.map.even = 1; /* will be inverted */
  }
  if( c)
    *c = &f->content;
  return BSD_EOK;
}

static bsd_status_t bsd_object( bsd_ctx_t *ctx, bsd_data_t *x, bs_classid_t classid) {
  const bs_class_t *klass = bs_classcoll_get( &ctx->classcoll, classid);
  union bsd_stackframecontent_t *f = NULL;
  bsd_status_t status;
  if( NULL == klass) {
    return BSD_EBADCLASSID;
  }
  x->type = BSD_OBJECT;
  x->content.classdef = klass;
  status = openContainer( ctx, BS_FOBJECT, BS_CTXID_GLOBAL, klass->nfields, &f);
  f->object.classdef = klass;
  return status;
}

/* Return a failure to read if there aren't at least n bytes;
 * set nread to this value, stating that n bytes have been read, otherwise. */
#define CHECK_LENGTH( n) if( 0<=length && length<(n)) return -(n); else nread=(n)

/* Return an error with the result of the expression it is not evaluated to BSD_EOK. */
#define CHECK_ERROR( expr) do { \
    bsd_status_t _r = (expr);   \
    if( _r != BSD_EOK) return bsd_error( x, _r); \
  } while( 0)

/* Return nread if the decoding function call `expr` succeeded,
 * Return an error if the decoding function returned an error
 * Leave the calling function go on if the decoding function didn't consume
 * any data but didn't cause an error either. */
#define CHECK_DECODE( expr) do { \
    int _nread = (expr); \
    if( _nread != 0) return _nread; \
    if( BSD_ERROR == x->type) return 0; \
  } while( 0)

/* TODO: explain how this works. */
#define CHECK_SUBDECODE( expr, expectedtype) do { \
    int _subread = (expr); \
    if( _subread <= 0) return -nread - _subread; \
    if( (expectedtype) != x->type) return bsd_error( x, BSD_EINVALID); \
    nread += _subread; \
  } while( 0)


// decoding functions for each context
static int bsd_global( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_chunked( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_uis( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_number( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_int32( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_float( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_double( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);
static int bsd_listmap( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length);

/*
 * Decoding helpers
 */

/**
 * Try to decode a number under given context.
 * This algorithm assume that all number kind are consecutive and in the same order.
 */
static int decodeInteger( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length,
    const bs_integer_encoding_t *enc) {
  int nread = 0;
  uint8_t opcode = buffer[0];
  CHECK_LENGTH( 1);

  x->type = BSD_INT;
  if( (enc->tiny_zero_opcode + enc->tiny_min) <= opcode && opcode <= (enc->tiny_zero_opcode + enc->tiny_max)) {
    /* tiny number */
    x->content.i = opcode - enc->tiny_zero_opcode;
  } else if( enc->small_pos_opcode <= opcode && opcode < enc->small_neg_opcode) {
    /* small positive number */
    CHECK_LENGTH( 2);
    x->content.i = ((opcode - enc->small_pos_opcode) << 8) + buffer[1] + enc->tiny_max + 1;
  } else if( enc->small_neg_opcode <= opcode && opcode < enc->medium_pos_opcode) {
    /* small negative number */
    CHECK_LENGTH( 2);
    x->content.i = -(((opcode - enc->small_neg_opcode) << 8) + buffer[1]) + enc->tiny_min - 1;
  } else if( enc->medium_pos_opcode <= opcode && opcode < enc->medium_neg_opcode) {
    /* medium positive number */
    CHECK_LENGTH( 3);
    x->content.i = ((opcode - enc->medium_pos_opcode) << 16) + (buffer[1] << 8) + buffer[2] + enc->small_max + 1;
  } else if( enc->medium_neg_opcode <= opcode && opcode < enc->large_pos_opcode) {
    /* medium negative number */
    CHECK_LENGTH( 3);
    x->content.i = -(((opcode - enc->medium_neg_opcode) << 16) + (buffer[1] << 8) + buffer[2]) + enc->small_min - 1;
  } else if( enc->large_pos_opcode <= opcode && opcode < enc->large_neg_opcode) {
    /* large positive opcode */
    CHECK_LENGTH( 4);
    x->content.i = ((opcode - enc->large_pos_opcode) << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3] + enc->medium_max + 1;
  } else if( enc->large_neg_opcode <= opcode && opcode <= enc->last_large_neg_opcode) {
    /* large negative opcode */
    CHECK_LENGTH( 4);
    x->content.i = -(((opcode - enc->large_neg_opcode) << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3]) + enc->medium_min - 1;
  } else if( enc->int32_opcode == opcode) {
    /* big endian 32 bits integer */
    CHECK_LENGTH( 5);
    int32_t val;
    memcpy( &val, buffer + 1, sizeof(int32_t));
    ntoh( &val, sizeof(int32_t), sendian.int32_);
    x->content.i = val;
  } else if( enc->int64_opcode == opcode) {
    /* big endian 64 bits integer */
    CHECK_LENGTH( 9);
    memcpy( &(x->content.i), buffer + 1, sizeof(int64_t));
    ntoh( &(x->content.i), sizeof(int64_t), sendian.int64_);
  } else {
    return 0;
  }

  return nread;
}

static int decodeCollection( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length, union bsd_stackframecontent_t **f, const bs_coll_encoding_t *enc) {
  int nread = 0;
  uint8_t opcode = buffer[0];
  CHECK_LENGTH( 1);

  if( enc->empty_opcode == opcode) {
    /* empty container */
    x->type = bsd_typeFromFrameKind( enc->fixed_kind);
    x->content.length = 0;
    CHECK_ERROR( openContainer( ctx, enc->fixed_kind, BS_CTXID_GLOBAL, 0, f));
  } else if( enc->small_untyped_opcode <= opcode && opcode <= enc->small_untyped_opcode + enc->small_limit - 1) {
    /* Small untyped container */
    x->type = bsd_typeFromFrameKind( enc->fixed_kind);
    x->content.length = opcode - enc->small_untyped_opcode + 1;
    CHECK_ERROR( openContainer( ctx, enc->fixed_kind, BS_CTXID_GLOBAL, x->content.length, f));
  } else if( enc->long_untyped_opcode == opcode) {
    /* Long untyped container */
    CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + 1, length + 1), BSD_INT);
    x->type = bsd_typeFromFrameKind( enc->fixed_kind);
    x->content.length = x->content.i + enc->small_limit + 1;
    CHECK_ERROR( openContainer( ctx, enc->fixed_kind, BS_CTXID_GLOBAL, x->content.length, f));
  } else if( enc->variable_untyped_opcode == opcode) {
    /* Variable untyped container */
    x->type = bsd_typeFromFrameKind( enc->variable_kind);
    CHECK_ERROR( openContainer( ctx, enc->variable_kind, BS_CTXID_GLOBAL, -1, f));
  } else if( enc->small_typed_opcode <= opcode && opcode <= enc->small_typed_opcode + enc->small_limit - 1) {
    /* Small typed container */
    CHECK_LENGTH( 2);
    x->type = bsd_typeFromFrameKind( enc->fixed_kind);
    x->content.length = opcode - enc->small_typed_opcode + 1;
    CHECK_ERROR( openContainer( ctx, enc->fixed_kind, buffer[1], x->content.length, f));
  } else if( enc->long_typed_opcode == opcode) {
    /* Long typed container */
    CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + 1, length + 1), BSD_INT);
    CHECK_LENGTH( nread + 1);
    x->type = bsd_typeFromFrameKind( enc->fixed_kind);
    x->content.length = x->content.i + enc->small_limit + 1;
    CHECK_ERROR( openContainer( ctx, enc->fixed_kind, buffer[nread-1], x->content.length, f));
  } else if( enc->variable_typed_opcode == opcode) {
    /* Variable untyped container */
    CHECK_LENGTH( 2);
    x->type = bsd_typeFromFrameKind( enc->variable_kind);
    CHECK_ERROR( openContainer( ctx, enc->variable_kind, buffer[1], -1, f));
  } else {
    return 0;
  }
  return nread;
}

static int decodeString( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length, const bs_string_encoding_t *enc) {
  int nread = 0;
  uint8_t opcode = buffer[0];
  CHECK_LENGTH( 1);

  x->type = BSD_STRING;
  if( enc->small_opcode <= opcode && opcode < enc->medium_opcode) {
    /* Small string */
    size_t len = opcode - enc->small_opcode;
    CHECK_LENGTH( len + 1);
    bsd_string( x, len, (const char *) buffer + 1);
  } else if( enc->medium_opcode <= opcode && opcode < enc->large_opcode) {
    /* Medium string */
    CHECK_LENGTH( 2);
    size_t len = ((opcode - enc->medium_opcode) << 8) + buffer[1] + enc->small_limit + 1;
    CHECK_LENGTH( len + 2);
    bsd_string( x, len, (const char *) buffer + 2);
  } else if( enc->large_opcode == opcode) {
    /* Large string */
    CHECK_LENGTH( 3);
    size_t len = ((buffer[1]) << 8) + buffer[2] + enc->medium_limit + 1;
    CHECK_LENGTH( len + 3);
    bsd_string( x, len, (const char *) buffer + 3);
  } else if( enc->chunked_opcode == opcode) {
    CHECK_ERROR( openContainer( ctx, BS_FCHUNKED, BS_CTXID_CHUNKED, -1, NULL));
    x->type = BSD_CHUNKED_STRING;
  }
  else {
    return 0;
  }
  return nread;
}

static int decodeClass( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread, i, named, fieldnread;
  size_t nfields, blocksize, namesize;
  bs_classid_t classid;
  const char *name;
  const uint8_t *fieldbuffer;
  void *endofclass;
  bs_class_t *classdef;

  CHECK_LENGTH( 1);
  switch( buffer[0]) {
  case 0x71: named = 1; break;
  case 0x72: named = 0; name = NULL; namesize = 0; break;
  default: return 0;
  }

  // decoding is done in two passes: 1st to compute the size and allocate right amount of memory,
  // second one to make the actual data copy.
  blocksize = sizeof( bs_class_t);

  CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread ), BSD_INT);
  classid = x->content.i;
  if( named) {
    CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread ), BSD_STRING);
    name = x->content.string.data;
    namesize = x->content.string.length + 1; // add null terminator
  }
  CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread), BSD_INT);
  nfields = x->content.i;

  // allocate class structure
  blocksize = sizeof( bs_class_t) + namesize + nfields * sizeof( struct bs_field_t);

  // 1st pass
  fieldbuffer = buffer;
  fieldnread = nread;
  for( i=0; i<nfields; i++) {
    if( named) {
      CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread ), BSD_STRING);
      blocksize += x->content.length + 1;
    }
    CHECK_LENGTH( nread + 1);
  }

  // memory allocation
  classdef = malloc( blocksize);
  if( NULL == classdef) return bsd_error( x, BSD_EMEMORY);
  classdef->classid = classid;
  classdef->nfields = nfields;
  classdef->mode = BS_CLASS_MANAGED;
  endofclass = ((void *)classdef) + sizeof( bs_class_t);
  classdef->classname = NULL;
  if( named) {
    memcpy( endofclass, name, namesize-1);
    ((char *)endofclass)[namesize-1] = '\0';
    classdef->classname = endofclass;
    endofclass += namesize;
  }
  classdef->fields = endofclass;
  endofclass += nfields * sizeof( struct bs_field_t);

  // 2nd pass (no possible parsing errors since processing is the same as 1st pass)
  buffer = fieldbuffer;
  nread = fieldnread;
  for( i=0; i<classdef->nfields; i++) {
    if( named) {
      CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread ), BSD_STRING);
      memcpy( endofclass, x->content.string.data, x->content.string.length);
      ((char *)endofclass)[x->content.string.length] = '\0';
      classdef->fields[i].name = endofclass;
      endofclass += x->content.string.length + 1;
    } else {
      classdef->fields[i].name = NULL;
    }
    CHECK_LENGTH( nread + 1);
    classdef->fields[i].ctxid = buffer[nread - 1];
  }

  bsd_addClass( ctx, classdef);
  x->type = BSD_CLASSDEF;
  x->content.classdef = classdef;
  return nread;
}

static void decodeNull( bsd_ctx_t *ctx, bsd_data_t *x) {
  struct bsd_stackframe_t * f = topframe( ctx);
  if( BS_FZLIST == f->kind || (BS_FZMAP == f->kind && f->content.map.even)) {
    x->type = BSD_CLOSE;
    x->content.cont_type = bsd_typeFromFrameKind( f->kind);
    ctx->stacksize--;
  } else {
    x->type = BSD_NULL;
  }
}

/*
 * Decoding routines
 */

static int bsd_global( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  CHECK_LENGTH( 1);
  uint8_t opcode = buffer[0];


  CHECK_DECODE( decodeInteger( ctx, x, buffer, length, &BS_GLOBAL_INTEGER));
  CHECK_DECODE( decodeString( ctx, x, buffer, length, &BS_GLOBAL_STRING));
  CHECK_DECODE( decodeCollection( ctx, x, buffer, length, NULL, &BS_GLOBAL_LIST));
  CHECK_DECODE( decodeCollection( ctx, x, buffer, length, NULL, &BS_GLOBAL_MAP));
  CHECK_DECODE( decodeClass( ctx, x, buffer, length));
  if( 0x60 <= opcode && opcode <= 0x6f) { /* object (short form) */
    CHECK_ERROR( bsd_object( ctx, x, opcode - 0x60));
    return nread;
  }
  switch( opcode) {
  case BS_G_NULL: /* null */
    decodeNull( ctx, x);
    break;
  case 0x01: /* boolean true */
    x->type = BSD_BOOL;
    x->content.bool = 1;
    break;
  case 0x02: /* boolean false */
    x->type = BSD_BOOL;
    x->content.bool = 0;
    break;
  case 0x70: /* object (long form) */
    CHECK_SUBDECODE( bsd_uis( ctx, x, buffer + nread, length - nread), BSD_INT);
    CHECK_ERROR( bsd_object( ctx, x, x->content.i + 0x10));
    break;
  case BS_G_FLOAT32: /* float */
    CHECK_LENGTH( 5);
    x->type = BSD_DOUBLE;
    float f;
    memcpy( &f, buffer + 1, sizeof(float));
    ntoh( &f, sizeof(float), sendian.float_);
    x->content.d = f;
    break;
  case BS_G_FLOAT64: /* double */
    CHECK_LENGTH( 9);
    x->type = BSD_DOUBLE;
    memcpy( &(x->content.d), buffer + 1, sizeof(double));
    ntoh( &(x->content.d), sizeof(double), sendian.double_);
    break;
  default: return bsd_error( x, BSD_EINVALID);
  }

  return nread;
}

static int bsd_uis( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  CHECK_LENGTH( 1);
  uint8_t opcode = buffer[0];

  CHECK_DECODE( decodeString( ctx, x, buffer, length, &BS_UIS_STRING));
  x->type = BSD_INT;
  if( 0x3b <= opcode && opcode <= 0xc6) {
    /* Tiny unsigned */
    x->content.i = opcode - 0x3b;
  } else if( 0xc7 <= opcode && opcode <= 0xe6) {
    /* Small unsigned */
    CHECK_LENGTH( 2);
    x->content.i = ((opcode - 0xc7) << 8) + buffer[1] + BS_UTI_MAX + 1;
  } else if( 0xe7 <= opcode && opcode <= 0xf6) {
    /* Medium unsigned */
    CHECK_LENGTH( 3);
    x->content.i = ((opcode - 0xe7) << 16) + (buffer[1] << 8) + buffer[2] + BS_USI_MAX + 1;
  } else if( 0xf7 <= opcode && opcode <= 0xfe) {
    /* Large unsigned */
    CHECK_LENGTH( 4);
    x->content.i = ((opcode - 0xf7) << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3] + BS_UMI_MAX + 1;
  } else if( 0xff == opcode) {
    /* XLarge unsigned */
    CHECK_LENGTH( 5);
    x->content.i = ((uint32_t)buffer[1] << 24) + ((uint32_t)buffer[2] << 16) + ((uint32_t)buffer[3] << 8) + (uint32_t)buffer[4];
  } else if( 0x00 == opcode) {
    decodeNull( ctx, x);
  } else {
    return bsd_error( x, BSD_EINVALID);
  }

  return nread;
}

static int bsd_chunked( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  CHECK_LENGTH( 2);
  size_t chunksize = (buffer[0] << 8) + buffer[1];

  if( 0 == chunksize) {
    x->type = BSD_CLOSE;
    x->content.cont_type = BSD_CHUNKED_STRING;
    ctx->stacksize--;
  } else {
    CHECK_LENGTH( chunksize + 2);
    x->type = BSD_CHUNK;
    x->content.chunk.data = (const char *) buffer + 2;
    x->content.chunk.length = chunksize;
  }

  return nread;
}

static int bsd_number( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  CHECK_LENGTH( 1);
  uint8_t opcode = buffer[0];

  CHECK_DECODE( decodeInteger( ctx, x, buffer, length, &BS_NUMBER_INTEGER));

  /* Only go on if integer decoding failed */
  switch( opcode) {

  case BS_N_NULL:
    decodeNull( ctx, x);
    break;

  case BS_N_FLOAT32:
    CHECK_LENGTH( 5);
    x->type = BSD_DOUBLE;
    float f;
    memcpy( &f, buffer + 1, sizeof(float));
    ntoh( &f, sizeof(float), sendian.float_);
    x->content.d = f;
    break;

  case BS_N_FLOAT64:
    CHECK_LENGTH( 9);
    x->type = BSD_DOUBLE;
    memcpy( &(x->content.d), buffer + 1, sizeof(double));
    ntoh( &(x->content.d), sizeof(double), sendian.double_);
    break;
  default: return bsd_error( x, BSD_EINVALID);
  }

  return nread;
}

static int bsd_int32( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  union { uint32_t u; int32_t i; } val;

  CHECK_LENGTH( 4);
  val.u = ((uint32_t)buffer[0] << 24) + ((uint32_t)buffer[1] << 16) + ((uint32_t)buffer[2] << 8) + (uint32_t)buffer[3];
  x->type = BSD_INT;
  x->content.i = val.i;

  // check for escape sequence
  if( 0x80000000U == val.u) {
    CHECK_LENGTH( 5);
    switch( buffer[4]) {
    case 0x00: decodeNull( ctx, x); break;
    case 0x01: break;
    default: return bsd_error( x, BSD_EINVALID);
    }
  }

  return nread;
}

static int bsd_float( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  union { uint32_t u; float f; } val;

  CHECK_LENGTH( 4);
  memcpy( &val.f, buffer, sizeof(float));
  ntoh( &val.f, sizeof(float), sendian.float_);
  x->content.d = val.f;
  x->type = BSD_DOUBLE;

  // check for escape sequence
  if( 0xFFFFFFFFU == val.u) {
    CHECK_LENGTH( 5);
    switch( buffer[4]) {
    case 0x00: decodeNull( ctx, x); break;
    case 0x01: break;
    default: return bsd_error( x, BSD_EINVALID);
    }
  }

  return nread;
}

static int bsd_double( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  union { uint64_t u; double d; } val;

  CHECK_LENGTH( 8);
  memcpy( &val.d, buffer, sizeof(double));
  ntoh( &val.d, sizeof(double), sendian.float_);
  x->content.d = val.d;
  x->type = BSD_DOUBLE;

  // check for escape sequence
  if( 0xFFFFFFFFFFFFFFFFULL == val.u) {
    CHECK_LENGTH( 9);
    switch( buffer[8]) {
    case 0x00: decodeNull( ctx, x); break;
    case 0x01: break;
    default: return bsd_error( x, BSD_EINVALID);
    }
  }

  return nread;
}

static int bsd_listmap( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread;
  CHECK_LENGTH( 1);
  uint8_t opcode = buffer[0];

  CHECK_DECODE( decodeCollection( ctx, x, buffer, length, NULL, &BS_LISTMAP_LIST));
  CHECK_DECODE( decodeCollection( ctx, x, buffer, length, NULL, &BS_LISTMAP_MAP));
  if( 0x00 == opcode) {
    decodeNull( ctx, x);
  } else {
    return bsd_error( x, BSD_EINVALID);
  }

  return nread;
}

/*
 * Public API
 */

int bsd_read( bsd_ctx_t *ctx, bsd_data_t *x, const uint8_t *buffer, int length) {
  int nread = 0;
  struct bsd_stackframe_t *f = topframe( ctx);
  x->type = BSD_NULL; // if x is already of type error, it will confuse error checking

  // check stack (fixed length terminated containers)
  if( f->missing == 0) {
    x->type = BSD_CLOSE;
    x->content.cont_type = bsd_typeFromFrameKind( f->kind);
    ctx->stacksize--;
    /* the other properties depends on the frame below the closed one */
    f = topframe( ctx);
    x->kind = bsd_getFrameDataKind( f);
    /* set field name for objects */
    if( BSD_KOBJFIELD == x->kind) {
      const bs_class_t *classdef = f->content.object.classdef;
      x->fieldname = classdef->fields[classdef->nfields - (f->missing + 1)].name;
    }
    return 0;
  }

  switch( getctxid( f)) {
  case BS_CTXID_GLOBAL:             nread = bsd_global( ctx, x, buffer, length);  break;
  case BS_CTXID_UNSIGNED_OR_STRING: nread = bsd_uis( ctx, x, buffer, length);     break;
  case BS_CTXID_NUMBER:             nread = bsd_number( ctx, x, buffer, length);  break;
  case BS_CTXID_INT32:              nread = bsd_int32( ctx, x, buffer, length);   break;
  case BS_CTXID_FLOAT:              nread = bsd_float( ctx, x, buffer, length);   break;
  case BS_CTXID_DOUBLE:             nread = bsd_double( ctx, x, buffer, length);  break;
  case BS_CTXID_LIST_OR_MAP:        nread = bsd_listmap( ctx, x, buffer, length); break;
  case BS_CTXID_CHUNKED:            nread = bsd_chunked( ctx, x, buffer, length); break;
//  case BS_CTXID_OBJECT:
//  case BS_CTXID_CLASSDEF:
//  case BS_CTXID_NAMEDCLASSDEF:
//  case BS_CTXID_INTERNALCLASSDEF:
  default:
    return bsd_error( x, BSD_EBADCONTEXT);
  }

  // stop any further action in case of error
  if( nread < 0 || BSD_ERROR == x->type) return nread;

  /* class definition does not count as value */
  if( BSD_CLASSDEF != x->type) {
    // decrement counter (for fixed containers)
    if( f->missing > 0) {
      f->missing--;
    }

    // map key/value decoding
    if( BS_FMAP == f->kind || BS_FZMAP == f->kind) {
      f->content.map.even ^= 1;
    }
  }

  // top frame may have changed
  if( topframe( ctx) > f) {
    x->kind = BSD_KNEWCONTAINER;
  } else {
    f = topframe( ctx);
    x->kind = bsd_getFrameDataKind( f);
    /* set field name for objects */
    if( BSD_KOBJFIELD == x->kind) {
      const bs_class_t *classdef = f->content.object.classdef;
      x->fieldname = classdef->fields[classdef->nfields - (f->missing + 1)].name;
    }
  }

  ctx->read += nread;
  return nread;
}

void bsd_init( bsd_ctx_t *ctx) {
  check_endian( & sendian);
  ctx->read = 0;
  ctx->broken = 0;
  ctx->stacksize = 0;
  ctx->stack[0].kind = BS_FTOP;
  ctx->stack[0].ctxid = BS_CTXID_GLOBAL;
  ctx->stack[0].missing = -1;
  bs_classcoll_init( &ctx->classcoll);
}

void bsd_reset( bsd_ctx_t *ctx) {
  bs_classcoll_reset( &ctx->classcoll);
}

int bsd_addClass( bsd_ctx_t *ctx, const bs_class_t *classdef) {
  return bs_classcoll_set( &ctx->classcoll, classdef);
}

static void hexdump( FILE *output, const uint8_t *buffer, int length) {
  int i;
  for( i=0; i<length-1; i++) fprintf(output, "%02x ", buffer[i]);
  fprintf(output, "%02x", buffer[length-1]);
}

void bsd_dump( bsd_ctx_t *ctx, FILE *output, const uint8_t *buffer, int length) {
  const uint8_t *begin = buffer;
  const uint8_t *end = buffer + length;

  fputs("Byte\tOpcode\tContext\tType\tValue\tAdditional bytes\tAdditional info\n", output);
  while( buffer < end || ctx->stacksize > 0) {
    bsd_data_t x;
    struct bsd_stackframe_t *f = topframe( ctx);
    // we need context in which the value is decoded, so we catch now rather than after the read
    bs_ctxid_t ctxid = getctxid( f);
    int r = bsd_read( ctx, &x, buffer, length);
    if( 0 == r && BSD_ERROR == x.type) {
      fprintf( output, "Error %d at byte %td.\n", x.content.error, buffer - begin);
      break;
    } else if( r < 0) {
      fprintf( output, "Error: %d bytes missing.\n", r);
      break;
    } else {
      int opcode_len = 0;
      fprintf( output, "%td\t", buffer - begin);
      if( r == 0) fputs( "N/A\tN/A\t", output);
      else {
        switch( ctxid) {
        case BS_CTXID_CHUNKED: opcode_len = 2; break;
        case BS_CTXID_FLOAT:
        case BS_CTXID_INT32:   opcode_len = 4; break;
        case BS_CTXID_DOUBLE:  opcode_len = 8; break;
        default:               opcode_len = 1; break;
        }
        hexdump( output, buffer, opcode_len); fputc('\t', output);

        switch( ctxid) {
#define CASE(x) case BS_CTXID_##x: fputs( #x"\t", output); break
        CASE(GLOBAL);
        CASE(UNSIGNED_OR_STRING);
        CASE(NUMBER);
        CASE(INT32);
        CASE(FLOAT);
        CASE(DOUBLE);
        CASE(LIST_OR_MAP);
        default: printf("%d\t", ctxid); break;
#undef CASE
        }
      }

      switch( x.type) {
#define CASE(x) case BSD_##x: fputs( #x"\t", output)
      CASE(CLOSE); fputs( "N/A\t", output); break;
      CASE(NULL); fputs( "N/A\t", output); break;
      CASE(INT); fprintf( output, "%" PRId64 "\t", x.content.i); break;
      CASE(BOOL); fputs( x.content.bool ? "true\t" : "false\t", output); break;
      CASE(DOUBLE); fprintf( output, "%f\t", x.content.d); break;
      CASE(STRING); fprintf( output, "length: %zu\t", x.content.string.length); break;
      CASE(CHUNKED_STRING); fputs( "N/A\t", output); break;
      CASE(CHUNK); fprintf( output, "length: %zu\t", x.content.chunk.length); break;
      CASE(LIST); fprintf( output, "length: %zu, ctx: %d\t", x.content.length, topframe( ctx)->ctxid); break;
      CASE(ZLIST); fprintf( output, "ctx: %d\t", topframe( ctx)->ctxid); break;
      CASE(MAP); fprintf( output, "length: %zu, ctx: %d\t", x.content.length, topframe( ctx)->ctxid); break;
      CASE(ZMAP); fprintf( output, "ctx: %d\t", topframe( ctx)->ctxid); break;
      CASE(OBJECT);
        if( NULL == x.content.classdef->classname) fprintf( output, "class: %d\t", x.content.classdef->classid);
        else fprintf( output, "class: %s\t", x.content.classdef->classname);
        break;
      CASE(CLASSDEF);
        if( NULL == x.content.classdef->classname) fprintf( output, "class: %d\t", x.content.classdef->classid);
        else fprintf( output, "class: %s\t", x.content.classdef->classname);
        break;
      default: fprintf(output, "%d\tN/A\t", x.type);
#undef CASE
      }

      int additional = r - opcode_len;
      if( 0 < additional && additional <= 10) hexdump( output, buffer+opcode_len, additional);
      else if( additional > 10) {
        hexdump( output, buffer+opcode_len, 3);
        fputs( "...", output);
        hexdump( output, buffer+r-3, 3);
      }
      else fputs( "N/A", output);
      fputs( "\t", output);
      // additional infos
      if( BSD_KOBJFIELD == x.kind && x.fieldname != NULL) {
        fprintf( output, "field: %s\t", x.fieldname);
      } else {
        fputs( "N/A\t", output);
      }
      fputs( "\n", output);
    }

    buffer += r;
    length -= r;
  }
}
