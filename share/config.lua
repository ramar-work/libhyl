-------------------------------------------------
-- config.lua
--
-- Configuration file for this Lua-based website.
-------------------------------------------------
return {
	static = {
		"/assets"
	, "/robots.txt"
	, "/favicon.ico"
	}
,	db = "yamama"
, title = "@@DOMAIN@@"
, root = ""
,	routes = {
		["/"] = { model = "single", views = { "single" } }
	}
}
