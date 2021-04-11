//tests.c - Should simply check input and output...
#include "src/lua.h"

#define PP "lua-filter-test"

#define HELP "\
-a, --all          Run all tests. \n\
-t, --test <arg>   Run one (or eventually multiple) tests. \n\
-v, --verbose      Be verbose. \n\
-h, --help         Show help. \n\
"

struct arg_t {
	char *test;
	int all;
	int verbose;
}; 


struct test_t {
	//A string to test
	const char *string;
	//A code (or number of args) to expect back
	int expected;
	//Using real error codes from argument handler could make a lot of sense
	int error;
} tests[] = {
	{ "echo.number( 0 )", 1, ZLUA_NO_ERROR },
	{ "echo.number( 'abc' )", 0, ZLUA_INCORRECT_ARGS },
	{ "echo.number( )", 0, ZLUA_MISSING_ARGS },
	{ "echo.string( 0 )", 1, ZLUA_NO_ERROR },
	{ "echo.string( 'abc' )", 0, ZLUA_INCORRECT_ARGS },
	{ "echo.string( )", 0, ZLUA_MISSING_ARGS },
	{ "echo.table( { a='b', c='d' } )", 1, ZLUA_NO_ERROR },
	{ "echo.table( 'abc' )", 0, ZLUA_INCORRECT_ARGS },
	{ "echo.table( )", 0, ZLUA_MISSING_ARGS },
	{ NULL }
};


int main ( int argc, char *argv[] ) {
	//Define
	lua_State *L = NULL;
	struct arg_t arg = { 0 };

	//Die on not enough args
	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	//Need to load the thing and some how run this...
	while ( *argv ) {
		if ( !strcmp( *argv, "-a" ) || !strcmp( *argv, "--all" ) )
			arg.all = 1;
		else if ( !strcmp( *argv, "-t" ) || !strcmp( *argv, "--test" ) )
			arg.test = *( ++argv );
		else if ( !strcmp( *argv, "-v" ) || !strcmp( *argv, "--verbose" ) )
			arg.verbose = 1;
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		argv++;
	}

	//Initialize the Lua state
	if ( !( L = luaL_newstate() ) ) {
		fprintf( stderr, PP ": Failed to initialize new Lua state.\n" );
		return 0;
	}

	if ( !lua_loadlibs( L, functions, 1 ) ) {
		fprintf( stderr, PP ": Failed to load external Lua libraries.\n" );
		return 0;
	}

	//Go through the tests and run them
	for ( struct test_t *t = tests; t->string; t++ ) {
		fprintf( stderr, "running test '%s' ", t->string );
		int status = luaL_dostring( L, t->string );
		fprintf( stderr, "status of block = %d\n", status );

		//In case of error, see the value and pop everything
		if ( lua_gettop( L ) > 0 ) {
			const char *err = lua_tostring( L, 1 );
			fprintf( stderr, "'%s'\n", err );
			lua_pop( L, lua_gettop( L ) );
		}
	}

	//Close stuff
	lua_close( L );  
	return 0;
}
