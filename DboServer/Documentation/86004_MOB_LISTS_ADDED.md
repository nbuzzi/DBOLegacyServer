# 86004.wps - MOB LISTS ADDED - THE MISSING PIECE!

## The Problem Discovered

**You found it!** The WPS file was missing `Action( "mob list" )` definitions!

In DBO's WPS system:
- `Action( "add mobgroup" )` tells the game to spawn a group
- **BUT** the game doesn't know WHICH mobs to spawn in that group
- `Action( "mob list" )` defines the mob pool for each group index

Without mob lists, `add mobgroup` spawns **NOTHING**!

## Solution Applied

Added **9 mob list definitions** at the beginning of the WPS file (before GameStage):

### Mob Lists Added:

| Group Index | Purpose | Mobs Included |
|-------------|---------|---------------|
| **86500** | Wave 1 | 68131411, 68131412, 68131413, 68131414, 68131390, 18311128 (bombs) |
| **86600** | Wave 2 | All above + 68131393 (Broly), 68131394-68131396 (Dragons) |
| **86003** | Boss Support | 68131391-68131393 (Broly variants), 68131394-68131399 (All Dragons) |
| **86006** | Regular Adds | 68131390, 68131411-68131414 (Standard mobs) |
| **86011** | Eggs | 68131390 (Vampa Beetle - placeholder) |
| **86012** | Main Boss | 68131410 (Mahoraga only) |
| **86013** | Mini-Boss 70% | 68131391 (SS Broly), 68131392 (Ozaru Broly), 68131393 (Broly) |
| **86014** | Mini-Boss 50% | 68131391-68131393 (Multiple Broly variants) |
| **86020** | Bombs | 18311128 (Bomb mob) |

### Mob Variety Now Available:

**Standard Enemies:**
- 68131411 (Raviel)
- 68131412 (Despojo)
- 68131413 (Custom mob 3)
- 68131414 (Custom mob 4)
- 68131390 (Vampa Beetle) - Fast small enemy

**Boss Tier:**
- 68131410 (Mahoraga) - Main boss

**Elite Enemies (Broly variants):**
- 68131391 (Super Saiyan Broly)
- 68131392 (Ozaru Broly) - Giant form
- 68131393 (Broly)

**Dragons (Elemental):**
- 68131394 (3 Star Dragon)
- 68131395 (4 Star Dragon)
- 68131396 (1 Star Dragon)
- 68131397 (Evil Dragon)
- 68131398 (Ice Dragon)
- 68131399 (Fire Dragon)

**Special:**
- 18311128 (Bomb mob)

### How It Works:

When the game executes:
```lua
Action( "add mobgroup" )
--[
    Param( "group", 86500 )
    Param( "no spawn wait", "true" )
--]
End()
```

It looks up the mob list for index 86500:
```lua
Action( "mob list" )
--[
    Param( "index", 86500 )
    Param( "mob1", "68131411,68131412,68131413,68131414" )
    Param( "mob2", "68131411,68131412,68131390" )
    Param( "mob3", "68131412,68131413,68131390" )
    Param( "mob4", "68131390,68131411,18311128" )
    Param( "mob5", "68131412,68131413,68131414" )
--]
```

The game **randomly picks mobs from these lists** when spawning the group!

## What Should Work Now:

✅ **Wave 1 mobgroup spawns** - Group 86500 will spawn random mobs from its list
✅ **Wave 2 mobgroup spawns** - Group 86600 with more variety including Broly and Dragons
✅ **Egg spawns** - Group 86011 (using Vampa Beetle as placeholder)
✅ **Boss support mobs** - Group 86003 with elite guards
✅ **HP phase adds** - Group 86006 spawns throughout fight
✅ **Mini-bosses** - Groups 86013 and 86014 spawn Broly variants at 70% and 50%
✅ **Bomb spawns** - Group 86020 for explosive hazards
✅ **Boss spawn** - Group 86012 spawns Mahoraga
✅ **Individual mob spawns still work** - All 250+ hardcoded spawns still active

## Massive Increase in Difficulty:

**Before:** Only individual spawns (predictable, same mobs)
**After:** Group spawns + individual spawns with:
- **RANDOM mob selection** from pools
- **Broly mini-bosses** at 70% and 50% HP
- **Dragon variety** (6 different dragons!)
- **More mob density** (groups + individuals)
- **Elemental variety** (Ice Dragon, Fire Dragon, Evil Dragon, etc.)

## Testing Priority:

1. **Wave 1** - Should spawn group 86500 mobs PLUS your individual spawns
2. **Wave 2** - Should spawn group 86600 mobs with dragons and Broly
3. **Boss Support** - When boss enters combat, group 86003 should spawn elite guards
4. **70% HP** - Mini-boss group 86013 spawns (Broly variants)
5. **50% HP** - Mini-boss group 86014 spawns (2 Broly variants)
6. **Eggs** - Group 86011 should spawn (currently using Vampa Beetle)
7. **Invincibility mechanics** - Boss should become invincible at 70%/50% until mini-bosses die

## Notes:

**Egg Mobs:** Currently using 68131390 (Vampa Beetle) as placeholder. If you have actual egg mob IDs, replace them in the mob list for group 86011.

**Mob IDs Must Exist:** All mob IDs used in mob lists must be defined in:
- `DboServer/ExecutionEnv/config/Mobs.txt` OR
- The game's mob table

Confirmed existing:
- 68131390-68131399 ✓
- 18311128 ✓

## Files Modified:

- `DboServer/ExecutionEnv/resource/server_data/wps/wps/86004.wps`
  - Added 9 mob list definitions (lines 29-121)
  - Total mob variety: 15 different mob types
  - Groups now functional: 86500, 86600, 86003, 86006, 86011, 86012, 86013, 86014, 86020

## Next Steps:

**Test the dungeon!** All mobgroup spawns should now work properly. You'll see:
- Mixed mob types in waves
- Random spawn variety
- Mini-bosses appearing
- Much higher difficulty!

If you want even MORE difficulty, I can:
1. Add more mob IDs to the lists (more variety in pools)
2. Increase spawn counts in mobgroup calls
3. Add continuous wave spawns during boss fight
4. Add more HP phase triggers

**The dungeon should be FULLY FUNCTIONAL now!**
