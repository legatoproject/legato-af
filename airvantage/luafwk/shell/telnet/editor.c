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


Line* allocline(Editor* ed)
{
  int len = ed->windowwidth;
  Line* l = MEM_ALLOC(sizeof(*l)+len);
  l->len = 0;
  return l;
}

static void checkmaxlines(Editor* ed, int nb)
{
  if (ed->nbOfLines + nb > ed->maxNbOfLines)
  {
    int m = ed->maxNbOfLines;
    nb += DEFAULT_MAX_NB_OF_LINES;
    ed->maxNbOfLines += nb;
    ed->lines = MEM_REALLOC(ed->lines, sizeof(*ed->lines) * ed->maxNbOfLines);
    memset(ed->lines+m, 0, nb * sizeof(*ed->lines));
  }
}

static Line* checkline(Editor* ed, int row)
{
  if (row>=ed->nbOfLines)
  {
    checkmaxlines(ed, row-ed->nbOfLines+1);
    ed->nbOfLines = row+1;
  }

  if (!ed->lines[row])
    ed->lines[row] = allocline(ed);

  return ed->lines[row];
}

static void insertbuflines(Editor* ed, int from, int nb)
{
  int i;
  checkmaxlines(ed, nb);

  for (i = ed->nbOfLines; i < ed->nbOfLines+nb; ++i) // free allocated lines that may be overwritten by the following move
      MEM_FREE(ed->lines[i]);

  memmove(ed->lines+from+nb, ed->lines+from, (ed->nbOfLines-from)*sizeof(*ed->lines));

  for (i = from; i < from+nb; ++i) // allocate inserted lines
    ed->lines[i] = allocline(ed);

  ed->nbOfLines += nb;
}

static void deletebufline(Editor* ed, int from, int nb)
{
  nb = min(nb, ed->nbOfLines-from);
  if (nb <= 0)
    return;

  int i;

  for (i = from; i < from+nb; ++i) // free lines to delete
      MEM_FREE(ed->lines[i]);

  memmove(ed->lines+from, ed->lines+from+nb, (ed->nbOfLines-from-nb)*sizeof(*ed->lines)); //fills in the gap !

  memset(ed->lines+ed->nbOfLines-nb, 0, nb*sizeof(*ed->lines)); // clear remaining entries

  ed->nbOfLines -= nb;
}

static void freelinesbuffer(Line** lines, int nb)
{
  int i;
  for (i = 0; i < nb; ++i)
      MEM_FREE(lines[i]);
  MEM_FREE(lines);
}

void tl_editor_init(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  if (ed->lines)
    freelinesbuffer(ed->lines, ed->maxNbOfLines);
  ed->maxNbOfLines = DEFAULT_MAX_NB_OF_LINES;
  ed->lines = MEM_CALLOC(DEFAULT_MAX_NB_OF_LINES, sizeof(*ed->lines));
  ed->nbOfLines = 1;
  ed->lines[0] = allocline(ed);
  MEM_FREE(ed->tempBuf);
  ed->tempBuf = MEM_ALLOC(ti->editor.windowwidth); // temporary buffer that has the size of 1 line (used by inserts functions)
}

void tl_editor_destroy(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  if (ed->lines)
      freelinesbuffer(ed->lines, ed->maxNbOfLines);
  MEM_FREE(ed->tempBuf);
}

// return a div_t struct were .rem is the col and .quot is the row
static div_t getcoordsfrompos(TeelInstance* ti, int pos)
{
  return div(pos, ti->editor.windowwidth);
}
// return the position corresponding to the row/col position in the edited area
static int getposfromcoords(TeelInstance* ti, int row, int col)
{
  int pos = ti->editor.windowwidth * row + col;
  assert(pos >= 0);
  return pos;
}

// get the position of the last char of the editing area
static int lastcharpos(Editor* ed)
{
  if (!ed->nbOfLines)
    return 0;

  return (ed->nbOfLines-1)*ed->windowwidth+ed->lines[ed->nbOfLines-1]->len;
}


static void movedisplaycursor(TeelInstance* ti, int hoffset, int voffset)
{
  if (hoffset<0)
  {
    if (hoffset>-4)
    {
      while(hoffset++)
        TL_PUTCHARS("\b");
    }
    else
      TL_PUTCHARSF("\033[%dD", -hoffset);
  }
  else if (hoffset>0)
  {
    if (hoffset==1)
      TL_PUTCHARS("\033[C");
    else
      TL_PUTCHARSF("\033[%dC", hoffset);
  }
  if (voffset<0)
  {
    if (voffset==-1)
      TL_PUTCHARS("\033[A");
    else
      TL_PUTCHARSF("\033[%dA", -voffset);
  }
  else if (voffset>0)
  {
    if (voffset==1)
      TL_PUTCHARS("\033[B");
    else
      TL_PUTCHARSF("\033[%dB", voffset);
  }
}

static void setcursorpos(TeelInstance* ti, int pos)
{
  Editor* ed = &ti->editor;
  div_t c = getcoordsfrompos(ti, ed->cursorpos);
  div_t p = getcoordsfrompos(ti, pos);
  movedisplaycursor(ti, p.rem-c.rem, p.quot-c.quot);
  ed->cursorpos = pos;
}



// return the position displaced of nb chars from position idx
static int positionfromoffset(TeelInstance* ti, int pos, int nb, int* clipped)
{
  Editor* ed = &ti->editor;
  int to = pos;

  div_t p = getcoordsfrompos(ti, to);
  int r = p.quot;
  int c = p.rem;

  if (nb > 0) // move forward
  {
    while (nb)
    {
      Line* l = ed->lines[r];
      assert(l);

      int teol = l->len - c; // remaining chars till end of current line
      if (nb > teol)
      {
        to += teol; // move till end of current line
        r++;
        nb -= teol;
        if (r >= ed->nbOfLines) // if there is no next line just break !
          break;
        if (l->len < ed->windowwidth)  // count the line break
          nb -= 1;
        to += ed->windowwidth - l->len; // move to the beginning of next line
        c = 0;
      }
      else
      {
        to += nb;
        nb = 0;
      }
    }
  }
  else // move backward
  {
    while (nb)
    {
      if (-nb > c)
      {
        to -= c; // move till beginning of current line
        r--;
        nb += c;
        if (r < 0)
          break;
        nb += 1;
        Line* l = ed->lines[r];
        assert(l);
        int o = ed->windowwidth - l->len;
        o = max(o, 1);
        to -= o;
        c = l->len;
      }
      else
      {
        to += nb;
        nb = 0;
      }
    }
  }

  if (clipped)
    *clipped = nb; // will be non null if unable to move as much as the given nb
  return to;
}

// you can stop on line break by adding the '\n' char to the list of stop chars
int tl_editor_offsetuntilchars(TeelInstance* ti, const char* stopchars, int pos, int direction)
{
  Editor* ed = &ti->editor;
  direction = clip(direction, -1, 1);
  if (!direction) direction = 1;

  int offset = 0;
  int pol = 1;
  int lastchar = (direction==-1?1:0) + lastcharpos(ed);
  int firstchar = (direction==1?-1:0) + ed->userstartpos;

  if (direction == -1 && pos>ed->userstartpos)
  {
    pos = positionfromoffset(ti, pos, -1, 0);
    offset--;
  }

  while ((pos > firstchar) && (pos < lastchar))
  {
    div_t p = getcoordsfrompos(ti, pos);
    Line* l = checkline(ed, p.quot);
    char c;
    if (p.rem >= l->len)
      c = '\n';
    else
      c = l->buf[p.rem];
    if (pol && !strchr(stopchars, c))
      pol = 0;
    else if (!pol && strchr(stopchars, c))
      break;

    pos = positionfromoffset(ti, pos, direction, 0);
    offset += direction;
  }

  if (direction == -1 && pos>ed->userstartpos) // so to position on the 1st char of the word
    offset++;

  return offset;
}

// compute the offset (in number of chars) of the position "to" from the position "from" of the editing area
// linebreaksize is the nb that is added for a line break
static int offsetfromposition(TeelInstance* ti, int from, int to, int linebreaksize)
{
  assert(to >= from);

  if (to == from)
    return 0;

  Editor* ed = &ti->editor;
  div_t t = getcoordsfrompos(ti, to);
  div_t f = getcoordsfrompos(ti, from);

  int offset = 0;
  while (1)
  {
    offset += t.rem;
    t.quot--;
    if (t.quot < f.quot)
      break;
    Line* l = checkline(ed, t.quot);
    int breakatend = l->len < ed->windowwidth;
    t.rem = l->len + breakatend*linebreaksize;
  }

  offset -= f.rem;

  return offset;
}

int tl_editor_getcursoroffset(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  return offsetfromposition(ti, ed->userstartpos, ed->cursorpos, 1);
}


// move the cursor nb chars.
// if nb<0 move left
// if nb>0 move right
// mode indicate the relative offset according to the enum name
// the move is clamped on the edited area limits
void tl_editor_movecursor(TeelInstance* ti, MoveMode mode, int nb)
{
  mode = clip(mode, -1, 1)+1;
  Editor* ed = &ti->editor;
  const int pos[] = { ed->userstartpos,
                      ed->cursorpos,
                      lastcharpos(ed) };
  int to = pos[mode];

  int clipped;
  to = positionfromoffset(ti, to, nb, &clipped);
  if (to < ed->userstartpos)
  {
    to = ed->userstartpos;
    clipped = 1;
  }


  if (clipped) // we were not able to move as much as asked, ding !
    TL_PUTCHARS("\a"); // ding
    //printf("DING!\n");

  if (ed->cursorpos == to)
    return;

  setcursorpos(ti, to);
}


// insert blank lines (in the buffer and the screen) from the current cursor position
static void insertblanklines(TeelInstance* ti, int nl, int after)
{
  Editor* ed = &ti->editor;
  int i;
  if (after)
    after = 1;
  div_t p = getcoordsfrompos(ti, ed->cursorpos);
  // scroll the screen down: make sure we have enough space to insert
  for (i = p.quot; i < ed->nbOfLines+nl-1; ++i)
    TL_PUTCHARS("\r\n");
  movedisplaycursor(ti, p.rem, -(ed->nbOfLines+nl-1-p.quot)+after);

  TL_PUTCHARSF("\033[%dL", nl); // insert blank lines on the screen
  insertbuflines(ed, p.quot+after, nl); // insert blank lines into the buffer
  movedisplaycursor(ti, 0, -after);
}



// write some chars from the current cursor position, overwriting any existing content
static void writechars(TeelInstance* ti, const char* buf, int size, int cleannewlines)
{
  Editor* ed = &ti->editor;
  div_t p = getcoordsfrompos(ti, ed->cursorpos);
  int teol = ed->windowwidth-p.rem; // till end of line
  int len = size;


  while (len)
  {
    Line* l = checkline(ed, p.quot);
    int s = min(len, teol);
    tl_putchars_unescaped(ti, buf, s);
    memcpy(l->buf+p.rem, buf, s);
    l->len = max(p.rem+s, l->len);
    if (s == teol) // a line is complete
    {
      teol = ed->windowwidth;
      p.rem = 0;
      TL_PUTCHARS("\r\n");
      if (cleannewlines)
        TL_PUTCHARS("\033[K");
      p.quot++;
    }
    len -= s;
    buf += s;
  }

  checkline(ed, p.quot); // make sure the line is allocated if the cursor just wrap
  ed->cursorpos += size;

  ///* DEBUG*/tl_flushout(ti);
}

// insert b0 followed by b1 in the line at the current position of the cursor.
// returns the number of bytes actually inserted
// outlen points to the number of bytes (if any) that were forced out of the current line.
// when outlen is non null then the cursor is set on the start of the next line
static int insertlinechars(TeelInstance* ti, const char* buf, int size, char* out, int* outlen)
{
  Editor* ed = &ti->editor;
  if (size==0)
    size = 1;
  assert(size > 0);

  div_t p = getcoordsfrompos(ti, ed->cursorpos);
  Line* l = checkline(ed, p.quot);

  // how much can we actually insert ?
  int teol = ed->windowwidth-p.rem;
  int toinsert = min(size, teol);
  int tooutput = max(l->len+toinsert-ed->windowwidth, 0);
  int breakatend = l->len < ed->windowwidth;


  char temp[tooutput];
  memcpy(temp, l->buf+l->len-tooutput, tooutput); // save what goes beyond the current line
  *outlen = tooutput;


  if (toinsert == teol) // insertion reach the last char of the line
  {
    writechars(ti, buf, toinsert, 0);
  }
  else // insertion does not reach the end of current line
  {
    if (l->len > p.rem) // only need to insert if cur idx is not on the last char of the line
    {
      if (toinsert == 1)
        TL_PUTCHARS("\033[@");
      else
        TL_PUTCHARSF("\033[%d@", toinsert);
      memmove(l->buf+p.rem+toinsert, l->buf+p.rem, l->len-tooutput-p.rem); // shift the chars by toinsert
      l->len += toinsert-tooutput;
    }

    writechars(ti, buf, toinsert, 0);

    if (l->len > ed->windowwidth-toinsert) // only need to clear last char if the line was already full minus toinsert
      TL_PUTCHARSF("\033[%dC\033[K\033[%dD", teol-toinsert, teol-toinsert);
  }


  // Push the outputed chars (if any)
  memcpy(out, temp, tooutput);

  p = getcoordsfrompos(ti, ed->cursorpos);
  if (tooutput && p.rem) // goes to the next line if the current line is full
  {
    assert(l->len == ed->windowwidth);
    TL_PUTCHARS("\r\n");
    ed->cursorpos += ed->windowwidth-p.rem;
  }
  if (l->len == ed->windowwidth && breakatend) // insert a blank line if the current line is full and had a break at end
    insertblanklines(ti, 1, ed->cursorpos%ed->windowwidth);

  ///* DEBUG*/tl_flushout(ti);

  return toinsert;
}



// Insert chars in the line buffer and update output accordingly
// It will insert chars at the cursor position
static void insertunescapedchars(TeelInstance* ti, const char* buf, int size)
{
  Editor* ed = &ti->editor;

  if (ed->cursorpos == lastcharpos(ed)) // cursor at end of line, just need to check if we need to wrap on the next line
  {
    writechars(ti, buf, size, 1);
  }
  else // cursor in the middle of the line
  {
    if(ed->editmode == 0) // we are in Insert mode
    {
      do
      {
        int s;
        int savedpos = ed->cursorpos;
        char* outbuf = ed->tempBuf;
        int outlen;

        // insert the 1st line
        s = insertlinechars(ti, buf, size, outbuf, &outlen);
        assert(outlen <= ed->windowwidth);
        buf += s; size -= s;
        if (!size) // all text was inserted
        {
          if (!outlen) // and nothing was pushed out of the line
            break;
          // we need to take care of some pushed out chars that need to be inserted on the next lines
          savedpos += s; // kept to replace the cursor at the correct position at the end
        }

        // insert whole lines
        int nl = (size+outlen)/ed->windowwidth;
        if ( nl > 0)
        {
          insertblanklines(ti, nl, 0);
          s = min(size, nl*ed->windowwidth); // nb of chars to write (whole lines)
          writechars(ti, buf, s, 0);
          if (size == s)
          {
            savedpos = ed->cursorpos;
            s = min(outlen, nl*ed->windowwidth-size);
            writechars(ti, outbuf, s, 0);
            outlen -= s;
            memmove(outbuf, outbuf+s, outlen);
            size = 0;
          }
          else
          {
            buf += s;
            size -= s;
          }
        }
        ///* DEBUG*/tl_flushout(ti);

        if (size || outlen)
        {
          if (size)
            savedpos = ed->cursorpos + size; // kept to replace the cursor at the correct position at the end
          //insert into remaining lines
          assert(outlen+size <= ed->windowwidth);
          memmove(outbuf+size, outbuf, outlen);
          memcpy(outbuf, buf, size);
          outlen += size;
          while(outlen)
            insertlinechars(ti, outbuf, outlen, outbuf, &outlen);
        }

        // goes back to the saved position
        setcursorpos(ti, savedpos);
      } while(0);

    }
    else  // we are in Overwrite mode
    {
      writechars(ti, buf, size, 0);
    }
  }
}


// Insert a line break at the current position
static void insertlinebreak(TeelInstance* ti)
{
  Editor* ed = &ti->editor;

  div_t p = getcoordsfrompos(ti, ed->cursorpos);

  if (ed->cursorpos == lastcharpos(ed)) // cursor at end of line
  {
    TL_PUTCHARS("\r\n");
    ed->cursorpos = getposfromcoords(ti, p.quot+1, 0);
    checkline(ed, p.quot+1);
  }
  else // cursor in the middle of the edit area
  {
    Line* l = checkline(ed, p.quot);
    if (p.rem == l->len) // cursor at end of line
    {
      TL_PUTCHARS("\r\n");
      ed->cursorpos = getposfromcoords(ti, p.quot+1, 0);
      insertblanklines(ti, 1, 0);
    }
    else // cursor in the middle of a line
    {
      char* outbuf = ed->tempBuf;
      int breakatend = l->len < ed->windowwidth;
      int outlen = l->len-p.rem;
      l->len = p.rem;
      TL_PUTCHARS("\033[K\r\n");
      ed->cursorpos = getposfromcoords(ti, p.quot+1, 0);

      if (breakatend)
        insertblanklines(ti, 1, 0);

      int savedpos = ed->cursorpos;

      insertlinechars(ti, l->buf+l->len, outlen, outbuf, &outlen);
      while(outlen)
        insertlinechars(ti, outbuf, outlen, outbuf, &outlen);

      setcursorpos(ti, savedpos);
    }
  }
}

void tl_editor_insertchars(TeelInstance* ti, const char* buf, int size)
{

  if (!size)
    return;

  while (size)
  {
    int s = size;
    int b = tl_detectlinebreak(buf, &s);
    if (s)
    {
      insertunescapedchars(ti, buf, s);
      buf += s;
      size -= s;
    }
    if (b)
    {
      insertlinebreak(ti);
      s = 0;
      while (buf[s] != '\n')
        s++;
      s++;
      buf += s;
      size -= s;
    }
  }
}


// delete some chars into one line.
// when available, refill byte until the end of line or buffer, which come first
// return the offset of the cursor
static int deletelinechars(TeelInstance* ti, int nb)
{
  assert(nb>0);
  Editor* ed = &ti->editor;
  div_t p = getcoordsfrompos(ti, ed->cursorpos); // cursor row/col position
  Line* l = checkline(ed, p.quot);

  int teol = l->len - p.rem;
  int breakatend = l->len < ed->windowwidth;
  int n;

  if (nb < teol+breakatend) // there is enough chars on that line to delete
  {
    if (nb == teol)
      TL_PUTCHARS("\033[K"); // erase until the end of line
    else if (nb == 1)
      TL_PUTCHARS("\033[P"); // erase the char
    else
      TL_PUTCHARSF("\033[%dP", nb); // erase the block
    memmove(l->buf+p.rem, l->buf+p.rem+nb, teol-nb);
    l->len -= nb;
    if (l->len == 0) // super wirdo bug of the linux console?! sometimes the last char is not deleted... force it to be deleted
      TL_PUTCHARS(" \b");
    n = nb;
  }
  else // the delete will span on next line and / or take the whole line
  {
    if (p.rem) // still some chars on that line
    {
      if (p.rem < l->len)
      {
        TL_PUTCHARS("\033[K"); // erase until the end of line
        l->len = p.rem;
      }
      n = teol+breakatend;
    }
    else // the whole line need to be deleted
    {
      if (p.quot == ed->nbOfLines-1)
        TL_PUTCHARS("\033[K"); // because of super wirdo bug in the linux console ? when trying to remove the last line it wont do it, so we just kill it
      else
        TL_PUTCHARS("\033[M");
      deletebufline(ed, p.quot, 1);
      return nb-teol-breakatend;
    }
  }

  // do we need to write some bytes after the moved block ?
  int pecl = ed->cursorpos - p.rem + l->len; // position of the end of current line
  int after = positionfromoffset(ti, pecl, nb-n+1, 0); //+1 for the "fake" line break that was created by the delete
  int end = lastcharpos(ed);
  if ((nb>teol || !breakatend) && after <= end)
  {
    div_t a = getcoordsfrompos(ti, after);
    assert(a.quot < ed->nbOfLines);
    Line* la = checkline(ed, a.quot);
    assert(a.rem <= la->len);
    int dnlb = ed->windowwidth-l->len > la->len-a.rem; // do we need to delete next line break?
    int tw = min(ed->windowwidth-l->len, la->len-a.rem); // how many chars to write
    setcursorpos(ti, pecl);
    writechars(ti, la->buf+a.rem, tw, 0);
    tw += dnlb; // this line has a line break
    nb += tw;
    p = getcoordsfrompos(ti, ed->cursorpos);
    if (p.rem != 0 && nb-n > 0) // make sure we are located at the beginning of next line ! (only if there are other chars to delete!)
    {
      TL_PUTCHARS("\r\n");
      ed->cursorpos += ed->windowwidth-p.rem;
    }
  }

  return nb-n;
}

// Delete chars in the line buffer and update output accordingly
// if nbofchars is positive delete chars located under and after the cursor
// if nbofchars is negative delete chars located before the cursor
// the delete is clipped in the current area
void tl_editor_deletechars(TeelInstance* ti, int nbofchars)
{

  if (!nbofchars)
    return;

  Editor* ed = &ti->editor;
  div_t p = getcoordsfrompos(ti, ed->cursorpos); // cursor row/col position
  Line* l = checkline(ed, p.quot);

  int clipped, pos;
  pos = positionfromoffset(ti, ed->cursorpos, nbofchars, &clipped);
  if (pos < ed->userstartpos)
  {
    clipped -= offsetfromposition(ti, pos, ed->userstartpos, 1);
    pos = ed->userstartpos;
  }

  if (clipped == nbofchars) // we are not able to delete all asked, ding !
    TL_PUTCHARS("\a"); // ding
  if (pos == ed->cursorpos)
    return;
  nbofchars -= clipped; // adjust the nb of chars so that it will not overflow

  // Optimize when deleting the last char of a line
  if (nbofchars == -1 && p.rem == l->len)
  {
    if (p.rem > 0) // not on the first idx of the line
    {
      TL_PUTCHARS("\b \b");
      l->len--;
      ed->cursorpos--;
    }
    else // on the first idx of the line
    {
      p.quot--;
      l = checkline(ed, p.quot);
      int nbae = l->len == ed->windowwidth; // no break at end

      if (p.quot < ed->nbOfLines-2) // delete the line if we are not at the end of the edit are
        TL_PUTCHARS("\033[M");

      if (l->len-nbae>0)
        TL_PUTCHARSF("\033[A\033[%dC \b", l->len-nbae);
      else
        TL_PUTCHARS("\033[A");
      l->len -= nbae;
      ed->cursorpos -= ed->windowwidth-l->len;
      deletebufline(ed, p.quot+1, 1);
    }
    return;
  }
  else
  {
    // Convert a negative nbofchars into the positive case, and move the cursor accordingly
    if (nbofchars<0)
    {
      tl_editor_movecursor(ti, RELATIVE_TO_CURRENTPOS, nbofchars);
      nbofchars = -nbofchars;
    }

    int savedpos = ed->cursorpos;
    int nb = nbofchars;

    while(nb)
    {
      nb = deletelinechars(ti, nb);
    }
    setcursorpos(ti, savedpos);
  }
}

void tl_editor_seteditcontent(TeelInstance* ti, char* buf, int size)
{
  Editor* ed = &ti->editor;
  int ovw = ed->editmode;
  ed->editmode = 1; // force overwrite mode!
  tl_editor_movecursor(ti, RELATIVE_TO_STARTPOS, 0);
  tl_editor_insertchars(ti, buf, size);
  int pos = ed->cursorpos;

  if (pos < lastcharpos(ed)) // trim the remaining lines, if any
  {
    div_t p = getcoordsfrompos(ti, pos);
    int i;
    for (i = p.quot; i < ed->nbOfLines; ++i)
    {
      TL_PUTCHARS("\033[K");
      if (i<ed->nbOfLines-1)
        TL_PUTCHARS("\r\n");
      Line* l = checkline(ed, i);
      l->len = p.rem;
      p.rem = 0;
    }
    ed->cursorpos = lastcharpos(ed);
    setcursorpos(ti, pos);
    ed->nbOfLines = p.quot+1;
  }

  ed->editmode = ovw;
}

// copy the current line into buf, and return the size in byte of the line
// buf can be null, in that case it only returns the size of the line
// chars included are chars defined by: from <= chars < to
static int copyline(TeelInstance* ti, char* buf, int from, int to)
{
  Editor* ed = &ti->editor;
  Line dummy;
  dummy.len = 0;

  if (from > to)
    return 0;

  // Compute the edited line size
  int ls = 0;
  int i;
  div_t f = getcoordsfrompos(ti, from);
  div_t t = getcoordsfrompos(ti, to);
  for (i=f.quot; i<=t.quot; i++)
  {
    Line* l = ed->lines[i];
    l = l?l:&dummy;
    int s = i==f.quot?min(f.rem, l->len):0; // start idx in that line
    int e = i==t.quot?min(t.rem, l->len):l->len; // end idx in that line
    int len = e-s;
    ls += len;
    if (buf)
    {
      memcpy(buf, l->buf+s, len);
      buf += len;
    }
    if (e==l->len && e<ed->windowwidth && i<t.quot) // add \r\n if there is a line break
    {
      ls += 2;
      if (buf)
      {
        buf[0] = '\r'; buf[1] = '\n';
        buf += 2;
      }
    }
  }
  return ls;
}

void tl_editor_getline(TeelInstance* ti, char** line, int* len, int* cursorpos)
{
  Editor* ed = &ti->editor;


  int s = ed->userstartpos;
  int e = lastcharpos(ed);
  int ls = copyline(ti, 0, s, e); // get line size
  if (line)
  {
    *line = MEM_ALLOC(ls+3); // 3 is the max the line formating can consume
    copyline(ti, *line, s, e);
  }
  if (len)
    *len = ls;
  if (cursorpos)
    *cursorpos = offsetfromposition(ti, ed->userstartpos, ed->cursorpos, 2);
}

void tl_editor_cleareditingarea(TeelInstance* ti)
{
  int i;
  Editor* ed = &ti->editor;

  for (i = 0; i < DEFAULT_MAX_NB_OF_LINES; ++i)
    if (ed->lines[i])
      ed->lines[i]->len = 0;

  for (; i < ed->maxNbOfLines; ++i)
  {
    MEM_FREE(ed->lines[i]);
    ed->lines[i] = 0;
  }
  ed->nbOfLines = 1;

  ed->cursorpos = 0;
  ed->userstartpos = 0;
}

void tl_editor_saveandcleareditingarea(TeelInstance* ti)
{
    MEM_FREE(ti->previousline);

  tl_editor_getline(ti, &ti->previousline, &ti->previouslinesize, 0);
  tl_editor_cleareditingarea(ti);
}
// Write the prompt at he beginning of the line
void tl_editor_writeprompt(TeelInstance* ti, const char* buf, int len)
{
  Editor* ed = &ti->editor;
  int pps = ed->userstartpos;
  int s;
  int savedpos = ed->cursorpos;
  setcursorpos(ti, 0);
  if (pps)
  {
    s = min(pps, len);
    writechars(ti, buf, s, 0);
    buf += s;
    len -= s;
    pps -= s;
  }
  if (len)
    insertunescapedchars(ti, (char*)buf, len);
  else if (pps)
    deletelinechars(ti, pps);

  savedpos += ed->cursorpos -ed->userstartpos;
  ed->userstartpos = ed->cursorpos;
  setcursorpos(ti, savedpos);
}

// Change the display size and redraw the editing area
void tl_editor_setdisplaysize(TeelInstance* ti, int width, int height)
{
  Editor* ed = &ti->editor;

  int offset = offsetfromposition(ti, 0, ed->cursorpos, 1);
  // Clear the display
  setcursorpos(ti, 0);
  TL_PUTCHARS("\033[J");

  // Set the new dimensions
  int windowwidth = ed->windowwidth;
  ed->windowwidth = width;
  ed->windowheight = height;

  // Reformat the line buffer
  int i, j;
  Line** lines = ed->lines;
  int nbOfLines = ed->nbOfLines;

  ed->lines = 0; // mark the buffer as free so the line wont be freed for now
  tl_editor_init(ti); // reallocate lines buffer

  for (i = 0, j = 0; i < nbOfLines; ++i)
  {
    Line* f = lines[i];
    assert(f);

    int len = f->len;
    int breakatend = (len < windowwidth) && (i<nbOfLines-1); // break at end only if not the last line !
    char* buf = f->buf;

    while (len)
    {
      Line* t = checkline(ed, j);
      int s = min(len, ed->windowwidth-t->len);
      memcpy(t->buf+t->len, buf, s);
      t->len += s;
      len -= s;
      buf += s;
      if (len)
        j++;
    }

    if (breakatend)
      j++;

    checkline(ed, j); // make sure the line is allocated...
  }

  // Free the old line buffer
  freelinesbuffer(lines, nbOfLines);

  // Redraw the editing area
  for (i = 0; i < ed->nbOfLines; ++i)
  {
    Line* l = checkline(ed, i);
    tl_putchars_unescaped(ti, l->buf, l->len);
    if (i < ed->nbOfLines-1)
      TL_PUTCHARS("\r\n");
  }
  ed->cursorpos = lastcharpos(ed);

  int pos = positionfromoffset(ti, 0, offset, 0);
  setcursorpos(ti, pos);
}

int tl_editor_editisempty(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  return lastcharpos(ed)-ed->userstartpos == 0;
}

void tl_editor_clearafter(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  int pos = ed->cursorpos;
  setcursorpos(ti, lastcharpos(ed));
  TL_PUTCHARS("\033[J");
  setcursorpos(ti, pos);
}

// location: -1 before editing area
//            1 after editing area
void tl_editor_outofband_begin(TeelInstance* ti, int location)
{
  Editor* ed = &ti->editor;
  ed->aoblocation = location;

  if (location == -1)
  {
    // scroll the screen down: make sure we have enough space to insert one line
    div_t p = getcoordsfrompos(ti, ed->cursorpos);
    int i;
    for (i = p.quot; i < ed->nbOfLines; ++i)
      TL_PUTCHARS("\r\n");
    movedisplaycursor(ti, 0, -ed->nbOfLines);
    TL_PUTCHARS("\033[L");
    ed->aobcol = 0;
    ed->aobrow = -1;
  }
  else if (location == 1)
  {
    div_t p = getcoordsfrompos(ti, ed->cursorpos);
    movedisplaycursor(ti, 0, ed->nbOfLines-p.quot-1);
    TL_PUTCHARS("\r\n");

    ed->aobrow = ed->nbOfLines;
    ed->aobcol = 0;
    TL_PUTCHARS("\033[K");
  }

}

void tl_editor_outofband_end(TeelInstance* ti)
{
  Editor* ed = &ti->editor;
  div_t p = getcoordsfrompos(ti, ed->cursorpos);
  movedisplaycursor(ti, p.rem-ed->aobcol, -ed->aobrow+p.quot);
}

int tl_editor_outofband_output(TeelInstance* ti, const char* buf, int len, int newline)
{
  Editor* ed = &ti->editor;
  int l = min(len, ed->windowwidth-ed->aobcol);
  tl_putchars_unescaped(ti, buf, l);
  ed->aobcol += l;

  if (newline)
  {
    if (ed->aoblocation == -1)
    {
      TL_PUTCHARS("\r\n");
      // scroll the screen down: make sure we have enough space to insert one line
      int i;
      for (i = 0; i < ed->nbOfLines; ++i)
        TL_PUTCHARS("\r\n");
      movedisplaycursor(ti, 0, -ed->nbOfLines);
      TL_PUTCHARS("\033[L");
      ed->aobcol = 0;
    }
    else if (ed->aoblocation == 1)
    {
      if (ed->nbOfLines+ed->aobrow < ed->windowheight)
      {
        TL_PUTCHARS("\r\n");
        TL_PUTCHARS("\033[K");
        ed->aobrow++;
        ed->aobcol = 0;
      }
    }
  }

  return l;
}


