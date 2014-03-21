Migration Script C Example: the source file
=========

~~~~{.c}
#include "lua.h"
#include "lauxlib.h"
#include "swi_log.h"
#include "MigrationScript.h"



int execute(lua_State* L){
  size_t vFromSize=0, vToSize=0;
  const char* vFrom = luaL_checklstring(L, 1, &vFromSize);
  const char* vTo = luaL_checklstring(L, 2, &vToSize);

  SWI_LOG("MIGRATIONSCRIPT", INFO, "execute: vFrom=[%s], vTo[%s] \n", vFrom, vTo);

  return 0;
}

static const luaL_Reg R[] =
{
{ "execute", execute },
{ NULL, NULL }
};

int luaopen_agent_migration(lua_State* L)
{
  luaL_register(L, "agent.migration", R);
  return 1;
}
~~~~
