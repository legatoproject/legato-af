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

/* sbd_read_*: Reading back sequential data from sdb tables. */

#include "sdb_internal.h"

/* initialize a reading context. */
void sdb_read_init( sdb_read_ctx_t *ctx, sdb_table_t *tbl) {
    ctx->tmpbuff         = NULL;
    ctx->bytes           = NULL;
    ctx->nreadbytes      = 0;
    ctx->nreadobjects    = 0;
    ctx->minibuff_len    = 0;
    ctx->minibuff_offset = 0;

    bsd_init( & ctx->bsd_ctx);

    switch( tbl->storage_kind) {
    case SDB_SK_RAM:
        ctx->storage_kind = SDB_SK_RAM;
        ctx->source.chunk = tbl->storage.ram.first_chunk;
        break;
    case SDB_SK_FILE:
        ctx->storage_kind = SDB_SK_FILE;
        ctx->source.file = tbl->storage.file;
        rewind( ctx->source.file);
        break;
    }
}

/* release resources associated to a reading context. */
void sdb_read_close( sdb_read_ctx_t *ctx) {
    if( ctx->tmpbuff) {
      free( ctx->tmpbuff);
      ctx->tmpbuff = NULL;
    }
    switch( ctx->storage_kind) {
    case SDB_SK_RAM:  break;
    case SDB_SK_FILE: break;
    }

}

/* Deserialize or skip the next data into bsd_data:
 * take it out of storage to give its bsd_deserialization and/or raw bytes.
 *
 * Return the number of bytes the data takes in serialized form,
 * or a negative error code.
 *
 * If skip is true, the size is returned but the object
 * content isn't actually returned: use this mode to skip a cell
 * you don't care about.
 *
 * If skip is false, the size is returned and the deserialized
 * object content is returned. For data which include a pointer (strings
 * and binaries), the validity of the data pointed to is only guaranteed
 * until next call to sdb_read_data or sddb_reset():
 *  - if the data fits in a single sdb_chunk_t, then a pointer to
 *    it is returned, which remains valid until sdb_reset() is called;
 *  - if the data sits across two chunks, then it is recomposed in  transient
 *    tmpbuff buffer of the reading context, which will be recomposed next time
 *    this reading context's tmpbuff is needed; i.e. potentially the next call
 *    to sdb_read_data().
 *
 * Fields updated:
 *  - bytes: points to the raw bytes if skip is false. The bytes might reside:
 *     - in their chunk if they're store in a single chunk;
 *     - in minibuff if they sit across 2 chunks but nbytes<=BSD_MINIBUFFSIZE;
 *     _ in tmpbuff if they sit across 2 chunks and don't fit in minibuff.
 *  - nbytes: number of bytes in the object.
 *  - chunk: chunk currently read.
 *  - tmpbuff: might be allocated if needed for the 1st time.
 *  - nreadbytes: total # of bytes read since sdb_read_init()
 *  - nreadobjects: incremented
 *  - minibuff: might hold the content pointed by bytes, if the data fits
 *    in it and was stored across 2 chunks.
 *
 */
int sdb_read_ram_data( sdb_read_ctx_t *ctx, bsd_data_t *bsd_data, int skip) {
    int read_in_chunk = ctx->nreadbytes % SDB_CHUNK_SIZE;
    int left_in_chunk = SDB_CHUNK_SIZE - read_in_chunk;
    int r;

    /* Attempt to read the next data;
     * This first attempt only guarantees that we can read the size
     * correctly (i.e. there's at least BSD_MINBUFFSIZE bytes available).
     * The case where the whole data doesn't fit at the end of the chunk
     * will be addressed in a 2nd step. */
    if( left_in_chunk < BSD_MINBUFFSIZE) {
        sdb_chunk_t *nchunk = ctx->source.chunk->next;
        if( ! nchunk) return SDB_EINTERNAL;
        memcpy( ctx->minibuff, ctx->source.chunk->data+read_in_chunk, left_in_chunk);
        memcpy( ctx->minibuff+left_in_chunk, nchunk->data, BSD_MINBUFFSIZE-left_in_chunk);
        r = bsd_read( & ctx->bsd_ctx, bsd_data, ctx->minibuff, BSD_MINBUFFSIZE);
        if( r > 0) { /* Read successfully from minibuff */
            ctx->bytes  = ctx->minibuff;
            ctx->nbytes = r;
            if( r >= left_in_chunk) ctx->source.chunk = ctx->source.chunk->next;
            ctx->nreadbytes += r;
            ctx->nreadobjects++;
            return r;
        }
    } else { /* enough bytes left in chunk to get the correct needed size. */
        r = bsd_read( & ctx->bsd_ctx, bsd_data, ctx->source.chunk->data+read_in_chunk, left_in_chunk);
    }

    if( 0 == r) { /* deserialization error. */
        return bsd_data->content.error;
    } else if( r > 0) { /* data retrieved entirely from the current chunk. */
        ctx->bytes = ctx->source.chunk->data+read_in_chunk;
        ctx->nbytes = r;
        ctx->nreadbytes += r;
        ctx->nreadobjects++;
        if( r == left_in_chunk) ctx->source.chunk = ctx->source.chunk->next;
        return r;
    } else { /* data sits across two chunks. */
        int needed = -r;
        sdb_chunk_t *nextchunk = ctx->source.chunk->next;
        if( ! nextchunk) return SDB_EINTERNAL;
        if( skip) { /* Just skip the cell, don't describe it in bsd_data. */
            bsd_data->kind = BSD_ERROR;
            ctx->bytes = NULL;
        } else { /* Data actually needs to be deserialized. */
            /* reserve the buffer if not already done. */
            unsigned char *b = ctx->tmpbuff;
            if( ! b) { //TODO: increase buffer size progressively?
                b = malloc( SDB_DATA_SIZE_LIMIT);
                if( b) { ctx->tmpbuff = b; } else { return SDB_EMEM; }
            }

            /* fill the buffer with the 2 needed halves. */
            memcpy( b, ctx->source.chunk->data + read_in_chunk, left_in_chunk);
            memcpy( b+left_in_chunk, nextchunk->data, needed - left_in_chunk);

            r = bsd_read( & ctx->bsd_ctx, bsd_data, b, needed);
            if( needed != r)
                return SDB_EINTERNAL;
            ctx->bytes       = ctx->tmpbuff;
        }
        ctx->nbytes       = needed;
        ctx->source.chunk = nextchunk;
        ctx->nreadbytes  += needed;
        ctx->nreadobjects ++;
        return needed;
    }
}

#ifdef SDB_FILE_SUPPORT
int sdb_read_file_data( sdb_read_ctx_t *ctx, bsd_data_t *bsd_data, int skip) {
    /* Read byte-per-byte, n_item==3, item_size=1:
     * it's OK to read only 1 or 2 bytes. */

//    int ftellb4 = (int) ftell( ctx->source.file); // debug cruft
    int nread,                 // # bytes read from buffer to bsd deserializer.
        nfromfile,              // # bytes read from file to complete minibuff.
        ninbuff;       // # bytes in minibuff (read from file + already there).
    if( ctx->minibuff_len > 0) {
        memmove(ctx->minibuff,
                ctx->minibuff + ctx->minibuff_offset,
                ctx->minibuff_len);
//        printf( "moved %d minibuff bytes from 0x%x+%d to 0x%x\n",
//                ctx->minibuff_len,
//                (unsigned) ctx->minibuff, ctx->minibuff_offset,
//                (unsigned) ctx->minibuff);
    }
    nfromfile = fread(
            ctx->minibuff + ctx->minibuff_len,
            1, sizeof( ctx->minibuff) - ctx->minibuff_len,
            ctx->source.file);
//    printf( "%d objects, %d bytes, ftell %d->%d: Tried to get %d more bytes, "
//            "there where already %d in it, to get a total of 3, got %d. "
//            "%d bytes in minibuff.\n",
//            ctx->nreadobjects, ctx->nreadbytes,
//            ftellb4, (int) ftell(ctx->source.file),
//            sizeof( ctx->minibuff) - ctx->minibuff_len, ctx->minibuff_len,
//            nfromfile, ctx->minibuff_len);
    ninbuff = ctx->minibuff_len + nfromfile;
    if( ninbuff == 0) {
        int r = ferror( ctx->source.file);
        if( r>0) return SDB_EBADFILE;
        else if( r<0) return r;
        else return 0;
    }
    nread = bsd_read( & ctx->bsd_ctx, bsd_data, ctx->minibuff, ninbuff);
//    printf("bsd_read() returns %d\n", nread);
    if( nread>=0) {
        ctx->nreadbytes += nread;
        ctx->nreadobjects++;
        ctx->nbytes = nread;
        ctx->bytes = ctx->minibuff;
        if( nread < ninbuff) { // recycle extra bytes in minibuff
            ctx->minibuff_offset = nread;
            ctx->minibuff_len = ninbuff - nread;
        } else {
            ctx->minibuff_len = 0;
        }
//        {
//            char buf[256];
//            bsd_tostring( bsd_data, buf);
//            printf("Retrieved this from minibuff: %s\n", buf);
//        }
        return nread;
    } else if( skip) { /* more than 3 bytes, but no need to read them. */
        int needed = -nread;
        /* Correct seek position to take into account what's been already
         * read in minibuf. */
        fseek( ctx->source.file, needed - ninbuff, SEEK_CUR);
        ctx->bytes = NULL;
        ctx->nreadbytes += needed;
        ctx->nreadobjects++;
        ctx->nbytes = needed;
        ctx->minibuff_len = 0;
        return needed;
    } else { /* more than 3 bytes, must read them. */
        int r, needed = -nread;
        unsigned char *b = ctx->tmpbuff;
        if( ! b) { //TODO: increase buffer size progressively?
            b = malloc( SDB_DATA_SIZE_LIMIT);
            if( b) { ctx->tmpbuff = b; } else { return SDB_EMEM; }
        }
        memcpy( b, ctx->minibuff, sizeof( ctx->minibuff));
        r = fread(
                b + sizeof( ctx->minibuff),
                needed - sizeof( ctx->minibuff), 1,
                ctx->source.file);
        if( ! r) {
            r = ferror( ctx->source.file);
            if( r) return r; else return SDB_EINTERNAL; }
        r = bsd_read( & ctx->bsd_ctx, bsd_data, b, needed);
        if( needed != r){ return SDB_EINTERNAL; }
        ctx->bytes       = ctx->tmpbuff;
        ctx->nreadbytes += needed;
        ctx->nreadobjects++;
        ctx->nbytes = needed;
        ctx->minibuff_len = 0;
//        {
//            char buf[256];
//            bsd_tostring( bsd_data, buf);
//            printf("Retrieved this from big buff: %s\n", buf);
//        }
        return needed;
    }
    return SDB_EINTERNAL;
}
#endif

#ifdef SDB_FLASH_SUPPORT
int sdb_read_flash_data( sdb_read_ctx_t *ctx, bsd_data_t *bsd_data, int skip) {
#error "flash storage not implemented"
}
#endif

int sdb_read_data( sdb_read_ctx_t *ctx, bsd_data_t *bsd_data, int skip) {
    switch( ctx->storage_kind) {
    case SDB_SK_RAM:  return sdb_read_ram_data( ctx, bsd_data, skip);
    case SDB_SK_FILE: return sdb_read_file_data( ctx, bsd_data, skip);
    }
    return SDB_EINTERNAL;
}
