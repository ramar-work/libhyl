#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zdb.h>
#include "lua.h"

#ifndef LDB_H
#define LDB_H

int db_exec ( lua_State * );

extern struct luaL_Reg db_set[];

#endif

