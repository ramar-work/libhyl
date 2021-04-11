#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lua.h"

#ifndef LDB_H
#define LDB_H

int db_exec ( lua_State *, const char *, const char * );

extern struct luaL_Reg db_set[];

#endif

