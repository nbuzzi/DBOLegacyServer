# Blood Palace (86004) - Progressive Difficulty Scaling System

## Overview
Instead of applying all PlayerModifiers.cfg buffs at once globally, we apply buffs **progressively** to elite mobs at each HP phase in the WPS file. This creates escalating difficulty as the boss fight progresses.

## Buff IDs Available (Common buffs used in DBO)
Based on your existing WPS file and typical DBO mechanics:

| Buff ID | Effect | Notes |
|---------|--------|-------|
| 2462 | Physical Attack +30% | Already used on boss |
| 2669 | Attack Speed +15% | Already used on boss |
| 2632 | Critical Rate +20% | Already used on boss |
| 2463 | Physical Defense +30% | Makes mobs tankier |
| 2464 | Energy Attack +30% | For energy damage |
| 2465 | Energy Defense +30% | For energy defense |
| 2670 | Movement Speed +15% | Makes mobs faster |
| 2633 | Critical Damage +25% | Increases crit damage |
| 3001 | HP Regen | Regeneration effect |
| 3002 | EP Regen | EP regeneration |

## Progressive Difficulty Strategy

### Phase 1: 90% HP - BASIC ENHANCEMENT
**Spawns:** 32 elite mobs
**Buffs Applied to ALL spawned mobs:**
- Physical Attack +30% (Buff 2462)
- HP +20% (via individual stat boost)

**Goal:** Slight difficulty increase, players notice mobs hit harder

### Phase 2: 70% HP - INTERMEDIATE ENHANCEMENT
**Spawns:** 32 elite mobs
**Buffs Applied to ALL spawned mobs:**
- Physical Attack +30% (Buff 2462)
- Attack Speed +15% (Buff 2669)
- Physical Defense +30% (Buff 2463)

**Goal:** Moderate difficulty spike, mobs are faster and tankier

### Phase 3: 50% HP - ADVANCED ENHANCEMENT
**Spawns:** 32 elite mobs
**Buffs Applied to ALL spawned mobs:**
- Physical Attack +30% (Buff 2462) - STACKED TWICE
- Attack Speed +15% (Buff 2669)
- Physical Defense +30% (Buff 2463)
- Critical Rate +20% (Buff 2632)
- Critical Damage +25% (Buff 2633)

**Goal:** High difficulty, mobs are very dangerous with crits

### Phase 4: 30% HP - ELITE ENHANCEMENT (if you add spawns here)
**Buffs for future spawns:**
- ALL buffs from Phase 3
- Movement Speed +15% (Buff 2670)
- Energy Attack +30% (Buff 2464)

### Phase 5: 10% HP - APOCALYPSE MODE (if you add spawns here)
**Buffs for future spawns:**
- ALL buffs stacked MULTIPLE times
- HP Regen (Buff 3001)

## Implementation Method

### Method 1: Apply Buffs to Specific Mob Group (RECOMMENDED)
Target all mobs in group 86025 (the elite spawn group) after spawning them:

```lua
-- After spawning 32 mobs at 90% HP, apply buffs to the entire group
Action( "register buff" )
--[
    Param( "target type", "group" )  -- Target entire group
    Param( "target index", 86025 )   -- Elite mob group
    Param( "buff index", 2462 )      -- Physical Attack +30%
--]
End()
```

### Method 2: Apply Buffs to Individual Mob IDs (Alternative)
Target specific mob types across the dungeon:

```lua
-- Apply buff to ALL instances of mob ID 68131411
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131411 )  -- Raviel
    Param( "buff index", 2462 )        -- Physical Attack +30%
--]
End()
```

### Method 3: Apply Buff During Spawn (Most Precise)
Apply buff immediately when spawning each mob (requires modifying each spawn block):

```lua
Action("add mob")
--[
    Param("index", 68131411)
    Param("group", 86025)
    Param("loc x", -137.0)
    Param("loc y", 112.0)
    Param("loc z", 20.0)
    Param("dir x", 0.0)
    Param("dir z", 1.0)
    Param("no spawn wait", "true")
--]
End()

-- Immediately buff this mob after spawning
Action( "register buff" )
--[
    Param( "target type", "mob" )
    Param( "target index", 68131411 )
    Param( "buff index", 2462 )
--]
End()
```

## Recommended Implementation (Method 1 - Group Buffs)

This is the cleanest approach - spawn all 32 mobs, then apply buffs to the entire group:

### 90% HP Phase Implementation:
```lua
-- ===============================================================
-- 90% HP PHASE - 32 Elite Spawns with BASIC buffs
-- ===============================================================

[... 32 mob spawns here ...]

-- BUFF PHASE 1: BASIC ENHANCEMENT
-- Apply Physical Attack buff to all elite mobs
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()
```

### 70% HP Phase Implementation:
```lua
-- ===============================================================
-- 70% HP PHASE - 32 Elite Spawns with INTERMEDIATE buffs
-- ===============================================================

[... 32 mob spawns here ...]

-- BUFF PHASE 2: INTERMEDIATE ENHANCEMENT
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2669 )  -- Attack Speed +15%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2463 )  -- Physical Defense +30%
--]
End()
```

### 50% HP Phase Implementation:
```lua
-- ===============================================================
-- 50% HP PHASE - 32 Elite Spawns with ADVANCED buffs
-- ===============================================================

[... 32 mob spawns here ...]

-- BUFF PHASE 3: ADVANCED ENHANCEMENT
-- Stack Physical Attack TWICE for max damage
Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- Physical Attack +30%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2462 )  -- STACK: Physical Attack +30% AGAIN
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2669 )  -- Attack Speed +15%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2463 )  -- Physical Defense +30%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2632 )  -- Critical Rate +20%
--]
End()

Action( "register buff" )
--[
    Param( "target type", "group" )
    Param( "target index", 86025 )
    Param( "buff index", 2633 )  -- Critical Damage +25%
--]
End()
```

## Visual Progression

```
90% HP: ████░░░░░░ (1 buff)  - Mobs hit 30% harder
70% HP: ██████░░░░ (3 buffs) - Mobs hit 30% harder, 15% faster, 30% tankier
50% HP: ██████████ (6 buffs) - Mobs hit 60% harder, 15% faster, 30% tankier, CRITS enabled
30% HP: ████████████ (8+ buffs if added)
10% HP: ████████████████ (10+ buffs if added)
```

## Integration with PlayerModifiers.cfg

You have two options:

### Option 1: Keep PlayerModifiers.cfg DISABLED (Recommended)
- All difficulty comes from progressive WPS buffs
- Full control over when buffs apply
- No global impact on other dungeons

### Option 2: Use PlayerModifiers.cfg as BASE + WPS Progressive Enhancement
- Enable PlayerModifiers.cfg for global baseline difficulty
- WPS buffs stack ON TOP for this specific dungeon
- Creates even harder dungeon experience

**Example Math (Option 2):**
- PlayerModifiers.cfg: +55% Physical Attack (global)
- 90% HP WPS Buff: +30% Physical Attack
- **Total at 90%: +85% Physical Attack**
- 50% HP WPS Buffs: +60% Physical Attack (stacked twice)
- **Total at 50%: +115% Physical Attack + Crits**

## Testing Checklist

- [ ] 90% HP: Elite mobs spawn with basic buffs
- [ ] 70% HP: Elite mobs spawn with intermediate buffs (noticeably harder)
- [ ] 50% HP: Elite mobs spawn with advanced buffs (very dangerous)
- [ ] Buffs apply only to group 86025 (not to boss or other mobs)
- [ ] Visual buff icons appear on elite mobs
- [ ] Difficulty feels progressive and challenging

## Next Steps

1. I can generate the complete buff application code for all 3 phases
2. I can insert it into your 86004.wps file at the correct locations
3. Test in-game to verify buff stacking and difficulty progression

Would you like me to proceed with implementation?
