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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include "teel_internal.h"



#define SEQ(s)  (s), sizeof(s)-1
KeySeqAction defaultksa[] =
{
  {SEQ("\x08"), tl_act_backspace},
  {SEQ("\x7F"), tl_act_backspace},
  {SEQ("\033[3~"), tl_act_delete},
  {SEQ("\033\x7F"), tl_act_deleteprevword},
  {SEQ("\033[3;3~"), tl_act_deletenextword},
  {SEQ("\033\033[3~"), tl_act_deletenextword},

  {SEQ("\n"), tl_act_editdone},
  {SEQ("\r\n"), tl_act_editdone},
  {SEQ("\r\0"), tl_act_editdone},
  {SEQ("\033\n"), tl_act_linebreak},
  {SEQ("\033\r\0"), tl_act_linebreak},

  {SEQ("\003"), tl_act_ip},
  {SEQ("\032"), tl_act_susp},
  {SEQ("\004"), tl_act_eof},

  {SEQ("\033[A"), tl_act_historypreventry},
  {SEQ("\033[B"), tl_act_historynextentry},
  {SEQ("\x09"), tl_act_autocomplete},
  {SEQ("\033[2~"), tl_act_overwriteinserttoggle},

  {SEQ("\033[1;3D"), tl_act_movetoprevword},
  {SEQ("\033\033[D"), tl_act_movetoprevword},
  {SEQ("\033[1;3C"), tl_act_movetonextword},
  {SEQ("\033\033[C"), tl_act_movetonextword},
  {SEQ("\033[C"), tl_act_moveright},
  {SEQ("\033[D"), tl_act_moveleft},
  {SEQ("\033[1~"), tl_act_movetostartpos},
  {SEQ("\033[7~"), tl_act_movetostartpos},
  {SEQ("\033[H"), tl_act_movetostartpos},
  {SEQ("\033OH"), tl_act_movetostartpos},
  {SEQ("\001"), tl_act_movetostartpos}, // Ctrl-A
  {SEQ("\033[4~"), tl_act_movetoendpos},
  {SEQ("\033[8~"), tl_act_movetoendpos},
  {SEQ("\033[F"), tl_act_movetoendpos},
  {SEQ("\033OF"), tl_act_movetoendpos},
  {SEQ("\005"), tl_act_movetoendpos}, // Ctrl-E

  {SEQ("\033[24~"), tl_act_debugF12},
//  {SEQ("\033[23~"), tl_act_debugF11},
//  {SEQ("\033[21~"), tl_act_test012string},
//  {SEQ("\033[20~"), tl_act_testabcstring},
};

int compareksa(const void* p1, const void* p2)
{
  KeySeqAction* a = (KeySeqAction*) p1;
  KeySeqAction* b = (KeySeqAction*) p2;
  int s = min(a->seqsize, b->seqsize);
  int r = memcmp(a->seq, b->seq, s);

  return r==0?a->seqsize-b->seqsize:r;
}

void tl_setkeyseqact_map(TeelInstance* ti, KeySeqAction* map, int len)
{
  if (ti->map)
      MEM_FREE(ti->map);

  // Copy and sort the array by key sequence order
  ti->map = MEM_ALLOC(sizeof(*ti->map)*len);
  ti->nbofseq = len;
  memcpy(ti->map, map, len*sizeof(*ti->map));
  qsort(ti->map, len, sizeof(*ti->map), compareksa);
}

TeelInstance* teel_initialize(TeelReadFunc reader, TeelWriteFunc writer, void* ud)
{
  TeelInstance* ti = MEM_ALLOC(sizeof(*ti));
  memset(ti, 0, sizeof(*ti));

  ti->reader = reader;
  ti->writer = writer;
  ti->ud = ud;

  // Compute/sort the default sequence-action map
  tl_setkeyseqact_map(ti, defaultksa, sizeof(defaultksa)/sizeof(defaultksa[0]));

  // Initialize Sequence searcher variables
  ti->seqsearch.currseq = ti->nbofseq-1;
  ti->seqsearch.posinseq = 0;

  // Initialize default windows size
  ti->editor.windowheight = 10;
  ti->editor.windowwidth = 20;

  tl_editor_init(ti);

  return ti;
}

int teel_destroy(TeelInstance* ti)
{
  // Free history entrylist, if any
  tl_history_destroy(ti);

  // Free output buffer
  FreeBuffer(&ti->output);

  // Free edited line
  tl_editor_destroy(ti);
  if (ti->previousline)
      MEM_FREE(ti->previousline);

  // Free Key map
  MEM_FREE(ti->map);

  // Free the object itself
  MEM_FREE(ti);

  return 0;
}


// Output chars with the writer function: what is displayed to the user...
void tl_putchars(TeelInstance* ti, const char* buf, unsigned int size)
{
  WriteBytes(&ti->output, buf, size);
}

void tl_putchars_unescaped(TeelInstance* ti, const char* buf, unsigned int size)
{
  int i;
  for (i = 0; i < size; ++i)
  {
    char c = buf[i];
    if (!c || ISCTL(c))
      c = UNCTL(c);
    WriteByte(&ti->output, c);
  }
}


void tl_putcharsf(TeelInstance* ti, const char* f, unsigned int len, ...)
{
  char t[12];
  int i;
  va_list l;

  va_start(l, len);
  for (i = 0; i < len; ++i)
  {
    if (f[i] == '%' && i+1<len && f[i+1]=='d')
    {
      int n = snprintf(t, sizeof(t), "%d", va_arg(l, int));
      assert(n<8);
      WriteBytes(&ti->output, t, n);
      i++; // skip the additional byte 'd'
    }
    else
      WriteByte(&ti->output, f[i]);
  }
  va_end(l);
}

void tl_flushout(TeelInstance* ti)
{
  if (ti->output.len)
  {
    ti->writer(ti->ud, ti->output.buffer, ti->output.len);
    FreeBuffer(&ti->output);
  }
}

// Change CTL chars into printable form.
// work in place, so the string has the same size !
void tl_unescape(char* buf, int size)
{
  int i;
  for (i = 0; i < size; ++i)
  {
    if (ISCTL(buf[i]))
      buf[i] = UNCTL(buf[i]);
  }
}


// return non nul if a line break is detected
// in that case *size contains the size of that line (not including the linebreak chars)
int tl_detectlinebreak(const char* buf, int* size)
{
  int i = 0;
  int s = *size;

  while(i<s && buf[i] != '\n')
    i++;
  if (i<s && buf[i-1] == '\r')
    i--;

  *size = i;

  return i<s;
}


static void tl_statechanged(TeelInstance* ti, TeelState laststate)
{
  if (ti->state == laststate)
    return;

  switch (laststate)
  {
    case EDITLINE_DONE:
      // Reset the newline state as soon as we read something on the input
      if (ti->previousline)
      {
        MEM_FREE(ti->previousline);
        ti->previousline = 0;
      }
      break;

    case HISTORY_BROWSE:
      tl_history_resetcontext(ti);
      break;

    case COMPLETION_DISP:
      tl_editor_clearafter(ti);
      ti->autocompletestate = 0;
      break;

    default:
      assert(0); // unknown state
    case CHARACTER_INPUT:
      break;
  }

}


TeelCmd teel_run(TeelInstance* ti)
{
  SeqSearch* ss = &ti->seqsearch;


  TeelCmd cmd;

  while (1)
  {
    tl_flushout(ti);
    int byte = ti->reader(ti->ud);
    if (byte < 0) {
      cmd = CMD_EOS;
      break;
    } else if (byte>127) { // transform any non ASCII 7bits into a '?' (we do not handle non 7bits ASCII chars)
      byte = 63;
    }

    ss->temp[ss->posinseq] = byte;
    int found = 0; // 0: notfound, 1:partially match, 2:found
    while (1)
    {
      if (ss->currseq < 0)
        break; // done, we wont match a sequence (no more sequences!)

      KeySeqAction* seq = &ti->map[ss->currseq];

      if (ss->posinseq <= seq->seqsize)
      {
        int v = ((unsigned char)seq->seq[ss->posinseq]);
        if (byte > v)
          break; // done, we wont match a sequence (because sequences are sorted)

        else if (byte == v)
        {
          if (ss->posinseq == seq->seqsize-1)
          {
            found = 2;  // we found our sequence !
            break;
          }
          else
          {
            found = 1; // we can still match a sequence, need to test the next char !
            ss->posinseq++;
            break;
          }
        }
        //else // byte > v => need to try the next sequence
      }

      ss->currseq--;
    }

    if (!found)
    {
      tl_editor_insertchars(ti, ss->temp, ss->posinseq+1);
      ss->posinseq = 0;
      ss->currseq = ti->nbofseq-1;

      // Do action that are related to state changes
      TeelState ls = ti->state;
      ti->state = CHARACTER_INPUT;
      tl_statechanged(ti, ls);
    }
    else if (found == 2)
    {
      TeelState ls = ti->state;
      ti->state = CHARACTER_INPUT; // default state is char input
      cmd = ti->map[ss->currseq].func(ti);
      ss->posinseq = 0;
      ss->currseq = ti->nbofseq-1;

      // Do action that are related to state changes
      tl_statechanged(ti, ls);

      if (cmd > CMD_NOP)
        break;
    }
    // else found==1 just continue with the next read char...
  }

  tl_flushout(ti);
  return cmd;
}

int tl_formatendofline(char* line, int len, unsigned int format)
{
  int a = 0;
  const char* t;
  if (format&2)
  {
    t = "";
    a+=1;
  }
  if (format&1)
  {
    t = "\r\n";
    a+=2;
  }
  if (line && a)
    memcpy(line+len, t, a);

  return len+a;
}

// if format&1 add trailing \r\n to the line buffer
// if format&2 add trailing \0 to the line buffer
// need to free line pointer when finished: it was allocated with a malloc
int teel_getline(TeelInstance* ti, char** line, int* len, unsigned int format)
{
  int ll;

  if (ti->previousline)
  {
    if (line)
      *line = ti->previousline;
    ll = ti->previouslinesize;
    ti->previousline = 0;
  }
  else
    tl_editor_getline(ti, line, &ll, 0);

  ll = tl_formatendofline(line?(*line):0, ll, format);

  if (len)
    *len = ll;
  return 0;
}


int teel_showprompt(TeelInstance* ti, const char* buf, int len)
{
  tl_editor_writeprompt(ti, buf, len); // Draw the prompt

  return 0;
}

int teel_setdisplaysize(TeelInstance* ti, int width, int height)
{
  width = max(width, 5);
  height = max(height, 5); // do not allow sizes too small !

  tl_editor_setdisplaysize(ti, width, height);

  return 0;
}


int teel_sethistorysize(TeelInstance* ti, int size)
{
  tl_history_sethistorysize(ti, size);
  return 0;
}

int teel_addhistory(TeelInstance* ti, const char* line, int len)
{
  // reset the read history position
  ti->history.idx = 0;

  // truncate trailing \0 if any
  if (len>=1 && line[len-1] == 0)
    len--;

  // truncate trailing \r\n if any
  if (len>=2 && line[len-2]=='\r' && line[len-1]=='\n')
    len -= 2;

  if (!len) // the line to add is empty !
    return -1;

  tl_history_addhistory(ti, line, len);
  return 0;
}


int teel_setautocompletefunc(TeelInstance* ti, TeelAutoCompleteFunc autocomplete)
{
  ti->autocomplete = autocomplete;
  return 0;
}

int teel_outputbeforeline(TeelInstance* ti, const char* buf, int len)
{
  if (!len || !buf)
    return -1;

  tl_editor_outofband_begin(ti, -1);
  while (len)
  {
    int s = len;
    int b = tl_detectlinebreak(buf, &s);
    tl_editor_outofband_output(ti, buf, s, b);
    buf += s;
    len -= s;

    if (b)
    {
      s = 0;
      while (buf[s] != '\n')
        s++;
      s++;
      buf += s;
      len -= s;
    }
  }
  tl_editor_outofband_end(ti);
  return 0;
}


