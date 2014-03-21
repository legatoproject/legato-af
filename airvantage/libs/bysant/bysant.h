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
#ifndef __BYSANT_H_INCLUDED__
#define __BYSANT_H_INCLUDED__

#include <stdint.h>
#include <stddef.h>

/* Bysant definitions used in both serialization and deserialization. */

/*
 * Data contexts IDs.
 */
typedef enum bs_ctxid_t {
  BS_CTXID_GLOBAL,
  BS_CTXID_UNSIGNED_OR_STRING,
  BS_CTXID_NUMBER,
  BS_CTXID_INT32,
  BS_CTXID_FLOAT,
  BS_CTXID_DOUBLE,
  BS_CTXID_LIST_OR_MAP,
  BS_CTXID_LAST, /* sentinel for the end of enum */
  /* internal contexts (regular contexts are 0-255, so there is no possible collision) */
  BS_CTXID_CHUNKED = 256,
  BS_CTXID_OBJECT,
} bs_ctxid_t;

/*
 * Stack frames kinds for both serializer and deserializer.
 */
typedef enum bs_stackframekind_t {
  BS_FTOP, /* top-level: no open container */
  BS_FMAP, /* fixed-size map */
  BS_FZMAP, /* variable size map */
  BS_FOBJECT, /* object */
  BS_FLIST, /* fixed-size list */
  BS_FZLIST, /* variable size list */
  BS_FCHUNKED, /* chunked string or binary */
  BS_FCLASSDEF, /* class definition */
} bs_stackframekind_t;

/* Numeric limits for each context. Limits are inclusive. */
/* Global Tiny Integer */
#define BS_GTI_MIN (-31)
#define BS_GTI_MAX ( 64)

/* Global Small Integer */
#define BS_GSI_MIN (-2079)
#define BS_GSI_MAX ( 2112)

/* Global Medium Integer */
#define BS_GMI_MIN (-264223)
#define BS_GMI_MAX ( 264256)

/* Global Large Integer */
#define BS_GLI_MIN (-33818655)
#define BS_GLI_MAX ( 33818688)

/* Global Strings */
#define BS_GSS_MAX (32)
#define BS_GMS_MAX (1056)
#define BS_GLS_MAX (66592)

/* Global Short Collection */
#define BS_GSC_MAX (9)

/* Global null & float opcodes */
#define BS_G_NULL 0
#define BS_G_FLOAT32 0xFE
#define BS_G_FLOAT64 0xFF

/* Number Tiny Integer */
#define BS_NTI_MIN (-97)
#define BS_NTI_MAX ( 97)

/* Number Small Integer */
#define BS_NSI_MIN (-4193)
#define BS_NSI_MAX ( 4193)

/* Number Medium Integer */
#define BS_NMI_MIN (-528481)
#define BS_NMI_MAX ( 528481)

/* Number Large Integer */
#define BS_NLI_MIN (-67637345)
#define BS_NLI_MAX ( 67637345)

/* Number null & float opcodes */
#define BS_N_NULL 0
#define BS_N_FLOAT32 0xFE
#define BS_N_FLOAT64 0xFF

/* UIS Unsigned Integers */
#define BS_UTI_MAX (139)
#define BS_USI_MAX (8331)
#define BS_UMI_MAX (1056907)
#define BS_ULI_MAX (135274635)

/* Lists and Maps Short Collection */
#define BS_LMSC_MAX (60)

/* Structures that contains opcodes and limits for data in different contexts */
typedef struct bs_coll_encoding_t {
  uint8_t empty_opcode, variable_typed_opcode, variable_untyped_opcode;
  size_t small_limit;
  uint8_t small_typed_opcode, small_untyped_opcode;
  size_t long_typed_opcode;
  uint8_t long_untyped_opcode;
  bs_stackframekind_t fixed_kind, variable_kind;
} bs_coll_encoding_t;

typedef struct bs_string_encoding_t {
  size_t small_limit;
  uint8_t small_opcode;
  size_t medium_limit;
  uint8_t medium_opcode;
  size_t large_limit;
  uint8_t large_opcode;
  uint8_t chunked_opcode;
} bs_string_encoding_t;

typedef struct bs_integer_encoding_t {
  int tiny_min, tiny_max;
  uint8_t tiny_zero_opcode;
  int small_min, small_max;
  uint8_t small_neg_opcode, small_pos_opcode;
  int medium_min, medium_max;
  uint8_t medium_neg_opcode, medium_pos_opcode;
  int large_min, large_max;
  uint8_t large_neg_opcode, large_pos_opcode, last_large_neg_opcode;
  uint8_t int32_opcode, int64_opcode;
} bs_integer_encoding_t;


extern const bs_coll_encoding_t BS_GLOBAL_LIST;
extern const bs_coll_encoding_t BS_GLOBAL_MAP;
extern const bs_coll_encoding_t BS_LISTMAP_LIST;
extern const bs_coll_encoding_t BS_LISTMAP_MAP;

extern const bs_string_encoding_t BS_GLOBAL_STRING;
extern const bs_string_encoding_t BS_UIS_STRING;

extern const bs_integer_encoding_t BS_GLOBAL_INTEGER;
extern const bs_integer_encoding_t BS_NUMBER_INTEGER;

/*
 * Internal class collection handling
 */

typedef unsigned int bs_classid_t;

typedef struct bs_class_t {
  bs_classid_t classid;   /* unique class indentifier. */
  const char *classname;  /* class name (can be NULL for short classes). */
  int nfields;            /* number of contexts items. */
  enum bs_classmode_t {   /* memory management mode (see bs_classcoll_set). */
    BS_CLASS_MANAGED,
    BS_CLASS_EXTERNAL,
  } mode;
  struct bs_field_t {     /* fields array. */
    const char *name;     /* field name (can be NULL for short classes). */
    bs_ctxid_t ctxid;     /* context for the field */
  } *fields;
} bs_class_t;

typedef struct bs_classcoll_t {
  const bs_class_t **classes;
  size_t nclasses;
} bs_classcoll_t;

/* Initializes class collection. */
int bs_classcoll_init( bs_classcoll_t *coll);

/* Cleanup/reset class collection. All managed classes are freed.
 * See bs_classcoll_set. */
void bs_classcoll_reset( bs_classcoll_t *coll);

/* Add a new class into the collection (or replace it if existing).
 * Given class has an attached mode:
 *   * BS_CLASS_MANAGED: the class will be freed when released (close,
 *     overwrite). This is intended to be used for classes defined in streams.
 *     The whole class must be contained in a single data block (only one free
 *     will be done).
 *   * BS_CLASS_EXTERNAL: the class will not be freed when not needed anymore.
 *     This is intended to be used for application internal classes defined in
 *     static structures.
 * Return 0 on success, non-zero on failure. The passed class is not freed on
 * failure.
 */
int bs_classcoll_set( bs_classcoll_t *coll, const bs_class_t* classdef);

/* Returns class that have the given class identifier (or NULL if not found). */
const bs_class_t* bs_classcoll_get( bs_classcoll_t *coll, bs_classid_t classid);

/* Returns class that have the given class name (or NULL if not found).
 * Potentially much slower than bs_classcoll_get() (linear in the number
 * of registererd classes). */
const bs_class_t* bs_classcoll_byname( bs_classcoll_t *coll, const char *name);


#endif /* __BYSANT_H_INCLUDED__ */
