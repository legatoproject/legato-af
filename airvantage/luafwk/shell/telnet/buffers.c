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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "buffers.h"


void InitBuffer(WriteBuffer* buf)
{
  buf->size = sizeof(buf->prealloc);
  buf->buffer = buf->prealloc;
  buf->len = 0;
}

void FreeBuffer(WriteBuffer* buf)
{
  if (buf->buffer != buf->prealloc)
      MEM_FREE(buf->buffer);
  buf->size = 0;
  buf->buffer = 0;
  buf->len = 0;
}

void GrowBuffer(WriteBuffer* buf, int size)
{
  if (buf->len + size > buf->size)
  {
    int newsize = max(buf->size * 2, buf->len + size);
    if (buf->buffer != buf->prealloc)
      buf->buffer = MEM_REALLOC(buf->buffer, newsize);
    else
    {
      buf->buffer = MEM_ALLOC(newsize);
      memcpy(buf->buffer, buf->prealloc, sizeof(buf->prealloc));
    }
    buf->size = newsize;
  }
}

void WriteByte(WriteBuffer* buf, int v)
{
  if (!buf->buffer)
    InitBuffer(buf);

  GrowBuffer(buf, 1);

  *(buf->buffer + buf->len) = v;
  buf->len++;
}

void WriteEscByte(WriteBuffer* buf, int v)
{
  if (v==255)
    WriteByte(buf, 255); // add the escape char
  WriteByte(buf, v);
}

void WriteBytes(WriteBuffer* buf, const char* s, int l)
{
  if (!buf->buffer)
    InitBuffer(buf);

  GrowBuffer(buf, l);

  memcpy(buf->buffer+buf->len, s, l);
  buf->len += l;
}

void InsertBytes(WriteBuffer* buf, int pos, const char* s, int l)
{
  if (!buf->buffer)
    InitBuffer(buf);

  assert(pos<=buf->len);

  GrowBuffer(buf, l);

  memmove(buf->buffer+pos+l, buf->buffer+pos, buf->len-pos);
  memcpy(buf->buffer+pos, s, l);
  buf->len += l;
}

void OverWriteBytes(WriteBuffer* buf, int pos, const char* s, int l)
{
  if (!buf->buffer)
    InitBuffer(buf);

  assert(pos<=buf->len);

  int growsize = max(pos+l - buf->len, 0);
  GrowBuffer(buf, growsize);

  memcpy(buf->buffer+pos, s, l);
  buf->len += growsize;
}

void DeleteBytes(WriteBuffer* buf, int pos, int nb)
{
  if (!buf->buffer)
    InitBuffer(buf);
  int end = pos+nb;
  assert(pos<=buf->len);
  assert(end<=buf->len);

  memcpy(buf->buffer+pos, buf->buffer+end, buf->len-end);

  buf->len -= nb;
}
void CopyCutBytes(WriteBuffer* buf, int pos, int nb, char* out)
{
  if (!buf->buffer)
    InitBuffer(buf);
  int end = pos+nb;
  assert(pos<=buf->len);
  assert(end<=buf->len);

  memcpy(buf->buffer+pos, out, nb);
  memcpy(buf->buffer+pos, buf->buffer+end, buf->len-end);

  buf->len -= nb;
}
void CopyBytes(WriteBuffer* buf, int pos, int nb, char* out)
{
  if (!buf->buffer)
    InitBuffer(buf);

  assert(pos<=buf->len);
  assert(pos+nb<=buf->len);

  memcpy(buf->buffer+pos, out, nb);
}

void ReadMark(ReadBuffer* buf)
{
  buf->mark = buf->p;
}
int ReadByte(ReadBuffer* buf)
{
  if (buf->len <= 0)
    return -1;
  int v = *((unsigned char*)buf->p++);
  buf->len--;

  return v;
}
int ReadEscByte(ReadBuffer* buf)
{
  int v = ReadByte(buf);
  if (v == 255)
    return ReadByte(buf);
  return v;
}
