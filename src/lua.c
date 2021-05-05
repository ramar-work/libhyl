#include <hypno.h>
#include "lua.h"

#define FPRINTF( ... ) \
	fprintf( stderr, __VA_ARGS__ )

const char libname[] = "lua";

void lua_stackdump ( lua_State *L );

static const char mkey[] = "model";

static const char rkey[] = "routes";

//TODO: This is an incredibly difficult way to make things read only
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
};


struct route_t { 
	int iroute_tlen;
	struct iroute_t { char *route; int index; } **iroute_tlist;
	zTable *src;
};


struct mvc_t {
	struct mvcmeta_t *mset;
	int flen, type;
	struct imvc_t {
		const char file[ 2048 ];
		//leave some space here...
		//const char *dir;
	} **imvc_tlist;
};


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
			FPRINTF( "Registering function %s.%s\n", set->namespace, f->name );
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
int lua_getarg( ) {
	return ZLUA_NO_ERROR;
}


void lua_dumpstack ( lua_State *L, int *sd ) {
	int top; 
	struct data { unsigned short count; unsigned short index; char end; const void *ptr; } data[64] = {0};
	struct data *dd = data;
	dd->index = 1;
	dd->count = 1;
	dd->end = 0;
	//const char spaces[] = "\t\t\t\t\t\t\t\t\t\t";
	const char spaces[] = "          ";

	//Return early if no values
	if ( ( top = lua_gettop( L ) ) == 0 ) {
		fprintf( stderr, "%s\n", "No values on stack." );
		return;
	}

	//Loop through all values on the stack
	for ( int it, depth=0, ix=0, index=lua_gettop( L ); index >= 1; ix++ ) {
		fprintf( stderr, "%s", &spaces[ 10 - depth ] );
		fprintf( stderr, "%d, %d -> %s ", index, ix, lua_typename( L, lua_type( L, index ) ) ); 
#if 0
fprintf( stderr, "\nptr: %p\n", dd->ptr );
fprintf( stderr, "count: %d\n", dd->count );
fprintf( stderr, "index: %d\n", dd->index );
#endif

		//TODO: Reject keys that aren't a certain type
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

			// two values, so index needs to go up once
			if ( count > 1 ) {
				index++, t = 1, dd->count -= 2; 
				fprintf( stderr, " -> " );
			}
			else if ( it == LUA_TTABLE ) {
				lua_pushnil( L );
				if ( lua_next( L, index ) != 0 ) {
					++dd, ++depth, dd->index = index, dd->count = 2, 
					dd->ptr = lua_topointer( L, index ), index = lua_gettop( L );
					fprintf( stderr, " ** TABLE AT %p has keys", dd->ptr );
				}
			}
			else if ( t ) {
				lua_pop( L, 1 );
				while ( depth && !lua_next( L, dd->index ) ) {
					fprintf( stderr, " ** NO MORE KEYS AT %p!", dd->ptr );
					dd->end = 1;
					--dd;
					--depth;
					index = dd->index;
					lua_pop( L, 1 );
					fprintf( stderr, "\n%s}", &spaces[ 10 - depth ] );
				}
				if ( depth ) {	
					fprintf( stderr, " ** MORE KEYS ARE ON TABLE AT %p!", dd->ptr );
					dd->count = 2, index = lua_gettop( L );
				}
			}
		}
		fprintf( stderr, "\n" );
		index--;
	}
}


static void lua_dumptable ( lua_State *L, int *pos, int *sd ) {
	lua_pushnil( L );
	//FPRINTF( "*pos = %d\n", *pos );

	while ( lua_next( L, *pos ) != 0 ) {
		//Fancy printing
		fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - *sd ] );
		//PRETTY_TABS( *sd );
		fprintf( stderr, "[%3d:%2d] => ", *pos, *sd );

		//Print both left and right side
		for ( int i = -2; i < 0; i++ ) {
			int t = lua_type( L, i );
			const char *type = lua_typename( L, t );
				//fprintf( stderr, "(%8s) %p", type, (void *)lua_tocfunction( L, i ) );

			if ( t == LUA_TSTRING )
				fprintf( stderr, "(%s) %s", type, lua_tostring( L, i ));
			else if ( t == LUA_TFUNCTION )
				fprintf( stderr, "(%s) %d", type, i /*(void *)lua_tocfunction( L, pos )*/ );
			else if ( t == LUA_TNUMBER )
				fprintf( stderr, "(%s) %lld", type, (long long)lua_tointeger( L, i ));
			else if ( t == LUA_TBOOLEAN)
				fprintf( stderr, "(%s) %s", type, lua_toboolean( L, i ) ? "true" : "false" );
			else if ( t == LUA_TTHREAD )
				fprintf( stderr, "(%s) %p", type, lua_tothread( L, i ) );
			else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
				fprintf( stderr, "(%s) %p", type, lua_touserdata( L, i ) );
			else if ( t == LUA_TNIL ||  t == LUA_TNONE )
				fprintf( stderr, "(%s) %p", type, lua_topointer( L, i ) );
			else if ( t == LUA_TTABLE ) {
			#if 1
				fprintf( stderr, "(%s) %p\n", type, lua_topointer( L, i ) );
				(*sd) ++, (*pos) += 2;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= 2;
			#else
				fprintf( stderr, "(%8s) %p {\n", type, lua_topointer( L, i ) );
				int diff = lua_gettop( L ) - *pos;

				(*sd) ++, (*pos) += diff;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= diff;
			#endif
				//PRETTY_TABS( *sd );
				fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - *sd ] );
				fprintf( stderr, "}" );
			}

			fprintf( stderr, "%s", ( i == -2 ) ? " -> " : "\n" );
			//PRETTY_TABS( *sd );
		}
		//getchar();
		//FPRINTF( "Popping key\n" );
		lua_pop( L, 1 );
	}
	return;
}


void lua_stackdump ( lua_State *L ) {
	//No top
	if ( lua_gettop( L ) == 0 ) {
		FPRINTF( "No values on stack...\n" );
		return;
	}

	//Loop again, but show the value of each key on the stack
	for ( int pos = 1; pos <= lua_gettop( L ); pos++ ) {
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		fprintf( stderr, "[%3d] => ", pos );

		if ( t == LUA_TSTRING )
			fprintf( stderr, "(%8s) %s", type, lua_tostring( L, pos ));
		else if ( t == LUA_TFUNCTION ) {
			fprintf( stderr, "(%8s) %d", type, pos /*(void *)lua_tocfunction( L, pos )*/ );
		}
		else if ( t == LUA_TNUMBER )
			fprintf( stderr, "(%8s) %lld", type, (long long)lua_tointeger( L, pos ));
		else if ( t == LUA_TBOOLEAN)
			fprintf( stderr, "(%8s) %s", type, lua_toboolean( L, pos ) ? "true" : "false" );
		else if ( t == LUA_TTHREAD )
			fprintf( stderr, "(%8s) %p", type, lua_tothread( L, pos ) );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
			fprintf( stderr, "(%8s) %p", type, lua_touserdata( L, pos ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
		else if ( t == LUA_TTABLE ) {
		#if 0
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
		#else
			fprintf( stderr, "(%8s) %p {\n", type, lua_topointer( L, pos ) );
			int sd = 1;
			lua_dumptable( L, &pos, &sd );
			fprintf( stderr, "}" );
		#endif
		}	
		fprintf( stderr, "\n" );
	}
	return;
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
	//if (( lerr = luaL_loadstring( L, (char *)ff )) != LUA_OK ) {
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
		if ( kt == LUA_TNUMBER ) {
//fprintf( stderr, "v: %lld\n", lua_tointeger( L, -2 ));
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1 );
		}
		else if ( kt == LUA_TSTRING ) {
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));
//fprintf( stderr, "k: %s\n", lua_tostring( L, -2 ));
		}
		else {
			//Invalid key type
			fprintf( stderr, "Got invalid key in table!" );
			return 0;
		}

		//Get value
		if ( vt == LUA_TNUMBER ) {
//fprintf( stderr, "v: %lld\n", lua_tointeger( L, -1 ));
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		}
		else if ( vt  == LUA_TSTRING ) {
		#if 1
//fprintf( stderr, "v: %s\n", lua_tostring( L, -1 ));
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
			exit( 0 );
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



static int is_reserved( const char *a ) {
	for ( int i = 0; i < sizeof( mvcmeta ) / sizeof( struct mvcmeta_t ); i ++ ) {
		if ( strcmp( a, mvcmeta[i].reserved ) == 0 ) return 1;
	}
	return 0;
}



static int make_route_list ( zKeyval *kv, int i, void *p ) {
	struct route_t *tt = (struct route_t *)p;
	const int routes_wordlen = 6;
	if ( kv->key.type == ZTABLE_TXT && !is_reserved( kv->key.v.vchar ) ) {
		char key[ 2048 ] = { 0 };
		lt_get_full_key( tt->src, i, (unsigned char *)&key, sizeof( key ) );
		struct iroute_t *ii = malloc( sizeof( struct iroute_t ) );
		ii->index = i, ii->route = zhttp_dupstr( &key[ routes_wordlen ] ), *ii->route = '/';
		add_item( &tt->iroute_tlist, ii, struct iroute_t *, &tt->iroute_tlen );
	}
	return 1;	
}



static void free_route_list ( struct iroute_t **list ) {
	for ( struct iroute_t **l = list; *l; l++ ) {
		free( (*l)->route );
		free( *l );
	}
	free( list );
}



static int make_mvc_list ( zKeyval *kv, int i, void *p ) {
	struct mvc_t *tt = (struct mvc_t *)p;
	char *key = NULL;

	if ( kv->key.type == ZTABLE_TXT && is_reserved( key = kv->key.v.vchar ) ) {
		if ( !strcmp( key, "model" ) )
			tt->mset = &mvcmeta[ 0 ], tt->type = kv->value.type;
		else if ( !strcmp( key, "query" ) )
			tt->mset = &mvcmeta[ 1 ], tt->type = kv->value.type;
		else if ( !strcmp( key, "view" ) || !strcmp( key, "views" ) ) {
			tt->mset = &mvcmeta[ 2 ], tt->type = kv->value.type;
		}
	}

	if ( tt->mset && kv->value.type == ZTABLE_TXT && memchr( "mvq", *tt->mset->reserved, 3 ) ) {
		struct imvc_t *imvc = malloc( sizeof( struct imvc_t ) );
		memset( imvc, 0, sizeof( struct imvc_t ) );
		snprintf( (char *)imvc->file, sizeof(imvc->file) - 1, "%s/%s.%s", 
			tt->mset->dir, kv->value.v.vchar, tt->mset->ext );
		add_item( &tt->imvc_tlist, imvc, struct imvc_t *, &tt->flen );
	}

	if ( kv->key.type == ZTABLE_TRM || tt->type == ZTABLE_TXT ) {
		tt->mset = NULL;	
	}
	return 1;	
}



void free_mvc_list ( void **list ) {
	for ( void **l = list; *l; l++ ) {
		free( *l );
	} 
	free( list );
}



#if 0
int load_lua_model( lua_State *L, const char *f, char **err, int errlen ) {
	//Open the configuration file
	if ( !lua_exec_file( L, "config.lua", err, errlen ) ) {
		return http_error( res, 500, "%s", err );
	}

	//If it's anything but a Lua table, we're in trouble
	if ( !lua_istable( L, 1 ) ) {
		return http_error( res, 500, "%s", err );
	}

	//Convert the Lua values to real values for extraction.
	if ( !lua_to_ztable( L, 1, zconfig ) ) {
		return http_error( res, 500, "%s", "Failed to convert Lua to zTable" );
	}

	//Destroy loaded table here...
	lua_pop( L, 1 );
}
#endif



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


//Checking for static paths is important, also need to check for disallowed paths
int path_is_static ( zTable *t, const char *uri ) {
	int i, size, ulen = strlen( uri );
	if ( ( i = lt_geti( t, "static" ) ) == -1 ) {
		return 0;
	}
	
	//Start at the pulled index, count x times, and reset?
	for ( int len, ii = 1, size = lt_counta( t, i ); ii < size; ii++ ) {
		zKeyval *kv = lt_retkv( t, i + ii );
		//pbuf[ ii - 1 ] = kv->value.v.vchar;
		len = strlen( kv->value.v.vchar );

		//I think I can just calculate the current path
		if ( len <= ulen && memcmp( kv->value.v.vchar, uri, len ) == 0 ) {
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


//Go through all of the different fields and pass things off
zTable * prepare_http_fields ( zTable *t, struct HTTPBody *req, char *err, int errlen ) {
	//Loop through all things
	const char *str[] = { "headers", "url", "body" };
	struct HTTPRecord **ii[] = { req->headers, req->url, req->body };

	//Initialize said table
	if ( !lt_init( t, NULL, 512 ) ) {
		return NULL;
	} 

	for ( int i = 0; i < 3; i++ ) {
		lt_addtextkey( t, str[ i ] );
		lt_descend( t );
		for ( struct HTTPRecord **r = ii[i]; r && *r; r++ ) {
		#if 0
			fprintf( stderr, "\t%p: %s -> ", *r, (*r)->field ); 
			write( 2, (*r)->value, (*r)->size );
			write( 2, "\n", 1 );
		#endif
			lt_addtextkey( t, (*r)->field );
			lt_addblobvalue( t, (*r)->value, (*r)->size );
			lt_finalize( t );
		}
		lt_ascend( t );
	}

	//Finally add a function (that should translate to Lua)
	lt_addtextkey( t, "__newindex" );
	lt_addtextvalue( t, "function (t,k,v) error( 'attempt to update a read-only table.', 2 ) end" );
	//If not, you'll have to use a function, but how would that translate?
	//would it be a C function or not?

	//Return a table
	return t;
}



//The entry point for a Lua application
const int filter 
( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {

	//Define variables and error positions...
	zTable zc, zm = {0}, zh = {0};
	char err[ 128 ] = {0}, cpath[ 2048 ] = {0}; 
	const char *db, *fqdn, *title, *root;
	zTable *zconfig = &zc, *zmodel = &zm, *zhttp = &zh; 
	zTable *zroutes = NULL, *croute = NULL;
	lua_State *L = NULL;
	int clen = 0;
	unsigned char *content = NULL;

	//Initialize
	if ( !( L = luaL_newstate() ) ) {
		return http_error( res, 500, "%s", "Failed to initialize Lua environment." );
	}

#if 0
	if ( !load_lua_model( L, "config.lua", zconfig, err, sizeof( err ) ) ) {
		return http_error( res, 500, "%s\n", err );
	}
#else
	//Get the actual path of the config file...
	snprintf( cpath, sizeof( cpath ) - 1, "%s/%s", conn->hconfig->dir, "config.lua" );

	//If this fails, do something
	if ( !lt_init( zconfig, NULL, 1024 ) ) {
		return http_error( res, 500, "%s", "lt_init out of memory." );
	}

	//Open the configuration file
	if ( !lua_exec_file( L, cpath, err, sizeof( err ) ) ) {
		lt_free( zconfig );
		lua_close( L );
		return http_error( res, 500, "%s", err );
	}

	//If it's anything but a Lua table, we're in trouble
	if ( !lua_istable( L, 1 ) ) {
		lt_free( zconfig );
		lua_close( L );
		return http_error( res, 500, "%s", err );
	}

	//Convert the Lua values to real values for extraction.
	if ( !lua_to_ztable( L, 1, zconfig ) ) {
		lt_free( zconfig );
		lua_close( L );
		return http_error( res, 500, "%s", "Failed to convert Lua to zTable" );
	}

	//Destroy loaded table here...
	lua_pop( L, 1 );
#endif

	//Need to delegate to static handler when request points to one of the static paths
	if ( path_is_static( zconfig, req->path ) ) {
		//Usually would not free this early, but we will this time.
		lt_free( zconfig );
		lua_close( L );
		return send_static( res, conn->hconfig->dir, req->path );
	}
	
	//Get the rotues from the config file.
	if ( !( zroutes = lt_copy_by_key( zconfig, rkey ) ) ) {
		lt_free( zconfig );
		lua_close( L );
		return http_error( res, 500, "%s", "Failed to copy routes from config." );
	}

	//Turn the routes into a list of strings, and search for a match
	struct route_t p = { .src = zroutes };
	lt_exec_complex( zroutes, 1, zroutes->count, &p, make_route_list );
	
	//Loop through the routes
	struct mvc_t pp = {0};
	for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
		if ( route_resolve( req->path, (*lroutes)->route ) ) {
			croute = lt_copy_by_index( zroutes, (*lroutes)->index );
			lt_exec_complex( croute, 1, croute->count, &pp, make_mvc_list );
			break;
		}
	}

	//Die when unavailable...
	if ( !croute ) {
		free_route_list( p.iroute_tlist );	
		lt_free( zroutes );
		lt_free( zconfig );
		lua_close( L );
		return http_error( res, 404, "Couldn't find path at %s\n", req->path );
	}

	//Destroy anything having to do with routes 
	lt_free( croute );
	free( croute );
	free_route_list( p.iroute_tlist );	
	lt_free( zroutes );
	free( zroutes );
	lt_free( zconfig );

	//Parse http here
	if ( !prepare_http_fields( zhttp, req, err, sizeof( err ) ) ) {
		return http_error( res, 500, "Failed to prepare HTTP for consumption." ); 
	}

	//Load standard libraries
	if ( !lua_loadlibs( L, functions, 1 ) ) {
		return http_error( res, 500, "Failed to initialize Lua standard libs." ); 
	}

	//Make read only (probably shouldn't be fatal)
	if ( !make_read_only( L, "http" ) ) {
		return http_error( res, 500, "Failed to make libs read-only." ); 
	}
	
	//Execute each model
	int ccount = 0, tcount = 0; 
	for ( struct imvc_t **m = pp.imvc_tlist; *m; m++ ) {
		if ( *(*m)->file != 'a' ) {
			continue;
		}
		#if 0	
		ircount = lua_gettop( L );

		//Check for model in Lua's global environment and convert that as well
		lua_getglobal( L, mkey );
		if ( lua_isnil( L, -1 ) ) {
			lua_pop( L, 1 );
		}
		#endif
		//Define
		char err[ 2048 ] = { 0 }, msymname[ 1024 ] = { 0 }, mpath[ 2048 ] = {0};
#if 0
		//If there are any values, they need to be inserted into Lua env
		if ( lt_countall( zmodel ) > 1 ) {
fprintf( stderr, "==== REINSERTING MODEL\n" );

			//If we fail to insert, this is a model error and worthy of a 500...
			if ( !ztable_to_lua( L, zmodel ) ) {
				return http_error( res, 500, "Error converting original model!" );
			}
		
			//Make the old model global, & destroy the ref to the old model...	
			lua_setglobal( L, mkey );
			lt_free( zmodel );
		}
#endif

fprintf( stderr, "==== BEGINNING OF INVOCATION\n" );
lua_dumpstack( L, NULL );

		//Open the file that will execute the model
		snprintf( mpath, sizeof( mpath ), "%s/%s", conn->hconfig->dir, (*m)->file );
		fprintf( stderr, "Executing model %s\n", mpath );

		//...
		if ( !lua_exec_file( L, mpath, err, sizeof( err ) ) ) {
			return http_error( res, 500, "%s", err );
		}

		//Debug dump of whatever model is there
fprintf( stderr, "==== NEW MODEL\n" );
		lua_dumpstack( L, NULL );
		//lua_stackdump( L );

		//Get name of model file in question 
		memcpy( msymname, &(*m)->file[4], strlen( (*m)->file ) - 8 );

		//Get a count of the values which came from the model
		tcount += ccount = lua_gettop( L );
		fprintf( stderr, "executed model contains %d values\n", ccount );

	#if 1
	#if 0
		if ( ccount ) {
			lua_getglobal( L, mkey );
			//if ( lua_isnil( L, -1 ) ) { lua_pop( L, 1 ); }
		}
	#endif
fprintf( stderr, "==== USE %s MODEL\n", tcount > 1 ? "PREVIOUS" : "ONE" );
		if ( tcount > 1 ) {
			lua_newtable( L );
			lua_getglobal( L, mkey );
			//if ( lua_isnil( L, -1 ) ) { lua_pop( L, 1 ); }
			lua_dumpstack( L, NULL );
		#if 0
			lua_settable( L, 1 );
			lua_stackdump( L );
			getchar();
			lua_setglobal( L, mkey );
			lua_stackdump( L );
		#endif
getchar();
		} 
		else if ( ccount ) {
			lua_setglobal( L, mkey );
		}
		//if ( tcount > 1 ) then we'll need to merge the previous model along with whatever was brought back
fprintf( stderr, "==== STACK AFTER GLOBAL SET\n" );
		lua_dumpstack( L, NULL );
getchar();
	#else	
		//Initialize a model here	(TODO: modulo value can be much smaller)
		if ( !lt_init( zmodel, NULL, 2048 ) ) {
			return http_error( res, 500, "Failed to init model table." );
		}

		for ( int vi = tcount; vi > 0; vi-- ) { /*for ( int vi = 1; vi <= tcount ; vi++ ) */ 
			if ( lua_isstring( L, vi ) ) 
				lt_addtextkey( zmodel, msymname ), lt_addtextvalue( zmodel, lua_tostring( L, vi ));
			else if ( lua_isinteger( L, vi ) || lua_isnumber( L, vi ) ) 
				lt_addtextkey( zmodel, msymname ), lt_addintvalue( zmodel, lua_tonumber( L, vi ));
			else if ( lua_islightuserdata( L, vi ) || lua_isuserdata( L, vi ) ) 
				lt_addtextkey( zmodel, msymname ), lt_addudvalue( zmodel, lua_touserdata(L, vi ));
			else if ( lua_istable( L, vi ) ) {
				//This function does not add any key
				if ( !lua_to_ztable( L, vi, zmodel ) ) {
					//TODO: Free all things
					lt_free( zmodel );
					lua_close( L );
					return http_error( res, 500, "%s", "Failed to convert Lua to zTable" );
				}
			}
			else {
				//TODO: Free all things
				lt_free( zmodel );
				lua_close( L );
				return http_error( res, 500, "%s", "Threads & functions as models not supported yet." );
			}

			if ( !lua_istable( L, vi ) ) {
				lt_finalize( zmodel );
			}
		}

		//Remove the values added
		lua_pop( L, tcount );
fprintf( stderr, "==== MODEL in C\n" );
lt_dump( zmodel );
getchar();
	#endif
	}

fprintf( stderr, "==== FINAL MODEL\n" );
		lua_getglobal( L, mkey );
		if ( lua_isnil( L, -1 ) ) {
			lua_pop( L, 1 );
		}
lua_dumpstack( L, NULL );
return http_error( res, 200, "No error, stop here." );

	//Lock the model for hashing's sake
	lt_lock( zmodel );

	//Load all views
	for ( struct imvc_t **v = pp.imvc_tlist; *v; v++ ) {
		const char *f = (*v)->file;
		if ( *f == 'v' ) {
			int len = 0, renlen = 0;
			char vpath[ 2048 ] = {0};
			unsigned char *src, *render;
			zRender * rz = zrender_init();
			zrender_set_default_dialect( rz );
			zrender_set_fetchdata( rz, zmodel );

			snprintf( vpath, sizeof( vpath ), "%s/%s", conn->hconfig->dir, f );
fprintf( stderr, "vp: %s\n", vpath );
#if 1
			if ( !( src = read_file( vpath, &len, err, sizeof( err ) )	) || !len ) {
				return http_error( res, 500, "rf: %s", err );
			}

			if ( !( render = zrender_render( rz, src, strlen((char *)src), &renlen ) ) ) {
				return http_error( res, 500, "%s", "Renderer error." );
			}
			zhttp_append_to_uint8t( &content, &clen, render, renlen ); 
			zrender_free( rz );
			free( src );
#endif
		}
	}

	//Destroy mvc list
	free_mvc_list( (void **)pp.imvc_tlist );

	//Model can contain other things like status, content-type, etc
	//Certain keys will cause other things to happen
	//... 

	//Set needed info for the response structure
	res->clen = clen;
	http_set_status( res, 200 ); 
	http_set_ctype( res, "text/html" );
	http_set_content( res, content, clen ); 

	//Return the finished message if we got this far
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_error( res, 500, err );
	}

	//Destroy HTTP
	lt_free( zhttp );

	//Destroy full model
	lt_free( zmodel );
	//free( content );

	//Destroy Lua
	lua_close( L );
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
