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

#define LD_LEN 128

#define LD_ERRBUF_LEN 1024

enum zlua_error {
	ZLUA_NO_ERROR,
	ZLUA_MISSING_ARGS,
	ZLUA_INCORRECT_ARGS
};


struct route_t { 
	int iroute_tlen;
	struct iroute_t { char *route; int index; } **iroute_tlist;
	zTable *src;
};


struct mvc_t {
	struct mvcmeta_t *mset;
	int flen, type; 
	int depth;   //track the depth, so you know when to stop iterating
	//int inherit; //a keyword

	int model;
	int view;
	int query;
	
	const char ctype[ 128 ];
	struct imvc_t {
		const char file[ 2048 ], base[ 128 ], ext[ 16 ];
		//leave some space here...
		//const char *dir;
	} **imvc_tlist;
};


struct luadata_t {
	struct HTTPBody *req, *res;
	lua_State *state;
#if 0
	const char *aroute;
	const char *rroute;
	const char *apath;
	const char *db;
	const char *fqdn;
	const char *root;
	const char *dctype;
#else
	const char aroute[ LD_LEN ];
	const char rroute[ LD_LEN ];
	const char apath[ LD_LEN ];
	const char db[ LD_LEN ];
	const char fqdn[ LD_LEN ];
	const char root[ LD_LEN ];
	const char dctype[ LD_LEN ];
#endif
#if 1
	int status; //can return a different status
	//other zTables could go here...
	zTable *zconfig;
	zTable *zroutes;
	zTable *zroute;
	zTable *zmodel;
	zTable *zhttp; //you might not need this anymore...
#endif
	struct mvc_t pp; 
	char err[ LD_ERRBUF_LEN ];
};

int lua_loadlibs( lua_State *, struct lua_fset *, int );
void lua_dumpstack ( lua_State * );
int ztable_to_lua ( lua_State *, zTable * ) ;
int lua_to_ztable ( lua_State *, int, zTable * ) ;
int lua_exec_file( lua_State *, const char *, char *, int );
int lua_merge ( lua_State * );

#define lua_stackdump(L) lua_dumpstack(L)

#endif
