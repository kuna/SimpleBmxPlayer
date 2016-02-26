-- commonly used script in game
-- these script is executed in global scope
-- don't edit this script unless you know what you're doing.
-- @lazykuna



--
-- CLAIM:
-- `Def, MergeTables` declaration copied from stepmania declaration.
-- (Scripts/02_ActorDef.lua)
-- All rights reserved.
--
local function MergeTables( left, right )
	local ret = { }
	for key, val in pairs(left) do
		ret[key] = val
	end

	for key, val in pairs(right) do
		if ret[key] then
			if type(val) == "function" and
			   type(ret[key]) == "function" then
				local f1 = ret[key]
				local f2 = val
				val = function(...)
					f1(...)
					return f2(...)
				end
			else
				warn( string.format( "%s\n\nOverriding \"%s\": %s with %s",
					 debug.traceback(), key, type(ret[key]), type(val)) )
			end
		end

		ret[key] = val
	end
	setmetatable( ret, getmetatable(left) )
	return ret
end

DefMetatable = {
	__concat = function(left, right)
		return MergeTables( left, right )
	end
}

-- This is used as follows:
--
--   t = Def.Class { table }
Def = {}
setmetatable( Def, {
	__index = function(self, Class)
		-- t is an actor definition table.  name is the type
		-- given to Def.  Fill in standard fields.
		return function(t)
			if not Util.IsRegisteredClass(Class) then
				error( Class .. " is not a registered actor class", 2 )
			end

			t.Class = Class

			local level = 2
			if t._Level then
				level = t._Level + 1
			end
			local info = debug.getinfo(level,"Sl");

			-- Source file of caller:
			local Source = info.source
			t._Source = Source

			t._Dir = Util.GetDir( Source )

			-- Line number of caller:
			t._Line = info.currentline

			setmetatable( t, DefMetatable )
			return t
		end
	end,
})



-- Load an actor template.
function LoadActorFunc( path )
	if path == "" then
		error( "Passing in a blank filename is a great way to eat up RAM. Good thing we warn you about this." )
	end

	local ResolvedPath = Util.ResolvePath( path )
	if not ResolvedPath then
		error( path .. ": not found" )
	end
	path = ResolvedPath

	local Type = Util.GetFileType( ResolvedPath )
	trace( "Loading " .. path .. ", type " .. tostring(Type) )

	if Type == "Lua" then
		-- Load the file.
		local chunk, errmsg = loadfile( path )
		if not chunk then error(errmsg) end
		return chunk
	end

	if Type == "Image" or Type == "Movie" then
		return function()
			return Def.Sprite {
				Texture = path
			}
		end
	end

	error( path .. ": unknown file type (" .. tostring(Type) .. ")" )
end

-- Load and create an actor template.
-- ... : attributes to be set
function LoadObject( path, ... )
	local t = LoadActorFunc( path )
	assert(t)
	return t(...)
end

-- DST - make DST command to function. (convinence function)
-- usage: DST.time(0).x(0).time(100).x(100)
--        => { {'time', 0}, {'x', 0}, {'time', 100}, {'x', 0} }

DST = {}
setmetatable( DST, {
	__index = function ( self, attr )
		-- returns a simple table including self
		return function (...)
			t = {...}
			table.insert(t, 1, attr)
			table.insert(self, t)
			return self
		end
	end
})


-- end of common.lua