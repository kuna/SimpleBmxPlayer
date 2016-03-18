-- cnd test
t = 9
if CND("abcd") then
	t = 10
end

-- table test
local tt = Def.test {
	src = t,
	OnInit = DST{
		time=0,x=0,y=0,
	},
	Def.test {
		its = "nested table!"
	},
}

return tt