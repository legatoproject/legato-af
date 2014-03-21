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
#include <stdarg.h>
#include <unistd.h> // ftruncate()

static int nextpwr2( int x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    return ++x;
}

static int new_conf_string( sdb_table_t *tbl, const char *str) {
    int length = strlen( str) + 1;
    int a = nextpwr2( tbl->conf_string_idx);
    int b = nextpwr2( tbl->conf_string_idx + length);
    int c;
    if( a != b) {
        char *d = realloc( tbl->conf_strings, b);
        if( ! d) return SDB_EMEM;
        tbl->conf_strings = d;
    }
    c = tbl->conf_string_idx;
    memcpy( tbl->conf_strings + c, str, length);
    tbl->conf_string_idx += length;
    return c;
}

/* Read back existing cells when table is opened with existing file. */
static void sdb_restore_file_cells( sdb_table_t *tbl) {
    struct sdb_read_ctx_t rctx;
    struct bsd_data_t bsd;
    int nread;
    sdb_read_init( & rctx, tbl);
    while( (nread = sdb_read_data( & rctx, & bsd, 1)) > 0) {
        // restore data analysis
        switch( bsd.kind) {
        case BSD_INT:    sdb_analyze_integer(tbl, bsd.content.i); break;
        case BSD_DOUBLE: sdb_analyze_noninteger(tbl, 1); break;
        default:         sdb_analyze_noninteger(tbl, 0); break;
        }

        tbl->nwrittenbytes += nread;
        tbl->nwrittenobjects ++;
    }
//          printf( "exited with code %d, found %d objects encoded in %d bytes\n",
//                  nread, tbl->nwrittenobjects, tbl->nwrittenbytes);
    sdb_read_close( & rctx);
}

/* Initialize the table passed as 1st parameter as an unconfigured table.
 * next steps: configure its columns with sdb_column(), write data in it,
 * periodically serialize or consolidate it. */
int sdb_initwithoutcolumns( sdb_table_t *tbl, const char *id, sdb_ncolumn_t ncolumns,
        enum sdb_storage_kind_t storage_kind) {
    int r;
#ifdef SDB_VERBOSE_PRINT
    printf( "Initialized %d bytes of sdb struct\n", sizeof( * tbl));
#endif

    if( 0 == ncolumns || SDB_NCOLUMN_INVALID == ncolumns)
        return SDB_EINVALID;

    memset( tbl, 0, sizeof( *tbl));

    tbl->conf_string_idx   = 0;
    tbl->conf_strings      = NULL;
    r = new_conf_string( tbl, id);
    if( r<0) goto fail_id;

    /* Allocate dynamic arrays. */
    tbl->columns = malloc( ncolumns * sizeof( tbl->columns[0]));
    if( ! tbl->columns) goto fail_columns;

    /* Other initializations. */
    tbl->state             = SDB_ST_UNCONFIGURED;
    tbl->ncolumns          = ncolumns;
    tbl->consolidation     = NULL;
    tbl->serialization_ctx = NULL;
    tbl->nwrittenbytes     = 0;
    tbl->nwrittenobjects   = 0;
    tbl->conf_col          = 0;
    tbl->maxwrittenobjects = 0;

    r = sdb_untrim( tbl);
    if( r<0) goto fail_untrim;

    switch(storage_kind) {
    case SDB_SK_RAM: {
        struct sdb_ram_storage_t *ram = & tbl->storage.ram;
        sdb_chunk_t *c = malloc( sizeof( * c)+SDB_MIN_CHUNK_SIZE-SDB_CHUNK_SIZE);
        if( ! c) goto fail_chunk;
        c->next = NULL;
        tbl->storage_kind = SDB_SK_RAM;
        ram->first_chunk = c;
        ram->last_chunk = c;
        ram->last_chunk_ptr = & ram->last_chunk;
        ram->last_chunk_size = SDB_MIN_CHUNK_SIZE;
        break;
    }
#ifdef SDB_FLASH_SUPPORT
    case SDB_SK_FLASH: {
        tbl->storage_kind = SDB_SK_FLASH;
#        error "flash storage not implemented"
        break;
    }
#endif
#ifdef SDB_FILE_SUPPORT
    case SDB_SK_FILE: {
        FILE *fd = fopen( id, "a+");
        if( ! fd) return SDB_EBADFILE;
        tbl->storage_kind = SDB_SK_FILE;
        tbl->storage.file = fd;

        // TODO: get in a state where config is retrieved and
        // column configuration through sdb_setcolumn() calls
        // can be skipped?
        break;
    }
#endif
    }

    /* Handle allocation failures. */
    if( 0) {
        fail_chunk:
        sdb_trim( tbl);
        fail_untrim:
        free( tbl->columns);
        fail_columns:
        free( tbl->conf_strings);
        fail_id:
        memset( tbl, 0, sizeof( *tbl));
        tbl->state = SDB_ST_BROKEN;
        return SDB_EMEM;
    }

    return SDB_EOK;
}

int sdb_untrim( sdb_table_t *tbl) {
    if( tbl->bss_ctx) return SDB_EBADSTATE; else {
        bss_ctx_t *bss = malloc( sizeof( *bss));
        if( ! bss) return SDB_EMEM;
        bss_init( bss, sdb_bss_writer, tbl);
        tbl->bss_ctx = bss;
        return SDB_EOK;
    }
}

int sdb_trim( sdb_table_t *tbl) {
    if( tbl->state != SDB_ST_READING) {
        return SDB_EBADSTATE;
    }
    if( tbl->bss_ctx) {
        free( tbl->bss_ctx);
        tbl->bss_ctx=NULL;
    }
    switch( tbl->storage_kind) {
    case SDB_SK_RAM: sdb_ram_trim( tbl); break;
    default: break;
    }
    return SDB_EOK;
}

/* Release resources reserved by the table.
 * Warning: it is an error, with unspecified result, to close a table
 * which is the consolidation destination of another table. If a consolidation
 * attempt is made on the source table, a memory corruption is likely to occur. */
void sdb_close( sdb_table_t *tbl) {
    if( tbl->state == SDB_ST_SERIALIZING) sdb_serialize_cancel( tbl);
    tbl->state = SDB_ST_BROKEN;
    if( tbl->columns) {
      free( tbl->columns);
      tbl->columns = NULL;
    }
    if( tbl->consolidation) {
        // columns are allocated in the same block as the cons struct.
        // free( tbl->consolidation->dst_columns);
        free( tbl->consolidation);
        tbl->consolidation = NULL;
    }

    switch( tbl->storage_kind) {
    case SDB_SK_RAM: {
        struct sdb_chunk_t *p = tbl->storage.ram.first_chunk, *q;
        while( p) {
            q=p->next; free( p); p=q;
        }
        tbl->storage.ram.first_chunk = tbl->storage.ram.last_chunk = NULL;
        break;
    }
    case SDB_SK_FILE:
        if(tbl->storage.file) {
          fclose( tbl->storage.file);
          tbl->storage.file = NULL;
        }
        break;
    }

    if( tbl->bss_ctx) {
        free( tbl->bss_ctx);
        tbl->bss_ctx=NULL;
    }

    free( tbl->conf_strings);
    tbl->conf_strings = NULL;
}

int sdb_init( sdb_table_t *tbl, const char *id,
        enum sdb_storage_kind_t storage_kind, ...) {
    enum { COUNTING, CONFIGURING, DONE } stage;
    const char *colname;
    va_list ap;
    sdb_ncolumn_t ncolumns = 0;
    for( stage=COUNTING; stage<DONE; stage++) {
        for(va_start( ap, storage_kind); NULL != (colname=va_arg( ap, const char *));) {
            enum sdb_serialization_method_t sm = va_arg( ap, int);
            double precision = 0.0;
            if( (SDB_SM_CONTAINER(sm) == SDB_SM_DELTAS_VECTOR) ||
                (SDB_SM_CONTAINER(sm) == SDB_SM_QUASIPERIODIC_VECTOR) ||
                (SDB_SM_CONTAINER(sm) == SDB_SM_SMALLEST && (sm & SDB_SM_FIXED_PRECISION)))
                precision = va_arg( ap, double);
            if( stage==COUNTING) {
                ncolumns++;
            } else {
                sdb_setcolumn( tbl, colname, sm, precision);
            }
        }
        va_end( ap);
        if( COUNTING==stage) {
            int r = sdb_initwithoutcolumns( tbl, id, ncolumns, storage_kind);
            if( SDB_EOK != r) return r;
        }
    }
    if( tbl->state != SDB_ST_READING) return SDB_EINTERNAL;
    return SDB_EOK;
}

/* Release all resources reserved by the table.
 * If there is a serialization in progress, it is canceled. */
int sdb_reset( sdb_table_t *tbl) {
    int i;
    if( tbl->state == SDB_ST_SERIALIZING) sdb_serialize_cancel( tbl);
    if( tbl->state != SDB_ST_READING) return SDB_EBADSTATE;

    switch( tbl->storage_kind) {
    case SDB_SK_RAM: {
        struct sdb_chunk_t *p, *q;
        struct sdb_ram_storage_t *ram = & tbl->storage.ram;
        p = ram->first_chunk;
        while( p) {
            q=p->next; free( p); p=q;
        }
        p = malloc( sizeof( struct sdb_chunk_t) + SDB_MIN_CHUNK_SIZE - SDB_CHUNK_SIZE);
        if( ! p) { tbl->state = SDB_ST_BROKEN; return SDB_EMEM; }
        p->next = NULL;
        ram->first_chunk = p;
        ram->last_chunk = p;
        ram->last_chunk_ptr =  & ram->last_chunk;
        ram->last_chunk_size = SDB_MIN_CHUNK_SIZE;
        break;
    }
    case SDB_SK_FILE:
        // identifier/filename is stored as the 1st conf string
        tbl->storage.file = freopen( tbl->conf_strings, "w+", tbl->storage.file); // erases content
        if( ! tbl->storage.file) return SDB_EBADFILE;
        break;
    }
    tbl->nwrittenbytes     = 0;
    tbl->nwrittenobjects   = 0;

    // reset data analysis
    for( i=0; i<tbl->ncolumns; i++) {
        sdb_column_t *c = tbl->columns + i;
        if( SDB_SM_SMALLEST == SDB_SM_CONTAINER(c->serialization_method)) {
            c->data_analysis.delta_sum = 0;
            c->data_analysis.all_integer = 1;
            c->data_analysis.all_numeric = 1;
        }
    }

    if( tbl->bss_ctx) bss_reset( tbl->bss_ctx);
    return SDB_EOK;
}

int sdb_setcolumn(  sdb_table_t *tbl, const char *label,
        enum sdb_serialization_method_t sm, double precision) {
    sdb_column_t *c = tbl->columns + tbl->conf_col;
    // TODO: if it's a persisted table, check consistency instead of setting
    // up configuration?
    if( tbl->state != SDB_ST_UNCONFIGURED) return SDB_EBADSTATE;
    c->serialization_method = sm;
    c->arg = precision;

    if( SDB_SM_SMALLEST == SDB_SM_CONTAINER(sm)) {
        c->data_analysis.original_arg = precision;
        c->data_analysis.delta_sum = 0;
        c->data_analysis.all_integer = 1;
        c->data_analysis.all_numeric = 1;
    }

    c->label_offset = new_conf_string( tbl, label);
    if( c->label_offset<0) {
        return SDB_EMEM;
    } else if( ++tbl->conf_col == tbl->ncolumns) {
        tbl->state = SDB_ST_READING;
        // Trim unnecessary space at the end of sting cfg;
        tbl->conf_strings = realloc( tbl->conf_strings, tbl->conf_string_idx);
        // read existing cells if table is a file
        if( tbl->storage_kind == SDB_SK_FILE) {
            sdb_restore_file_cells( tbl);
        }
    }
    return SDB_EOK;
}

int sdb_setconstable(  sdb_table_t *src,  sdb_table_t *dst) {
    struct sdb_consolidation_t *cons;
    if( (src->state == SDB_ST_BROKEN) || (src->state == SDB_ST_UNCONFIGURED)) return SDB_EBADSTATE;
    if( src->consolidation) return SDB_EINVALID;
    cons = malloc( sizeof( * cons) + dst->ncolumns * sizeof( * cons->dst_columns));
    if( ! cons) return SDB_EMEM;
    cons->dst = dst;
    cons->dst_columns  = (void*) (cons+1); // TODO: use the struct hack [http://c2.com/cgi/wiki?StructHack]?
    cons->conf_col     = 0;
    src->consolidation = cons;
    return SDB_EOK;
}

int sdb_setconscolumn( sdb_table_t *src,
        sdb_nrow_t src_col,
        sdb_consolidation_method_t method) {
    struct sdb_cons_column_t *cc;
    struct sdb_consolidation_t *cons = src->consolidation;
    if( (src->state == SDB_ST_BROKEN) || (src->state == SDB_ST_UNCONFIGURED)) return SDB_EBADSTATE;
    if( ! cons) return SDB_EINVALID;
    if( src_col >= src->ncolumns) return SDB_EINVALID;
    if( cons->conf_col >= cons->dst->ncolumns) return SDB_EINVALID;

    cc             = cons->dst_columns + cons->conf_col++;
    cc->method     = method;
    cc->src_column = src_col;
    return SDB_EOK;
}

sdb_ncolumn_t sdb_getcolnum( sdb_table_t *tbl, const char *name) {
    sdb_ncolumn_t i;
    if( (tbl->state == SDB_ST_BROKEN) || (tbl->state == SDB_ST_UNCONFIGURED)) return SDB_NCOLUMN_INVALID;
    for( i=0; i < tbl->ncolumns; i++) {
        const char *colname = tbl->conf_strings + tbl->columns[i].label_offset;
        if( ! strcmp( name, colname)) return i;
    }
    return SDB_NCOLUMN_INVALID;
}

const char *sdb_getcolname( sdb_table_t *tbl, sdb_ncolumn_t icol) {
  if( (tbl->state == SDB_ST_BROKEN) || (tbl->state == SDB_ST_UNCONFIGURED)) return NULL;
    return tbl->conf_strings + tbl->columns[icol].label_offset;
}
