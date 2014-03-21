/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
/*
 * pointer_list.c
 *
 *  Created on: Mar 25, 2009
 *      Author: cbugot
 */

#include "pointer_list.h"
#include <stdlib.h> //malloc
#include <string.h> //memcpy

#define POINTERLIST_MAGIC_ID 0x12af85d3

struct PointerList_s
{
  unsigned int magic;
  unsigned int allocatedSize;
  unsigned int nbOfElements;
  void** buffer;
  unsigned int readIdx;
  unsigned int writeIdx;
};


#define CHECK_CONTEXT(ctx)  if (!ctx || ctx->magic != POINTERLIST_MAGIC_ID) return RC_BAD_FORMAT

rc_ReturnCode_t PointerList_Create(PointerList** list_, unsigned int prealloc)
{
  PointerList* list;

  *list_ = 0;
  if (!prealloc)
    prealloc = 8;

  list = malloc(sizeof(*list));
  if (!list)
    return RC_NO_MEMORY;

  list->buffer = malloc(sizeof(*list->buffer) * prealloc);
  if (!list->buffer)
  {
    free(list);
    return RC_NO_MEMORY;
  }

  list->magic = POINTERLIST_MAGIC_ID;
  list->allocatedSize = prealloc;
  list->nbOfElements = 0;
  list->readIdx = 0;
  list->writeIdx = 0;

  *list_ = list;

  return RC_OK;
}

rc_ReturnCode_t PointerList_Destroy(PointerList* list)
{
  CHECK_CONTEXT(list);
  list->magic = ~list->magic;
  free(list->buffer);
  free(list);
  return RC_OK;
}

rc_ReturnCode_t PointerList_GetSize(PointerList* list, unsigned int* nbOfElements, unsigned int* allocatedSize)
{
  CHECK_CONTEXT(list);
  if (nbOfElements)
    *nbOfElements = list->nbOfElements;
  if (allocatedSize)
    *allocatedSize = list->allocatedSize;

  return RC_OK;
}

rc_ReturnCode_t PointerList_PushLast(PointerList* list, void* pointer)
{
  CHECK_CONTEXT(list);

  // Check if we need to grow the buffer
  if (list->nbOfElements == list->allocatedSize-1)
  {
    // Allocate the new buffer
    void** buffer = malloc(list->allocatedSize*2*sizeof(*list->buffer));
    if (!buffer)
      return RC_NO_MEMORY;
    // Copy the data from the previous buffer
    if (list->readIdx < list->writeIdx)
      memcpy(buffer, list->buffer + list->readIdx, (list->writeIdx - list->readIdx)*sizeof(*list->buffer));
    else
    {
      memcpy(buffer, list->buffer + list->readIdx, (list->allocatedSize - list->readIdx)*sizeof(*list->buffer));
      memcpy(buffer+(list->allocatedSize - list->readIdx), list->buffer, list->writeIdx*sizeof(*list->buffer));
    }
    free(list->buffer);
    list->readIdx = 0;
    list->writeIdx = list->nbOfElements;

    list->buffer = buffer;
    list->allocatedSize *= 2;
  }

  list->buffer[list->writeIdx] = pointer;
  list->writeIdx = (list->writeIdx+1) % list->allocatedSize;
  list->nbOfElements++;

  return RC_OK;
}

rc_ReturnCode_t PointerList_PopFirst(PointerList* list, void** pointer)
{
  CHECK_CONTEXT(list);

  // Check if there are no elements at all in the list !
  if (!list->nbOfElements)
  {
    *pointer = NULL;
    return RC_NOT_FOUND;
  }

  *pointer = list->buffer[list->readIdx];
  list->readIdx = (list->readIdx+1) % list->allocatedSize;
  list->nbOfElements--;

  return RC_OK;
}

rc_ReturnCode_t PointerList_Poke(PointerList* list, unsigned int index, void* pointer)
{
  CHECK_CONTEXT(list);

  if (index >= list->nbOfElements)
    return RC_OUT_OF_RANGE;

  unsigned int i = (index+list->readIdx) % list->allocatedSize;
  list->buffer[i] = pointer;

  return RC_OK;
}

rc_ReturnCode_t PointerList_Peek(PointerList* list, unsigned int index, void** pointer)
{
  CHECK_CONTEXT(list);

  if (index >= list->nbOfElements)
  {
    *pointer = NULL;
    return RC_OUT_OF_RANGE;
  }

  unsigned int i = (index+list->readIdx) % list->allocatedSize;
  *pointer = list->buffer[i];
  return RC_OK;
}

rc_ReturnCode_t PointerList_Remove(PointerList* list, unsigned int index, void** pointer)
{
  CHECK_CONTEXT(list);

  if (index >= list->nbOfElements)
  {
    *pointer = NULL;
    return RC_OUT_OF_RANGE;
  }

  unsigned int i = (index+list->readIdx) % list->allocatedSize;
  *pointer = list->buffer[i];
  list->nbOfElements--;

  unsigned int n;
  for (n = list->nbOfElements - index; n>0; n--, i++)
  {
    if (i == list->allocatedSize-1)
    {
      list->buffer[i] = list->buffer[0];
      i = 0;
    }
    else
      list->buffer[i] = list->buffer[i+1];
  }

  list->writeIdx = (list->writeIdx - 1 + list->allocatedSize) % list->allocatedSize;

  return RC_OK;
}

