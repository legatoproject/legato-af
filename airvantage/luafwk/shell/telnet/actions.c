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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "teel_internal.h"

TeelCmd tl_act_backspace(TeelInstance* ti)
{
  tl_editor_deletechars(ti, -1);
  return CMD_NOP;
}
TeelCmd tl_act_deleteprevword(TeelInstance* ti)
{
  int offset = tl_editor_offsetuntilchars(ti, "\"#;&|^$=`'{}()<>\n\t ", ti->editor.cursorpos, -1);
  tl_editor_deletechars(ti, offset);
  return CMD_NOP;
}
TeelCmd tl_act_deletenextword(TeelInstance* ti)
{
  int offset = tl_editor_offsetuntilchars(ti, "\"#;&|^$=`'{}()<>\n\t ", ti->editor.cursorpos, 1);
  tl_editor_deletechars(ti, offset);
  return CMD_NOP;
}
TeelCmd tl_act_delete(TeelInstance* ti)
{
  tl_editor_deletechars(ti, 1);
  return CMD_NOP;
}
TeelCmd tl_act_editdone(TeelInstance* ti)
{
  tl_editor_movecursor(ti, RELATIVE_TO_ENDPOS, 0);
  TL_PUTCHARS("\r\n");
  tl_editor_saveandcleareditingarea(ti);
  ti->state = EDITLINE_DONE;
  return CMD_DONE;
}
TeelCmd tl_act_linebreak(TeelInstance* ti)
{
  char lb[]={'\r', '\n'};
  tl_editor_insertchars(ti, lb, 2);
  return CMD_NOP;
}
TeelCmd tl_act_moveleft(TeelInstance* ti)
{
  tl_editor_movecursor(ti, RELATIVE_TO_CURRENTPOS, -1);
  return CMD_NOP;
}
TeelCmd tl_act_moveright(TeelInstance* ti)
{
  tl_editor_movecursor(ti, RELATIVE_TO_CURRENTPOS, 1);
  return CMD_NOP;
}
TeelCmd tl_act_movetoprevword(TeelInstance* ti)
{
  int offset = tl_editor_offsetuntilchars(ti, "\"#;&|^$=`'{}()<>\n\t ", ti->editor.cursorpos, -1);
  tl_editor_movecursor(ti, RELATIVE_TO_CURRENTPOS, offset);
  return CMD_NOP;
}
TeelCmd tl_act_movetonextword(TeelInstance* ti)
{
  int offset = tl_editor_offsetuntilchars(ti, "\"#;&|^$=`'{}()<>\n\t ", ti->editor.cursorpos, 1);
  tl_editor_movecursor(ti, RELATIVE_TO_CURRENTPOS, offset);
  return CMD_NOP;
}
TeelCmd tl_act_movetostartpos(TeelInstance* ti)
{
  tl_editor_movecursor(ti, RELATIVE_TO_STARTPOS, 0);
  return CMD_NOP;
}
TeelCmd tl_act_movetoendpos(TeelInstance* ti)
{
  tl_editor_movecursor(ti, RELATIVE_TO_ENDPOS, 0);
  return CMD_NOP;
}

TeelCmd tl_act_exit(TeelInstance* ti)
{
  exit(0);
  return CMD_NOP;
}
TeelCmd tl_act_susp(TeelInstance* ti)
{
  TL_PUTCHARS("\r\n");
  tl_editor_saveandcleareditingarea(ti);
  ti->state = EDITLINE_DONE;
  return CMD_SUSP;
}
TeelCmd tl_act_ip(TeelInstance* ti)
{
  TL_PUTCHARS("\r\n");
  tl_editor_saveandcleareditingarea(ti);
  ti->state = EDITLINE_DONE;
  return CMD_IP;
}
TeelCmd tl_act_eof(TeelInstance* ti)
{
  if (tl_editor_editisempty(ti))
  {
    TL_PUTCHARS("\r\n");
    return CMD_EOF;
  }

  return CMD_NOP;
}
TeelCmd tl_act_overwriteinserttoggle(TeelInstance* ti)
{
  ti->editor.editmode = 1 - ti->editor.editmode;
  return CMD_NOP;
}

TeelCmd tl_act_historypreventry(TeelInstance* ti)
{
  ti->state = HISTORY_BROWSE;

  History* h = &ti->history;
  int pos = h->idx+1;

  HistoryEntry* e = tl_history_getentry(ti, pos);
  if (!e)
    TL_PUTCHARS("\a"); // ding
  else
  {
    if (pos == 1)
    {
      char* line;
      int len;
      int offset = tl_editor_getcursoroffset(ti);
      tl_editor_getline(ti, &line, &len, 0);
      h->curline = line;
      h->curlen = len;
      h->curpos = offset;
    }
    ti->history.idx = pos;
    tl_editor_seteditcontent(ti, e->buf, e->len);
  }

  return CMD_NOP;
}
TeelCmd tl_act_historynextentry(TeelInstance* ti)
{
  ti->state = HISTORY_BROWSE;

  History* h = &ti->history;
  int pos = h->idx-1;

  if (pos<0)
  {
    TL_PUTCHARS("\a"); // ding
    return CMD_NOP;
  }

  ti->history.idx = pos;

  if (!pos)
  {
    tl_editor_seteditcontent(ti, h->curline, h->curlen);
    tl_editor_movecursor(ti, RELATIVE_TO_STARTPOS, h->curpos);
    MEM_FREE(h->curline);
    h->curline = 0;
    h->curlen = h->curpos = 0;
    return CMD_NOP;
  }

  HistoryEntry* e = tl_history_getentry(ti, pos);
  assert(e);
  tl_editor_seteditcontent(ti, e->buf, e->len);

  return CMD_NOP;
}


static void display_autocomplete_choices(TeelInstance* ti, char** tab, int nb)
{
  int i;

  // compute max len
  int colwidth = 0;
  for (i = 0; i < nb; ++i)
  {
    colwidth = max(colwidth, strlen(tab[i]));
    //printf("%s\n", tab[i]);
  }

  colwidth++; // add one for whitespace

  tl_editor_clearafter(ti);

  tl_editor_outofband_begin(ti, 1);
  Editor* ed = &ti->editor;
  int nbofcols = ((ed->windowwidth - colwidth) / colwidth) + 1;
  for (i = 0; i < nb; i++)
  {
    int l = strlen(tab[i]);
    tl_editor_outofband_output(ti, tab[i], l, 0);
    int newline = (i % nbofcols == nbofcols - 1);
    if (newline)
      tl_editor_outofband_output(ti, 0, 0, 1);
    else
    {
      int r = colwidth - l;
      for (; r > 0; r--)
        tl_editor_outofband_output(ti, " ", 1, 0);
    }
  }
  tl_editor_outofband_end(ti);
}


static void completeline(TeelInstance* ti, char* tocomplete, int tocompletelen, char* line, int cursorpos)
{
  // check if already typed char are part of the proposal so to only insert the non typed part
  int l = tocompletelen;
  char* w = tocomplete;
  int o = min(l, cursorpos);
  char* p = line + cursorpos;
  while (o>0)
  {
    if(memcmp(p-o, w, o) == 0)
    break;
    o--;
  }
  tl_editor_insertchars(ti, w+o, l-o);
}

static int cmp(char* s1, int l1, char* s2, int l2)
{
    int i;
    int l = min(l1, l2);

    for(i=0; i<l; i++)
        if (s1[i] != s2[i]) break;

    return i;
}

TeelCmd tl_act_autocomplete(TeelInstance* ti)
{
  if (!ti->autocomplete)
  {
    TL_PUTCHARS("\a"); // ding
    return CMD_NOP;
  }


  char* path;
  int cursor;
  int linelen;
  tl_editor_getline(ti, &path, &linelen, &cursor);

  char** tab;
  int nb;
  ti->autocomplete(ti->ud, path, cursor, &nb, &tab);

  if (!nb || !tab) // no auto complete proposals
  {
    MEM_FREE(path);
    TL_PUTCHARS("\a"); // ding
    return CMD_NOP;
  }

  else if (nb == 1 || ti->autocompletestate) // only one choice or choices have been showed already
    completeline(ti, tab[0], strlen(tab[0]), path, cursor);

  else
  {
    // Check if there is a common part in all choices, if so add it to the edited line
    int i;
    int l = strlen(tab[0]);
    for (i=1; i<nb; i++)
    {
        l = cmp(tab[i-1], l, tab[i], strlen(tab[i]));
        if (!l) break;
    }
    if (l)
        completeline(ti, tab[0], l, path, cursor);

    // Display all the choices as well
    display_autocomplete_choices(ti, tab, nb);
    ti->state = COMPLETION_DISP;
    ti->autocompletestate = 1;
  }

  MEM_FREE(path);
  MEM_FREE(tab);
  return CMD_NOP;
}

TeelCmd tl_act_testabcstring(TeelInstance* ti)
{
  char s[] = "abcdefghijklmnopqrs";
  tl_editor_insertchars(ti, s, sizeof(s)-1);
  return CMD_NOP;
}
TeelCmd tl_act_test012string(TeelInstance* ti)
{
  //printf("tl_act_test012string\n");
  char s[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
  tl_editor_insertchars(ti, s, sizeof(s)-1);
  return CMD_NOP;
}
TeelCmd tl_act_debugF11(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  teel_setdisplaysize(ti, ed->windowwidth-3, ed->windowheight);
  return CMD_NOP;
}
TeelCmd tl_act_debugF12(TeelInstance* ti)
{
  char* line;
  int cursor;
  int linelen;
  tl_editor_getline(ti, &line, &linelen, &cursor);

  int i;
  for (i = 0; i < linelen; ++i) {
    printf("%X ", line[i]);
  }
  printf("\n");
  free(line);
  return CMD_NOP;
}


