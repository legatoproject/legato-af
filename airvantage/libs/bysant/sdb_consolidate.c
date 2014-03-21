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
#include "sdb_internal.h"
#include <stdlib.h> // qsort



/* Get the consolidation context ready to process the consolidation
 * of a column of nrows values, using the specified method. */
static int cons_init( struct sdb_cons_ctx_t *ctx,
        enum sdb_consolidation_method_t method,
        sdb_nrow_t nrows) {
    ctx->method    = method;
    ctx->state     = SDB_CCS_RUNNING;
    ctx->iteration = 0;
    ctx->broken    = 0;
    ctx->stopped   = 0;
    ctx->nrows     = nrows;

    switch( method) {
    case SDB_CM_SUM: case SDB_CM_MEAN:
        ctx->content.sum = 0; break;

    case SDB_CM_MEDIAN:
        ctx->content.median = malloc( sizeof( double) * nrows);
        if( ! ctx->content.median) {
            ctx->broken  = 1;
            ctx->stopped = 1;
            return SDB_EMEM;
        }
        break;

    default: break;
    }
    return SDB_EOK;
}

/* Parse a value to consolidate. If this function returns a non-zero
 * value, then there is no need to parse the remaining elements. */
static void cons_reduce( struct sdb_cons_ctx_t *cons_ctx,
        struct bsd_data_t *data,
        int offset,
        int length) {
    double d;
    int i;
    enum  sdb_consolidation_method_t method = cons_ctx->method;
    union sdb_cons_ctx_content_t    *u      = & cons_ctx->content;

    if( SDB_CCS_RUNNING != cons_ctx->state) return;

    i = (cons_ctx->iteration) ++;

    switch( method) {
    case SDB_CM_MAX:
    case SDB_CM_MIN:
    case SDB_CM_MEAN:
    case SDB_CM_SUM:
    case SDB_CM_MEDIAN:
      /* Retrieve the double value from data. */
      switch( data->type) {
      case BSD_INT:    d = (double) data->content.i; break;
      case BSD_DOUBLE: d = data->content.d; break;
      default: cons_ctx->state = SDB_CCS_BROKEN; return;
      }

      /* Perform reduction operation on double d. */
      switch( method) {
      case SDB_CM_MAX:  if( 0==i || d>u->max) u->max=d; return;
      case SDB_CM_MIN:  if( 0==i || d<u->min) u->min=d; return;
      case SDB_CM_MEAN:
      case SDB_CM_SUM: u->sum += d; return;
      case SDB_CM_MEDIAN: u->median[i] = d; return;
      default: return; /* for warning suppression: this case cannot happen. */
      }

    case SDB_CM_FIRST:
    case SDB_CM_LAST:
    case SDB_CM_MIDDLE:
      switch( method) {
      case SDB_CM_FIRST:  if( i != 0) return; else break;
      case SDB_CM_LAST:   if( i != cons_ctx->nrows-1) return; else break;
      case SDB_CM_MIDDLE: if( i != cons_ctx->nrows/2) return; else break;
      default: return; /* for warning suppression: this case cannot happen. */
      }
      u->streampos.offset = offset;
      u->streampos.length = length;
      return;
    }
}

/* Double comparator for the quicksort. */
static int cmp_pdouble( const void *p1, const void *p2) {
    double d1 = * (double *) p1, d2 = * (double *) p2;
    if( d1==d2) return 0;
    if( d1<d2) return -1;
    else return 1;
}

/* Copy an amount of data, at a given offset, from a source table
 * to the end of a destination table.
 * Some static variables are remembered across calls, so that
 * if source data segments are given in increasing order
 */
static int copy_data_ram( sdb_table_t *src, sdb_table_t *dst,
        int offset, int length) {
    static sdb_table_t *last_src = NULL;
    static int last_offset = 0;
    static sdb_chunk_t *src_chunk = NULL;
    int n_skip_chunks, in_first_chunk, chunk_offset, r;

    /* Retrieve the source chunk where data starts. */
    if( last_src == src && last_offset <= offset) {
        n_skip_chunks = offset/SDB_CHUNK_SIZE - last_offset/SDB_CHUNK_SIZE;
    } else {
        src_chunk = src->storage.ram.first_chunk;
        n_skip_chunks = offset/SDB_CHUNK_SIZE;
    }
    for(; n_skip_chunks; n_skip_chunks--) src_chunk = src_chunk->next;

    /* Perform the writing in 1 or 2 steps. */
    chunk_offset = offset%SDB_CHUNK_SIZE;
    in_first_chunk = SDB_CHUNK_SIZE - chunk_offset;
    if( in_first_chunk >= length) {
        r = sdb_bss_writer( src_chunk->data + chunk_offset, length, dst);
        if( r<0) return r;
        offset += length;
    } else {
        offset = length - in_first_chunk;
        r = sdb_bss_writer( src_chunk->data + chunk_offset, in_first_chunk, dst);
        if( r<0) return r;
        src_chunk = src_chunk->next;
        r = sdb_bss_writer( src_chunk->data, offset, dst);
        if( r<0) return r;
    }
    last_offset = offset; last_src = src;
    return SDB_EOK;
}

static int copy_data_file( sdb_table_t *src, sdb_table_t *dst,
        int offset, int length) {
    return SDB_EINTERNAL;
}

static int copy_data( sdb_table_t *src, sdb_table_t *dst,
        int offset, int length) {
    int r;
    switch( src->storage_kind) {
    case SDB_SK_RAM: r = copy_data_ram( src, dst, offset, length); break;
    case SDB_SK_FILE: r = copy_data_file( src, dst, offset, length); break;
    default: r = SDB_EINTERNAL; break;
    }
    if( SDB_EOK == r) {
      /* as data is copied directly, nwrittenbytes is updated but not nwrittenobjects. */
      dst->nwrittenobjects++;
    }
    return r;
}
/* Finalize a consolidation by writing the result in the specified
 * column of the table. The consolidation context has been initialized
 * with cons_init(), and every value of the source column has been
 * passed to cons_reduce() until one of them returned non-zero. */
static void cons_finalize( struct sdb_cons_ctx_t *ctx,
        sdb_table_t *src, sdb_table_t *dst) {
    union sdb_cons_ctx_content_t *u = & ctx->content;
    int r;
    if( SDB_CCS_BROKEN == ctx->state) {
        sdb_null( dst);
        if( SDB_CM_MEDIAN == ctx->method && u->median) {
            free( u->median);  u->median = NULL;
        }
        return;
    }

    switch( ctx->method) {

    case SDB_CM_FIRST:
    case SDB_CM_LAST:
    case SDB_CM_MIDDLE:
        r = copy_data( src, dst, u->streampos.offset, u->streampos.length);
        if( r != SDB_EOK) sdb_null( dst);
        return;

    case SDB_CM_MAX:    r = sdb_number( dst, u->max); return;
    case SDB_CM_MEAN:   r = sdb_number( dst, u->sum / ctx->nrows); return;
    case SDB_CM_MIN:    r = sdb_number( dst, u->min); return;
    case SDB_CM_SUM:    r = sdb_number( dst, u->sum); return;

    case SDB_CM_MEDIAN:
        qsort( u->median, ctx->nrows, sizeof( double), cmp_pdouble);
        r = sdb_number( dst, u->median [ctx->nrows/2]);
        free( u->median); u->median = NULL;
        return;
    }
}

static void cons_close( struct sdb_cons_ctx_t *ctx) {
    if( SDB_CM_MEDIAN == ctx->method && ctx->content.median) {
        free( ctx->content.median);
        ctx->content.median = NULL;
    }
}

/* If the table is configured to consolidate itself into another,
 * consolidate each destination column from the source table. */
int sdb_consolidate( sdb_table_t *src) {
    struct sdb_consolidation_t *cons = src->consolidation;
    struct sdb_cons_ctx_t      *cctx;
    struct sdb_table_t         *dst;
    struct sdb_read_ctx_t       rctx;
    sdb_ncolumn_t              *matrix;
    sdb_ncolumn_t               i_src_col, i_dst_col, n_src_col, n_dst_col;
    sdb_nrow_t                  i_src_row, n_src_row;

    if( ! cons) return SDB_ENOCONS;
    dst       = cons->dst;
    n_src_col = src->ncolumns;
    n_dst_col = dst->ncolumns;
    n_src_row = src->nwrittenobjects / src->ncolumns;
    if( n_src_row <= 0) return SDB_EEMPTY;
    if( (dst->state != SDB_ST_READING) || (src->state != SDB_ST_READING)) return SDB_EBADSTATE;

    /* Create one consolidation context per destination column. */
    cctx = malloc( n_dst_col * sizeof( *cctx));
    if( ! cctx) goto cctx_malloc_fail;
    for( i_dst_col = 0;  i_dst_col < n_dst_col;  i_dst_col++) {
        int r = cons_init( cctx + i_dst_col,
                cons->dst_columns[i_dst_col].method,
                n_src_row);
        if( SDB_EOK != r) goto cons_init_fail;
    }

    sdb_read_init( & rctx, src);

    /* Create the consolidation matrix:
     * conceptually a 2D matrix with n_src_col rows of at most n_dst_col
     * columns: to each src column, it associates the list of dst columns
     * which consolidate it.
     *
     * A first extra column is reserved to hold the number of significant cells
     * in this row.
     *
     * Given C's cumbersome handling of 2D dynamically sized arrays, the matrix
     * is projected unto a 1D vector. */
    matrix = malloc( n_src_col * (1+n_dst_col) * sizeof( matrix [1]));
    if( ! matrix) goto matrix_malloc_fail;
    /* number of dst columns associated with a given source column. */
# define MATRIX_N_DST_COL( src_col)    (matrix [(src_col)*(1+n_dst_col)])
    /* i-th dst column associated with src_col. */
# define MATRIX_DST_COL(   src_col, i) (matrix [(src_col)*(1+n_dst_col)+i+1])

    /* Empty the matrix. */
    for( i_src_col = 0;  i_src_col < n_src_col; i_src_col++)
        MATRIX_N_DST_COL( i_src_col) = 0;

    /* Fill the matrix. */
    for( i_dst_col = 0; i_dst_col < n_dst_col; i_dst_col++) {
        i_src_col = cons->dst_columns[i_dst_col].src_column;
        MATRIX_DST_COL( i_src_col, MATRIX_N_DST_COL( i_src_col)++) = i_dst_col;
    }

#ifdef SDB_VERBOSE_PRINT /* Check the matrix' content: */
    printf( "Consolidation matrix:\n");
    for( i_src_col = 0;  i_src_col < n_src_col; i_src_col++) {
        sdb_ncolumn_t n_cons = MATRIX_N_DST_COL( i_src_col), i_cons;
        printf( "SRC column %i used by %i DST columns:",i_src_col, n_cons);
        for( i_cons = 0;  i_cons < n_cons; i_cons++) {
            i_dst_col = MATRIX_DST_COL( i_src_col, i_cons);
            printf(" %i", i_dst_col);
        }
        printf( ".\n");
    }
#endif

    /* For each source cell: */
    for( i_src_row = 0;  i_src_row < n_src_row;  i_src_row++) {
        for( i_src_col = 0;  i_src_col < n_src_col;  i_src_col++) {
            /* # of dst columns which consolidate this cell. */
            sdb_ncolumn_t n_cons = MATRIX_N_DST_COL( i_src_col), i_cons;
            struct bsd_data_t bsd_data;
            int offset = rctx.nreadbytes;
            int length = sdb_read_data( & rctx, & bsd_data, n_cons>0);
            if( length<0) goto reading_fail;
            /* For each dst cell consolidating this src cell: */
            for( i_cons = 0;  i_cons < n_cons; i_cons++) {
                i_dst_col = MATRIX_DST_COL( i_src_col, i_cons);
                cons_reduce( cctx + i_dst_col, & bsd_data, offset, length);
            }
        }
    }

    /* Finalize the consolidation contexts. */
    for( i_dst_col = 0;  i_dst_col < n_dst_col;  i_dst_col++) {
        cons_finalize( cctx + i_dst_col, src, dst);
        cons_close(  cctx + i_dst_col);
    }
    free( cctx);
    free( matrix);
    return SDB_EOK;

    if( 0) {
        reading_fail:
        sdb_read_close( & rctx);
        matrix_malloc_fail:
        i_dst_col = n_dst_col;
        cons_init_fail:
        for(; i_dst_col; i_dst_col--) cons_close(cctx + i_dst_col - 1);
        free( cctx);
        cctx_malloc_fail:
        return SDB_EMEM;
    }
}
