-- diagnostic.lua

-- print route pairs off
for n,t in pairs { route = route, http = http } do
	for k,v in pairs( t ) do 
		print( n, t, k, v )
		if type( v ) == "table" then 
			for kk,vv in pairs(v) do
				print( kk, vv )
			end
		end
	end
end


