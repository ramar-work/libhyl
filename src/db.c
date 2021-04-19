#include "db.h"

int db_exec ( lua_State *L ) {
	//Check for arguments...
	const int arg = 1;
	luaL_checktype( L, arg, LUA_TTABLE );

	//Within this table need to exist a few keys
	//"bindargs" - for bind arguments, 
	//"query", "file" - for SQL code evaluation
lua_stackdump( L );
#if 1
	zTable *t = malloc( sizeof( zTable ) );
	lt_init( t, NULL, 128 ); 
#else
	//Doing this statically will save some energy
	zTable t, *tt;
	lt_init( &t, zKeyval * 128, 128 );
	//Last step is differentiating between copies and references
	//dchar = dynamically allocated char
	//schar = statically allocated char
	//Or
	//lt_copytextkey / lt_settextkey
#endif

	//Convert to C table for slightly easier key extraction	
	if ( !lua_to_ztable( L, 1, t ) ) {
		//free the table
		//put message on stack for Lua
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//Check for needed args, starting with dbname (or conn str, whatever)
	const char *query = NULL;
	const char *conn  = NULL;
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
fprintf( stdout, "%s\n", query ); fflush( stdout );

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

	zTable *r;
	if ( !( r = zdb_to_ztable( &zdb, "results" ) ) ) {
		fprintf( stdout, "failed....\n" ); fflush( stdout );
		return luaL_error( L, "conversion error: %s", zdb.err );
	}

	lt_dump( r );	

	zdb_close( &zdb );	
	return 0;
}

struct luaL_Reg db_set[] = {
 	{ "exec", db_exec }
,	{ NULL }
};


