-------------------------------------------------
-- config.lua
-- ==========
--
-- @summary
-- Server config file for example htdocs of libhypno. Running the target 
-- `make examples` will start a server with these four sites loaded.  
--
-- NOTE: The names below will need to be listed in your /etc/hosts file to
-- resolve on your system.
--
-- @usage
-- Sites can be added by simply creating the directory you want, and 
-- adding it to the list below.
--
-- @changelog
-- nothing yet...
-- 
-------------------------------------------------
return {
	wwwroot = "examples",
	hosts = {
		-- Default host in case no domain is specified
		["localhost"] = { 
			root_default = "/index.html",
			dir = "example.com",
			filter = "lua"
		}
	}
}
