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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "buffers.h"
#include "teel.h"
#include "telnet.h"


#define OBJ_NAME "TelnetInterpreter"

typedef enum
{
  IC_NOP, // nothing special to do
  IC_EOS, // not enough byte to complete the parsing of the current stream
  IC_LINE, // a line of data is ready to be executed by the local system
  IC_SUSP, // local process should be put on background
  IC_IP, // local process should be interrupted
  IC_CLOSE, // client requested to close the connection
  IC_NB_OF_CMD,

} InterpretCommand;


static const char* ic_names[] =
{
    "nop", "nop", "line", "suspend", "interrupt", "close"
};


typedef enum
{
  UNKNOWN_MODE, // used for initialization only
  TELNET_LINE_MODE, // used when only line by line editing is required (do not allow advanced feature). Will not echo
  TELNET_EDIT_MODE, // used for advanced features (completion, history, line edition, etc.). Will echo
} EditMode;

typedef enum
{
  EOF_ = 236,
  SUSP = 237,
  ABORT= 238,
  IP   = 244,
  SE   = 240,
  SB   = 250,
  WILL = 251,
  WONT = 252,
  DO   = 253,
  DONT = 254,
  IAC  = 255
} TAction;

typedef enum
{
  ECHO = 1,
  SUPPRESS_GO_AHEAD = 3,
  TIMING_MARK = 6,
  NAWS = 31,
  LINEMODE = 34,
} TOption;


typedef struct
{
  WriteBuffer out;
  ReadBuffer in;
  EditMode editmode;
  TeelInstance* teel; // used for edit mode
  int autocompletefuncref; // Lua reference to the auto complete function
  WriteBuffer linemodebuffer; // only used for line mode
  lua_State* L;
} TelnetState;



#define CHECK_BREAK(v) { if ((v)<0) break; }
#define CHECK_RETURN(v, r) { if ((v)<0) return (r); }
#define max(a, b) ((a)<(b)?(b):(a))


static void SendOption(TelnetState* ts, int action, TOption option)
{
  char b[3] = {IAC, action, option};
  WriteBytes(&ts->out, b, sizeof(b));
}


static InterpretCommand parseoption(TelnetState* ts, TAction action, TOption option)
{

  switch (option)
  {
    case TIMING_MARK:
      // Send a reply when this is a request
      if (action == DO)
        SendOption(ts, WILL, TIMING_MARK);

      break;

    case SUPPRESS_GO_AHEAD:
      break;

    case ECHO:
      if (action == WILL) // we do the echo, not the client !
        SendOption(ts, DONT, ECHO);

      break;

    case NAWS:
      // nothing special to do here...
      break;

    default:
      // Default unknown option, reply DONT/WONT
      if (action == DO || action == WILL)
        SendOption(ts, action<=WONT?DONT:WONT, option);
      break;
  }
  return IC_NOP;
}


static InterpretCommand parsesuboption(TelnetState* ts, TOption suboption, ReadBuffer* input)
{
  switch (suboption)
  {
    case NAWS:
    {
      int v0, v1;
      int width, height;
      CHECK_RETURN(v1 = ReadEscByte(input), IC_EOS);
      CHECK_RETURN(v0 = ReadEscByte(input), IC_EOS);
      width = (v1 << 8) + v0;
      CHECK_RETURN(v1 = ReadEscByte(input), IC_EOS);
      CHECK_RETURN(v0 = ReadEscByte(input), IC_EOS);
      height = (v1 << 8) + v0;
      CHECK_RETURN(v1 = ReadByte(input), IC_EOS);
      CHECK_RETURN(v0 = ReadByte(input), IC_EOS);
      assert(v1==IAC);
      assert(v0==SE);
      teel_setdisplaysize(ts->teel, width, height);
      break;
    }
    default:
      printf("Telnet Interpreter: received a non supported sub option negotiation sequence, skipping it...");
      break;
  }

return IC_NOP;
}

static int teel_reader(void* ud)
{
  TelnetState* ts = (TelnetState*) ud;
  int val = -1;
  ReadBuffer* input = &ts->in;
  do
  {
    ReadMark(input);
    CHECK_BREAK(val = ReadByte(input));

    if (val == IAC)
    {
      CHECK_BREAK(val = ReadByte(input));

      if (val >= WILL && val <= DONT)
      {
        int opt;
        CHECK_BREAK(opt = ReadByte(input));
        parseoption(ts, val, opt);
      }
      else if (val == SB)
      {
        int sopt;
        CHECK_BREAK(sopt = ReadByte(input));
        parsesuboption(ts, sopt, input);
      }
      else if (val == IP)
      {
        val = 3;
        break;
      }
      else if (val == SUSP)
      {
        val = 26;
        break;
      }
      else if (val == EOF_)
      {
        val = 4;
        break;
      }
      else if (val == IAC) // escape code
      {
        val = 255;
        break;
      }
      else
        printf("Telnet interpreter: unknown character code: 255, %d. Skipping it...\n", val);
    }
    else // standard char...
      break;
  } while (1);

  ReadMark(input);
  return val;
}

static int teel_writer(void* ud, const char* buf, unsigned int size)
{
  TelnetState* ts = (TelnetState*) ud;
  WriteBytes(&ts->out, buf, size);
  return size;
}

static int output(TelnetState* ts);
static void teel_autocomplete(void* ud, char* path, int len, int* nb, char*** tab)
{
  TelnetState* ts = (TelnetState*) ud;
  lua_State* L = ts->L;

  lua_rawgeti(L, LUA_REGISTRYINDEX, ts->autocompletefuncref); // retrieve the auto complete function associated to that userdata

  if (lua_isnil(L, -1))
  {
    lua_pop(L, 1); // remove the nil from the stack to make it balanced
    *nb = 0;
    *tab = 0;
    return;
  }
  lua_pushlstring(L, path, len);
  int err = lua_pcall(L, 1, 2, 0);
  if (err != 0) // error during the call to get the table of completion proposals
  {
    output(ts); // output the error on the screen !
    lua_pop(L, 1); // remove the error from the stack to make it balanced
    *nb = 0;
    *tab = 0;
    return;
  }
  const char* s;
  size_t l;
  int n, i;
  s = lua_tolstring(L, -1, &l);
  n = lua_tointeger(L, -2);

  if (!n) // no completion proposals
  {
    *nb = 0;
    *tab = 0;
    lua_pop(L, 2); // make the stack balanced !
    return;
  }

  int msize = n*sizeof(char*)+l+1;
  char** t = MEM_ALLOC(msize);
  char*b = (char*)t + n*sizeof(char*);
  memcpy(b, s, l);
  b[l] = 0; // terminate with a nul char


  for (i = 0; i < n; ++i)
  {
    t[i] = b;
    b += strlen(b)+1;
  }
  assert(b == (char*)t+msize);

  lua_pop(L, 2); // make the stack balanced !

  *nb = n;
  *tab = t;
}


// A table must be given as parameter specifying various settings
//    mode: line / edit (default: line)
//    history: number specifying the max number of history entries
//    autocomplete: a function for auto completion

static int l_newinterpreter(lua_State *L)
{
  TelnetState* ts;
  ts = lua_newuserdata(L, sizeof(*ts));
  // Assign the metatable to the userdata
  luaL_getmetatable(L, OBJ_NAME);
  lua_setmetatable(L, -2);


  // Initialize default values
  memset(ts, 0, sizeof(*ts));
  ts->L = L;

  // default settings
  ts->editmode = TELNET_LINE_MODE;
  ts->autocompletefuncref = LUA_NOREF;

  // Parse params
  int historysize = 0;
  if (lua_type(L, -2) == LUA_TTABLE)
  {
    // Edit mode setting
    lua_getfield(L, -2, "mode");
    const char* m = lua_tostring(L, -1);
    if (m)
    {
      if (!strcmp("line", m))
        ts->editmode = TELNET_LINE_MODE;
      else if (!strcmp("edit", m))
        ts->editmode = TELNET_EDIT_MODE;
      else
        luaL_error(L, "mode should be either 'line' or 'edit' (got '%s' instead)", m);
    }
    lua_pop(L, 1);

    if (ts->editmode == TELNET_EDIT_MODE)
    {
      // History Size
      lua_getfield(L, -2, "history");
      historysize = lua_tointeger(L, -1);
      lua_pop(L, 1);

      // Autocomplete function
      lua_getfield(L, -2, "autocomplete");
      if (!lua_isnil(L, -1))
      {
        if (lua_type(L, -1) == LUA_TFUNCTION)
        {
          ts->autocompletefuncref = luaL_ref(L, LUA_REGISTRYINDEX);
          lua_pushnil(L); // to make it even with the following pop
        }
        else
          luaL_error(L, "autocomplete must be a function");
      }

      lua_pop(L, 1);
    }
  }




  if (ts->editmode == TELNET_EDIT_MODE)
  {
    // Do teel initialization and setup
    ts->teel = teel_initialize(teel_reader, teel_writer, ts);
    if (!ts->teel)
      return luaL_error(L, "Failed to initialize teel library");

    teel_sethistorysize(ts->teel, historysize);

    teel_setautocompletefunc(ts->teel, teel_autocomplete);

    // Request that all char are sent when typed
    SendOption(ts, DO, SUPPRESS_GO_AHEAD);
    SendOption(ts, WILL, SUPPRESS_GO_AHEAD);

    // Request to echo
    SendOption(ts, WILL, ECHO);

    // Request negotiation of windows size
    SendOption(ts, DO, NAWS);
  }

  return 1;
}



static InterpretCommand processeditmode(TelnetState* ts)
{
  InterpretCommand cmd = IC_NOP;
  TeelCmd tc = CMD_NOP;

  while (tc == CMD_NOP)
    tc = teel_run(ts->teel);

  switch(tc)
  {
    case CMD_EOS: cmd = IC_EOS; break;
    case CMD_DONE: cmd = IC_LINE; break;
    case CMD_IP: cmd = IC_IP; break;
    case CMD_SUSP: cmd = IC_SUSP; break;
    case CMD_EOF: cmd = IC_CLOSE; break;
    default: assert(0); break; // unknown command !
  }

  return cmd;
}


static InterpretCommand processlinemode(TelnetState* ts)
{
  InterpretCommand cmd = IC_EOS;
  ReadBuffer* input = &ts->in;
  WriteBuffer* line = &ts->linemodebuffer;
  int val;
  while (1)
  {
    ReadMark(input);
    CHECK_BREAK(val = ReadByte(input));

    if (val == IAC)
    {
      CHECK_BREAK(val = ReadByte(input));

      if (val >= WILL && val <= DONT)
      {
        int opt;
        CHECK_BREAK(opt = ReadByte(input));
        parseoption(ts, val, opt);
      }
      else if (val == SB)
      {
        int sopt;
        CHECK_BREAK(sopt = ReadByte(input));
        parsesuboption(ts, sopt, input);
      }
      else if (val == IP)
      {
        WriteBytes(&ts->out, "\r\n", 2);
        FreeBuffer(line);
        cmd = IC_IP;
        break;
      }
      else if (val == SUSP)
      {
        WriteBytes(&ts->out, "\r\n", 2);
        FreeBuffer(line);
        cmd = IC_SUSP;
        break;
      }
      else if (val == EOF_)
      {
        if (!line->len) // only close if there the current line is empty !
        {
          cmd = IC_CLOSE;
          break;
        }
      }
      else if (val == 255)
      {
        WriteByte(line, 255);
      }
      else
      {
        printf("Telnet interpreter: unknown character code: 255, %d. Skipping it...\n", val);
      }
    }
    else if (val == 10) // New line
    {
      WriteBytes(line, "\r\n", 2); // store and echo the newline
      cmd = IC_LINE;
      break;
    }
    else if (val == 13) // CR, NULL and CR/NL is interpreted as a new line
    {
      CHECK_BREAK(val = ReadByte(input));
      if (val == 0 || val == 10)
      {
        WriteBytes(line, "\r\n", 2); // store and echo the newline
        cmd = IC_LINE;
        break;
      }
      else // false detection, just store and echo as is.
      {
        WriteByte(line, 10);
        WriteByte(line, val);
      }
    }
    else if (val == 4) // EOF the user requested to close the terminal
    {
      if (!line->len) // only close if there the current line is empty !
      {
        cmd = IC_CLOSE;
        break;
      }
    }
    else if (val == 3) // IP (equivalent to IAC IP)
    {
      WriteBytes(&ts->out, "\r\n", 2);
      FreeBuffer(line);
      cmd = IC_IP;
      break;
    }
    else if (val == 19) // IP (equivalent to IAC SUSP)
    {
      WriteBytes(&ts->out, "\r\n", 2);
      FreeBuffer(line);
      cmd = IC_SUSP;
      break;
    }
    else // other char are just stored
    {
      WriteByte(line, val);
    }
  }

  return cmd;
}

static TelnetState* getts(lua_State *L)
{
  TelnetState* ts = luaL_checkudata(L, 1, OBJ_NAME);
  ts->L = L;
  return ts;
}

// call: interpret(telnetstate, readbytes)
// return: boolean: again, string: towritebytes, string: command, [string: arg]
static int l_interpret(lua_State *L)
{
  TelnetState* ts = getts(L);
  size_t inputlen = 0;
  const char* inputbuffer = 0;
  if (lua_type(L, 2) == LUA_TSTRING)
    inputbuffer = luaL_checklstring(L, 2, &inputlen);

  ReadBuffer* in = &ts->in;

  if (in->len && inputbuffer) // make the two buffers (old + new data) only one
  {
    int len = in->len + inputlen;
    char* buf = MEM_ALLOC(len);
    memcpy(buf, in->p, in->len);
    memcpy(buf+in->len, inputbuffer, inputlen);
    MEM_FREE(in->buffer);
    in->p = in->mark = in->buffer = buf;
    in->len = len;

  }
  else if (inputbuffer) // we only have new data
  {
    in->buffer = 0; // this buffer is not allocated
    in->p = in->mark = (char*) inputbuffer;
    in->len = inputlen;
  }


  InterpretCommand cmd;
  if (ts->editmode == TELNET_EDIT_MODE)
    cmd = processeditmode(ts);
  else
    cmd = processlinemode(ts);



  if (cmd != IC_EOS)
    ReadMark(in);
  else
    cmd = IC_NOP; // EOS command is equivalent to a NOP for the caller

  // Again ?
  if (in->len) // Only need to be called again if we did not exhaust the input buffers
    lua_pushboolean(L, 1);
  else
    lua_pushboolean(L, 0);

  // Store the unprocessed bytes, if any and if they are not already stored
  if (in->mark < in->p && !in->buffer)
  {
    int len = in->p-in->mark+in->len;
    in->buffer = MEM_ALLOC(len);
    memcpy(in->buffer, in->mark, len);
    in->p = in->mark = in->buffer;
    in->len = len;
  }
  // free the input buffer if it was allocated and is empty
  else if (in->mark == in->p && !in->len && in->buffer)
  {
    MEM_FREE(in->buffer);
    in->buffer = in->p = in->mark = 0;
    in->len = 0;
  }


  // Byte to send
  if (ts->out.buffer)
  {
    lua_pushlstring(L, ts->out.buffer, ts->out.len);
    FreeBuffer(&ts->out);
  }
  else
    lua_pushnil(L);



  // Command
  assert(cmd < IC_NB_OF_CMD);
  lua_pushstring(L, ic_names[cmd]); // push command

  // Optional argument
  int opt = 0;
  if (cmd == IC_LINE)
  {
    opt++;
    if (ts->editmode == TELNET_EDIT_MODE)
    {
      char* line;
      int len;
      teel_getline(ts->teel, &line, &len, 1);
      lua_pushlstring(L, line, len);
      MEM_FREE(line);
    }
    else // TELNET_LINE_MODE
    {
      lua_pushlstring(L, ts->linemodebuffer.buffer, ts->linemodebuffer.len);
      FreeBuffer(&ts->linemodebuffer);
    }
  }

  return 3 + opt;
}


static int l_close(lua_State *L)
{
  TelnetState* ts = getts(L);

  luaL_unref(L, LUA_REGISTRYINDEX, ts->autocompletefuncref); // erase the entry for that user data in the registry

  if (ts->teel)
  {
    teel_destroy(ts->teel);
    ts->teel = 0;
  }

  FreeBuffer(&ts->linemodebuffer);
  FreeBuffer(&ts->out);

  ReadBuffer* in = &ts->in;
  MEM_FREE(in->buffer);
  in->buffer = 0;

  return 0;
}

static int l_showprompt(lua_State *L)
{
  TelnetState* ts = getts(L);

  size_t len;
  const char* p = luaL_checklstring(L, 2, &len);

  if (ts->editmode == TELNET_EDIT_MODE)
    teel_showprompt(ts->teel, p, len);
  else
    WriteBytes(&ts->out, p, len);


  return 0;
}

static char* tabify(const char* buf, size_t* l)
{
  #define TAB_SIZE  8
  char* b;
  size_t i, j, o;
  size_t len = *l;
  int found = 0;

  // compute the number of chars to add because of tabs
  for (i=0, j=0, o=0; i < len; ++i)
  {
    if ((buf[i]=='\t'))
    {
      j += TAB_SIZE-(o%TAB_SIZE);
      found = 1;
    }
    else
    {
      j++;
      if ((buf[i]=='\n'))
        o = 0;
      else
        o++;
    }
  }

  if (!found)
    return 0;

  b = MEM_ALLOC(j);

  // replace tabs by spaces
  for (i=0, j=0, o=0; i < len; ++i)
  {
    if ((buf[i]=='\t'))
    {
      int k = TAB_SIZE-(o%TAB_SIZE);
      for (; k>0; k--)
        b[j++] = ' ';
    }
    else
    {
      b[j++] = buf[i];
      if ((buf[i]=='\n'))
        o = 0;
      else
        o++;
    }
  }

  *l = j;
  return b;
}

static int output(TelnetState* ts)
{
  lua_State* L = ts->L;
  size_t len;
  const char* buf = luaL_checklstring(L, -1, &len);
  char* b = 0;

  b = tabify(buf, &len);
  if (b)
    buf = b;

  if (ts->editmode == TELNET_EDIT_MODE)
  {
    // teel adds newlines automatically -> remove it
    if (len>0 && buf[len-1] == '\n')
      len--;
    if (len>0 && buf[len-1] == '\r')
      len--;

    teel_outputbeforeline(ts->teel, buf, len);
  }
  else
    WriteBytes(&ts->out, buf, len);


  MEM_FREE(b);
  return 0;
}

static int l_output(lua_State *L)
{
  TelnetState* ts = getts(L);

  return output(ts);
}


static int l_addhistory(lua_State *L)
{
  TelnetState* ts = getts(L);

  // history is only available in edit mode
  if (ts->editmode != TELNET_EDIT_MODE)
    return 0;

  size_t len;
  const char* buf = luaL_checklstring(L, 2, &len);

  teel_addhistory(ts->teel, buf, len);

  return 0;
}


static const struct luaL_reg telnet_f [] = {
  {"new", l_newinterpreter},
  {NULL, NULL}
};

static const struct luaL_reg telnet_m [] = {
  {"interpret", l_interpret},
  {"close", l_close},
  {"showprompt", l_showprompt},
  {"output", l_output},
  {"addhistory", l_addhistory},
  {NULL, NULL}
};


int luaopen_telnet(lua_State *L)
{
  luaL_newmetatable(L, OBJ_NAME);

  lua_pushstring(L, "__gc");
  lua_pushcfunction(L, l_close);
  lua_settable(L, -3);

  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */

  luaL_openlib(L, NULL, telnet_m, 0);

  luaL_openlib(L, "telnet", telnet_f, 0);
  return 1;
}
