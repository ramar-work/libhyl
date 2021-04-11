#include "lib.h"

//All of the other libraries come in here...
#include "echo.h"

struct lua_fset functions[] = {
	{ "echo", echo_set }
,	{ NULL }
};


