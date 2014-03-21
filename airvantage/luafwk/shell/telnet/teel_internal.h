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

#ifndef TEEL_INTERNAL_H_
#define TEEL_INTERNAL_H_

#include "mem_define.h"

#include "teel.h"
#include "buffers.h"
#include "actions.h"
#include "editor.h"
#include "history.h"

#define MAX_KEY_SEQUENCE_LENGTH 8

#define CHECK_BREAK(v) { if ((v)<0) break; }
#define CHECK_RETURN(v, r) { if ((v)<0) return (r); }
#define max(a, b) ((a)<(b)?(b):(a))
#define min(a, b) ((a)<(b)?(a):(b))
#define clip(v, a, b) (min(max((v), (a)), (b)))
#define TL_PUTCHARS(s) tl_putchars(ti, (s), sizeof(s)-1)
#define TL_PUTCHARSF(s, a...) tl_putcharsf(ti, (s), sizeof(s)-1, a)


#define CTL(x)    ((x) & 0x1F)
#define ISCTL(x)  ((x) && (x) < ' ')
#define UNCTL(x)  ((x) + 64)
#define ALT(x)    ((x) | 0x80)
#define ISALT(x)  ((x) & 0x80)
#define UNALT(x)  ((x) & 0x7F)


typedef TeelCmd (*SequenceFunc)(TeelInstance* ti);
typedef struct
{
  const char* seq;
  unsigned int seqsize;
  SequenceFunc func;
} KeySeqAction;

typedef struct
{
  char temp[MAX_KEY_SEQUENCE_LENGTH];
  int posinseq;
  int currseq;
} SeqSearch;

typedef enum
{
  CHARACTER_INPUT,
  HISTORY_BROWSE,
  EDITLINE_DONE,
  COMPLETION_DISP,
} TeelState;


struct _TeelInstance
{
  TeelReadFunc reader;
  TeelWriteFunc writer;
  void* ud; // User Data given in the read/write functions
  KeySeqAction* map;
  unsigned int nbofseq;
  SeqSearch seqsearch;
  Editor editor;
  WriteBuffer output;
  History history;
  TeelState state;

  char* previousline; // the previous line that was edited. Used to be returned into teel_getline() function
  int previouslinesize;

  TeelAutoCompleteFunc autocomplete;
  int autocompletestate;

  //unsigned int newline:1;
};



//void tl_unescape(char* buf, int size);
int tl_detectlinebreak(const char* buf, int* size);
void tl_putcharsf(TeelInstance* ti, const char* f, unsigned int len, ...);
void tl_putchars(TeelInstance* ti, const char* buf, unsigned int size);
void tl_putchars_unescaped(TeelInstance* ti, const char* buf, unsigned int size);

#endif /* TEEL_INTERNAL_H_ */
