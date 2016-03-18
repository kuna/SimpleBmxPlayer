-- DON'T ADD THIS FILE in general situation,
-- This script will overwrite some important basic functions.
-- Just for lua grammar testing.

function LoadImage( path, ... )
	trace( "LoadImage: " .. path )
end

function LoadFont( path, ... )
	trace( "LoadFont: " .. path )
end

function LoadTFont( path, ... )
	trace( "LoadTFont: " .. path )
end

function SetSwitch( name )
	-- do nothing
end

function CND( conditions )
	-- always return true to go through all clause
	return true
end