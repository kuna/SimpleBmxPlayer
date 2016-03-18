-- cnd test
t = 9
if CND("abcd") then
	t = 10
end

-- table test
local tt = Def.test {
	test = t,
}

--return t