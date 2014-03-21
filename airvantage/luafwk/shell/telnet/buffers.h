/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Cuero Bugot for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "mem_define.h"




#define WRITEBUFFER_PREALLOCATED_SIZE 16

#define max(a, b) ((a)<(b)?(b):(a))
#define min(a, b) ((a)<(b)?(a):(b))

typedef struct
{
  char* buffer; // initial 'allocated' buffer pointer. Will always point to the start of the buffer
  int len; // bytes currently hold in the buffer
  int size; // max size of this buffer (= size of the malloc), may be updated when calling the GrowBuffer() function
  char prealloc[WRITEBUFFER_PREALLOCATED_SIZE]; // prealloced bytes (prevent a malloc for small size buffer)
} WriteBuffer;

typedef struct
{
  char* buffer;
  char* p;
  char* mark;
  int len;
} ReadBuffer;

typedef struct
{
  char* p;
  int len;
  int size;
} Buffer;

void InitBuffer(WriteBuffer* buf);
void FreeBuffer(WriteBuffer* buf);
void GrowBuffer(WriteBuffer* buf, int size);
void WriteByte(WriteBuffer* buf, int v);
void WriteEscByte(WriteBuffer* buf, int v);
void WriteBytes(WriteBuffer* buf, const char* s, int l);
void InsertBytes(WriteBuffer* buf, int pos, const char* s, int l);
void OverWriteBytes(WriteBuffer* buf, int pos, const char* s, int l);
void DeleteBytes(WriteBuffer* buf, int pos, int nb);
void CopyCutBytes(WriteBuffer* buf, int pos, int nb, char* out);
void CopyBytes(WriteBuffer* buf, int pos, int nb, char* out);


void ReadMark(ReadBuffer* buf);
int ReadByte(ReadBuffer* buf);
int ReadEscByte(ReadBuffer* buf);



#endif /* BUFFERS_H_ */
