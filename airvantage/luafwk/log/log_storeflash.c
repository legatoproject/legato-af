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
#include "stdlib.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "log_store.h"
#include "log_storeflash.h"

//comment
//if not defined, then it's up to the OS to decide when to flush to persistent storage
//can be more efficient
#define AUTOFLUSH

#define FLASH_FILE_NAME_BASE  "logflashstore_file"
//buffer size used for backward reading
static char* buf_file_names = NULL;

static FILE* File1 = NULL;
//FILE* File2 = NULL;
static char* filename1 = NULL;
static char* filename2 = NULL;
static int file_size_limit = 0;

// check if files swapping is needed, if so, remove file old_file, move current_file to old file
static void swap_files(int size_to_add, int size_limit,
    const char* current_file, const char* old_file)
{
  struct stat st;
  if (stat(current_file, &st))
  {
    return;
  }
  if ((st.st_size + size_to_add) > size_limit)
  {
    fclose(File1);
    remove(old_file);
    rename(current_file, old_file);
    File1 = fopen(filename1, "a");
  }

}

//store in files
//param checks
//do file swap if needed, then write new log(s) in current file
int l_logflashstore(lua_State *L)
{
  size_t len = 0;
  const char *s = luaL_checklstring(L, 1, &len);

  if (NULL == File1)
    LUA_RETURN_ERROR("logflashstore: logflashinit not done");


  swap_files(len, file_size_limit, filename1, filename2);

  size_t res = fwrite(s, 1, len, File1);
  if (res != len)
  {
    lua_pushnil(L);
    lua_pushfstring(L,
        "logflashstore: internal error, cannot write to file [%s]", filename1);
    return 2;
  }

#ifdef AUTOFLUSH
  fflush(File1);
#endif

  lua_pushstring(L, "ok");
  return 1;
}

//init: create files names to store log, store size limits for ram/flash store.
//ram and flash path given as params, init check access to those paths
int l_logflashinit(lua_State *L)
{
  if (NULL != File1)
  {
    lua_pushstring(L, "logflashinit: init already done");
    return 1;
  }

  if (lua_type(L, 1) != LUA_TTABLE){
    LUA_RETURN_ERROR("logflashinit: Provided parameter is not correct: need table param with 'size' and 'path' fields");
  }

  LUA_CHECK_FIELD_TYPE(1, "size", LUA_TNUMBER, "logflashinit");
  file_size_limit = luaL_checkinteger(L, -1);
  LUA_CHECK_FIELD_TYPE(1, "path", LUA_TSTRING, "logflashinit");
  size_t len_flash_path = 0;
  const char* flash_path = luaL_checklstring(L, -1, &len_flash_path);

  char* log_path = NULL;
  char* mkdir_cmd = NULL;
  lua_getglobal(L, "LUA_AF_RW_PATH");
  if (lua_isstring(L, -1))
  {
    const char* working_dir = lua_tostring(L, -1);
    log_path = malloc(strlen(working_dir) + len_flash_path + 1);
    if (log_path != NULL)
      sprintf(log_path, "%s%s", working_dir, flash_path);
  }
  else
  {
    log_path = malloc(2 + len_flash_path + 1);
    if (log_path != NULL)
      sprintf(log_path, "./%s", flash_path);
  }
  if (log_path == NULL)
  {
      LUA_RETURN_ERROR("logflashinit: Allocation failure");
  }
  len_flash_path = strlen(log_path);
  int mkdir_status = -1;
  mkdir_cmd = malloc(9 + len_flash_path + 1);
  if (mkdir_cmd != NULL)
  {
    sprintf(mkdir_cmd, "mkdir -p %s", log_path);
    mkdir_status = system((const char*)mkdir_cmd);
  }
  else
  {
      LUA_RETURN_ERROR("logflashinit: Allocation failure");
  }
  if ((mkdir_status != 0) || access(log_path, W_OK))
  {
    if (log_path != NULL)
    {
      free(log_path);
      log_path = NULL;
    }
    if (mkdir_cmd != NULL)
    {
      free(mkdir_cmd);
      mkdir_cmd = NULL;
    }
    LUA_RETURN_ERROR("logflashinit: Provided path is not correct: cannot create");
  }

  if (log_path[len_flash_path - 1] == '/')
    len_flash_path--;

  buf_file_names = malloc(2 * (len_flash_path + 25));
  if (NULL == buf_file_names)
  {
      if (log_path != NULL)
      {
        free(log_path);
        log_path = NULL;
      }
      if (mkdir_cmd != NULL)
      {
        free(mkdir_cmd);
        mkdir_cmd = NULL;
      }
      LUA_RETURN_ERROR("Malloc error at init!");
  }

  filename1 = buf_file_names;
  filename2 = buf_file_names + len_flash_path + 25;
  snprintf(filename1, len_flash_path + 25, "%.*s/%s1.log",
      (int) len_flash_path, log_path, FLASH_FILE_NAME_BASE);
  snprintf(filename2, len_flash_path + 25, "%.*s/%s2.log",
      (int) len_flash_path, log_path, FLASH_FILE_NAME_BASE);

  if (log_path != NULL)
  {
    free(log_path);
    log_path = NULL;
  }
  if (mkdir_cmd != NULL)
  {
    free(mkdir_cmd);
    mkdir_cmd = NULL;
  }

  //create empty files if not existing yet
  File1 = fopen(filename2, "a");
  fclose(File1);
  File1 = fopen(filename1, "a");
  //keep File1 opened for upcoming write calls

  lua_pushstring(L, "ok");
  return 1;
}

int l_logflashgetsource(lua_State *L)
{
  //if logflash is not init, return error
  //if nothing is stored (totalsize == 0), return ltn12.source.string("Nothing to read in flash");
  //else return   ltn12.source.cat(ltn12.source.file(io.open(filename1)),  ltn12.source.file(io.open(filename2)) )

  // TODO reinit flash logs content: empty. check behavior towards returned ltn12 source.

  if (NULL == File1)
    LUA_RETURN_ERROR("logflashgetsource: logflashinit not done");

  struct stat st;
  stat(filename1, &st);
  int totalsize = st.st_size;
  stat(filename2, &st);
  totalsize += st.st_size;
  if (!totalsize)
  {
    lua_getglobal (L, "ltn12"); // ltn12
    lua_getfield(L, 1, "source"); //ltn12, ltn12.source
    lua_remove(L, 1);// ltn12.source
    lua_getfield(L, 1, "string"); //ltn12.source, ltn12.source.string
    lua_remove(L, 1); // ltn12.source.string
    lua_pushstring(L, "Nothing to read in flash");
    lua_call(L, 1, 1); // ltn12.source.string("nothing to read")
    return 1;
  }

  lua_getglobal (L, "io"); //io
  lua_getglobal (L, "ltn12"); // io, ltn12
  lua_getfield(L, 2, "source"); //io, ltn12, ltn12.source
  lua_remove(L, 2); //io, ltn12.source
  lua_getfield(L, 2, "file"); //io,  ltn12.source, ltn12.source.file,
  lua_getfield(L, 1, "open"); //io,  ltn12.source,  ltn12.source.file, io.open
  lua_pushstring(L, filename2); // io, , ltn12.source,  ltn12.source.file, io.open, "filename2"
  lua_call(L, 1, 1); // io, ltn12.source,ltn12.source.file,  F2
  lua_call(L, 1, 1); // io, ltn12.source, ltn12.source.file(F2)

  lua_getfield(L, 2, "file"); //io, ltn12.source, ltn12.source.file(F2), ltn12.source.file,
  lua_getfield(L, 1, "open"); //io, ltn12.source,  ltn12.source.file(F2), ltn12.source.file, io.open
  lua_remove(L, 1); // ltn12.source,  ltn12.source.file(F2), ltn12.source.file, io.open
  lua_pushstring(L, filename1); // ltn12.source,  ltn12.source.file(F2), ltn12.source.file, io.open, "filename1"
  lua_call(L, 1, 1); // ltn12.source,ltn12.source.file(F2),  ltn12.source.file, F1
  lua_call(L, 1, 1); // ltn12.source, ltn12.source.file(F2),  ltn12.source.file(F1)
  lua_getfield(L, 1, "cat"); // ltn12.source, ltn12.source.file(F2),  ltn12.source.file(F1) ,ltn12.source.cat
  lua_remove(L, 1); // ltn12.source.file(F2),  ltn12.source.file(F1) ,ltn12.source.cat
  lua_insert(L, 1); // ltn12.source.cat,  ltn12.source.file(F2),  ltn12.source.file(F1)

  lua_call(L, 2, 1);// ltn12.source.cat(F2,F1)

  return 1;
}

int l_logflashdebug(lua_State* L)
{
  return 0;
}

