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
#include <assert.h>
#include <string.h>
#include "teel_internal.h"

void tl_history_addhistory(TeelInstance* ti, const char* line, int len)
{
  History* h = &ti->history;

  if (h->size <= 0)
    return;

  int prev = (h->widx-1+h->size) % h->size;
  if (h->list[prev] && len ==  h->list[prev]->len && !memcmp(line, h->list[prev]->buf, len)) // check if the same as last entry, if so, do not add it a second time !
    return;

  int idx = h->widx;
  if (h->list[idx])
      MEM_FREE(h->list[idx]);
  h->list[idx] = MEM_ALLOC(len+sizeof(*h->list[0]));
  h->list[idx]->buf = (char*) (h->list[idx]+1);
  memcpy(h->list[idx]->buf, line, len);
  h->list[idx]->len = len;
  idx = (idx + 1) % h->size;
  h->widx = idx;
  return;
}


void tl_history_sethistorysize(TeelInstance* ti, int size)
{
  if (size <= 0)
    return;

  History* h = &ti->history;
  int s = min(size, h->size);
  HistoryEntry** p = MEM_CALLOC(size, sizeof(*h->list));
  int i;

  for (i = 0; i < s; i++)
    p[s-i-1] = h->list[(h->widx-i+h->size)%h->size];

  for (i=s; i<h->size; i++)
      MEM_FREE(h->list[(h->widx-i+h->size)%h->size]);

  h->size = size;
  h->idx = h->widx = 0;
  MEM_FREE(h->list);
  h->list = p;
}

void tl_history_destroy(TeelInstance* ti)
{
  History* h = &ti->history;
  if (h->list)
  {
    int i;
    for (i = 0; i < h->size; ++i)
      if (h->list[i])
          MEM_FREE(h->list[i]);
    MEM_FREE(h->list);
  }
}


HistoryEntry* tl_history_getentry(TeelInstance* ti, int pos)
{
  History* h = &ti->history;
  if (!h->list)
    return 0;

  if (pos > h->size || pos <= 0)
    return 0;

  int idx = (h->widx+h->size-pos) % h->size;

  return h->list[idx];
}

void tl_history_resetcontext(TeelInstance* ti)
{
  History* h = &ti->history;
  if (h->curline)
  {
      MEM_FREE(h->curline);
    h->curline = 0;
    h->curlen = h->curpos = 0;
  }

  h->idx = 0;
}

