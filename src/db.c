#include "db.h"

//Accepts three arguments { db, [query,file], bind_args }
int db_exec ( lua_State *L ) {
	//Check for arguments...
	luaL_checktype( L, 1, LUA_TTABLE );

	//Within this table need to exist a few keys
#if 1
	zTable *results, *t = malloc( sizeof( zTable ) );
	lt_init( t, NULL, 32 ); 
#else
	//Doing this statically will save time, energy and memory 
#endif

	//Convert to C table for slightly easier key extraction	
	if ( !lua_to_ztable( L, 1, t ) ) {
		//free the table
		//put message on stack for Lua
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//Check for needed args, starting with dbname (or conn str, whatever)
	const char *conn  = NULL, *query = NULL;
	int pos;
	if ( ( pos = lt_geti( t, "db" ) ) > -1 ) 
		conn = lt_text_at( t, pos );
	else {
		return luaL_error( L, "Connection unspecified." );
	}
		

	//Then get the query
	if ( ( pos = lt_geti( t, "string" ) ) > -1 ) 
		query = lt_text_at( t, pos );
	else if ( ( pos = lt_geti( t, "file" ) ) > -1 ) {
		const char *file = lt_text_at( t, pos );
		int a = 0;
		//query = read_file( file );
		return luaL_error( L, "SQL by file not yet complete." );
	}
	else {
		return luaL_error( L, "Neither 'string' nor 'file' were present in call to %s", "db_exec" );
	}

	//Finally, handle bind args if there are any
	//...

	//Then we have to initialize the underlying lib
	zdb_t zdb = { 0 }; 
	zdbv_t zdbv = { 0 }; //bind args go here...

	//Open or connect to database, run the query and close the connection 
	if ( !zdb_open( &zdb, conn, ZDB_SQLITE ) ) {
		return luaL_error( L, "Connection error." );
	}

	if ( !zdb_exec( &zdb, query, NULL ) ) {
		return luaL_error( L, "SQL execution error: %s", zdb.err );
	}

	if ( !( results = zdb_to_ztable( &zdb, "results" ) ) ) {
		fprintf( stdout, "failed....\n" ); fflush( stdout );
		return luaL_error( L, "conversion error: %s", zdb.err );
	}

	//Everything should be done by this point, so we have to prepare the results
	lua_pop( L, 1 );	

	//It's infinitely easier to write this first...
	if ( !ztable_to_lua( L, results ) ) {
		zdb_close( &zdb );
		lt_free( results );
		return luaL_error( L, "ztable to Lua failed...\n" );
	}

	//Add a status code
	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	//Add a status code
	lua_pushstring( L, "count" );
	lua_pushnumber( L, zdb.rows );
	lua_settable( L, 1 );

	lua_stackdump( L );
	zdb_close( &zdb );	
	lt_free( results );
	return 1;
}

struct luaL_Reg db_set[] = {
 	{ "exec", db_exec }
,	{ NULL }
};


