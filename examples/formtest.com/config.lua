-- config.lua
--
-- Configuration file for this Lua-based website.
return {
	static = {
		"/assets"
	, "/robots.txt"
	}
,	db = "gb"
, title = "localhost"
, root = ""
,	routes = {
		["/"] = { model = "single", views = { "single" } }

		-- Let's just see if a model will sustain changes
	,	["series"] = { 
			model = { "z", "y", "x" }
		, views = { "changed" }
		}

		-- Do models overwrite each other? 
	,	["overwrite"] = { 
			model = { "u", "v", "w" }
		, views = { "changed" }
		}

	,	["form"] = { 
			model = { "form" }
		, views = { "intro", "multi", "outro" } 
		}

	}
}