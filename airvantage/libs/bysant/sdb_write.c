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

#include "sdb_internal.h"
//#include <assert.h>
#include <stdlib.h> // size_t

// Next power of two, works on a 16 bits unsigned int.
static int next_pow2( int x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    return ++x;
}

/* Make sure that current_chunk is at least needed_size long.
 * If current_chunk is not big enough, it is reallocated to the
 * next power of 2. */
static int set_last_chunk_size( sdb_table_t *tbl, int new_size) {
    struct sdb_ram_storage_t *ram = & tbl->storage.ram;
    // when current_chunk_size is set to 0, it actually holds 0x10000 bytes.
    int prev_size =  ram->last_chunk_size;
    if(prev_size==0) prev_size = 0x10000;
    //assert (ram->current_chunk->next==NULL);
    if( new_size != prev_size) {
        /* Block needs to grow */
        int struct_size = sizeof(sdb_chunk_t) - SDB_CHUNK_SIZE + new_size;
        struct sdb_chunk_t *c = realloc( ram->last_chunk, struct_size);
        if( ! c) return SDB_EMEM;
        if( ram->first_chunk == ram->last_chunk) ram->first_chunk = c;
        *ram->last_chunk_ptr = c;
        ram->last_chunk = c;
        ram->last_chunk_size = new_size==0x10000 ? 0 : new_size;
#ifdef SDB_VERBOSE_PRINT
        {
            int
                sbytes  = tbl->nwrittenbytes,                     // bytes used.
                nchunks = sbytes/SDB_CHUNK_SIZE,    // bytes in complete chunks.
                sbuff   = nchunks * SDB_CHUNK_SIZE + new_size, // b in last chk.
                smem    = sbuff + (nchunks+1) * (sizeof( sdb_chunk_t)- SDB_CHUNK_SIZE), // malloced bytes.
                lstused = sbytes - nchunks * SDB_CHUNK_SIZE, // used in last buff.
                tblsize = sizeof( sdb_table_t),   // bytes in other table parts.
                cfgsize = tbl->conf_string_idx,
                colsize = tbl->ncolumns * sizeof( struct sdb_column_t),
                bsssize = tbl->bss_ctx ? sizeof( bss_ctx_t) : 0;
            printf("Resized chunk %d->%d: %d/%d/%d bytes in:\n"
                " - %d chunks of %d/%d bytes;\n"
                " - 1 chunk of %d/%d/%d bytes;\n"
                " - %d more bytes: %d table, %d conf strings, %d columns, %d bss.\n",
                prev_size, new_size,
                sbytes, sbuff, smem+tblsize+colsize+cfgsize+bsssize,
                nchunks,
                SDB_CHUNK_SIZE, sizeof( sdb_chunk_t),
                lstused, new_size, new_size + sizeof( sdb_chunk_t) - SDB_CHUNK_SIZE,
                tblsize+colsize+cfgsize+bsssize,tblsize, cfgsize, colsize, bsssize);
        }
#endif
    }
    return SDB_EOK;
}

int sdb_ram_trim( sdb_table_t *tbl) {
    int size = tbl->nwrittenbytes % SDB_CHUNK_SIZE;
    int r = SDB_EOK;
    if( 0==size) { // empty chunks are not allowed
        r = set_last_chunk_size( tbl, SDB_MIN_CHUNK_SIZE);
    } else if( size != tbl->storage.ram.last_chunk_size) {
        r = set_last_chunk_size( tbl, size);
    }
    return r;
}

/* Append a new chunk, of size at least needed_size, after the current chunk.
 * The current_chunk is expected to be already at maximum size. */
static int add_chunk( struct sdb_ram_storage_t *ram, int needed_size) {
    int allocated_size = next_pow2( needed_size);
    if( allocated_size < SDB_MIN_CHUNK_SIZE) allocated_size = SDB_MIN_CHUNK_SIZE;
    int struct_size = sizeof(sdb_chunk_t) - SDB_CHUNK_SIZE + allocated_size;
    struct sdb_chunk_t *c = malloc( struct_size);
    if( ! c) return SDB_EMEM;
    //assert (ram->current_chunk->next==NULL);
    c->next = NULL;
    ram->last_chunk_ptr = & ram->last_chunk->next;
    ram->last_chunk = c;
    *ram->last_chunk_ptr = c;
    //printf("ADDC: %d\n", allocated_size);
    ram->last_chunk_size = allocated_size==0x10000 ? 0 : allocated_size;
    return SDB_EOK;
}

/* Write hessian serialization output into the table storage heap.
 * Allocate a new chunk if necessary.
 *
 * For the sake of simplicity, when the chunk is exactly filled at the end of
 * the writing operation, the next chunk is allocated immediately. This avoids
 * a bug: when a buffer is completely full, one has written_in_chunk ==
 * tbl->nwrittenbytes % SDB_CHUNK_SIZE == 0, whereas the block is actually
 * full, not empty. To avoid distinguishing full chunks from empty ones, we
 * simply never leave a full chunk.
 */
// TODO: triple-check what happens when a writing fails in the
// middle of a serialization op.
static int sdb_bss_ram_writer( unsigned const char *data,  int length, sdb_table_t *tbl) {
  struct sdb_ram_storage_t *ram =& tbl->storage.ram;
  int written_in_chunk = tbl->nwrittenbytes % SDB_CHUNK_SIZE;
  int left_in_chunk = SDB_CHUNK_SIZE - written_in_chunk;
  if( length >= SDB_DATA_SIZE_LIMIT) {
      return SDB_ETOOBIG;
  } else if( left_in_chunk > length) { /* fits in current chunk. */
    int r, needed_size =  next_pow2( written_in_chunk + length);
    if( ram->last_chunk_size!=0 && ram->last_chunk_size<needed_size) {
        r = set_last_chunk_size( tbl, needed_size);
        if( r) return r;
    }
    memcpy( ram->last_chunk->data + written_in_chunk, data, length);
  } else { /* write across two chunks. */
    int r;
    struct sdb_chunk_t *chunk1, *chunk2;
    if( ram->last_chunk_size != 0) {
        r = set_last_chunk_size( tbl, SDB_CHUNK_SIZE); // 1st chunk to max size
        if( r) return r;
    }
    chunk1 = ram->last_chunk;
    r = add_chunk( ram, length - left_in_chunk);
    if( r) return SDB_EMEM;
    chunk2 = ram->last_chunk;
    memcpy( chunk1->data + written_in_chunk, data, left_in_chunk);
    memcpy( chunk2->data, data + left_in_chunk, length - left_in_chunk);
  }
  tbl->nwrittenbytes += length;
  return length;
}

#ifdef SDB_FLASH_SUPPORT
static int sdb_bss_flash_writer( unsigned const char *data,  int length, sdb_table_t *tbl) {
#error "flash storage not implemented"
}
#endif
#ifdef SDB_FILE_SUPPORT
static int sdb_bss_file_writer( unsigned const char *data,  int length, sdb_table_t *tbl) {
    size_t r = fwrite( data, length, 1, tbl->storage.file);
    if( ! r) {
        return ferror( tbl->storage.file);
    } else {
        /* flush at each end of row. */
        if( 0 == (tbl->nwrittenobjects+1) % tbl->ncolumns) fflush( tbl->storage.file);
        return length;
    }
}
#endif

int sdb_bss_writer( unsigned const char *data,  int length, void *ctx) {
    sdb_table_t *tbl = (sdb_table_t *) ctx;
    if( tbl->state != SDB_ST_READING) return SDB_EBADSTATE;
    switch( tbl->storage_kind) {
    case SDB_SK_RAM: return sdb_bss_ram_writer( data, length, tbl);
#ifdef SDB_FLASH_SUPPORT
    case SDB_SK_FLASH: return sdb_bss_flash_writer( data, length, tbl);
#endif
#ifdef SDB_FILE_SUPPORT
    case SDB_SK_FILE: return sdb_bss_file_writer( data, length, tbl);
#endif
    }
    return SDB_EINTERNAL;
}

/* Data analysis functions */

static int gcd( int a, int b) {
    while( 0 != b) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/* Cancels data analysis whenever a non-numeric value is stored. */
void sdb_analyze_noninteger( sdb_table_t *tbl, unsigned char numeric) {
    sdb_column_t *column = tbl->columns + (tbl->nwrittenobjects % tbl->ncolumns);
    if( SDB_SM_CONTAINER(column->serialization_method) == SDB_SM_SMALLEST) {
        column->data_analysis.all_integer = 0;
        column->data_analysis.all_numeric = column->data_analysis.all_numeric && numeric;
    }
}

void sdb_analyze_integer( sdb_table_t *tbl, int i) {
    sdb_column_t *column = tbl->columns + (tbl->nwrittenobjects % tbl->ncolumns);
    if( SDB_SM_CONTAINER(column->serialization_method) == SDB_SM_SMALLEST
            && column->data_analysis.all_integer) {
        if( tbl->nwrittenobjects < tbl->ncolumns) {
            column->data_analysis.gcd = i; // used to initialize GCD calculation correctly (if any)
        } else {
            column->data_analysis.delta_sum += i - column->data_analysis.prev_value;
        }

        if( !(column->serialization_method & SDB_SM_FIXED_PRECISION)) {
            column->data_analysis.gcd = gcd(i, column->data_analysis.gcd);
        }

        column->data_analysis.prev_value = i;
    }
}

/* Public API. */

int sdb_raw( sdb_table_t *tbl, unsigned const char *serialized_cell, int length) {
  int r;
  if( tbl->state != SDB_ST_READING) return SDB_EBADSTATE;
  if( tbl->maxwrittenobjects &&
      tbl->maxwrittenobjects >= tbl->nwrittenobjects)
    return SDB_EFULL;
  sdb_untrim( tbl);
  sdb_analyze_noninteger(tbl, 0);
  r = sdb_bss_writer( serialized_cell, length, tbl);
  if( r<0) { return 0; }
  else if( r != length) { return SDB_EINTERNAL; }
  else { tbl->nwrittenobjects++; return SDB_EOK; }
}

// special case to handle 4 byte forced packing
int sdb_double( sdb_table_t *tbl, double d) {
    int r;
    if( tbl->state != SDB_ST_READING) return SDB_EBADSTATE;
    if( tbl->maxwrittenobjects &&
        tbl->maxwrittenobjects >= tbl->nwrittenobjects)
        return SDB_EFULL;
    sdb_untrim( tbl);

    if( tbl->columns[tbl->nwrittenobjects % tbl->ncolumns].serialization_method & SDB_SM_4_BYTES_FLOATS) {
        d = (double) (float) d;
    }

    sdb_analyze_noninteger(tbl, 1);
    r = bss_double(tbl->bss_ctx, d);
    if( r) {
        return r;
    } else {
        tbl->nwrittenobjects++;
        return SDB_EOK;
    }
}

#define WRITER( name,  sdb_params, bss_args, analysis) \
    int sdb_##name sdb_params { \
        int r; \
        if( tbl->state != SDB_ST_READING) return SDB_EBADSTATE; \
        if( tbl->maxwrittenobjects && \
            tbl->maxwrittenobjects >= tbl->nwrittenobjects) \
            return SDB_EFULL; \
        sdb_untrim( tbl); \
        analysis; \
        r = bss_##name bss_args; \
        if( r) { return r; } else { \
            tbl->nwrittenobjects++; \
            return SDB_EOK; \
        } \
    }

WRITER( lstring, (sdb_table_t *tbl, const char *data, int length),
                 (tbl->bss_ctx, data, length), sdb_analyze_noninteger(tbl, 0))
WRITER( string,  (sdb_table_t *tbl, const char *data), (tbl->bss_ctx, data), sdb_analyze_noninteger(tbl, 0))
WRITER( int,     (sdb_table_t *tbl, int i),            (tbl->bss_ctx, i),    sdb_analyze_integer(tbl, i))
WRITER( bool,    (sdb_table_t *tbl, int b),            (tbl->bss_ctx, b),    sdb_analyze_noninteger(tbl, 0))
WRITER( null,    (sdb_table_t *tbl),                   (tbl->bss_ctx),       sdb_analyze_noninteger(tbl, 0))

int sdb_number( sdb_table_t *tbl, double d) {
    int ix = (int) d;
    if(((double) ix) == d) {
        return sdb_int( tbl, ix);
    } else {
        return sdb_double( tbl ,d);
    }
}
