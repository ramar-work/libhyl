return {
	db = "none",
	title = "stresstest.com",
	fqdn = "stresstest.com",
	static = { "/assets", "/ROBOTS.TXT", "/favicon.ico" },
	routes = {
		default = { model="hello",view="hello" },
		stub = {
			[":id=number"] = { model="recipe",view="recipe" },
		},
	}	
}
