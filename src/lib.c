#include "lib.h"

//All of the other libraries come in here...
#include "echo.h"
#include "db.h"

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "db", db_set }
,	{ NULL }
};


