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

/* Serialize a table content.
 * The serialization is streamed, i.e. it can return BSS_EAGAIN if the writer
 * can't take the whole output in one piece.
 * In such cases, the serialization function must be called again when the
 * writer isn't in overflow anymore, until it returns BSS_EOK.
 *
 * The implementation of the resume feature relies on a trick inspired by Duff's Device
 * (cf. Google for details): a switch statement wraps the code that might be resumed,
 * and case labels are placed where the code might want to resume.
 */

#include "sdb_internal.h"
#include <math.h>
#include <limits.h>
#include <stdlib.h> // bsearch

#define N_MODE_CANDIDATES 32

#define TRY( w, S) do {               \
        int _r = (w);                 \
        if(BSS_EOK != _r) return _r; \
        else ctx->stage = S;          \
} while( 0)


/* Serializing a column: the entire table must be read in sequence, and each
 * column cell must be extracted and treated (here written straight
 * to the target list). */
static bss_status_t serialize_column_list( struct sdb_table_t *tbl) {

    struct sdb_serialization_ctx_t *ctx = tbl->serialization_ctx;
    struct sdb_read_ctx_t *read_ctx = & ctx->read_ctx;
    struct bss_ctx_t *bss_ctx = ctx->bss_ctx;
    struct bsd_data_t bsd_data;
    int nrows = tbl->nwrittenobjects / tbl->ncolumns;
    int nobjectstoread = nrows * tbl->ncolumns; //TODO could optimize away end of last row
    int incolumn = 1;

    switch (ctx->stage) {
    default: return BSS_EINTERNAL;

    case SDB_SS_MAP_LABEL_SENT:
        // TODO: find a smarter CTXID depending on content's type
        TRY( bss_list( bss_ctx, nrows, BS_CTXID_GLOBAL), SDB_SS_COLUMN_SENDING_CELLS);
        while( read_ctx->nreadobjects < nobjectstoread) {
            incolumn = read_ctx->nreadobjects%tbl->ncolumns == ctx->current_column;
            int r = sdb_read_data( read_ctx, & bsd_data, ! incolumn);
            if( r<0)
                return r;
    case SDB_SS_COLUMN_SENDING_CELLS:
        if( incolumn) {
            /* bss_raw might be run more than once, it will handle cases
             * where only part of the data has been sent transparently,
             * thanks to bss' transaction system.
             *
             * If we jump here directly, because serialization has been resumed
             * in state SDB_SS_COLUMN_SENDING_CELLS, incolumn is always true:
             * the previous serialization attempt stopped in this state,
             * because the call to bss_raw() below failed on BSS_EAGAIN.
             * Therefore, read_ctx was already pointing to something to
             * serialize, and we were "incolumn" indeed. */
            TRY( bss_raw( bss_ctx, read_ctx->bytes, read_ctx->nbytes), SDB_SS_COLUMN_SENDING_CELLS);
        }
        }
        ctx->stage = SDB_SS_COLUMN_CONTENT_SENT;
        TRY( bss_close( bss_ctx), SDB_SS_COLUMN_CLOSED);
    }
    return BSS_EOK;
}

static int get_bsd_value( sdb_read_ctx_t *ctx, double *value) {
    bsd_data_t data;
    if( bsd_read(& ctx->bsd_ctx, & data, ctx->bytes, ctx->nbytes) < ctx->nbytes) return 0; // read error, bad type

    if( data.type == BSD_INT) {
        *value = data.content.i;
    } else if( data.type == BSD_DOUBLE) {
        *value = data.content.d;
    } else {
        return 0; // bad type.
    }
    return 1;
}

/**
 * Floor a value to the integer to serialize for DeltasVector.
 * See dvinteger function in hessian.m3da for details.
 */
static int deltasvector_integer(double value, double precision) {
    if( value >= 0) {
        return floor(value) + (fmod(value, 1) >= (1 - precision) ? 1 : 0);
    } else {
        double rem = fmod(value, 1);
        return floor(value) + ((rem > 0 && rem <= precision) ? 1 : 0);
    }
}

static bss_status_t serialize_column_deltasvector( struct sdb_table_t *tbl) {

    struct sdb_serialization_ctx_t *ctx = tbl->serialization_ctx;
    struct sdb_read_ctx_t *read_ctx = & ctx->read_ctx;
    struct bss_ctx_t *bss_ctx = ctx->bss_ctx;
    struct bsd_data_t bsd_data;
    struct sdb_column_t *column = tbl->columns + ctx->current_column;
    int nrows = tbl->nwrittenobjects / tbl->ncolumns;
    int nobjectstoread = nrows * tbl->ncolumns; //TODO could optimize away end of last row
    int incolumn = 1;

    switch (ctx->stage) {
    default: return BSS_EINTERNAL;

    case SDB_SS_MAP_LABEL_SENT:
        TRY( bss_object(bss_ctx, SDB_CLSID_DELTAS_VECTOR), SDB_SS_COLUMN_OBJECT_DEFINED); //TODO: use constants for class ID
    case SDB_SS_COLUMN_OBJECT_DEFINED:
        TRY( bss_double(bss_ctx, column->arg), SDB_SS_COLUMN_FACTOR_SENT);
        while( read_ctx->nreadobjects < nobjectstoread) {
            incolumn = read_ctx->nreadobjects%tbl->ncolumns == ctx->current_column;
            int r = sdb_read_data( read_ctx, & bsd_data, ! incolumn);
            if( r<0)
                return r;
            // the process is roughly the same on both situations
    case SDB_SS_COLUMN_FACTOR_SENT:
    case SDB_SS_COLUMN_SENDING_CELLS:
    case SDB_SS_COLUMN_START_VALUE_SENT:
        if( incolumn) {
            double value;
            // do NOT use bsd_data here because the process could be interrupted which would lead to uninitialized values
            if( !get_bsd_value( read_ctx, &value)) return BSS_EINVALID;

            // on first value, open the deltas list for next values
            // a sub-switch is needed because of value which must be initialized in any case
            switch (ctx->stage) {
            case SDB_SS_COLUMN_FACTOR_SENT: {
                double start = deltasvector_integer(value / column->arg, fabs(value/1e15));
                TRY( bss_int(bss_ctx, start), SDB_SS_COLUMN_START_VALUE_SENT);
                ctx->previous = value;
            }
            case SDB_SS_COLUMN_START_VALUE_SENT:
                // TODO: There might be a better CTXID
                TRY( bss_list(bss_ctx, nrows-1, BS_CTXID_NUMBER), SDB_SS_COLUMN_SENDING_CELLS); // TODO: can be typed
                break;
            default: {
                int int_delta = deltasvector_integer((value - ctx->previous) / column->arg, fabs(value/1e15));
                TRY( bss_int(bss_ctx, int_delta), SDB_SS_COLUMN_SENDING_CELLS);
                if( int_delta != 0) {
                    ctx->previous = value;
                }
            }
            }
        }
        }
        ctx->stage = SDB_SS_COLUMN_CONTENT_SENT;
        // close the deltas list and then DV container
    case SDB_SS_COLUMN_CONTENT_SENT:
        TRY( bss_close( bss_ctx), SDB_SS_COLUMN_INNER_LIST_CLOSED);
    case SDB_SS_COLUMN_INNER_LIST_CLOSED:
        TRY( bss_close( bss_ctx), SDB_SS_COLUMN_CLOSED);
    }
    return BSS_EOK;
}

/**
 * Handle QPV cell serializing
 * This code has been pulled out of serialize_column_quasiperiodicvector to be clearer.
 */
static bss_status_t serialize_cell_quasiperiodicvector(sdb_serialization_ctx_t *ctx, double period) {
    double value;
    struct bss_ctx_t *bss_ctx = ctx->bss_ctx;
    if( !get_bsd_value( &(ctx->read_ctx), &value)) return BSS_EINVALID;

    // first value, previous value not set yet
    if( ctx->stage == SDB_SS_COLUMN_FACTOR_SENT || ctx->stage == SDB_SS_COLUMN_START_VALUE_SENT) {
        switch( ctx->stage) {
        default: return BSS_EINTERNAL;
        case SDB_SS_COLUMN_FACTOR_SENT:
            TRY( bss_double( bss_ctx, value), SDB_SS_COLUMN_START_VALUE_SENT);
        case SDB_SS_COLUMN_START_VALUE_SENT:
            // TODO: Check whether that's the best CTXID possible
            TRY( bss_list( bss_ctx, -1, BS_CTXID_NUMBER), SDB_SS_COLUMN_SENDING_CELLS); // TODO: can be typed
            ctx->current_shift = 0;
        }
    } else {
        double shift = value - (ctx->previous + period);
        // try to correct float inaccuracies
        if( fabs(shift) <= fabs(value / 1e15)) {
            ctx->current_shift += 1;
        } else {
            // this can be executed if shift sending has failed, the shift count shall not be sent again
            switch( ctx->stage) {
            default: return BSS_EINTERNAL;
            case SDB_SS_COLUMN_SENDING_CELLS:
                TRY( bss_int(bss_ctx, ctx->current_shift), SDB_SS_COLUMN_SHIFT_SENT);
            case SDB_SS_COLUMN_SHIFT_SENT:
                TRY( bss_double( bss_ctx, shift), SDB_SS_COLUMN_SENDING_CELLS);
                ctx->current_shift = 0;
            }
        }
    }
    ctx->previous = value;
    return BSS_EOK;
}

static bss_status_t serialize_column_quasiperiodicvector(struct sdb_table_t *tbl) {

    struct sdb_serialization_ctx_t *ctx = tbl->serialization_ctx;
    struct sdb_read_ctx_t *read_ctx = & ctx->read_ctx;
    struct bss_ctx_t *bss_ctx = ctx->bss_ctx;
    struct bsd_data_t bsd_data;
    struct sdb_column_t *column = tbl->columns + ctx->current_column;
    int nrows = tbl->nwrittenobjects / tbl->ncolumns;
    int nobjectstoread = nrows * tbl->ncolumns; //TODO could optimize away end of last row
    int incolumn = 1;

    switch (ctx->stage) {
    default: return BSS_EINTERNAL;

    case SDB_SS_MAP_LABEL_SENT:
        TRY( bss_object(bss_ctx, SDB_CLSID_QUASI_PERIODIC_VECTOR), SDB_SS_COLUMN_OBJECT_DEFINED); //TODO: use constants for class ID
    case SDB_SS_COLUMN_OBJECT_DEFINED:
        TRY( bss_double(bss_ctx, column->arg), SDB_SS_COLUMN_FACTOR_SENT);

        while( read_ctx->nreadobjects < nobjectstoread) {
            incolumn = read_ctx->nreadobjects%tbl->ncolumns == ctx->current_column;
            int r = sdb_read_data( read_ctx, & bsd_data, ! incolumn);
            if( r<0)
                return r;
    case SDB_SS_COLUMN_FACTOR_SENT:
    case SDB_SS_COLUMN_START_VALUE_SENT:
    case SDB_SS_COLUMN_SENDING_CELLS:
    case SDB_SS_COLUMN_SHIFT_SENT:
        if( incolumn) {
            bss_status_t res = serialize_cell_quasiperiodicvector(ctx, column->arg);
            if( res != BSS_EOK) return res;
        }
        }
        ctx->stage = SDB_SS_COLUMN_CONTENT_SENT;
    case SDB_SS_COLUMN_CONTENT_SENT:
        // finalize the container: send the last shift count
        TRY( bss_int(bss_ctx, ctx->current_shift), SDB_SS_COLUMN_LAST_SHIFT_SENT);

    case SDB_SS_COLUMN_LAST_SHIFT_SENT:
        // close shift list
        TRY( bss_close(bss_ctx), SDB_SS_COLUMN_INNER_LIST_CLOSED);

    case SDB_SS_COLUMN_INNER_LIST_CLOSED:
        TRY( bss_close( bss_ctx), SDB_SS_COLUMN_CLOSED);
    }

    return BSS_EOK;
}

static void serialize_close( struct sdb_table_t *tbl) {
    sdb_read_close( & tbl->serialization_ctx->read_ctx);
    free( tbl->serialization_ctx);
    tbl->serialization_ctx = NULL;
}

static bss_status_t serialize_init( struct sdb_table_t *tbl, bss_ctx_t *bss_ctx) {
    struct sdb_serialization_ctx_t *ctx = malloc( sizeof( *ctx));
    if( ! ctx) return SDB_EMEM;
    tbl->serialization_ctx = ctx;
    memset( ctx, 0, sizeof( *ctx)); // NULL all pointers
    ctx->current_column = 0;
    ctx->stage = SDB_SS_INITIALIZED;
    ctx->bss_ctx = bss_ctx;
    // current_column and read_ctx will be initialized in the serialization loop.
    return BSS_EOK;
}

/* Returns the size in bytes that would take given integer/double */
// FIXME: put it in hessians ?
static int bss_int_size(int x) {
    if( -(0x10)<=x && x<=0x2f)            return 1;
    else if( -(0x800)<=x && x<=0x7ff)     return 2;
    else if( -(0x40000)<=x && x<=0x3ffff) return 3;
    else                                  return 5;
}

static int bss_double_size(double x) {
    int y = (int) x;
    if( (double) y == x && -0x8000 <= y && y < 0x8000) {
        if( y == 0 || y == 1)            return 1;
        else if( -0x80 <= y && y < 0x80) return 2;
        else                             return 3;
    }
    else if ( ((float) x) == x)          return 5;
    else                                 return 9;
}

struct data_mc_t { int key; int value; };

static int cmp_modecandidates( const void *e1, const void *e2) {
    const struct data_mc_t *x1 = e1, *x2 = e2;
    return x2->key - x1->key;
}

/* Compute the smallest serialization container using  data analysis and stored data.
 * The method is to estimate as precisely as possible final size and take the smallest one.
 * Store the result in serialization_data struct.
 */
static int compute_serialization_methods( struct sdb_table_t *tbl) {
    // Data used for 2nd pass computations.
    struct data_analysis_t {
        int vsize,dvsize, qpvsize;                            // Computed sizes.
        double dvfactor;                            // Data for DV computations.
        int qpvperiod, qpvcurrentn;                // Data for QPV computations.
        double dprevious;                            // Previous data as double.
        int iprevious;                                  // Previous data as int.
        struct data_mc_t modecandidates[N_MODE_CANDIDATES];
        int mc_len;
    };

    sdb_ncolumn_t nsmallest = 0;
    sdb_ncolumn_t current_smallest;
    int i;
    struct data_analysis_t *analysis_data = NULL;
    int return_code = SDB_EOK;
    struct sdb_read_ctx_t read_ctx;

    // count columns that needs serialization computation
    for( i=0; i<tbl->ncolumns; i++) {
        struct sdb_column_t *column = tbl->columns + i;
        if( SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method)) {
            // if any data is not numeric, DV and QPV are not able to serialize them
            if( !column->data_analysis.all_numeric) {
                column->data_analysis.method = SDB_SM_LIST;
            }
            // QPV period guessing is integer only, DV support floats only when factor is forced.
            else if( !column->data_analysis.all_integer &&
                    !(column->serialization_method & SDB_SM_FIXED_PRECISION)) {
                column->data_analysis.method = SDB_SM_LIST;
            }
            // otherwise the serialization method must be computed
            else {
                column->data_analysis.method = SDB_SM_SMALLEST;
                nsmallest++;
            }
        }
    }

    if( 0 == nsmallest) return SDB_EOK; // no column to analyze.

    analysis_data = malloc( sizeof(struct data_analysis_t) * nsmallest);
    if( !analysis_data) return SDB_EMEM;

    // Initialize analysis data for each column
    for( i=0, current_smallest=0; i<tbl->ncolumns; i++) {
        struct sdb_column_t *column = tbl->columns + i;
        if( SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method) &&
                SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->data_analysis.method)) {

            struct data_analysis_t *data = analysis_data + (current_smallest++);
            data->vsize = 0;
            data->dvfactor = (column->serialization_method & SDB_SM_FIXED_PRECISION) ?
                    column->data_analysis.original_arg : column->data_analysis.gcd;
            data->dvsize = bss_double_size(data->dvfactor);

            data->mc_len = 0;
        }
    }

    // First analysis pass: delta vectors + find most representative deltas
    // for quasi-periodic vectors.
    sdb_read_init( & read_ctx, tbl);
    for( i=0, current_smallest=0; i<tbl->nwrittenobjects; i++) {
        int column_index = read_ctx.nreadobjects%tbl->ncolumns;
        struct sdb_column_t *column = tbl->columns + column_index;
        bsd_data_t read_data;

        /* Is this column one of those to minimize? */
        int is_smallest = SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method) &&
                SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->data_analysis.method);
        int r = sdb_read_data( & read_ctx, & read_data, ! is_smallest);
        if( r<0) { return_code = r; goto compute_serialization_methods_exit; }
        if( ! is_smallest) continue;

        struct data_analysis_t *data = analysis_data + (current_smallest++ % nsmallest);
        double dvalue = (BSD_INT == read_data.type) ? ((double)read_data.content.i) : read_data.content.d;
        // This will cause errors, QPV size will be marked as wrong later.
        int ivalue = (BSD_INT == read_data.type) ? read_data.content.i : 0;

        data->vsize += read_ctx.nbytes;
        if( i<tbl->ncolumns) { // first cell
            data->dvsize += bss_double_size(dvalue/data->dvfactor);
        } else { // find QPV most representative deltas
            int idelta = ivalue - data->iprevious;
            struct data_mc_t idelta_c = { .key=idelta, .value=1 };
            struct data_mc_t *mc = bsearch( & idelta_c, data->modecandidates, data->mc_len, sizeof( struct data_mc_t),
                    cmp_modecandidates);
            if( mc) { /* Increment number of occurrences */
                mc->value++;
            } else if( data->mc_len < N_MODE_CANDIDATES) {
                mc = data->modecandidates + data->mc_len++;
                mc->key = idelta; mc->value = 1;
                // TODO: the table is almost sorted already; just do a memmove
                // to insert the entry at the correct position.
                qsort( data->modecandidates, data->mc_len, sizeof( int[2]), cmp_modecandidates);
            } else {
                /* Too many candidates, give up. */
            }
            // some corner-cases can lead to slight inaccuracies here?
            data->dvsize += bss_int_size(floor((dvalue - data->dprevious)/data->dvfactor));
        }
        data->dprevious = dvalue;
        data->iprevious = ivalue;
    }

    /* Retrieve the most common delta, to compute QVP sizes */
    for( i=0; i<nsmallest; i++) {
        int j, best_specimen, best_occurrences=0;
        struct data_analysis_t *data = analysis_data + i;
        for( j=0; j<data->mc_len; j++) {
            int specimen    = data->modecandidates[j].key;
            int occurrences = data->modecandidates[j].value;
            if( occurrences>best_occurrences) {
                best_specimen = specimen;
                best_occurrences = occurrences;
            }
        }
        data->qpvperiod=best_specimen;
    }

    // Second pass: compute QPV size now that we know the most frequent delta
    sdb_read_init( & read_ctx, tbl);
    for( i=0, current_smallest=0; i<tbl->nwrittenobjects; i++) {
        int column_index = read_ctx.nreadobjects % tbl->ncolumns;
        struct sdb_column_t *column = tbl->columns + column_index;
        bsd_data_t read_data;

        /* Is this column one of those to minimize? */
        int is_smallest = SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method) &&
                SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->data_analysis.method);
        int r = sdb_read_data( & read_ctx, & read_data, ! is_smallest);
        if( r<0) { return_code = r; goto compute_serialization_methods_exit; }
        if( ! is_smallest) continue;

        struct data_analysis_t *data = analysis_data + (current_smallest++ % nsmallest);
        double dvalue = (BSD_INT == read_data.type) ? ((double)read_data.content.i) : read_data.content.d;
        // This will cause errors, QPV size will be marked as wrong later.
        int ivalue = (BSD_INT == read_data.type) ? read_data.content.i : 0;

        if( i<tbl->ncolumns) { // first cell: size of start + period
            data->qpvsize = bss_int_size(ivalue) + bss_int_size(data->qpvperiod);
            data->qpvcurrentn = 0;
        } else {
            int qpvshift = ivalue - (data->iprevious + data->qpvperiod);
            if( 0 == qpvshift) {
                data->qpvcurrentn++;
            } else {
                data->qpvsize += bss_int_size(qpvshift) + bss_int_size(data->qpvcurrentn);
                data->qpvcurrentn = 0;
            }
        }
        if( i >= tbl->nwrittenobjects - tbl->ncolumns) { // Last cell
            data->qpvsize += bss_int_size(data->qpvcurrentn);
        }
        data->dprevious = dvalue;
        data->iprevious = ivalue;
    }

    // Finalize computation
    for( i=0, current_smallest=0; i<tbl->ncolumns; i++) {
        struct sdb_column_t *column = tbl->columns + i;
        if( SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method) &&
                SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->data_analysis.method)) {
            struct data_analysis_t *data = analysis_data + (current_smallest++);
            if( !column->data_analysis.all_integer) {
                data->qpvsize = INT_MAX;
            }
//#define SDB_VERBOSE_PRINT
#ifdef SDB_VERBOSE_PRINT
            printf("Data analysis results for %s/%s:\n"
                    " - List: size %d bytes\n"
                    " - Deltas Vector: size %d bytes; factor %f\n"
                    " - Quasi Periodic Vector: size %d bytes; period %d\n\n",
                    tbl->conf_strings,
                    tbl->conf_strings + column->label_offset,
                    data->vsize, data->dvsize, data->dvfactor, data->qpvsize, data->qpvperiod);
#endif

            // See also the "--FIXME M3DA QPV" tags in stagedb.lua tests
            // (some tests has been disabled/chaged)
            if( data->qpvsize < data->dvsize && data->qpvsize < data->vsize) {
                column->arg = data->qpvperiod;
                column->data_analysis.method = SDB_SM_QUASIPERIODIC_VECTOR;
            } else if( data->dvsize < data->vsize) {
                column->arg = data->dvfactor;
                column->data_analysis.method = SDB_SM_DELTAS_VECTOR;
            } else {
                column->data_analysis.method = SDB_SM_LIST;
            }
        }
    }

    compute_serialization_methods_exit:
    sdb_read_close( & read_ctx);
    free( analysis_data);
    return return_code;
}

static bss_status_t serialize_table( struct sdb_table_t *tbl) {
    struct sdb_serialization_ctx_t *ctx = tbl->serialization_ctx;
    struct bss_ctx_t *bss_ctx = ctx->bss_ctx;
    switch( ctx->stage) {
    default: return BSS_EINTERNAL;

    case SDB_SS_INITIALIZED:
        // TODO: check CTXID
        TRY( bss_map( bss_ctx, -1, BS_CTXID_GLOBAL), SDB_SS_MAP_OPENED);

        int r = compute_serialization_methods(tbl);
        if( r<0) return r;

    case SDB_SS_MAP_OPENED:
        for( ctx->current_column = 0;
                ctx->current_column<tbl->ncolumns;
                ctx->current_column++) {

    case SDB_SS_COLUMN_CLOSED:
        TRY( bss_string( bss_ctx,
                tbl->conf_strings + tbl->columns[ctx->current_column].label_offset),
                SDB_SS_MAP_LABEL_SENT);
        sdb_read_init( & ctx->read_ctx, tbl);

    case SDB_SS_MAP_LABEL_SENT:
    case SDB_SS_COLUMN_OBJECT_DEFINED:
    case SDB_SS_COLUMN_FACTOR_SENT:
    case SDB_SS_COLUMN_START_VALUE_SENT:
    case SDB_SS_COLUMN_SENDING_CELLS:
    case SDB_SS_COLUMN_CONTENT_SENT:
    case SDB_SS_COLUMN_SHIFT_SENT:
    case SDB_SS_COLUMN_LAST_SHIFT_SENT:
    case SDB_SS_COLUMN_INNER_LIST_CLOSED: {
        sdb_column_t *column = tbl->columns + ctx->current_column;
        int sm = (SDB_SM_SMALLEST == SDB_SM_CONTAINER(column->serialization_method)) ?
                column->data_analysis.method : column->serialization_method;
        switch(SDB_SM_CONTAINER(sm)) {
        case SDB_SM_LIST:
        case SDB_SM_FASTEST:
            TRY( serialize_column_list( tbl), SDB_SS_COLUMN_CLOSED);
            break;
        case SDB_SM_DELTAS_VECTOR:
            TRY( serialize_column_deltasvector( tbl), SDB_SS_COLUMN_CLOSED);
            break;
        case SDB_SM_QUASIPERIODIC_VECTOR:
            TRY( serialize_column_quasiperiodicvector( tbl), SDB_SS_COLUMN_CLOSED);
            break;
        default:
            return BSS_EINVALID; // bad value, not implemented, ...
        }
    }
    sdb_read_close( & ctx->read_ctx);

        }
        ctx->stage = SDB_SS_ALL_COLUMNS_SENT;
    case SDB_SS_ALL_COLUMNS_SENT:
        TRY( bss_close( bss_ctx), SDB_SS_MAP_CLOSED);
    case SDB_SS_MAP_CLOSED:;
    }
    serialize_close( tbl);
    return BSS_EOK;
}


int sdb_serialize_cancel( sdb_table_t *tbl) {
    if( tbl->state != SDB_ST_SERIALIZING) return SDB_EBADSTATE;
    serialize_close( tbl);
    tbl->state = SDB_ST_READING;
    return SDB_EOK;
}

bss_status_t sdb_serialize( sdb_table_t *tbl, bss_ctx_t *bss_ctx) {
    int r;
    switch( tbl->state) {
    case SDB_ST_READING:
        if( tbl->nwrittenobjects < tbl->ncolumns) return SDB_EOK; // empty table
        r = serialize_init( tbl, bss_ctx);
        if( r) return r;
        tbl->state = SDB_ST_SERIALIZING;
        /* fall through. */

    case SDB_ST_SERIALIZING:
        if( bss_ctx !=  tbl->serialization_ctx->bss_ctx) return BSS_EINVALID;
        r = serialize_table( tbl);
        if( SDB_EOK == r) tbl->state = SDB_ST_READING;
        return r;

    default:
        return SDB_EBADSTATE;
    }
}

