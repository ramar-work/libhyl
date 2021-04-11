-- config.lua
--
-- Configuration file for this Lua-based website.
return {
	static = {
		"/assets"
	, "/robots.txt"
	}
,	db = "yamama"
, title = "localhost"
, root = ""
,	routes = {
		["/"] = { model = "single", views = { "single" } }
	,	["books"] = { 
			model = { "books", "print-books", "edit-books", "add-books", "add-fields-books" }
		, views = { "intro", "books", "outro" } 
		}
	,	["book"] = { model = { "books", "single-book" }, views = { "intro", "b", "outro" } }
	,	["home"] = { 
			system = "jaundice",
			model = "nobody", 
			views = { "a", "b", "c" } 
		}
	}
}
