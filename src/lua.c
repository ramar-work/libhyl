#include <hypno.h>
#include "lua.h"

#define FPRINTF( ... ) \
	fprintf( stderr, __VA_ARGS__ )

const char libname[] = "lua";

void lua_stackdump ( lua_State *L );

static const char mkey[] = "model";

static const char rkey[] = "routes";

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
	return 1;
}


//checking args is putting me in a weird zone... 
int lua_getarg( ) {
	return ZLUA_NO_ERROR;
}


#ifdef DEBUG_H
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
#endif



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
			len = snprintf( err, errlen, "zWalkerory allocation error at %s: ", f );
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
		int kt, vt;

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1 );
		else if ( kt  == LUA_TSTRING ) {
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));
		}
		//Get value
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
		else if ( vt == LUA_TTABLE ) {
			lt_descend( t );
			lua_to_ztable( L, index + 2, t ); 
			lt_ascend( t );
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



//The entry point for a Lua application
const int filter 
( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {
#if 0
	//Test responses quickly
	char er[2048] = {0};
	char msg[2048] = {0};
	snprintf( msg, sizeof(msg), "Yo, its Lua: %p", conn );
	http_set_status( res, 200 ); 
	http_set_ctype( res, "text/html" );
	http_copy_content( res, msg, strlen( msg ) ); 
	if ( !http_finalize_response( res, er, sizeof(er) ) ) {
		return http_error( res, 500, er );
	}
#endif

	//Define variables and error positions...
	zTable zc, zm = {0};
	zTable *zconfig = &zc, *zmodel = &zm, *zroutes = NULL, *croute = NULL;
	char err[ 128 ] = {0}, cpath[ 2048 ] = {0};
	const char *db, *fqdn, *title, *spath, *root;
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
fprintf( stderr, "full cpath is: %s\n", cpath );
return 0;

	//If this fails, do something
	if ( !lt_init( zconfig, NULL, 1024 ) ) {
		return http_error( res, 500, "%s", "lt_init out of memory." );
	}

	//Open the configuration file
	if ( !lua_exec_file( L, "config.lua", err, sizeof( err ) ) ) {
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
#endif

#if 0
	//EVERYTHING HERE REALLY SHOULD GO IN A PLACE OF ITS OWN...
#else
	//Get the rotues from the config file.
	if ( !( zroutes = lt_copy_by_key( zconfig, rkey ) ) ) {
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
		return http_error( res, 404, "Couldn't find path at %s\n", req->path );
	}

	//Destroy anything having to do with routes 
	lt_free( croute );
	free( croute );
	free_route_list( p.iroute_tlist );	
	lt_free( zroutes );
	free( zroutes );
	lt_free( zconfig );
#endif

#if 1
	//...
	lua_loadlibs( L, functions, 1 );

	//Open our extra libraries too (if requested via config file...)
	//load_lua_libs( L );

	//Execute each model
	for ( struct imvc_t **m = pp.imvc_tlist; *m; m++ ) {
		const char *f = (*m)->file;

		if ( *f == 'a' ) {
			//Define
			char err[ 2048 ] = { 0 }, msymname[ 1024 ] = { 0 };
			int ircount = 0, tcount = 0; 

			//If there are any values, they need to be inserted into Lua env
			if ( lt_countall( zmodel ) > 1 ) {
				//If we fail to insert, this is a model error and worthy of a 500...
				if ( !ztable_to_lua( L, zmodel ) ) {
					return http_error( res, 500, "Error converting original model!" );
				}
			
				//Make the old model global, & destroy the ref to the old model...	
				lua_setglobal( L, mkey );
				lt_free( zmodel );
			}

			//Open the file that will execute the model
			if ( !lua_exec_file( L, f, err, sizeof( err ) ) ) {
				return http_error( res, 500, "%s", err );
			}

			//Get name of model file in question 
			memcpy( msymname, &f[4], strlen( f ) - 8 );
			ircount = lua_gettop( L );
	
			//Check for model in Lua's global environment and convert that as well
			lua_getglobal( L, mkey );
			if ( lua_isnil( L, -1 ) ) {
				lua_pop( L, 1 );
			}

			//Get a count of the values which came from the model
			tcount = lua_gettop( L );
	
			//Initialize a model here	(TODO: modulo value can be much smaller)
			if ( !lt_init( zmodel, NULL, 2048 ) ) {
				return http_error( res, 500, "Failed to init model table." );
			}

			for ( int vi = 1; vi <= tcount; vi++ ) {
				if ( lua_isstring( L, vi ) ) 
					lt_addtextkey( zmodel, msymname ), lt_addtextvalue( zmodel, lua_tostring( L, vi ));
				else if ( lua_isinteger( L, vi ) || lua_isnumber( L, vi ) ) 
					lt_addtextkey( zmodel, msymname ), lt_addintvalue( zmodel, lua_tonumber( L, vi ));
				else if ( lua_islightuserdata( L, vi ) || lua_isuserdata( L, vi ) ) 
					lt_addtextkey( zmodel, msymname ), lt_addudvalue( zmodel, lua_touserdata(L, vi ));
			#if 0
				//TODO: We can evenaully handle threads and functions from here...
			#endif
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
					return http_error( res, 500, "%s", "Lua threads and functions as models not supported yet." );
				}

				if ( !lua_istable( L, vi ) ) {
					lt_finalize( zmodel );
				}
			}

			//Remove the values added
			lua_pop( L, tcount );
		}
	}
#endif

#if 1
	//Lock the model for hashing's sake
	lt_lock( zmodel );

	//Load all views
	for ( struct imvc_t **v = pp.imvc_tlist; *v; v++ ) {
		const char *f = (*v)->file;
		if ( *f == 'v' ) {
			int len = 0, renlen = 0;
			unsigned char *src, *render;
			zRender * rz = zrender_init();
			zrender_set_default_dialect( rz );
			zrender_set_fetchdata( rz, zmodel );

			if ( !( src = read_file( f, &len, err, sizeof( err ) )	) || !len ) {
				return http_error( res, 500, "%s", err );
			}

			if ( !( render = zrender_render( rz, src, strlen((char *)src), &renlen ) ) ) {
				return http_error( res, 500, "%s", "Renderer error." );
			}
			zhttp_append_to_uint8t( &content, &clen, render, renlen ); 
			zrender_free( rz );
			free( src );
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
	http_copy_content( res, content, clen ); 

	//Return the finished message if we got this far
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_error( res, 500, err );
	}
#endif

	//Destroy full model
	lt_free( zmodel );

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
