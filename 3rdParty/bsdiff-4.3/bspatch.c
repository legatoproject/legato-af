/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
__FBSDID("$FreeBSD: src/usr.bin/bsdiff/bspatch/bspatch.c,v 1.1 2005/08/06 01:59:06 cperciva Exp $");
#endif

#include <bzlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef SIERRA_BSPATCH
#include "legato.h"
#include "pa_flash.h"
#include "pa_patch.h"
#include "bspatch.h"
#endif // SIERRA_BSPATCH

static off_t offtin(u_char *buf)
{
	off_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

#ifdef SIERRA_BSPATCH
le_result_t bsPatch
(
    pa_patch_Context_t *patchContextPtr,
                            ///< [IN] Context for the patch
    char *patchfile,        ///< [IN] File containing the patch
    uint32_t *crc32Ptr,     ///< [OUT] Pointer to return the CRC32 of the patch applied
    bool lastPatch,         ///< [IN] True if this is the last patch in this context
    bool forceClose         ///< [IN] Force close of device and resources
)
#else
int main(int argc,char * argv[])
#endif // SIERRA_BSPATCH
{
	FILE * f, * cpf, * dpf, * epf;
	BZFILE * cpfbz2, * dpfbz2, * epfbz2;
	int cbz2err, dbz2err, ebz2err;
#ifndef SIERRA_BSPATCH
	int fd;
#endif // SIERRA_BSPATCH
	ssize_t oldsize,newsize;
	ssize_t bzctrllen,bzdatalen;
	u_char header[32],buf[8];
#ifdef SIERRA_BSPATCH
	u_char *old = NULL, *new = NULL;
#else
	u_char *old, *new;
#endif
	off_t oldpos,newpos;
	off_t ctrl[3];
	off_t lenread;
	off_t i;
#ifdef SIERRA_BSPATCH
	off_t oldposmax;
        off_t patchHdrOffset;
        pa_patch_Desc_t desc = NULL;
        size_t readSize;
        le_result_t res = LE_FAULT;

        f = NULL;
	cpf = NULL;
        dpf = NULL;
        epf = NULL;
	cpfbz2 = NULL;
        dpfbz2 = NULL;
        epfbz2 = NULL;

        if( forceClose )
        {
            // If forceClose set, close descriptor and release all resources
            LE_CRIT( "Closing and releasing patch resources and MTD due to forceClose\n" );
            goto errorfcloseall;
        }

        patchHdrOffset = patchContextPtr->patchOffset;

        LE_INFO("OrigNum %d DestNum %d, ubiVolId %u SSz %"PRIxS" offset %lx, lastPatch %d\n",
                patchContextPtr->origImageDesc.flash.mtdNum,
                patchContextPtr->destImageDesc.flash.mtdNum,
                patchContextPtr->origImageDesc.flash.ubiVolId,
                patchContextPtr->segmentSize,
                patchContextPtr->patchOffset,
                lastPatch);
        if ((f = fopen(patchfile, "r")) == NULL)
        {
            LE_ERROR("fopen(%s): %m\n", patchfile);
            return LE_FAULT;
        }
#else
	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	/* Open patch file */
	if ((f = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);
#endif // SIERRA_BSPATCH

	/*
	File format:
		0	8	"BSDIFF40"
		8	8	X
		16	8	Y
		24	8	sizeof(newfile)
		32	X	bzip2(control block)
		32+X	Y	bzip2(diff block)
		32+X+Y	???	bzip2(extra block)
	with control block a set of triples (x,y,z) meaning "add x bytes
	from oldfile to x bytes from the diff block; copy y bytes from the
	extra block; seek forwards in oldfile by z bytes".
	*/

	/* Read header */
	if (fread(header, 1, 32, f) < 32) {
		if (feof(f))
#ifdef SIERRA_BSPATCH
                {
                    LE_ERROR("Corrupt patch");
                    goto errorfclose;
                }
                LE_ERROR( "fread(%s)", patchfile);
                goto errorfclose;
#else
			errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", argv[3]);
#endif // SIERRA_BSPATCH
	}

	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0)
#ifdef SIERRA_BSPATCH
        {
            LE_ERROR("Corrupt patch\n");
            goto errorfclose;
        }
#else
		errx(1, "Corrupt patch\n");
#endif // SIERRA_BSPATCH

	/* Read lengths from header */
	bzctrllen=offtin(header+8);
	bzdatalen=offtin(header+16);
	newsize=offtin(header+24);
	if((bzctrllen<0) || (bzdatalen<0) || (newsize<0))
#ifdef SIERRA_BSPATCH
        {
            LE_ERROR("Corrupt patch\n");
            goto errorfclose;
        }
#else
		errx(1,"Corrupt patch\n");
#endif // SIERRA_BSPATCH

	/* Close patch file and re-open it via libbzip2 at the right places */
#ifndef SIERRA_BSPATCH
	if (fclose(f))
		err(1, "fclose(%s)", argv[3]);
	if ((cpf = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);
	if (fseeko(cpf, 32, SEEK_SET))
		err(1, "fseeko(%s, %lld)", argv[3],
		    (long long)32);
	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", cbz2err);
	if ((dpf = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);
	if (fseeko(dpf, 32 + bzctrllen, SEEK_SET))
		err(1, "fseeko(%s, %lld)", argv[3],
		    (long long)(32 + bzctrllen));
	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", dbz2err);
	if ((epf = fopen(argv[3], "r")) == NULL)
		err(1, "fopen(%s)", argv[3]);
	if (fseeko(epf, 32 + bzctrllen + bzdatalen, SEEK_SET))
		err(1, "fseeko(%s, %lld)", argv[3],
		    (long long)(32 + bzctrllen + bzdatalen));
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL)
		errx(1, "BZ2_bzReadOpen, bz2err = %d", ebz2err);

	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

#else
	if (fclose(f))
        {
            LE_ERROR("fclose\n");
            goto errorfclose;
        }
        f = NULL;
	if ((cpf = fopen(patchfile, "r")) == NULL)
        {
            LE_ERROR("fopen(%s)", patchfile);
            goto errorfcloseall;
        }
	if (fseeko(cpf, 32, SEEK_SET))
        {
            LE_ERROR("fseeko(%s, %lld)", patchfile, (long long)32);
            goto errorfcloseall;
        }
	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL)
        {
            LE_ERROR("BZ2_bzReadOpen, bz2err = %d", cbz2err);
            goto errorfcloseall;
        }
	if ((dpf = fopen(patchfile, "r")) == NULL)
        {
            LE_ERROR("fopen(%s)", patchfile);
            goto errorfcloseall;
        }
	if (fseeko(dpf, 32 + bzctrllen, SEEK_SET))
        {
            LE_ERROR("fseeko(%s, %lld)", patchfile, (long long)(32 + bzctrllen));
            goto errorfcloseall;
        }
	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL)
        {
            LE_ERROR("BZ2_bzReadOpen, bz2err = %d", dbz2err);
            goto errorfcloseall;
        }
	if ((epf = fopen(patchfile, "r")) == NULL)
        {
            LE_ERROR("fopen(%s)", patchfile);
            goto errorfcloseall;
        }
	if (fseeko(epf, 32 + bzctrllen + bzdatalen, SEEK_SET))
        {
            LE_ERROR("fseeko(%s, %lld)", patchfile, (long long)(32 + bzctrllen + bzdatalen));
            goto errorfcloseall;
        }
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL)
        {
            LE_ERROR("BZ2_bzReadOpen, bz2err = %d", ebz2err);
            goto errorfcloseall;
        }

	oldsize = patchContextPtr->origImageSize;
        if( newsize > patchContextPtr->segmentSize )
        {
            LE_ERROR("Unable to apply patch. newsize is too big: %"PRIdS" > %"PRIuS"\n",
                     newsize, patchContextPtr->segmentSize);
            goto errorfcloseall;
        }

        res = pa_patch_Open( patchContextPtr, &desc, &old, &new );
        if( LE_OK != res )
        {
            LE_ERROR("pa_patch_Open fails");
            goto errorfcloseall;
        }
        res = pa_patch_ReadSegment( desc, patchHdrOffset, old, &readSize );
        if( (LE_OK != res) && (LE_OUT_OF_RANGE != res) )
        {
            LE_ERROR("Readegment fails %d", res);
            goto errorfcloseall;
        }

        oldposmax = readSize + patchHdrOffset;
#endif // SIERRA_BSPATCH

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);
			if ((lenread < 8) || ((cbz2err != BZ_OK) &&
			    (cbz2err != BZ_STREAM_END)))
#ifdef SIERRA_BSPATCH
                        {
                            LE_ERROR("Corrupt Patch\n");
                            goto errorfcloseall;
                        }
#else
				errx(1, "Corrupt patch\n");
#endif // SIERRA_BSPATCH
			ctrl[i]=offtin(buf);
		};

		/* Sanity-check */
		if(newpos+ctrl[0]>newsize)
#ifdef SIERRA_BSPATCH
                {
                    LE_ERROR("Corrupt Patch\n");
                    goto errorfcloseall;
                }
#else
			errx(1,"Corrupt patch\n");
#endif // SIERRA_BSPATCH

		/* Read diff string */
		lenread = BZ2_bzRead(&dbz2err, dpfbz2, new + newpos, ctrl[0]);
		if ((lenread < ctrl[0]) ||
		    ((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END)))
#ifdef SIERRA_BSPATCH
                {
                    LE_ERROR("Corrupt Patch\n");
                    goto errorfcloseall;
                }
#else
			errx(1, "Corrupt patch\n");
#endif // SIERRA_BSPATCH

		/* Add old data to diff string */
		for(i=0;i<ctrl[0];i++)
#ifndef SIERRA_BSPATCH
			if((oldpos+i>=0) && (oldpos+i<oldsize))
				new[newpos+i]+=old[oldpos+i];
#else
                {
                    if( ((oldpos+i) >= oldposmax) || ((oldpos+i) < patchHdrOffset) )
                    {
                        LE_DEBUG("Flush PATCH and reading at offset %lx (at blk %lx)\n",
                                 oldpos, oldpos /patchContextPtr->segmentSize);
                        patchHdrOffset = ((oldpos+i) / patchContextPtr->segmentSize)
                                         * patchContextPtr->segmentSize;
                        LE_DEBUG("Reread at patchHdrOffset=%lx\n", patchHdrOffset);
                        res = pa_patch_ReadSegment( desc, patchHdrOffset, old, &readSize );
                        if( (LE_OK != res) /*&& (LE_OUT_OF_RANGE != res)*/ )
                        {
                            LE_ERROR("ReadNextSegment fails: res = %d", res);
                            goto errorfcloseall;
                        }
                        oldposmax = readSize + patchHdrOffset;
                    }

                    if(((oldpos + i) >= 0) && ((oldpos + i) < oldsize))
                    {
                        new[newpos + i] += old[oldpos - patchHdrOffset + i];
                    }
                }
#endif // SIERRA_BSPATCH

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
#ifdef SIERRA_BSPATCH
                {
                    LE_ERROR("Corrupt Patch\n");
                    goto errorfcloseall;
                }
#else
			errx(1,"Corrupt patch\n");
#endif // SIERRA_BSPATCH

		/* Read extra string */
		lenread = BZ2_bzRead(&ebz2err, epfbz2, new + newpos, ctrl[1]);
		if ((lenread < ctrl[1]) ||
		    ((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END)))
#ifdef SIERRA_BSPATCH
                {
                    LE_ERROR("Corrupt Patch lenread %ld ctrl[1] = %ld\n",
                             lenread, ctrl[1]);
                    goto errorfcloseall;
                }
#else
			errx(1, "Corrupt patch\n");
#endif // SIERRA_BSPATCH

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&cbz2err, cpfbz2);
	BZ2_bzReadClose(&dbz2err, dpfbz2);
	BZ2_bzReadClose(&ebz2err, epfbz2);
	if (fclose(cpf) || fclose(dpf) || fclose(epf))
#ifndef SIERRA_BSPATCH
		err(1, "fclose(%s)", argv[3]);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,0666))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
		err(1,"%s",argv[2]);

	free(new);
	free(old);

	return 0;
#else
        LE_INFO("crc32Ptr = %p", crc32Ptr);
        if( crc32Ptr )
        {
            *crc32Ptr = le_crc_Crc32( new, newsize, *crc32Ptr );
            LE_DEBUG("newsize=%zx crc32=%x\n", newsize, *crc32Ptr);
        }

        if( LE_OK != pa_patch_WriteSegment( desc,
                                            patchContextPtr->patchOffset,
                                            new,
                                            newsize) )
        {
            goto errorfclose;
        }

        pa_patch_Close( desc, lastPatch, patchContextPtr->patchOffset + newsize );
        return LE_OK;

errorfcloseall:
	/* Clean up the bzip2 reads */
        if( cpfbz2 )
        {
	    BZ2_bzReadClose(&cbz2err, cpfbz2);
        }
        if( dpfbz2 )
        {
	    BZ2_bzReadClose(&dbz2err, dpfbz2);
        }
        if( epfbz2 )
        {
	    BZ2_bzReadClose(&ebz2err, epfbz2);
        }
        if( cpf )
        {
	    fclose(cpf);
        }
        if( dpf )
        {
            fclose(dpf);
        }
        if( epf )
        {
            fclose(epf);
        }
errorfclose:
        if( f )
        {
	    fclose(f);
        }
        if( desc )
        {
            res = pa_patch_Close( desc, false, 0 );
        }
        return (forceClose ? res : LE_FAULT);
#endif
}
