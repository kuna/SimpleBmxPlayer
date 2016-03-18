-- used for LR2 compatibility for Play scene.
-- these script is executed in global scope
-- don't edit this script unless you know what you're doing.
-- @lazykuna

-- check conditions and active handler/switch properly.

P1OP = GetInt("P1OP")
if P1OP == 0 then
	SetSwitch("P1GrooveGauge")
end

