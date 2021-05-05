#include <stdio.h>
#include "../src/lua.h"

char *files[] = {
	"tests/lua/string.lua",
 	"tests/lua/table.lua",
#if 0
 	"tests/lua/deeptable.lua",
 	"tests/lua/multtable.lua",
#endif
  NULL
};

int main ( int argc, char *argv[] ) {
	
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };
	
	for ( char **f = files; *f; f++ ) {
		//run a few files and see what you get?
		fprintf( stderr, "executing code at %s\n", *f );
		if ( !lua_exec_file( L, *f, err, sizeof( err ) ) ) {
			fprintf( stderr, "Couldn't run lua file %s: %s\n", *f, err );
			continue;
		}

		//Dump it	
		lua_dumpstack( L, NULL );

		//Wipe it
		int a = lua_gettop( L );
		lua_pop( L, a );
	}	

	lua_close( L );
	return 0;
}
