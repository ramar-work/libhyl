#include <stdio.h>
#include "../src/lua.h"

char *files[] = {
 	//"tests/lua/values.lua",
 	//"tests/lua/deeptable-alpha.lua",
 	//"tests/lua/deeptable-mixed.lua",
 	"tests/lua/multtable-simple.lua",
 	//"tests/lua/multtable.lua",
  NULL
};

int main ( int argc, char *argv[] ) {
	
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };
	
	for ( char **f = files; *f; f++ ) {
		//run a few files and see what you get?
		fprintf( stderr, "Executing code at %s\n", *f );
		if ( !lua_exec_file( L, *f, err, sizeof( err ) ) ) {
			fprintf( stderr, "Couldn't run lua file %s: %s\n", *f, err );
			continue;
		}

		//Dump it	
		lua_merge( L );

		//Wipe it
		lua_pop( L, lua_gettop( L ) );
		fprintf( stderr, "\n" );
	}	

	lua_close( L );
	return 0;
}
