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

#ifndef HISTORY_H_
#define HISTORY_H_

#include "mem_define.h"

typedef struct
{
  char* buf;
  int len;
} HistoryEntry;

typedef struct
{
  HistoryEntry** list;
  int size;   // max size of the history entry
  int widx;   // position of where to write the next history entry
  int idx;    // used to browse the history list

  char* curline;    // current line params
  int curlen;
  int curpos;
} History;

void tl_history_sethistorysize(TeelInstance* ti, int size);
void tl_history_destroy(TeelInstance* ti);
void tl_history_addhistory(TeelInstance* ti, const char* line, int len);
HistoryEntry* tl_history_getentry(TeelInstance* ti, int pos);
void tl_history_resetcontext(TeelInstance* ti);


#endif /* HISTORY_H_ */
