-- ============================================================
-- BLOOD PALACE (86004) - DYNAMIC PROGRESSIVE DIFFICULTY
-- ============================================================
-- This system applies STACKING BUFFS to ALL elite mobs (group 86025)
-- at each HP phase, making them progressively stronger WITHOUT
-- needing different mob IDs or CustomDropEvent changes
-- ============================================================

-- ============================================================
-- 90% HP PHASE - TIER 1 BUFFS (Basic Enhancement)
-- ============================================================
-- After spawning 32 elite mobs, apply Tier 1 buffs to the entire group

-- Physical Attack +30%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- ============================================================
-- 70% HP PHASE - TIER 2 BUFFS (Intermediate - STACK ON TOP OF TIER 1)
-- ============================================================
-- After spawning 32 elite mobs, apply Tier 2 buffs
-- These STACK with Tier 1 buffs already on mobs from previous phase!

-- Physical Attack +30% (STACKS with previous = +60% total)
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30% AGAIN
--]
End()

-- Attack Speed +15%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2669 )  -- Attack Speed +15%
--]
End()

-- Physical Defense +30%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2463 )  -- Physical Defense +30%
--]
End()

-- ============================================================
-- 50% HP PHASE - TIER 3 BUFFS (Advanced - STACK EVEN MORE)
-- ============================================================
-- After spawning 32 elite mobs, apply Tier 3 buffs
-- These STACK with Tier 1 + Tier 2 buffs!

-- Physical Attack +30% AGAIN (now +90% total from 3x stacks!)
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30% THIRD TIME
--]
End()

-- Attack Speed +15% AGAIN (stacks to +30% total)
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2669 )  -- Attack Speed +15% AGAIN
--]
End()

-- Physical Defense +30% AGAIN (stacks to +60% total)
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2463 )  -- Physical Defense +30% AGAIN
--]
End()

-- Critical Rate +20%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2632 )  -- Critical Rate +20%
--]
End()

-- Critical Damage +25%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2633 )  -- Critical Damage +25%
--]
End()

-- Energy Attack +30%
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2464 )  -- Energy Attack +30%
--]
End()

-- ============================================================
-- OPTIONAL: Apply buffs to ALL EXISTING elite mobs (not just new spawns)
-- ============================================================
-- To buff mobs that spawned in PREVIOUS phases, target specific mob IDs:

-- Buff all Raviel (68131411) mobs that spawned at 90% and 70%
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131411 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all Despojo (68131412) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131412 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all Custom3 (68131413) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131413 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all Custom4 (68131414) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131414 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all Vampa Beetles (68131390) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131390 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all SS Broly (68131391) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131391 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- Buff all Broly (68131393) mobs
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131393 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

-- ============================================================
-- SUMMARY OF PROGRESSIVE BUFF STACKING
-- ============================================================
-- Phase 1 (90% HP):
--   - New spawns get: +30% Phys Attack
--   - Existing mobs: (none yet)
--
-- Phase 2 (70% HP):
--   - New spawns get: +60% Phys Attack, +15% Atk Speed, +30% Def
--   - OLD 90% spawns ALSO get buffed (if targeting by mob ID)
--
-- Phase 3 (50% HP):
--   - New spawns get: +90% Phys Attack, +30% Atk Speed, +60% Def, +20% Crit, +25% Crit Dmg, +30% Eng Attack
--   - ALL previous spawns (90% + 70%) ALSO get these buffs!
--
-- Result: By 50% HP, you have 96 elite mobs ALL with massive stacked buffs!
-- ============================================================
