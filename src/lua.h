#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zhttp.h>
#include <ztable.h>
#include <zrender.h>
#include <zmime.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <router.h>
#include <megadeth.h>
#include "lib.h"

#ifndef LLUA_H
#define LLUA_H

enum zlua_error {
	ZLUA_NO_ERROR,
	ZLUA_MISSING_ARGS,
	ZLUA_INCORRECT_ARGS
};

int lc ( struct HTTPBody *, struct HTTPBody * );
int lua_loadlibs( lua_State *, struct lua_fset *, int );
void lua_stackdump ( lua_State * );
void lua_dumpstack ( lua_State *, int * );
int ztable_to_lua ( lua_State *, zTable * ) ;
int lua_to_ztable ( lua_State *, int, zTable * ) ;
int lua_exec_file( lua_State *, const char *, char *, int );
#endif
