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

/* Internal declarations for staging DB. */

#ifndef __SDB_INTERNAL_H_INCLUDED__
#define __SDB_INTERNAL_H_INCLUDED__

#include "stagedb.h"
#include "bysantd.h" // deserialization
#include <string.h>
#include <malloc.h>

//#define SDB_VERBOSE_PRINT

/* To build a consolidated cell, we need a method and a source
 * column number (the source table it comes from is already known,
 * common to the whole destination table).*/
typedef struct sdb_cons_column_t {
  enum sdb_consolidation_method_t method;                 // how to consolidate?
  sdb_ncolumn_t src_column;               // which source column to consolidate?
} sdb_cons_column_t;

/* Description of a consolidation: stored in the src table,
 * keeps track of the dst table and of one consolidation method
 * per dst column. */
typedef struct sdb_consolidation_t {
  struct sdb_table_t *dst;                                 // destination table.
  struct sdb_cons_column_t *dst_columns;   // one entry per column in dst table.
  sdb_ncolumn_t conf_col;                // tmp counter for table configuration.
} sdb_consolidation_t;

/* Consolidation context: store temporary data needed to reduce every
 * number of the source column. */
typedef struct sdb_cons_ctx_t {
  enum sdb_consolidation_method_t method;       // consolidation method applied.
  enum sdb_cons_ctx_state_t {
    SDB_CCS_RUNNING = 0,                             // still accumulating data.
    SDB_CCS_DONE    = 1,                     // has seen all the data it needed.
    SDB_CCS_BROKEN  = 3                                    // broke on an error.
  } state;
  sdb_nrow_t iteration;       // # of the cell currently parsed (0 ... nrows-1).
  int broken, stopped;                          // true if something went wrong.
  sdb_nrow_t nrows;               // # of rows in the column being consolidated.
  union sdb_cons_ctx_content_t {              // method-specific temporary data.
    double max, min, sum;               // sum also serves for mean computation.
    double *median;      // array of nrow doubles, to be sorted at finalization.
    /* data to recopy, just keep a pointer on their serialized form: */
    struct { int offset; int length; } first, middle, last, streampos;
  } content;
} sdb_cons_ctx_t;

/* Column description, one per column in a table. Holds serialization info,
 * but not consolidation ones, which are only kept in consolidated tables,
 * and if so, in a separate table. */
typedef struct sdb_column_t {
  int label_offset; // offset of the name string in conf_strings
  enum sdb_serialization_method_t serialization_method;
  double arg;           // Extra argument, meaning depend on serialization method.
  /* Statistics collected during data writing. Only used with "shortest"
   * serialization. */
  struct sdb_data_analysis_t {
      double original_arg;        // original arg value as it will be overwritten.
      int gcd;                          // Greatest Common Divisor of all entries.
      int prev_value;                                           // Previous value.
      int delta_sum;                      // Sum of all differences between items.
      enum sdb_serialization_method_t method;      // Chosen serialization method.
      int all_integer:1;  // Flag set to true when any non integer data is stored.
      int all_numeric:1;  // Flag set to true when any non numeric data is stored.
  } data_analysis;
} sdb_column_t;

/* Serialized data in RAM is kept in chained fixed-size chunks. */
#define SDB_CHUNK_SIZE 0x10000
#if( SDB_CHUNK_SIZE > SDB_DATA_SIZE_LIMIT)
#error "Chunks must be at least as big as the largest legal data"
/* This is so that a given data always fit in at most 2 chunks. */
#endif
typedef struct sdb_chunk_t {
  struct sdb_chunk_t *next;            // pointer on next chunk,maybe NULL.
  unsigned char data [SDB_CHUNK_SIZE];                     // data content.
} sdb_chunk_t;

/* Context used to read back serialized data,
 * for serialization and consolidation. */
typedef struct sdb_read_ctx_t {
  enum sdb_storage_kind_t storage_kind;
  union {
    struct sdb_chunk_t *chunk;                         // chunk currently read.
    FILE *file;                                         // file currently read.
  } source;
  unsigned char *tmpbuff;// If a temporary buffer is ever needed, it goes here.
  unsigned nbytes;                       // # of bytes in the last read object.
  unsigned char *bytes;                   // raw bytes of the last read object.
  int   nreadbytes;                                // How many bytes been read.
  int   nreadobjects;                       // How many objects have been read.
  unsigned char minibuff[BSD_MINBUFFSIZE];     // buffer used to get data size.
  unsigned minibuff_len:2;  // # bytes left unused in minibuff, unused for RAM.
  unsigned minibuff_offset:2;   // offset at which unused minibuff data starts.
  struct bsd_ctx_t bsd_ctx;                         // deserialization context.
} sdb_read_ctx_t;

typedef struct sdb_serialization_ctx_t {
    /* Serialization state, some state are used only with particular containers */
    enum sdb_serialization_stage_t {
        SDB_SS_INITIALIZED,
        SDB_SS_MAP_OPENED,
        SDB_SS_MAP_LABEL_SENT,
        SDB_SS_COLUMN_OBJECT_DEFINED,
        SDB_SS_COLUMN_FACTOR_SENT,
        SDB_SS_COLUMN_START_VALUE_SENT,
        SDB_SS_COLUMN_SENDING_CELLS,
        SDB_SS_COLUMN_SHIFT_SENT,
        SDB_SS_COLUMN_CONTENT_SENT,
        SDB_SS_COLUMN_LAST_SHIFT_SENT,
        SDB_SS_COLUMN_INNER_LIST_CLOSED,
        SDB_SS_COLUMN_CLOSED,
        SDB_SS_ALL_COLUMNS_SENT,
        SDB_SS_MAP_CLOSED
    } stage;
    sdb_ncolumn_t current_column;
    struct sdb_read_ctx_t read_ctx;
    struct bss_ctx_t *bss_ctx;
    // FIXME: make a separate structure ? (not always used)
    double previous; // for DV/QPV serialization.
    sdb_nrow_t current_shift; // for QPV serialization.
} sdb_serialization_ctx_t;

/* Configure a reading context. */
void sdb_read_init(  sdb_read_ctx_t *ctx, sdb_table_t *tbl);
/* Release the resources associated to the context. */
void sdb_read_close( sdb_read_ctx_t *ctx);
/* Read the next data in the table. */
int  sdb_read_data(  sdb_read_ctx_t *ctx, bsd_data_t *bsd_data, int skip);

/* Write hessian serialization output in chunks. */
int sdb_bss_writer( unsigned const char *data,  int len, void *ctx);

int sdb_untrim( sdb_table_t *tbl);
int sdb_ram_trim( sdb_table_t *tbl);

/* Data analysis for shortest serialization for on-the-fly analysis. These
 * functions must be called before nwrittenobjects is incremented as use it
 * to get column. */
void sdb_analyze_integer( sdb_table_t *tbl, int i);
void sdb_analyze_noninteger( sdb_table_t *tbl, unsigned char numeric);

#endif
