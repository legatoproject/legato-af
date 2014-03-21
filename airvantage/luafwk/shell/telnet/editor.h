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

#ifndef EDITOR_H_
#define EDITOR_H_

#include "mem_define.h"

#define DEFAULT_MAX_NB_OF_LINES 2

typedef struct
{
  int len;
  char buf[];
} Line;

typedef enum
{
  RELATIVE_TO_STARTPOS    = -1,
  RELATIVE_TO_CURRENTPOS  =  0,
  RELATIVE_TO_ENDPOS      =  1,
} MoveMode;


typedef struct
{
  Line** lines;
  int nbOfLines;
  int maxNbOfLines;
  char* tempBuf;
  int cursorpos;  // position of the display cursor
  int userstartpos; // position of the start of the user area
  int windowwidth;
  int windowheight;
  int editmode; // 0: insert mode, 1: overwrite mode

  int aoblocation; // out of band output variables
  int aobrow;
  int aobcol;
} Editor;



void tl_editor_init(TeelInstance* ti);
void tl_editor_destroy(TeelInstance* ti);
void tl_editor_movecursor(TeelInstance* ti, MoveMode mode, int nb);
int tl_editor_offsetuntilchars(TeelInstance* ti, const char* stopchars, int pos, int direction);
int tl_editor_getcursoroffset(TeelInstance* ti);
void tl_editor_writeprompt(TeelInstance* ti, const char* buf, int len);
void tl_editor_insertchars(TeelInstance* ti, const char* buf, int size);
void tl_editor_deletechars(TeelInstance* ti, int nbofchars);
void tl_editor_seteditcontent(TeelInstance* ti, char* buf, int size);
void tl_editor_getline(TeelInstance* ti, char** line, int* len, int* cursorpos);
void tl_editor_saveandcleareditingarea(TeelInstance* ti);
void tl_editor_cleareditingarea(TeelInstance* ti);
void tl_editor_setdisplaysize(TeelInstance* ti, int width, int height);
int tl_editor_editisempty(TeelInstance* ti);
void tl_editor_clearafter(TeelInstance* ti);
void tl_editor_outofband_begin(TeelInstance* ti, int location);
void tl_editor_outofband_end(TeelInstance* ti);
int tl_editor_outofband_output(TeelInstance* ti, const char* buf, int len, int newline);

#endif /* EDITOR_H_ */
