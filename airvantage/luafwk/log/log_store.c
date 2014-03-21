/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#include "lua.h"
#include "lauxlib.h"
#include "mem_define.h"
#include "stdint.h"
#include "mem_define.h"
#include <string.h>

//get logstoreflash API
#include "log_store.h"
#include "log_storeflash.h"

static uint16_t buf_size = 0;
static uint8_t* buf = NULL;

static uint16_t current = 0;
static uint16_t first = 0;

//offsetin is the offset where to read in global buffer buf
//size: the size of the next log entry
//offsetout where actually begin the next log, take in account possible
void readnextsize(uint16_t offsetin, uint16_t* size, uint16_t* offsetout)
{
  *offsetout = (offsetin + 2) % (buf_size);
  *size = buf[offsetin] + (((uint16_t)(buf[(offsetin + 1) % buf_size])) << 8) ;
}

//offset must an index in buffer, located before string size
int getnextcopyindexes(uint16_t offset, uint16_t* firstindex,
    uint16_t* firstsize, uint16_t* secondindex, uint16_t* secondsize)
{

  //this call will affect firstindex, and  firstindex
  readnextsize(offset, firstsize, firstindex);

  if (*firstindex + *firstsize > buf_size - 1)
  {//2blocs to read, firstsize is total size of 2 blocs to copy
    *secondindex = 0;
    //secondsize is total size minus the size from current position to the end of the buffer
    *secondsize = (*firstsize) - (buf_size - *firstindex);
    //firstindex keeps value affected by readnextsize
    //firstsize is the size from current position to the end of the buffer
    *firstsize = buf_size - *firstindex;
    return 2;
  }
  else
  {//1bloc to read
    //firstindex, and  firstindex keep value affected by readnextsize
    return 1;
  }
}

//write at offset current in buf
void writelog(uint8_t* bufin, uint16_t lenght)
{
  if (current + lenght > buf_size)
  {
    uint16_t pt1_size = buf_size - current;
    memcpy(buf + current, bufin, pt1_size);
    bufin += pt1_size;
    lenght -= pt1_size;
    current = 0;
  }

  memcpy(buf + current, bufin, lenght);
  current += lenght;
}

//write size at offset current in buf
void writesize(uint16_t size)
{
  buf[current] = size & 0xFF;
  buf[(current + 1) % buf_size] = (size) >> 8;
  current = (current + 2) % buf_size;
}

static int l_lograminit(lua_State *L)
{
  // check arguments
  luaL_checktype(L, 1, LUA_TTABLE);
  LUA_CHECK_FIELD_TYPE(1, "size", LUA_TNUMBER, "lograminit");

  buf_size = lua_tointeger(L, -1);
  if (NULL != buf)
  {
    lua_pushstring(L, "lograminit: init already done");
    return 1;
  }
  if (buf_size > 2048)
    LUA_RETURN_ERROR("lograminit: provided buffer size is too big : max is 2048");

  buf = MEM_ALLOC(buf_size); // FIXME: This buffer is never freed
  if (NULL == buf)
    LUA_RETURN_ERROR("lograminit: malloc failed");

  memset(buf, 0, buf_size);
  current = 0;
  first = 0;
  lua_pushstring(L, "ok");
  return 1;
}

static int l_logramstore(lua_State *L)
{
  size_t loglen = 0;
  const char *s = luaL_checklstring(L, 1, &loglen);

  if (NULL == buf)
    LUA_RETURN_ERROR("logramstore: no init done! Log rejected!");

  if (loglen + sizeof(current) > buf_size)
    LUA_RETURN_ERROR("logramstore: buffer too big! Log rejected!");

  if (!loglen)
    LUA_RETURN_ERROR("logramstore: new log is an empty buffer. Log rejected!");

  uint16_t log_size = (uint16_t) loglen;

  //uint16_t new_current = current + sizeof(current) + len;
  int spare_space = 0;
  if (current >= first)
    spare_space = buf_size - current + first;
  else
    spare_space = first - current;

  //if need to erase some data, inform it Lua VM by a signal
  if (spare_space < (log_size + sizeof(current)))
  {
    lua_getglobal(L, "sched");
    lua_getfield(L, -1, "signal");
    lua_pushstring(L, "logramstore");
    lua_pushstring(L, "erasedata");
    lua_call(L, 2, 0);
    lua_getfield( L, -1, "step");  // sched, sched.step
    lua_pcall(    L, 0, 0, 0);     // sched, -
    lua_pop(      L, 1);
    //stack is in initial state
  }
  //last lua call can have provoked a buffer flush, check if data erase is still needed.
  spare_space = 0;
  if (current >= first)
    spare_space = buf_size - current + first;
  else
    spare_space = first - current;

  if (spare_space < (log_size + sizeof(current)))
  {
    //actually overwrite older logs by moving first pointer
    int32_t space_to_free = log_size + sizeof(current) - spare_space;
    uint16_t tmp_size = 0;
    while (space_to_free > 0)
    {
      readnextsize(first, &tmp_size, &first);
      first = (first + tmp_size) % buf_size;
      space_to_free -= sizeof(current) + tmp_size;
    }
  }
  writesize(log_size);
  writelog((uint8_t*) s, log_size);

  lua_pushstring(L, "ok");
  return 1;
}

int l_logramget(lua_State* L)
{
  if (NULL == buf)
    LUA_RETURN_ERROR("logramget: lograminit not done!");

  if (current == first)
    LUA_RETURN_ERROR("logramget: no log stored in buffer, nothing to retrieve");

  luaL_Buffer lbuf;
  luaL_buffinit(L, &lbuf);
  uint16_t tmp_offset = first;
  uint16_t copyoffset1 = 0, copyoffset2 = 0, copysize1 = 0, copysize2 = 0,
      nb_parts = 0;
  while (tmp_offset != current)
  {
    copyoffset1 = 0, copyoffset2 = 0, copysize1 = 0, copysize2 = 0;
    nb_parts = getnextcopyindexes(tmp_offset, &copyoffset1, &copysize1,
        &copyoffset2, &copysize2);

    luaL_addlstring(&lbuf, (const char*)buf + copyoffset1, copysize1);
    if (nb_parts == 2)
    {
      luaL_addlstring(&lbuf, (const char*)buf + copyoffset2, copysize2);
      tmp_offset = copyoffset2 + copysize2;
    }
    else
      tmp_offset = copyoffset1 + copysize1;

    if (tmp_offset > buf_size)
      break;
  }
  luaL_pushresult(&lbuf);

  //finally, reinit buffer
  current = 0;
  first = 0;
  memset(buf, 0, buf_size);

  return 1;
}

int l_logramdebug(lua_State* L)
{
  if (NULL == buf)
    LUA_RETURN_ERROR("logramdebug: lograminit not done!");

  printf("debug: size=%d\n", buf_size);
  int i = 0;
  for (; i < buf_size; i++)
  {
    printf("[%d]=[%c][%d]", i, buf[i], buf[i]);
    if (i == first)
      printf(" <-- first ");
    if (i == current)
      printf(" <-- current ");
    printf("\n");
  }
  return 0;
}

static const luaL_Reg R[] =
{
{ "lograminit", l_lograminit },
{ "logramstore", l_logramstore },
{ "logramget", l_logramget },
{ "logramdebug", l_logramdebug },

{ "logflashinit", l_logflashinit },
{ "logflashstore", l_logflashstore },
{ "logflashgetsource", l_logflashgetsource },
{ "logflashdebug", l_logflashdebug },
{ NULL, NULL } };

int luaopen_log_store(lua_State* L)
{
  luaL_register(L, "log.store", R);
  return 1;

}

