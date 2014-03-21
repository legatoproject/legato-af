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

/* AWTDA Staging databases.
 *
 * Allows to declare, fill and serialize data tables with a fixed number
 * of named columns.
 *
 * These tables can also be consolidated into one another; for instance,
 * a destination table can accumulate the mean value of a source table's
 * column everytime the source table gets full.
 *
 * Tables are not relational; the only way to get data out of them is to
 * have them serialized into an AWTDA message.
 */
#ifndef __STAGEDB_H_INDLUDED__
#define __STAGEDB_H_INDLUDED__

#ifdef __OAT_API_VERSION__
#define SDB_FLASH_SUPPORT
#else
#define SDB_FILE_SUPPORT
#endif

#include "bysants.h" // serialization

#ifdef SDB_FLASH_SUPPORT
// read/write from/to flash
#endif
#ifdef SDB_FILE_SUPPORT
#include <stdio.h> // fopen, fread, fwrite, fclose
#endif

typedef unsigned char  sdb_ncolumn_t;   // max = 256 columns
typedef unsigned short sdb_nrow_t;      // max = 65535 rows
typedef unsigned short sdb_data_size_t; // max = 64KB per serialized data
#define SDB_MIN_CHUNK_SIZE  0x100
#define SDB_DATA_SIZE_LIMIT 0x10000
#define SDB_NCOLUMN_INVALID 0xff

typedef enum sdb_classid_t {
    SDB_CLSID_DELTAS_VECTOR = 3,
    SDB_CLSID_QUASI_PERIODIC_VECTOR = 4
} sdb_classid_t;

typedef enum sdb_error_t {
    SDB_EOK           =   0,
    SDB_EBADSTATE     =  (-1),
    SDB_ETOOBIG       =  (-2),
    SDB_EINVALID      =  (-3),
    SDB_EMEM          =  (-4),
    SDB_ENOCONS       =  (-5),
    SDB_EBADFILE      =  (-6),
    SDB_ENILFORBIDDEN =  (-7),
    SDB_EFULL         =  (-8),
    SDB_EEMPTY        =  (-9),
    SDB_EINTERNAL     =  (-101),
} sdb_error_t;

/* Describe a table, in which data can be added, and which can flush itself
 * through consolidation and/or serialization whenever appropriate. */
typedef struct sdb_table_t {
    enum sdb_table_state_t {
        SDB_ST_UNCONFIGURED,        // Not all columns have been configured yet.
        SDB_ST_READING,                                 // Accepting data input.
        SDB_ST_SERIALIZING,   // Waiting for flush output stream to be consumed.
        SDB_ST_BROKEN                              // Experienced a fatal error.
    } state;                                           // Table's current state.
    sdb_ncolumn_t ncolumns;                          // # of columns (constant).
    struct sdb_column_t *columns;           // array of 'ncolumns' column descr.
    struct sdb_consolidation_t  *consolidation; // optional consolidation descr.
    enum sdb_storage_kind_t {
        SDB_SK_RAM,
#ifdef SDB_FLASH_SUPPORT
        SDB_SK_FLASH,
#endif
#ifdef SDB_FILE_SUPPORT
        SDB_SK_FILE,
#endif
    } storage_kind;
    union sdb_storage_t {
        struct sdb_ram_storage_t {
            // First and last data chunks:
            struct sdb_chunk_t *first_chunk, *last_chunk, **last_chunk_ptr;
            // Size of the last data chunk:
            // always a power of 2, and at least SDB_MIN_CHUNK_SIZE
            // WARNING: 0 in this field actually means 0x10000!
            sdb_data_size_t last_chunk_size;
        } ram;
#ifdef SDB_FLASH_SUPPORT
#error "flash storage not implemented"
#endif
#ifdef SDB_FILE_SUPPORT
        FILE *file;
#endif
    } storage;
    int nwrittenbytes;                 // # of bytes currently stored in chunks.
    int nwrittenobjects;             // # of objects currently stored in chunks.
    int maxwrittenobjects;         // max # of objects allowed. If 0, unlimited.
    sdb_ncolumn_t conf_col;              // tmp counter for table configuration.
    char  *conf_strings;                        // store column and table names.
    struct bss_ctx_t *bss_ctx;          // serialization to the staging storage.
    struct sdb_serialization_ctx_t *serialization_ctx; // only when serializing.
    short  conf_string_idx;             // where id and column names are stored.
    unsigned nilforbidden: 1;  // if true, trying to push a nil causes an error.
    unsigned checkxtrakeys: 1;                     // (used by Lua exportation).
} sdb_table_t;

// TODO could be converted to: union {
//    struct { *_chunk; nwritten_*; } reading;
//    struct { conf_* unconfigured; } } u;

/* Different ways to consolidate a column into a single value: */
typedef enum sdb_consolidation_method_t {
    SDB_CM_FIRST,
    SDB_CM_LAST,
    SDB_CM_MAX,
    SDB_CM_MEAN,
    SDB_CM_MEDIAN,
    SDB_CM_MIDDLE,
    SDB_CM_MIN,
    SDB_CM_SUM
} sdb_consolidation_method_t;

/* How a column must be serialized into the streamed AWTDA message.
 * The value must be one of the containers and optionally additional flags to
 * specify a lossy serialization (e.g. SDB_SM_SHORTEST|SDB_SM_4_BYTES_FLOATS).
 */
enum sdb_serialization_method_t {
    // Container: automatic selection
    SDB_SM_FASTEST = 0,                            // Low CPU usage, big result.
    SDB_SM_SMALLEST = 1,                        // High CPU usage, small result.
    // Container: manual selection
    SDB_SM_LIST = 2,                                            // Hessian List.
    SDB_SM_DELTAS_VECTOR = 3,                            // AWTDA Deltas vector.
    SDB_SM_QUASIPERIODIC_VECTOR = 4,             // AWTDA Quasi-periodic vector.

    // Optional lossy flags
    SDB_SM_4_BYTES_FLOATS = 1<<4,                     // Limits float precision.
    SDB_SM_FIXED_PRECISION = 1<<5,           // User-defined precision/compacity
                                     // compromise, only for SHORTEST container.
} sdb_serialization_method_t;
#define SDB_SM_CONTAINER(sm) (sm & 0x0F)

#define SDB_DEFAULT_SERIALIZATION_METHOD SDB_SM_SMALLEST

/* Initialize a table structure, return SDB_EOK or SDB_EMEM.
 * The result table must still have its columns configured with
 * calls to sdb_column() before it can accept data.
 *
 * This form is primarily useful when the column number,
 * names and serialization methods are not known at compile-time.
 * In simple cases, the sdb_init() interface is lighter and more readable. */
int sdb_initwithoutcolumns( sdb_table_t *tbl, const char *id, sdb_ncolumn_t ncolumns,
        enum sdb_storage_kind_t storage_kind);

/* Initialize a table structure, return SDB_EOK or SDB_EMEM.
 * The columns are passed as '...' parameters:
 *  - const char *column_name
 *  - serialization_method
 *  - optionally a precision as a double (see sdb_setcolumn for its meaning and
 *    applicable cases).
 * The columns description ends with NULL. */
int sdb_init( sdb_table_t *tbl, const char *id,
        enum sdb_storage_kind_t storage_kind, ...);

/* Release resources reserved by the table.
 * It is an error, with unspecified result, to close a table
 * which is the consolidation destination of another table. If a consolidation
 * attempt is made on the source table, a memory corruption is likely to occur. */
void sdb_close( sdb_table_t *tbl);

/* Remove all data currently storedin the table.
 * The table must be in reading mode, not unconfigured or streaming a result.*/
int sdb_reset( sdb_table_t *tbl);

/* Trim a DB: remove unused buffer space, kill the serialization buffer.
 * The first attempt to add data in the table after a trimming will be
 * much slower. */
int sdb_trim( sdb_table_t *tbl);

/* Configure the next column.
 * This must be called 'ncolumns' times before the table can be
 * used. The 'precision' argument is applicable only under certain
 * conditions:
 *   * for SDB_SM_SHORTEST when SDB_SM_FIXED_PRECISION is fixed,
 *     it is the deltas vector factor.
 *   * for SDB_SM_DELTAS_VECTOR, it is the factor
 *   * for SDB_SM_QUASIPERIODIC_VECTOR, it is the period.
 */
int sdb_setcolumn(     sdb_table_t *tbl, const char *label,
        enum sdb_serialization_method_t sm, double precision);

/* Declare 'dst' as the target consolidation table for 'src'.
 * A table can only be the source of one consolidation.
 * For the consolidation to be configured, sdb_setconscolumn() must be called
 * once per destination column, to describe how it is generated.
 */
int sdb_setconstable(  sdb_table_t *src,  sdb_table_t *dst);
int sdb_setconscolumn( sdb_table_t *src,
        sdb_nrow_t src_col,
        sdb_consolidation_method_t method);

/* Set a maximum # of rows accepted by the table.
 * If a table has a max # of rows, and adding new elements in it would create
 * more rows than allowed, SDB_EFULL will be returned.
 * sdb_setmaxrows() itself will return SDB_EFULL if the table already contains
 * more rows than would be allowed. */
int sdb_setmaxrows( sdb_table_t *tbl, sdb_nrow_t nrows);

int sdb_consolidate( sdb_table_t *src);

/* Retrieve a column number from its name.
 * Might return SDB_NCOLUMN_INVALID if there is no column by that name. */
sdb_ncolumn_t sdb_getcolnum( sdb_table_t *tbl, const char *name);
/* Retrieve a column name by its number.
 * Might return NULL if the table hasn't got enough columns. */
const char *sdb_getcolname( sdb_table_t *tbl, sdb_ncolumn_t icol);

/* Start or resume the serialization of a database's content.
 * Once started, the table remains in streaming mode, refusing any attempt
 * to write in it, until the serialization is completed.
 *
 * The hessian serialization is performed over an hessian serialization context
 * bss_ctx, which must be passed as parameter already initialized. If the
 * function has to be called more than once, the same bss_ctx must be passed
 * everytime.
 *
 * Return:
 *  - SDB_EOK    if the serialization has been  completed;
 *  - SDB_EAGAIN if the serialization must be resumed when the bss writer
 *               isn't in overflow anymore;
 *  - Another error code upon failure.
 */
// TODO: what to do with the table in case of error?
// should reset be allowed on broken table to restart anew?
bss_status_t sdb_serialize( sdb_table_t *src, bss_ctx_t *bss_ctx);

/* Cancel a serialization in progress. Return 0, or SDB_EBADSTATE if
 * the table wasn't serializing. */
int sdb_serialize_cancel( sdb_table_t *tbl);

int sdb_raw(     sdb_table_t *tbl, unsigned const char *serialized_cell, int length);
int sdb_lstring( sdb_table_t *tbl, const char *data, int length);
int sdb_binary(  sdb_table_t *tbl, const char *data, int length);
int sdb_string(  sdb_table_t *tbl, const char *data);
int sdb_int(     sdb_table_t *tbl, int    i);
int sdb_bool(    sdb_table_t *tbl, int    b);
int sdb_double(  sdb_table_t *tbl, double d);
int sdb_number(  sdb_table_t *tbl, double d);
int sdb_null(    sdb_table_t *tbl);

#endif
