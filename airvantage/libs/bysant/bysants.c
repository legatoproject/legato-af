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
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "bysants.h"

static struct bss_stackframe_t *topframe( bss_ctx_t *ctx) {
  return ctx->stack + ctx->stacksize;
}

/* returns the context for the next value to write to the frame. In particular
 * it returns correct context for map keys. */
static bs_ctxid_t getctxid( struct bss_stackframe_t *f) {
  if( (BS_FMAP == f->kind || BS_FZMAP == f->kind) && f->content.map.even) {
    return BS_CTXID_UNSIGNED_OR_STRING; /* map key */
  }
  if( BS_FOBJECT == f->kind) {
    const bs_class_t *class = f->content.object.class;
    return class->fields[class->nfields - f->missing].ctxid;
  }
  return f->ctxid;
}

#define START_TRANSACTION do {                         \
    bss_status_t _r = startTransaction( ctx);          \
    if( _r) return _r;                                 \
  } while( 0)

#define COMMIT_AND_RETURN do {                         \
    bss_status_t _r = commitTransaction( ctx);         \
    if( ! _r) ctx->acknowledged = ctx->written;        \
    return _r;                                         \
  } while( 0)

#define TRY( it) do {                                  \
    bss_status_t _r = (it);                            \
    switch( _r) {                                      \
    case BSS_EOK:    break;                            \
    case BSS_EAGAIN: return _r;                        \
    default: /* Error other than overflow. */          \
      if( ctx->written != ctx->acknowledged)           \
        ctx->broken = 1;                               \
      return _r;                                       \
    }                                                  \
  } while( 0)

static bss_status_t startTransaction( bss_ctx_t *ctx) {
  struct bss_stackframe_t *f = topframe( ctx);
  ctx->stacksize = ctx->acknowledged_stacksize;
  if( ctx->broken) {
    return BSS_EBROKEN;
  } else if( (BS_FLIST == f->kind || BS_FMAP == f->kind || BS_FOBJECT == f->kind) && 0 == f->missing) {
    return BSS_ESIZE;
  } else {
    ctx->skipped = 0;
    return BSS_EOK;
  }
}

static bss_status_t commitTransaction( bss_ctx_t *ctx) {
  struct bss_stackframe_t *f = topframe( ctx);
  switch( f->kind) {
  case BS_FZMAP:
    f->content.map.even ^= 1;
    break;
  case BS_FMAP:
    f->content.map.even ^= 1;
    /* fall through. */
  case BS_FLIST:
  case BS_FOBJECT:
    if( 0 == f->missing)
      return BSS_ESIZE;
    f->missing--;
    break;
  case BS_FZLIST:
  case BS_FCHUNKED:
  case BS_FTOP:
    break;
  default:
    return BSS_EINTERNAL;
  }
  ctx->acknowledged = ctx->written;
  ctx->acknowledged_stacksize = ctx->stacksize;
  return BSS_EOK;
}

/* Write a block of 'len' bytes, pointed by 'buffer'. */
static bss_status_t writeData( bss_ctx_t *ctx, const uint8_t *buffer, size_t len) {
  int n;
  int toskip = ctx->written - ctx->acknowledged - ctx->skipped;
  if( toskip > 0) {
    if( toskip >= len) {
      ctx->skipped += len;
      return BSS_EOK;
    }
    else {
      ctx->skipped += toskip;
      buffer += toskip;
      len -= toskip;
    }
  }

  n = ctx->writer( buffer, len, ctx->writerctx);
  if( n < 0) return n;
  ctx->written += n;
  ctx->skipped += n;
  return n == len ? BSS_EOK : BSS_EAGAIN;
}

/* Write 'n' bytes, passed as vararg arguments. */
#define WRITE_BYTES_MAX 4
static bss_status_t writeBytes( bss_ctx_t *ctx, size_t n, ...) {
  uint8_t buffer[WRITE_BYTES_MAX];
  va_list ap;
  size_t i;
  if( n > WRITE_BYTES_MAX) return BSS_EINTERNAL;
  va_start( ap, n);
  for( i = 0; i < n; i++)
    buffer[i] = va_arg( ap, int);
  va_end( ap);
  return writeData( ctx, buffer, n);
}

/* Write 1 byte. */
static bss_status_t writeByte( bss_ctx_t *ctx, uint8_t k) {
  return writeData( ctx, &k, 1);
}

/*
 * Inernal functions.
 * As same data is encoded differently depending on context, context specific data is wrapped into
 * structs for each data type.
 * As these functions are used at multiple places, no transaction is done inside them, transactions
 * are done only in public functions.
 */
#define COMPUTE_OFFSET(x, ifneg, ifpos) (( x<0) ? ((-x) - (-ifneg+1)) : (x - (ifpos+1)))

static bss_status_t writeIntegerBigEndian( bss_ctx_t *ctx, int64_t x, size_t nbytes) {
  int i;
  for( i = (nbytes - 1) * 8; i >= 0; i -= 8)
    TRY( writeByte( ctx, (x>>i) & 0xff));
  return BSS_EOK;
}

static bss_status_t writeInteger( bss_ctx_t *ctx, int64_t x, const bs_integer_encoding_t *enc) {
  if( enc->tiny_min <= x && x <= enc->tiny_max) {
    return writeByte( ctx, enc->tiny_zero_opcode + x);

  } else if( enc->small_min <= x && x <= enc->small_max) {
    int offset = COMPUTE_OFFSET(x, enc->tiny_min, enc->tiny_max);
    uint8_t opbase = (x < 0) ? enc->small_neg_opcode : enc->small_pos_opcode;
    return writeBytes( ctx, 2, opbase + (offset >> 8), offset & 0xff);

  } else if( enc->medium_min <= x && x <= enc->medium_max) {
    int offset = COMPUTE_OFFSET(x, enc->small_min, enc->small_max);
    uint8_t opbase = (x < 0) ? enc->medium_neg_opcode : enc->medium_pos_opcode;
    return writeBytes( ctx, 3, opbase + (offset >> 16), (offset >> 8) & 0xff, offset & 0xff);

  } else if( enc->large_min <= x && x <= enc->large_max) {
    int offset = COMPUTE_OFFSET(x, enc->medium_min, enc->medium_max);
    uint8_t opbase = (x < 0) ? enc->large_neg_opcode : enc->large_pos_opcode;
    return writeBytes( ctx, 4, opbase + (offset >> 24), (offset >> 16) & 0xff, (offset >> 8) & 0xff,
        offset & 0xff);

  } else if( (int64_t) ((int32_t) x) == x) {
    TRY( writeByte( ctx, enc->int32_opcode));
    TRY( writeIntegerBigEndian( ctx, x, 4));
    return BSS_EOK;

  } else {
    TRY( writeByte( ctx, enc->int64_opcode));
    TRY( writeIntegerBigEndian( ctx, x, 8));
    return BSS_EOK;
  }
}

static bss_status_t writeUnsignedInteger( bss_ctx_t *ctx, uint32_t x) {
  if( x <= BS_UTI_MAX) {
    return writeByte( ctx, x + 0x3b);
  } else if( x <= BS_USI_MAX) {
    int offset = x - (BS_UTI_MAX + 1);
    return writeBytes( ctx, 2, 0xc7 + (offset >> 8), offset & 0xff);
  } else if( x <= BS_UMI_MAX) {
    int offset = x - (BS_USI_MAX + 1);
    return writeBytes( ctx, 3, 0xe7 + (offset >> 16), (offset >> 8) & 0xff, offset & 0xff);
  } else if( x <= BS_ULI_MAX) {
    int offset = x - (BS_UMI_MAX + 1);
    return writeBytes( ctx, 4, 0xf7 + (offset >> 24), (offset >> 16) & 0xff, (offset >> 8) & 0xff,
        offset & 0xff);
  } else {
    TRY( writeByte( ctx, 0xff));
    TRY( writeIntegerBigEndian( ctx, x, 4));
    // works as uint32_t fits in int64_t
    return BSS_EOK;
  }
}

/*
 * Floating point encoding
 */

#ifdef __OAT_API_VERSION__
static int FLOAT_BYTES [] = {3, 2, 1, 0};
static int DOUBLE_BYTES [] = {3, 2, 1, 0, 7, 6, 5, 4};
#else
static int FLOAT_BYTES[] = { 3, 2, 1, 0 };
static int DOUBLE_BYTES[] = { 7, 6, 5, 4, 3, 2, 1, 0 };
#endif

typedef struct bss_float_encoding_t {
  uint8_t float32_opcode, float64_opcode;
} bss_float_encoding_t;
static const bss_float_encoding_t GLOBAL_FLOAT_OPCODES = { BS_G_FLOAT32, BS_G_FLOAT64 };
static const bss_float_encoding_t NUMBER_FLOAT_OPCODES = { BS_N_FLOAT32, BS_N_FLOAT64 };

static bss_status_t writeFloat32( bss_ctx_t *ctx, float x) {
  int i;
  // WARNING! check endianness!
  union float_as_byte_char {
    float f;
    uint8_t k[4];
  } u;
  u.f = x;
  for( i = 0; i < sizeof(FLOAT_BYTES) / sizeof(FLOAT_BYTES[0]); i++)
    TRY( writeByte( ctx, u.k[FLOAT_BYTES[i]]));
  return BSS_EOK;
}

static bss_status_t writeFloat64( bss_ctx_t *ctx, double x) {
  int i;
  // WARNING! check endianness!
  union float_as_byte_char {
    double f;
    uint8_t k[8];
  } u;
  u.f = x;
  for( i = 0; i < sizeof(DOUBLE_BYTES) / sizeof(DOUBLE_BYTES[0]); i++)
    TRY( writeByte( ctx, u.k[DOUBLE_BYTES[i]]));
  return BSS_EOK;
}

static bss_status_t writeFloat( bss_ctx_t *ctx, double x, const bss_float_encoding_t *enc) {
  if( (double) (float) x == x) {
    TRY( writeByte( ctx, enc->float32_opcode));
    TRY( writeFloat32( ctx, (float) x));
  } else {
    TRY( writeByte( ctx, enc->float64_opcode));
    TRY( writeFloat64( ctx, x));
  }
  return BSS_EOK;
}

/*
 * String encoding.
 */

static bss_status_t writeChunk( bss_ctx_t *ctx, const void *data, size_t len) {
  while( len > 0xffff) {
    TRY( writeBytes( ctx, 2, 0xff, 0xff));
    TRY( writeData( ctx, data, 0xffff));
    data += 0xffff;
    len -= 0xffff;
  }
  /* <64KB chunk. */
  if( len > 0) {
    TRY( writeBytes( ctx, 2, len>>8, len&0xff));
    TRY( writeData( ctx, data, len));
  }
  return BSS_EOK;
}

static bss_status_t writeString( bss_ctx_t *ctx, const void *data, size_t len,
    const bs_string_encoding_t *enc) {
  if( len <= enc->small_limit) {
    TRY( writeByte( ctx, enc->small_opcode + len));
  } else if( len <= enc->medium_limit) {
    int sent_len = len - (enc->small_limit + 1);
    TRY( writeBytes( ctx, 2, enc->medium_opcode + (sent_len >> 8), sent_len & 0xff));
  } else if( len <= enc->large_limit) {
    int sent_len = len - (enc->medium_limit + 1);
    TRY( writeBytes( ctx, 3, enc->large_opcode, sent_len >> 8, sent_len & 0xff));
  } else {
    /* longer strings are chunked. This does not use the public bss_*
     * functions because it would make nested transactions. */
    TRY( writeByte( ctx, enc->chunked_opcode));
    TRY( writeChunk( ctx, data, len));
    TRY( writeBytes( ctx, 2, 0x00, 0x00));
    return BSS_EOK;
  }
  TRY( writeData( ctx, (const unsigned char *) data, len));
  return BSS_EOK;
}

/* Write a container header, and adds a frame to the stack. */
static bss_status_t openContainer(
    bss_ctx_t *ctx,
    bs_stackframekind_t kind,
    bs_ctxid_t ctxid,
    int prefix,
    struct bss_stackframe_t **dest) {

  struct bss_stackframe_t *f;
  if( (BSS_STACK_SIZE-1) == ctx->stacksize) return BSS_ETOODEEP;
  ctx->stacksize++;
  f = topframe( ctx);
  f->kind = kind;
  f->ctxid = ctxid;
  if( dest) *dest = f;
  if( prefix >= 0)
    return writeByte( ctx, prefix);
  else return BSS_EOK;
}

static bss_status_t openCollection(
    bss_ctx_t *ctx,
    int len,
    bs_ctxid_t ctxid,
    struct bss_stackframe_t **dest,
    const bs_coll_encoding_t *enc) {
  int hastype = BS_CTXID_GLOBAL != ctxid;

  if( ctxid >= BS_CTXID_LAST) return BSS_EBADFIELD;

  if( len < 0) {
    uint8_t opcode = hastype ? enc->variable_typed_opcode : enc->variable_untyped_opcode;
    TRY( openContainer( ctx, enc->variable_kind, ctxid, opcode, dest));
  } else if( len == 0) {
    TRY( openContainer( ctx, enc->fixed_kind, ctxid, enc->empty_opcode, dest));
  } else if( len <= enc->small_limit) {
    uint8_t opcode = hastype ? enc->small_typed_opcode : enc->small_untyped_opcode;
    TRY( openContainer( ctx, enc->fixed_kind, ctxid, len + opcode - 1, dest));
  } else { // length > MAX
    uint8_t opcode = hastype ? enc->long_typed_opcode : enc->long_untyped_opcode;
    TRY( openContainer( ctx, enc->fixed_kind, ctxid, opcode, dest));
    TRY( writeUnsignedInteger( ctx, len - (enc->small_limit + 1)));
  }

  if( len != 0 && hastype) {
    TRY( writeByte( ctx, ctxid));
  }
  return BSS_EOK;
}

/* Write null token for given context */
static void getNullToken( bs_ctxid_t ctxid, const uint8_t** buffer, size_t *len) {
  switch( ctxid) {
  case BS_CTXID_INT32:
    *buffer = (const uint8_t*) "\x80\x00\x00\x00\x00";
    *len = 5;
    return;
  case BS_CTXID_FLOAT:
    *buffer = (const uint8_t*) "\xff\xff\xff\xff\x00";
    *len = 5;
    return;
  case BS_CTXID_DOUBLE:
    *buffer = (const uint8_t*) "\xff\xff\xff\xff\xff\xff\xff\xff\x00";
    *len = 9;
    return;
  default:
    *buffer = (const uint8_t*) "\x00";
    *len = 1;
    return;
  }
}

/*
 * Public API
 */

void bss_init( bss_ctx_t *ctx, bss_writer_t *writer, void *writerctx) {
  ctx->writer = writer;
  ctx->writerctx = writerctx;
  bs_classcoll_init( &ctx->classcoll);
  bss_reset( ctx);
}

void bss_reset( bss_ctx_t *ctx) {
  bs_classcoll_reset( &ctx->classcoll);
  ctx->written = 0;
  ctx->acknowledged = 0;
  ctx->skipped = 0;
  ctx->broken = 0;
  ctx->stacksize = 0;
  ctx->acknowledged_stacksize = 0;
  ctx->stack[0].kind = BS_FTOP;
  ctx->stack[0].ctxid = BS_CTXID_GLOBAL;
}

bss_status_t bss_close( bss_ctx_t *ctx) {
  struct bss_stackframe_t *frame = topframe( ctx);
  const uint8_t *buffer;
  size_t len;

  switch( frame->kind) {

  /* 'Z'-terminated containers: */
  case BS_FZMAP:
    START_TRANSACTION;
    if( !frame->content.map.even) return BSS_EBADMAP;
    getNullToken( BS_CTXID_UNSIGNED_OR_STRING, &buffer, &len);
    TRY( writeData( ctx, buffer, len));
    break;
  case BS_FZLIST:
    START_TRANSACTION;
    getNullToken( frame->ctxid, &buffer, &len);
    TRY( writeData( ctx, buffer, len));
    break;

    /* containers without terminators: */
  case BS_FOBJECT:
  case BS_FLIST:
  case BS_FMAP:
    if( 0 != frame->missing)
      return BSS_ESIZE;
    break;

  case BS_FCHUNKED:
    /* insert a last empty chunk to terminate open chunk sequences: */
    START_TRANSACTION;
    TRY( writeBytes( ctx, 2, 0, 0));
    break;

  case BS_FTOP:
    return BSS_ENOCONTAINER;
  default:
    return BSS_EINTERNAL;
  }
  ctx->stacksize--;
  COMMIT_AND_RETURN;
}

bss_status_t bss_object( bss_ctx_t *ctx, bs_classid_t classid) {
  int direct = classid < 0x10;
  struct bss_stackframe_t *f = NULL;
  const bs_class_t *class = bs_classcoll_get( &ctx->classcoll, classid);

  if( BS_CTXID_GLOBAL != getctxid( topframe( ctx))) return BSS_EBADCONTEXT;
  if( class == NULL) return BSS_EBADCLASSID;

  START_TRANSACTION;
  TRY( openContainer( ctx, BS_FOBJECT, BS_CTXID_OBJECT, direct ? 0x60+classid : 0x70, & f));
  if( !direct) TRY( writeUnsignedInteger( ctx, classid-0x10));

  f->missing = class->nfields + 1; /* will be decremented by commitTransaction() */
  f->content.object.class = class;
  COMMIT_AND_RETURN;
}

bss_status_t bss_class( bss_ctx_t *ctx, const bs_class_t *classdef, int internal) {
  int named = classdef->classname != NULL;
  int i;

  // check classdef
  for(i=0; i<classdef->nfields; i++) {
    if( named && NULL == classdef->fields[i].name) return BSS_EBADFIELD;
    if( classdef->fields[i].ctxid >= BS_CTXID_LAST) return BSS_EBADFIELD;
  }

  // send class before adding it to classcoll because errors could happen and
  // result in an inconsistent state
  if( ! internal) {
    if( BS_CTXID_GLOBAL != getctxid( topframe( ctx))) return BSS_EBADCONTEXT;
    START_TRANSACTION;
    TRY( writeByte( ctx, named ? 0x71 : 0x72));
    TRY( writeUnsignedInteger( ctx, classdef->classid));
    if( named) {
      TRY( writeString( ctx, classdef->classname, strlen( classdef->classname), & BS_UIS_STRING));
    }
    TRY( writeUnsignedInteger( ctx, classdef->nfields));

    // write fields
    for( i=0; i<classdef->nfields; i++) {
      if( named) {
        const char *name = classdef->fields[i].name;
        TRY( writeString( ctx, name, strlen( name), & BS_UIS_STRING));
      }
      TRY( writeByte( ctx, classdef->fields[i].ctxid));
    }
  }

  // add class to collection
  if( bs_classcoll_set( &ctx->classcoll, classdef) != 0) {
    ctx->broken = 1;
    return BSS_EINTERNAL;
  }

  if( internal) {
    return BSS_EOK;
  } else {
    /* Class definitions don't count as objects; so if they appear in a container
     * which counts its items at `commitTransaction()`, we have to preemptively
     * cancel this count, by adding a missing element (which will be re-deleted
     * by commitTransaction()) or flipping the odd/even flag, for containers which
     * need an even number of items. */
    struct bss_stackframe_t *top = topframe( ctx);
    switch( top->kind) {
    case BS_FZMAP:
      top->content.map.even ^= 1;
      break;
    case BS_FMAP:
      top->content.map.even ^= 1;
      /* fall through. */
    case BS_FLIST:
    case BS_FOBJECT:
      top->missing++;
      break;
    default:
      break;
    }
    COMMIT_AND_RETURN;
  }
}

bss_status_t bss_map( bss_ctx_t *ctx, int len, bs_ctxid_t ctxid) {
  struct bss_stackframe_t *f = NULL;
  const bs_coll_encoding_t *enc;

  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_GLOBAL:
    enc = &BS_GLOBAL_MAP;
    break;
  case BS_CTXID_LIST_OR_MAP:
    enc = &BS_LISTMAP_MAP;
    break;
  default:
    return BSS_EBADCONTEXT;
  }
  if( ctxid >= BS_CTXID_LAST) return BSS_EBADMAP;

  START_TRANSACTION;
  TRY( openCollection(ctx, len, ctxid, &f, enc));
  /* will be decremented by commitTransaction() and then decremented again for both keys and values. */
  if( len >= 0) f->missing = (len * 2) + 1;
  f->content.map.even = 0; /* will be inverted by commitTransaction() */
  COMMIT_AND_RETURN;
}

bss_status_t bss_list( bss_ctx_t *ctx, int len, bs_ctxid_t ctxid) {
  struct bss_stackframe_t *f = NULL;
  const bs_coll_encoding_t *enc = 0;

  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_GLOBAL:
    enc = &BS_GLOBAL_LIST;
    break;
  case BS_CTXID_LIST_OR_MAP:
    enc = &BS_LISTMAP_LIST;
    break;
  default:
    return BSS_EBADCONTEXT;
  }
  if( ctxid >= BS_CTXID_LAST) return BSS_EBADCTXID;

  START_TRANSACTION;
  TRY( openCollection( ctx, len, ctxid, &f, enc));
  if( len >= 0) f->missing = len + 1; /* will be decremented by commitTransaction() */
  COMMIT_AND_RETURN;
}

bss_status_t bss_chunked( bss_ctx_t *ctx) {
  uint8_t opcode;

  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_GLOBAL:
    opcode = BS_GLOBAL_STRING.chunked_opcode;
    break;
  case BS_CTXID_UNSIGNED_OR_STRING:
    opcode = BS_UIS_STRING.chunked_opcode;
    break;
  default:
    return BSS_EBADCONTEXT;
  }

  START_TRANSACTION;
  TRY( openContainer(ctx, BS_FCHUNKED, BS_CTXID_CHUNKED, opcode, NULL));
  COMMIT_AND_RETURN;
}

bss_status_t bss_chunk( bss_ctx_t *ctx, void *data, size_t len) {
  struct bss_stackframe_t *f = topframe( ctx);

  if( BS_CTXID_CHUNKED != f->ctxid) return BSS_EBADCONTEXT;
  if( len <= 0) return BSS_EINVALID;
  START_TRANSACTION;
  TRY( writeChunk( ctx, data, len));
  COMMIT_AND_RETURN;
}

bss_status_t bss_int( bss_ctx_t *ctx, int64_t x) {
  const bs_integer_encoding_t *limits;

  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_GLOBAL:
    limits = &BS_GLOBAL_INTEGER;
    break;
  case BS_CTXID_NUMBER:
    limits = &BS_NUMBER_INTEGER;
    break;
  case BS_CTXID_UNSIGNED_OR_STRING:
    if( x < 0) return BSS_EBADCONTEXT;
    START_TRANSACTION;
    TRY( writeUnsignedInteger( ctx, x));
    COMMIT_AND_RETURN;
  case BS_CTXID_INT32: {
    int32_t y = (int32_t) x;
    if( x != y) return BSS_EOUTOFBOUNDS;
    START_TRANSACTION;
    TRY( writeIntegerBigEndian( ctx, y, 4));
    if( 0x80000000 == y) {
      TRY( writeByte( ctx, 0x01));
    }
    COMMIT_AND_RETURN;
  }
  default:
    return BSS_EBADCONTEXT;
  }

  START_TRANSACTION;
  TRY( writeInteger( ctx, x, limits));
  COMMIT_AND_RETURN;
}

bss_status_t bss_bool( bss_ctx_t *ctx, int x) {
  if( BS_CTXID_GLOBAL != getctxid( topframe( ctx))) return BSS_EBADCONTEXT;
  START_TRANSACTION;
  TRY( writeByte( ctx, x ? 0x01 : 0x02));
  COMMIT_AND_RETURN;
}

bss_status_t bss_double( bss_ctx_t *ctx, double x) {
  const bss_float_encoding_t *opcodes;

  /* first, look for specific contexts */
  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_FLOAT: {
    union { float f; uint32_t i; } ux;
    ux.f = (float) x;
    START_TRANSACTION;
    TRY( writeFloat32( ctx, ux.f));
    if( 0xFFFFFFFF == ux.i) {
      TRY( writeByte( ctx, 0x01));
    }
    COMMIT_AND_RETURN;
  }
  case BS_CTXID_DOUBLE: {
    union { double d; uint64_t i; } ux;
    ux.d = x;
    START_TRANSACTION;
    TRY( writeFloat64( ctx, ux.d));
    if( 0xFFFFFFFFFFFFFFFFLL == ux.i) {
      TRY( writeByte( ctx, 0x01));
    }
    COMMIT_AND_RETURN;
  }
  default:
    break;
  }

  /* otherwise, send as integer if possible */
  int64_t y = (int64_t) x;
  if( (double) y == x) {
    return bss_int( ctx, y);
  }

  switch( getctxid( topframe( ctx))) {
  case BS_CTXID_GLOBAL:
    opcodes = &GLOBAL_FLOAT_OPCODES;
    break;
  case BS_CTXID_NUMBER:
    opcodes = &NUMBER_FLOAT_OPCODES;
    break;
  default:
    return BSS_EBADCONTEXT;
  }

  START_TRANSACTION;
  TRY( writeFloat( ctx, x, opcodes));
  COMMIT_AND_RETURN;
}

bss_status_t bss_lstring( bss_ctx_t *ctx, const char *data, size_t len) {
  const bs_string_encoding_t *encoding;
  struct bss_stackframe_t *top = topframe( ctx);

  switch( getctxid( top)) {
  case BS_CTXID_GLOBAL:
    encoding = &BS_GLOBAL_STRING;
    break;
  case BS_CTXID_UNSIGNED_OR_STRING:
    encoding = &BS_UIS_STRING;
    break;
  default:
    return BSS_EBADCONTEXT;
  }

  START_TRANSACTION;
  TRY( writeString(ctx, data, len, encoding));
  COMMIT_AND_RETURN;
}

bss_status_t bss_string( bss_ctx_t *ctx, const char *data) {
  return bss_lstring( ctx, data, strlen( data));
}

bss_status_t bss_null( bss_ctx_t *ctx) {
  struct bss_stackframe_t *top = topframe( ctx);

  /* null cannot be stored in variable lists */
  if( BS_FZLIST == top->kind) return BSS_EINVALID;
  /* null cannot be a key of a map */
  if( (BS_FMAP == top->kind || BS_FZMAP == top->kind) && top->content.map.even)
    return BSS_EINVALID;

  const uint8_t *buffer;
  size_t len;
  getNullToken( getctxid( top), &buffer, &len);
  START_TRANSACTION;
  TRY( writeData( ctx, buffer, len));
  COMMIT_AND_RETURN;
}

bss_status_t bss_raw( bss_ctx_t *ctx, const unsigned char *data, size_t len) {
  START_TRANSACTION;
  TRY( writeData( ctx, (const uint8_t *) data, len));
  COMMIT_AND_RETURN;
}

