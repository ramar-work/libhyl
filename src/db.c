#include "db.h"

int db_exec ( lua_State *L, const char *name, const char *query ) {
	//Check for arguments...
	const int arg = 1;
	luaL_checktype( L, arg, LUA_TTABLE );

	//Within this table need to exist a few keys
	//"bindargs" - for bind arguments, 
	//"query", "file" - for SQL code evaluation
	
	return 0;
}


