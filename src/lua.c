#include <hypno.h>
#include "lua.h"

#define FPRINTF( ... ) \
	fprintf( stderr, __VA_ARGS__ )

#define lua_pushibt(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushboolean(L, v), lua_settable(L, p)

#define lua_pushict(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushcclosure(L, v), lua_settable(L, p)

#define lua_pushift(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushcfunction(L, v), lua_settable(L, p)

#define lua_pushint(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushnumber(L, v), lua_settable(L, p)

#define lua_pushiit(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushinteger(L, v), lua_settable(L, p)

#define lua_pushist(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushstring(L, v), lua_settable(L, p)

#define lua_pushsbt(L, s, v, p) \
	lua_pushstring(L, s), lua_pushboolean(L, v), lua_settable(L, p)

#define lua_pushsct(L, s, v, p) \
	lua_pushstring(L, s), lua_pushcclosure(L, v), lua_settable(L, p)

#define lua_pushsft(L, s, v, p) \
	lua_pushstring(L, s), lua_pushcfunction(L, v), lua_settable(L, p)

#define lua_pushsnt(L, s, v, p) \
	lua_pushstring(L, s), lua_pushnumber(L, v), lua_settable(L, p)

#define lua_pushsit(L, s, v, p) \
	lua_pushstring(L, s), lua_pushinteger(L, v), lua_settable(L, p)

#define lua_pushsst(L, s, v, p) \
	lua_pushstring(L, s), lua_pushstring(L, v), lua_settable(L, p)

const char libname[] = "lua";
const char rname[] = "route";
const char def[] = "default";
const char confname[] = "config.lua";
static const char mkey[] = "model";
static const char rkey[] = "routes";


//TODO: This is an incredibly difficult way to make things 
//read-only...  Is there a better one?
const char read_only_block[] = " \
	function make_read_only( t ) \
		local tt = {} \
		local mt = { \
			__index = t	 \
		,	__newindex = function (t,k,v) error( 'something went wrong', 1 ) end \
		}	\
		setmetatable( tt, mt ) \
		return tt \
	end \
";


static struct mvcmeta_t { 
	const char *dir; 
	const char *reserved; 
	const char *ext;
} mvcmeta [] = {
	{ "app", "model", "lua" }
,	{ "sql", "query", "sql" }
,	{ "views", "views", "tpl" }
,	{ NULL, "content-type", NULL }
//,	{ NULL, "inherit", NULL }
};



//....
int run_lua_buffer( lua_State *L, const char *buffer ) {
	//Load a buffer	
	luaL_loadbuffer( L, buffer, strlen( buffer ), "make-read-only-function" );
	if ( lua_pcall( L, 0, LUA_MULTRET, 0 ) != LUA_OK ) {
		fprintf( stdout, "lua string exec failed: %s", (char *)lua_tostring( L, -1 ) );
		//This shouldn't fail, but if it does you should stop...
		return 0;
	}
	return 1;
}



//In lieu of a C-only way to make members read only, use this
int make_read_only ( lua_State *L, const char *table ) {
	//Certain tables (and their children) need to be read-only
	int err = 0;
	const char fmt[] = "%s = make_read_only( %s )";
	char execbuf[ 256 ] = { 0 };
	snprintf( execbuf, sizeof( execbuf ), fmt, table, table );

	//Load a buffer	
	luaL_loadbuffer( L, execbuf, strlen( execbuf ), "make-read-only" );
	if ( ( err = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		fprintf( stdout, "lua string exec failed: %s", (char *)lua_tostring( L, -1 ) );
		//This shouldn't fail, but if it does you should stop...
		return 0;
	}
	return 1;
}





#if 0
//Load Lua libraries
static lua_State * lua_load_libs( lua_State **L ) {
	if ( !( *L = luaL_newstate() ) ) {
		return NULL;
	}	
	luaL_openlibs( *L );
	return *L;
}
#endif
//Should return an error b/c there are some situations where this does not work.
int lua_loadlibs( lua_State *L, struct lua_fset *set, int standard ) {
	//Load the standard libraries
	if ( standard )	{
		luaL_openlibs( L );
	}

	//Now load everything written elsewhere...
	for ( ; set->namespace; set++ ) {
		lua_newtable( L );
		for ( struct luaL_Reg *f = set->functions; f->name; f++ ) {
			lua_pushstring( L, f->name );
			lua_pushcfunction( L, f->func );	
			lua_settable( L, 1 );
		}
		lua_setglobal( L, set->namespace );
	}

	//And finally, add some functions that we'll need later (if this fails, meh)
	if ( !run_lua_buffer( L, read_only_block ) ) {
		return 0;
	}
	return 1;
}



//checking args is putting me in a weird zone... 
int lua_getarg() {
	return ZLUA_NO_ERROR;
}



//Dump a stack
void lua_istack ( lua_State *L ) {
	fprintf( stderr, "\n" );
	for ( int i = 1; i <= lua_gettop( L ); i++ ) {
		fprintf( stderr, "[%d] => %s", i, lua_typename( L, lua_type( L, i ) ) );
		if ( lua_type( L, i ) == LUA_TSTRING ) {
			fprintf( stderr, " > %s", lua_tostring( L, i ) );
		}
		fprintf( stderr, "\n" );
	}
}



//TODO: Reject keys that aren't a certain type
void lua_dumpstack ( lua_State *L ) {
	const char spaces[] = /*"\t\t\t\t\t\t\t\t\t\t"*/"          ";
	const int top = lua_gettop( L );
	struct data { unsigned short count, index; } data[64] = {0}, *dd = data;
	dd->count = 1;

	//Return early if no values
	if ( ( dd->index = top ) == 0 ) {
		fprintf( stderr, "%s\n", "No values on stack." );
		return;
	}

	//Loop through all values on the stack
	for ( int it, depth=0, ix=0, index=top; index >= 1; ix++ ) {
		fprintf( stderr, "%s[%d:%d] ", &spaces[ 10 - depth ], index, ix );

		for ( int t = 0, count = dd->count; count > 0; count-- ) {
			if ( ( it = lua_type( L, index ) ) == LUA_TSTRING )
				fprintf( stderr, "(%s) %s", lua_typename( L, it ), lua_tostring( L, index ) );
			else if ( it == LUA_TFUNCTION )
				fprintf( stderr, "(%s) %d", lua_typename( L, it ), index );
			else if ( it == LUA_TNUMBER )
				fprintf( stderr, "(%s) %lld", lua_typename( L, it ), (long long)lua_tointeger( L, index ) );
			else if ( it == LUA_TBOOLEAN)
				fprintf( stderr, "(%s) %s", lua_typename( L, it ), lua_toboolean( L, index ) ? "T" : "F" );
			else if ( it == LUA_TTHREAD )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_tothread( L, index ) );
			else if ( it == LUA_TLIGHTUSERDATA || it == LUA_TUSERDATA )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_touserdata( L, index ) );
			else if ( it == LUA_TNIL || it == LUA_TNONE )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_topointer( L, index ) );
			else if ( it == LUA_TTABLE ) {
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_topointer( L, index ) );
			}

			//Handle keys
			if ( count > 1 )
				index++, t = 1, dd->count -= 2, fprintf( stderr, " -> " );
			//Handle new tables
			else if ( it == LUA_TTABLE ) {
				lua_pushnil( L );
				if ( lua_next( L, index ) != 0 ) {
					++dd, ++depth; 
					dd->index = index, dd->count = 2, index = lua_gettop( L );
				}
			}
			//Handle situations in which a table is on the "other side"
			else if ( t ) {
				lua_pop( L, 1 );
				//TODO: This is quite gnarly... Maybe clarify a bit? 
				while ( depth && !lua_next( L, dd->index ) ) {
					( ( index = dd->index ) > top ) ? lua_pop( L, 1 ) : 0;
					--dd, --depth, fprintf( stderr, "\n%s}", &spaces[ 10 - depth ] );
				}
				( depth ) ? dd->count = 2, index = lua_gettop( L ) : 0;
			}
		}
		fprintf( stderr, "\n" );
		index--;
	}
}



//Takes all items in a table and merges them into 
//a single table (removes whatever was there)
int lua_merge( lua_State *L ) {
	const int top = lua_gettop( L );
	struct data { unsigned short index, tinsert, tpull; } data[64] = {0}, *dd = data;

	//Return early if no values
	if ( top < 2 ) {
		return 1;
	}

	//Push a blank table at the beginning
	lua_newtable( L ), lua_insert( L, 1 );

	//set some defaults
	dd->index = 1, dd->tinsert = 1;

	//Loop through all values on the stack
	for ( int kt, it, depth = 0, index = top + 1; index > 1; ) {
		if ( !depth ) {
			if ( ( it = lua_type( L, index ) ) == LUA_TTABLE ) {
				lua_pushnil( L );
				if ( lua_next( L, index ) )
					dd++, dd->tinsert = 1, dd->index = 1, dd->tpull = index, depth++;
				else {
					lua_pushnumber( L, dd->index );
					lua_newtable( L );
					lua_settable( L, dd->tinsert );
					dd->index ++;
				}
			}
			else {
				lua_pushnumber( L, dd->index );
				if ( ( it = lua_type( L, index ) ) == LUA_TSTRING )
					lua_pushstring( L, lua_tostring( L, index ) );
				else if ( it == LUA_TNUMBER ) {
					lua_pushinteger( L, lua_tointeger( L, index ) );
				}
				lua_settable( L, dd->tinsert );
				dd->index ++;
			}
		}
		else {
			//Add the key again to maintain lua_next behavior
			if ( ( kt = lua_type( L, -2 ) ) == LUA_TNUMBER )
				lua_pushnumber( L, lua_tonumber( L, -2 ) );
			else if ( kt == LUA_TSTRING )
				lua_pushstring( L, lua_tostring( L, -2 ) );
			else {
				//fprintf( stderr, "Invalid key type: %s\n", lua_typename( L, kt ) );
				fprintf( stderr, "ERROR: HANDLE OTHER KEY TYPES!\n" );
				return 0;
			}

			lua_insert( L, dd->tpull + 1 ), lua_settable( L, dd->tinsert );

			if ( !lua_next( L, dd->tpull ) ) {
				lua_pop( L, 1 ), dd--, depth--;
			}
		}
		( !depth ) ? index-- : 0;
	}
	return 1;
}



//A better load file
int lua_exec_file( lua_State *L, const char *f, char *err, int errlen ) {
	int len = 0, lerr = 0;
	struct stat check;

	if ( !f || !strlen( f ) ) {
		snprintf( err, errlen, "%s", "No filename supplied to load or execute." );
		return 0;
	}

	//Since this is supposed to accept a file, why not just check for existence?
	if ( stat( f, &check ) == -1 ) {
		snprintf( err, errlen, "File %s inaccessible: %s.", f, strerror(errno) );
		return 0;
	}

	if ( check.st_size == 0 ) {
		snprintf( err, errlen, "File %s is zero-length.  Nothing to execute.", f );
		return 0;
	}

	//Load the string, execute
	if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) {
		if ( lerr == LUA_ERRSYNTAX )
			len = snprintf( err, errlen, "Syntax error at %s: ", f );
		else if ( lerr == LUA_ERRMEM )
			len = snprintf( err, errlen, "Memory allocation error at %s: ", f );
	#ifdef LUA_53
		else if ( lerr == LUA_ERRGCMM )
			len = snprintf( err, errlen, "GC meta-method error at %s: ", f );
	#endif
		else if ( lerr == LUA_ERRFILE )
			len = snprintf( err, errlen, "File access error at %s: ", f );
		else {
			len = snprintf( err, errlen, "Unknown error occurred at %s: ", f );
		}
	
		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA LOAD ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}

	//Then execute
	if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		if ( lerr == LUA_ERRRUN ) 
			len = snprintf( err, errlen, "Runtime error when executing %s: ", f );
		else if ( lerr == LUA_ERRMEM ) 
			len = snprintf( err, errlen, "Memory allocation error at %s: ", f );
		else if ( lerr == LUA_ERRERR ) 
			len = snprintf( err, errlen, "Error while running message handler for %s: ", f );
	#ifdef LUA_53
		else if ( lerr == LUA_ERRGCMM ) {
			len = snprintf( err, errlen, "Error while running __gc metamethod at %s: ", f );
		}
	#endif

		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA EXEC ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}

	return 1;	
}


	
//Copy all records from a zTable to a Lua table at any point in the stack.
int ztable_to_lua ( lua_State *L, zTable *t ) {
	short ti[ 64 ] = { 0 }, *xi = ti; 

	//Push a table and increase Lua's "save table" index
	lua_newtable( L );
	*ti = 1;

	//Reset table's index
	lt_reset( t );

	//Loop through all values and copy
	for ( zKeyval *kv ; ( kv = lt_next( t ) ); ) {
		zhValue k = kv->key, v = kv->value;

		if ( k.type == ZTABLE_NON )
			return 1;	
		else if ( k.type == ZTABLE_INT ) //Arrays start at 1 in Lua, so increment by 1
			lua_pushnumber( L, k.v.vint + 1 );				
		else if ( k.type == ZTABLE_FLT )
			lua_pushnumber( L, k.v.vfloat );				
		else if ( k.type == ZTABLE_TXT )
			lua_pushstring( L, k.v.vchar );
		else if ( kv->key.type == ZTABLE_BLB)
			lua_pushlstring( L, (char *)k.v.vblob.blob, k.v.vblob.size );
		else if ( k.type == ZTABLE_TRM ) {
			lua_settable( L, *( --xi ) );
		}

		if ( v.type == ZTABLE_NUL )
			;
		else if ( v.type == ZTABLE_USR )
			lua_pushstring( L, "usrdata received" );
		else if ( v.type == ZTABLE_INT )
			lua_pushnumber( L, v.v.vint );				
		else if ( v.type == ZTABLE_FLT )
			lua_pushnumber( L, v.v.vfloat );				
		else if ( v.type == ZTABLE_TXT )
			lua_pushstring( L, v.v.vchar );
		else if ( v.type == ZTABLE_BLB )
			lua_pushlstring( L, (char *)v.v.vblob.blob, v.v.vblob.size );
		else if ( v.type == ZTABLE_TBL ) {
			lua_newtable( L );
			*( ++xi ) = lua_gettop( L );
		}
		else /* ZTABLE_TRM || ZTABLE_NON */ {
			return 0;
		}

		if ( v.type != ZTABLE_NON && v.type != ZTABLE_TBL && v.type != ZTABLE_NUL ) {
			lua_settable( L, *xi );
		}
	}
	return 1;
}



//Convert Lua tables to regular tables
int lua_to_ztable ( lua_State *L, int index, zTable *t ) {
	lua_pushnil( L );

	while ( lua_next( L, index ) != 0 ) {
		int kt = lua_type( L, -2 ); 
		int vt = lua_type( L, -1 );
		//fprintf( stderr, "%s -> %s\n", lua_typename( L, kt ), lua_typename( L, vt ) );

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if ( kt == LUA_TNUMBER )
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1 );
		else if ( kt == LUA_TSTRING )
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));
		else {
			//Invalid key type
			fprintf( stderr, "Got invalid key in table!" );
			return 0;
		}

		//Get value
		if ( vt == LUA_TNUMBER )
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING ) {
		#if 1
			lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
		#else
			const char *a = NULL;
			if ( ( a = lua_tostring( L, -1 ) ) )
				lt_addtextvalue( t, a );
			else {
				lt_addtextvalue( t, (char *)"" );
			}	
		#endif
		}
		else if ( vt == LUA_TTABLE ) {
			lt_descend( t );
			//tables with nothing should not recurse...
			lua_to_ztable( L, index + 2, t ); 
			lt_ascend( t );
		}
		else {
			fprintf( stderr, "Got invalid value in table!" );
			return 0;
		}

		//FPRINTF( "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING ) {
			lt_finalize( t );
		}
		lua_pop(L, 1);
	}
	lt_lock( t );
	return 1;
}



//Check if there is a reserved keyword being requested
static int is_reserved( const char *a ) {
	for ( int i = 0; i < sizeof( mvcmeta ) / sizeof( struct mvcmeta_t ); i ++ ) {
		if ( strcmp( a, mvcmeta[i].reserved ) == 0 ) return 1;
	}
	return 0;
}



//Make a route list
static int make_route_list ( zKeyval *kv, int i, void *p ) {
	struct route_t *tt = (struct route_t *)p;
	const int routes_wordlen = 6;
	if ( kv->key.type == ZTABLE_TXT && !is_reserved( kv->key.v.vchar ) ) {
		char key[ 2048 ] = { 0 };
		lt_get_full_key( tt->src, i, (unsigned char *)&key, sizeof( key ) );
		//replace all '.' with '/'
		for ( char *k = key; *k; k++ ) ( *k == '.' ) ? *k = '/' : 0;	
		struct iroute_t *ii = malloc( sizeof( struct iroute_t ) );
		ii->index = i, ii->route = zhttp_dupstr( &key[ routes_wordlen ] ), *ii->route = '/';
		add_item( &tt->iroute_tlist, ii, struct iroute_t *, &tt->iroute_tlen );
	}
	return 1;	
}



//Create a list of resources (an alternate version of this will inherit everything) 
static int make_mvc_list ( zKeyval *kv, int i, void *p ) {
	struct mvc_t *tt = (struct mvc_t *)p;
	char *key = NULL;
	int ctype = 0;

	if ( tt->depth == 1 ) {
		if ( kv->key.type == ZTABLE_TXT && is_reserved( key = kv->key.v.vchar ) ) {
			if ( !strcmp( key, "model" ) )
				tt->mset = &mvcmeta[ 0 ], tt->type = kv->value.type, tt->model = 1;
			else if ( !strcmp( key, "query" ) )
				tt->mset = &mvcmeta[ 1 ], tt->type = kv->value.type, tt->query = 1;
			else if ( !strcmp( key, "content-type" ) )
				tt->mset = &mvcmeta[ 3 ], tt->type = kv->value.type, ctype = 1;
			else if ( !strcmp( key, "view" ) || !strcmp( key, "views" ) ) {
				tt->mset = &mvcmeta[ 2 ], tt->type = kv->value.type, tt->view = 1;
			}
		}

		//write content type
		if ( tt->mset && ctype ) {
			memcpy( (char *)tt->ctype, kv->value.v.vchar, strlen( kv->value.v.vchar ) );
			return 1;
		}
	}

	if ( kv->value.type == ZTABLE_TBL ) {
		tt->depth++;
		return 1;
	}

	if ( tt->mset && kv->value.type == ZTABLE_TXT && memchr( "mvq", *tt->mset->reserved, 3 ) ) {
		struct imvc_t *imvc = malloc( sizeof( struct imvc_t ) );
		memset( imvc, 0, sizeof( struct imvc_t ) );
		snprintf( (char *)imvc->file, sizeof(imvc->file) - 1, "%s/%s.%s", 
			tt->mset->dir, kv->value.v.vchar, tt->mset->ext );
		snprintf( (char *)imvc->base, sizeof(imvc->base) - 1, "%s.%s", 
			kv->value.v.vchar, tt->mset->ext );
		snprintf( (char *)imvc->ext, sizeof(imvc->ext) - 1, "%s", 
			tt->mset->ext );
		add_item( &tt->imvc_tlist, imvc, struct imvc_t *, &tt->flen );
	}

	if ( kv->key.type == ZTABLE_TRM || tt->type == ZTABLE_TXT ) {
		tt->mset = NULL;	
	}
	if ( kv->key.type == ZTABLE_TRM ) {
		tt->depth--;
	}
	return 1;	
}



//Free MVC list
void free_mvc_list ( void ***list ) {
	for ( void **l = *list; l && *l; l++ ) {
		free( *l );
	} 
	free( *list ), *list = NULL;
}



//Free route list
static void free_route_list ( struct iroute_t **list ) {
	for ( struct iroute_t **l = list; *l; l++ ) {
		free( (*l)->route ), free( *l );
	}
	free( list );
}



//Load Lua configuration
int load_lua_config( struct luadata_t *l ) {
	char *db, *fqdn, cpath[ 2048 ] = { 0 };

	//If this fails, do something
	if ( !lt_init( l->zconfig, NULL, 1024 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't initalize configuration table." );
		return 0;
	}

	//Create a better path
	snprintf( cpath, sizeof(cpath) - 1, "%s/%s", l->root, confname );

	//Open the configuration file
	if ( !lua_exec_file( l->state, cpath, l->err, LD_ERRBUF_LEN ) ) {
		return 0;
	}

	//If it's anything but a Lua table, we're in trouble
	if ( !lua_istable( l->state, 1 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Configuration is not a Lua table." );
		return 0;
	}

	//Convert the Lua values to real values for extraction.
	if ( !lua_to_ztable( l->state, 1, l->zconfig ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Failed to convert Lua to zTable" );
		return 0;
	}

	//Set other keys here
	if ( ( db = lt_text( l->zconfig, "db" ) ) ) {
		memcpy( (void *)l->db, db, strlen( db ) ); 
	}

	if ( ( fqdn = lt_text( l->zconfig, "fqdn" ) ) ) {
		memcpy( (void *)l->fqdn, fqdn, strlen( fqdn ) ); 
	}

	//Destroy loaded table here...
	lua_pop( l->state, 1 );
	return 1;
}



//Checking for static paths is important, also need to check for disallowed paths
static int path_is_static ( struct luadata_t *l ) {
	int i, size, ulen = strlen( l->req->path );
	if ( ( i = lt_geti( l->zconfig, "static" ) ) == -1 ) {
		return 0;
	}
	
	//Start at the pulled index, count x times, and reset?
	for ( int len, ii = 1, size = lt_counta( l->zconfig, i ); ii < size; ii++ ) {
		zKeyval *kv = lt_retkv( l->zconfig, i + ii );
		//pbuf[ ii - 1 ] = kv->value.v.vchar;
		len = strlen( kv->value.v.vchar );

		//I think I can just calculate the current path
		if ( len <= ulen && memcmp( kv->value.v.vchar, l->req->path, len ) == 0 ) {
			return 1;
		}
	}

	return 0;
}



//Send a static file
static const int send_static ( struct HTTPBody *res, const char *dir, const char *uri ) {
	//Read_file and return that...
	struct stat sb;
	int dlen = 0;
	char err[ 2048 ] = { 0 }, spath[ 2048 ] = { 0 };
	unsigned char *data;
	const struct mime_t *mime;
	memset( spath, 0, sizeof( spath ) );
	snprintf( spath, sizeof( spath ) - 1, "%s/%s", dir, ++uri );
	fprintf( stderr, "dir: %s\n", spath );	

	//check if the path is there at all (read-file can't do this)
	if ( stat( spath, &sb ) == -1 ) {
		return http_error( res, 404, "static read failed: %s", err );
	}
	//read_file and send back
	if ( !( data = read_file( spath, &dlen, err, sizeof( err ) ) ) ) {
		return http_error( res, 500, "static read failed: %s", err );
	}

	//Gotta get the mimetype too	
	if ( !( mime = zmime_get_by_filename( spath ) ) ) {
		mime = zmime_get_default();
	}

	//Send the message out
	res->clen = dlen;
	http_set_status( res, 200 ); 
	http_set_ctype( res, mime->mimetype );
	http_set_content( res, data, dlen );
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_error( res, 500, err );
	}

	return 1;	
}


//...
void dump_records( struct HTTPRecord **r ) {
	for ( struct HTTPRecord **a = r; a && *a; a++ ) {
		fprintf( stderr, "%p: %s -> ", *a, (*a)->field ); 
		write( 2, (*a)->value, (*a)->size );
		write( 2, "\n", 1 );
	}
}


static char * getpath( char *rpath, char *apath, int plen ) {
	if ( plen < strlen( rpath ) ) return NULL;
	for ( char *p = apath, *path = rpath; *path && *path != '?'; ) *(p++) = *(path++);
	return apath;
}


#if 0
static int setroutes ( struct luadata_t *l ) {
	struct route_t p = { 0 };
	struct mvc_t pp = {0};
	zTable *croute = NULL;

	//Get the routes from the config file.
	if ( !( l->zroutes = lt_copy_by_key( l->zconfig, rkey ) ) ) {
		l->status = 500;
		snprintf( l->err, LD_ERRBUF_LEN, "%s", "Failed to copy routes from config." );
		return 0;
	}

	//Turn the routes into a list of strings, and search for a match
	//p.src = zroutes;
	lt_exec_complex( p.src = l->zroutes, 1, l->zroutes->count, &p, make_route_list );
	
	//Loop through each route and find the thing 
	l->pp.depth = 1;
	for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
		if ( ( l->rroute = route_resolve( l->apath, (*lroutes)->route ) ) ) {
			croute = lt_copy_by_index( l->zroutes, (*lroutes)->index );
			lt_exec_complex( croute, 1, croute->count, &pp, make_mvc_list );
			break;
		}
	}

	//Die when unavailable...
	if ( !croute ) {
		free_route_list( p.iroute_tlist );	
		l->status = 404;
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't find path at %s\n", l->apath );
		return 0;
	}

#if 0
	//If a route was found, break it up
	if ( !( l->zroute = getproutes( rpath, rroute ) ) ) {
		free_route_list( p.iroute_tlist );	
		l->status = 404;
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't initialize route map.\n" );
		return 0;
	}

	//Mark the active route from here and leave
	l->aroute = lt_text( l->zroute, "route.active" );	
#endif
	free_route_list( p.iroute_tlist );	
	return 1;
}
#endif



//Initialize routes in Lua
int init_lua_routes ( struct luadata_t *l ) {
	zWalker w = {0}, w2 = {0};
	const char *active, *path = l->apath + 1, *resolved = l->rroute + 1;
	char **routes = { NULL };
	int index = 0, rlen = 0, pos = 1;
	
	//Add a table.
	lua_newtable( l->state );

	//Handle root requests
	if ( !*path ) {
		lua_pushinteger( l->state, 1 ), lua_pushstring( l->state, def ); 
		lua_settable( l->state, pos );
		lua_pushstring( l->state, "active" ), lua_pushstring( l->state, def ); 
		lua_settable( l->state, pos );
		memcpy( (void *)l->aroute, def, strlen (def) );
		return 1;
	} 
	
	//Loop twice to set up the map
	for ( char stub[1024], id[1024]; strwalk( &w, path, "/" ); ) {
		//write the length of the block between '/'
		memset( stub, 0, sizeof(stub) );
		memcpy( stub, w.src, ( w.chr == '/' ) ? w.size - 1 : w.size );
		for ( ; strwalk( &w2, resolved, "/" ); ) {
			int size = ( w2.chr == '/' ) ? w2.size - 1 : w2.size;
			//if there is an equal, most likely it's an id
			if ( *w2.src != ':' )
				lua_pushinteger( l->state, ++index );	
			else {
				//Find the key/id name
				for ( char *p = (char *)w2.src, *b = id; *p && ( *p	!= '=' || *p != '/' ); ) {
					*(b++) = *(p++);
				}
				//Check that id is not active, because that's a built-in
				if ( strcmp( id, "active" ) == 0 ) {
					return 0;
				}
				
				//Add a numeric key first, then a text key
				lua_pushinteger( l->state, ++index );
				lua_pushstring( l->state, stub );
				lua_settable( l->state, pos );
				lua_pushstring( l->state, id );
			}
			break;
		}
		//copy the value (stub) to value in table
		lua_pushstring( l->state, stub );
		lua_settable( l->state, pos );
		active = &path[ w.pos ];
	}

	lua_pushstring( l->state, "active" );
	lua_pushstring( l->state, active );
	lua_settable( l->state, pos );
	memcpy( (void *)l->aroute, active, strlen (active) );
	return 1;
}



//Initialize HTTP in Lua
int init_lua_http ( struct luadata_t *l ) {
	//Loop through all things
	const char *str[] = { "headers", "url", "body" };
	struct HTTPRecord **ii[] = { l->req->headers, l->req->url, l->req->body };

	//Add one table for all structures
	lua_newtable( l->state );

	//Then add key for others
	for ( int pos=3, i = 0; i < 3; i++ ) {
		lua_pushstring( l->state, str[i] ), lua_newtable( l->state );
		for ( struct HTTPRecord **r = ii[i]; r && *r; r++ ) {
			lua_pushstring( l->state, (*r)->field );
			lua_pushlstring( l->state, ( char * )(*r)->value, (*r)->size );
			lua_settable( l->state, pos );
		}
		lua_settable( l->state, 1 );
	}

	//Set global name
	return 1;
}



//Both of these functions return zTables, and this is heavy on memory and does a useless context switch
//Doing it all in Lua makes more sense...
struct lua_readonly_t {
	const char *name;
	int (*exec)( struct luadata_t * );
	//int (*exec)( lua_State *, struct HTTPBody *, const char *, const char * );	
} lua_readonly[] = {
	{ "http", init_lua_http }
, { "route", init_lua_routes }
, { NULL }
};

#if 0
//consider this instead, but it will be extern...
struct lua_lib_t {
	...
} lua_libs[] = {
	{ "some_lib", libs }
, { NULL }
};
#endif


int free_ld ( struct luadata_t *l ) {
	lua_close( l->state );
	lt_free( l->zconfig );
	if ( l->zroutes ) {
		lt_free( l->zroutes );
		free( l->zroutes );
	}
	lt_free( l->zmodel );
	free_mvc_list( (void ***)&(l->pp.imvc_tlist) );
	return 1;
}


//The entry point for a Lua application
const int filter 
( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {

	//Define variables and error positions...
	zTable zc = {0}, zm = {0};
	struct luadata_t ld = {0};
	int clen = 0;
	unsigned char *content = NULL;

	//Initialize the data structure
	ld.req = req, ld.res = res; 
	ld.zconfig = &zc, ld.zmodel = &zm, ld.zroutes = NULL;
	memcpy( (void *)ld.root, conn->hconfig->dir, strlen( conn->hconfig->dir ) );

	//Then initialize the Lua state
	if ( !( ld.state = luaL_newstate() ) ) {
		return http_error( res, 500, "%s", "Failed to initialize Lua environment." );
	}

	if ( !load_lua_config( &ld ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s\n", ld.err );
	}

	//Need to delegate to static handler when request points to one of the static paths
	if ( path_is_static( &ld ) ) {
		free_ld( &ld );
		return send_static( res, ld.root, req->path );
	}

	//req->path needs to be modified to return just the path without the ?
	if ( !getpath( req->path, (char *)ld.apath, LD_LEN ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", "Failed to extract path." );
	}

	//Get the routes from the config file.
	if ( !( ld.zroutes = lt_copy_by_key( ld.zconfig, rkey ) ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", "Failed to copy routes from config." );
	}

	//Turn the routes into a list of strings, and search for a match
	struct route_t p = { .src = ld.zroutes };
	lt_exec_complex( ld.zroutes, 1, ld.zroutes->count, &p, make_route_list );
	
	//Loop through the routes
	ld.pp.depth = 1;
	int notfound = 1;
	for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
		if ( route_resolve( ld.apath, (*lroutes)->route ) ) {
			memcpy( (void *)ld.rroute, (*lroutes)->route, strlen( (*lroutes)->route ) );
			zTable * croute = lt_copy_by_index( ld.zroutes, (*lroutes)->index );
			lt_exec_complex( croute, 1, croute->count, &ld.pp, make_mvc_list );
			lt_free( croute );
			free( croute );
			notfound = 0;
			break;
		}
	}

	//Die when unavailable...
	if ( notfound ) {
		free_route_list( p.iroute_tlist );	
		free_ld( &ld );
		return http_error( res, 404, "Couldn't find path at %s\n", ld.apath );
	}

#if 0
	//If a route was found, break it up
	if ( !( zroute = getproutes( ld.apath, rroute ) ) ) {
		return http_error( res, 500, "Couldn't initialize route map.\n" );
	}
#endif

	//Destroy anything having to do with routes 
	free_route_list( p.iroute_tlist );	

	//Loop through the structure and add read-only structures to Lua, 
	//you could also add the libraries, but that is a different method
	for ( struct lua_readonly_t *t = lua_readonly; t->name; t++ ) {
		if ( !t->exec( &ld ) ) {
			free_ld( &ld );
			return http_error( req, ld.status, ld.err );
		}
		lua_setglobal( ld.state, t->name );
	}

	//Load standard libraries
	if ( !lua_loadlibs( ld.state, functions, 1 ) ) {
		free_ld( &ld );
		return http_error( res, 500, "Failed to initialize Lua standard libs." ); 
	}

	//Execute each model
	int ccount = 0, tcount = 0; 
	for ( struct imvc_t **m = ld.pp.imvc_tlist; m && *m; m++ ) {
		//Define
		char err[2048] = {0}, msymname[1024] = {0}, mpath[2048] = {0};

		//Check for a file
		if ( *(*m)->file == 'a' ) {
			//Open the file that will execute the model
			if ( *(*m)->base != '@' )
				snprintf( mpath, sizeof( mpath ), "%s/%s", ld.root, (*m)->file );
			else {
				snprintf( mpath, sizeof( mpath ), "%s/%s/%s.%s", ld.root, "app", ld.aroute, (*m)->ext );
			}

			//...
			//fprintf( stderr, "Executing model %s\n", mpath );
			if ( !lua_exec_file( ld.state, mpath, ld.err, sizeof( ld.err ) ) ) {
				free_ld( &ld );
				return http_error( res, 500, "%s", ld.err );
			}

			//Get name of model file in question 
			memcpy( msymname, &(*m)->file[4], strlen( (*m)->file ) - 8 );

			//Get a count of the values which came from the model
			tcount += ccount = lua_gettop( ld.state );

			//Merge previous models
			if ( tcount > 1 ) {
				lua_getglobal( ld.state, mkey );
				( lua_isnil( ld.state, -1 ) ) ? lua_pop( ld.state, 1 ) : 0;
				lua_merge( ld.state );	
				lua_setglobal( ld.state, mkey );
			} 
			else if ( ccount ) {
				lua_setglobal( ld.state, mkey );
			}
		}
	}

	//Push whatever model is there
	lua_getglobal( ld.state, mkey ); 
	if ( lua_isnil( ld.state, -1 ) )
		lua_pop( ld.state, 1 );
	else { 
		//TODO: Find an optimal hash size (also consider using a static hash table)	
		if ( !lt_init( ld.zmodel, NULL, 2048 ) ) {
			free_ld( &ld );
			return http_error( res, 500, "Could not allocate table for model." );
		}
		if ( !lua_to_ztable( ld.state, 1, ld.zmodel ) ) {
			free_ld( &ld );
			return http_error( res, 500, "Error in model conversion." );
		}
		lt_lock( ld.zmodel );
	}

	//Load all views
	for ( struct imvc_t **v = ld.pp.imvc_tlist; *v; v++ ) {
		if ( *(*v)->file == 'v' ) {
			int len = 0, renlen = 0;
			char vpath[ 2048 ] = {0};
			unsigned char *src, *render;
			zRender * rz = zrender_init();
			zrender_set_default_dialect( rz );
			zrender_set_fetchdata( rz, ld.zmodel );
			
			if ( *(*v)->base != '@' )
				snprintf( vpath, sizeof( vpath ), "%s/%s", ld.root, (*v)->file );
			else {
				snprintf( vpath, sizeof( vpath ), "%s/%s/%s.%s", ld.root, "views", ld.aroute, (*v)->ext );
			}

			//fprintf( stderr, "Loading view at: %s\n", vpath );
			if ( !( src = read_file( vpath, &len, ld.err, LD_ERRBUF_LEN )	) || !len ) {
				zrender_free( rz ), free( src ), free_ld( &ld );
				return http_error( res, 500, "Error opening view '%s': %s", vpath, ld.err );
			}

			if ( !( render = zrender_render( rz, src, strlen((char *)src), &renlen ) ) ) {
				zrender_free( rz ), free( src ), free_ld( &ld );
				return http_error( res, 500, "%s", "Renderer error." );
			}

			zhttp_append_to_uint8t( &content, &clen, render, renlen ); 
			zrender_free( rz ), free( render ), free( src );
		}
	}

	//Set needed info for the response structure
	res->clen = clen;
	http_set_status( res, 200 ); 
	http_set_ctype( res, "text/html" );
	http_set_content( res, content, clen ); 

	//Return the finished message if we got this far
	if ( !http_finalize_response( res, ld.err, LD_ERRBUF_LEN ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", ld.err );
	}

	//Destroy model & Lua
	free_ld( &ld ), free( content );
	return 1;
}



#ifdef RUN_MAIN
int main (int argc, char *argv[]) {
	struct HTTPBody req = {0}, res = {0};
	char err[ 2048 ] = { 0 };

	//Populate the request structure.  Normally, one will never populate this from scratch
	req.path = zhttp_dupstr( "/books" );
	req.ctype = zhttp_dupstr( "text/html" );
	req.host = zhttp_dupstr( "example.com" );
	req.method = zhttp_dupstr( "GET" );
	req.protocol = zhttp_dupstr( "HTTP/1.1" );

	//Assemble a message from here...
	if ( !http_finalize_request( &req, err, sizeof( err ) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1; 
	}

	//run the handler
	if ( !lua_handler( &req, &res ) ) {
		fprintf( stderr, "lmain: HTTP funct failed to execute\n" );
		write( 2, res.msg, res.mlen );
		http_free_request( &req );
		http_free_response( &res );
		return 1;
	}

	//Destroy res, req and anything else allocated
	http_free_request( &req );
	http_free_response( &res );

	//After we're done, look at the response
	return 0;
}
#endif
