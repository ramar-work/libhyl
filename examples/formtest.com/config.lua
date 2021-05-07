-- config.lua
--
-- Configuration file for this Lua-based website.
return {
	static = {
		"/assets"
	, "/ROBOTS.TXT"
	}
,	db = "gb"
, title = "localhost"
, root = ""
,	routes = {
		["/"] = { model = "single", views = { "single" } }

	,	["{aa,bb,cc}"] = { 
			model = { "z", "@", "x" }
		, views = { "changed" }
		}
	
	, ["complex"] = {
			model = { "z", "y", "x" }
		, views = { "this" }
		, ["{high,low,mid}"] = {
				model = { "z", "y", "x" }
			, views = { "boom" }
			, [ "{xx,yy,zz}" ] = {
					model = { "z", "y", "x" }
				, views = { "@" }
				}
			}
		}

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
