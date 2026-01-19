-- ENHANCEMENTS FOR 86004.wps - COPY AND PASTE INTO FILE
-- Insert this RIGHT AFTER the boss enters combat (after line 1082)

-- ===================================================================
-- OPTION 1: BOSS RECEIVES INITIAL BUFFS ON COMBAT START
-- ===================================================================

-- Boss gets defensive buffs immediately
Action( "register buff" )
--[
	Param( "target type", "mob" )
	Param( "target index", 68131410 )
	Param( "buff index", 6621 )  -- Defense
--]
End()

Action( "register buff" )
--[
	Param( "target type", "mob" )
	Param( "target index", 68131410 )
	Param( "buff index", 6621 )  -- Defense (stack 2)
--]
End()

Action( "register buff" )
--[
	Param( "target type", "mob" )
	Param( "target index", 68131410 )
	Param( "buff index", 6620 )  -- Attack Power
--]
End()

-- ===================================================================
-- OPTION 3: CONTINUOUS MOB PRESSURE - Spawn waves every 25 seconds
-- ===================================================================

Action( "function" )
--[
	Condition( "child" )
	--[
		-- Loop: Spawn support mobs every 25 seconds
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 25 )
			--]
			End()
		--]
		End()

		-- Spawn 5 support mobs
		Action("add mob")
		--[
			Param("index", 68131412)
			Param("group", 86021)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 35.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131411)
			Param("group", 86021)
			Param("loc x", -145.0)
			Param("loc y", 112.0)
			Param("loc z", 25.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131413)
			Param("group", 86021)
			Param("loc x", -165.0)
			Param("loc y", 112.0)
			Param("loc z", 25.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131412)
			Param("group", 86021)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 5.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131414)
			Param("group", 86021)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 20.0)
			Param("no spawn wait", "true")
		--]
		End()

		-- Wait another 25 seconds
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 25 )
			--]
			End()
		--]
		End()

		-- Spawn 5 MORE support mobs
		Action("add mob")
		--[
			Param("index", 68131411)
			Param("group", 86021)
			Param("loc x", -148.0)
			Param("loc y", 112.0)
			Param("loc z", 30.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131413)
			Param("group", 86021)
			Param("loc x", -162.0)
			Param("loc y", 112.0)
			Param("loc z", 30.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131412)
			Param("group", 86021)
			Param("loc x", -148.0)
			Param("loc y", 112.0)
			Param("loc z", 10.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131414)
			Param("group", 86021)
			Param("loc x", -162.0)
			Param("loc y", 112.0)
			Param("loc z", 10.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 68131411)
			Param("group", 86021)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 20.0)
			Param("no spawn wait", "true")
		--]
		End()

		-- Loop continues until boss is defeated
	--]
	End()

	-- OPTION 5: AREA DENIAL - Spawn bombs in patterns every 20 seconds
	Condition( "child" )
	--[
		-- Wait 20 seconds before first bomb pattern
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 20 )
			--]
			End()
		--]
		End()

		-- BOMB PATTERN 1: Circle around boss
		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 32.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -143.0)
			Param("loc y", 112.0)
			Param("loc z", 20.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -155.0)
			Param("loc y", 112.0)
			Param("loc z", 8.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -167.0)
			Param("loc y", 112.0)
			Param("loc z", 20.0)
			Param("no spawn wait", "true")
		--]
		End()

		-- Wait 20 seconds
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 20 )
			--]
			End()
		--]
		End()

		-- BOMB PATTERN 2: X-pattern
		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -145.0)
			Param("loc y", 112.0)
			Param("loc z", 30.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -165.0)
			Param("loc y", 112.0)
			Param("loc z", 30.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -145.0)
			Param("loc y", 112.0)
			Param("loc z", 10.0)
			Param("no spawn wait", "true")
		--]
		End()

		Action("add mob")
		--[
			Param("index", 18311128)
			Param("group", 86020)
			Param("loc x", -165.0)
			Param("loc y", 112.0)
			Param("loc z", 10.0)
			Param("no spawn wait", "true")
		--]
		End()

		-- Loop continues
	--]
	End()

	-- OPTION 4: DAMAGE REFLECTION/SHIELD - Refresh every 30 seconds
	Condition( "child" )
	--[
		-- Wait 30 seconds
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 30 )
			--]
			End()
		--]
		End()

		-- Apply reflection/defense buffs
		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 6621 )  -- Defense
		--]
		End()

		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 6620 )  -- Attack Power
		--]
		End()

		-- Wait another 30 seconds
		Action( "wait" )
		--[
			Condition( "check time" )
			--[
				Param( "time", 30 )
			--]
			End()
		--]
		End()

		-- Reapply buffs
		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 6621 )  -- Defense
		--]
		End()

		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 6622 )  -- Speed
		--]
		End()

		-- Loop continues
	--]
	End()
--]
End()

-- ===================================================================
-- OPTION 2: MORE INVINCIBILITY PHASES
-- Add these to the 95% and 85% HP phases
-- ===================================================================

Action( "function" )
--[
	-- NEW: 95% HP - Invincibility Phase
	Condition( "child" )
	--[
		Action( "wait" )
		--[
			Condition( "check lp" )
			--[
				Param( "type", "mob" )
				Param( "group", 86012 )
				Param( "index", 68131410 )
				Param( "lp", 95 )
			--]
			End()
		--]
		End()

		-- Make boss INVINCIBLE
		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()

		-- Spawn mini-boss
		Action( "add mobgroup" )
		--[
			Param( "group", 86013 )
			Param( "no spawn wait", "true" )
		--]
		End()

		-- Wait for mini-boss death
		Action( "wait" )
		--[
			Condition( "check mobgroup" )
			--[
				Param( "group", 86013 )
				Param( "count", 0 )
			--]
			End()
		--]
		End()

		-- Remove invincibility
		Action( "unregister buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()
	--]
	End()

	-- NEW: 85% HP - Invincibility Phase
	Condition( "child" )
	--[
		Action( "wait" )
		--[
			Condition( "check lp" )
			--[
				Param( "type", "mob" )
				Param( "group", 86012 )
				Param( "index", 68131410 )
				Param( "lp", 85 )
			--]
			End()
		--]
		End()

		-- Make boss INVINCIBLE
		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()

		-- Spawn TWO mini-bosses
		Action( "add mobgroup" )
		--[
			Param( "group", 86014 )
			Param( "no spawn wait", "true" )
		--]
		End()

		-- Wait for mini-bosses death
		Action( "wait" )
		--[
			Condition( "check mobgroup" )
			--[
				Param( "group", 86014 )
				Param( "count", 0 )
			--]
			End()
		--]
		End()

		-- Remove invincibility
		Action( "unregister buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()
	--]
	End()

	-- NEW: 40% HP - Invincibility Phase
	Condition( "child" )
	--[
		Action( "wait" )
		--[
			Condition( "check lp" )
			--[
				Param( "type", "mob" )
				Param( "group", 86012 )
				Param( "index", 68131410 )
				Param( "lp", 40 )
			--]
			End()
		--]
		End()

		-- Make boss INVINCIBLE
		Action( "register buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()

		-- Spawn THREE mini-bosses
		Action( "add mobgroup" )
		--[
			Param( "group", 86013 )
			Param( "no spawn wait", "true" )
		--]
		End()

		Action( "add mobgroup" )
		--[
			Param( "group", 86014 )
			Param( "no spawn wait", "true" )
		--]
		End()

		-- Wait for all mini-bosses death
		Action( "wait" )
		--[
			Condition( "check mobgroup" )
			--[
				Param( "group", 86013 )
				Param( "count", 0 )
			--]
			End()
		--]
		End()

		Action( "wait" )
		--[
			Condition( "check mobgroup" )
			--[
				Param( "group", 86014 )
				Param( "count", 0 )
			--]
			End()
		--]
		End()

		-- Remove invincibility
		Action( "unregister buff" )
		--[
			Param( "target type", "mob" )
			Param( "target index", 68131410 )
			Param( "buff index", 2385 )
		--]
		End()
	--]
	End()
--]
End()
