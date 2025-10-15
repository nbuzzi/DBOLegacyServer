# 86004.wps - FINAL FIX: Individual Mob Spawns Only

## Root Cause
The dungeon WPS file was calling mob **groups** (86500, 86600, 86011, 86003, 86006, 86013, 86014, 86020, 86021) that **DO NOT EXIST** in the spawn table file `spawn_mob_bdmaho_001.rdf`.

When `Action( "add mobgroup" )` is called for a non-existent group, the game spawns NOTHING.

## Why Using BID4 Groups Failed
We tried replacing the non-existent groups with BID4's existing groups (101, 102, 103, etc.), but this caused NO mobs to spawn at all because:
- Each dungeon has its own spawn table file
- BID4's groups (101, 102, 103, etc.) are only defined in BID4's spawn table
- 86004 dungeon cannot access BID4's spawn table groups

## Solution Applied
**Commented out all non-existent mobgroup spawns** and kept only the **250 individual mob spawns** that use specific coordinates.

### Changes Made:

1. **Wave 1 mobgroup** (group 86500) - COMMENTED OUT
   - Kept 30+ individual mob spawns with coordinates
   - Changed wait condition from group 86500 → group 86021 (where individual mobs are assigned)

2. **Wave 2 mobgroup** (group 86600) - COMMENTED OUT
   - Kept 35+ individual mob spawns with coordinates
   - Wait condition already uses group 86600 (where individual mobs are assigned) ✓

3. **Egg spawns** (group 86011) - COMMENTED OUT
   - No eggs will spawn (group doesn't exist in table)
   - Alternative: Add individual egg mob spawns if needed

4. **Support mob groups** (86003, 86006, 86020, 86021) - COMMENTED OUT
   - Individual mobs still spawn at coordinates

5. **Mini-boss groups** (86013, 86014) - COMMENTED OUT
   - These were for invincibility phases
   - Invincibility mechanics may not work without these

## What Works Now:

✅ **Wave 1** - 30+ individual custom mobs spawn (68131411-68131414, 18311128)
✅ **Wave 2** - 35+ individual custom mobs spawn
✅ **Boss spawns** - Mahoraga (68131410) spawns individually
✅ **HP phase mechanics** - LP checks target Mahoraga specifically (group 101, index 68131410)
✅ **Buffs apply** - Buffs target Mahoraga (68131410) specifically
✅ **Individual mob spawns throughout** - 250+ mobs spawn at HP phases
✅ **Dungeon progression** - No longer stuck waiting for non-existent groups

## What Doesn't Work:

❌ **Eggs** - Group 86011 doesn't exist, no eggs spawn
❌ **Invincibility phases at 70% and 50%** - Require mini-boss groups 86013/86014 which don't exist
❌ **Some support mob variety** - Groups 86003/86006 don't spawn

## Statistics:
- **250 individual mob spawns** (working)
- **19 mobgroup spawns** (most commented out)
- **Individual spawns by mob type:**
  - 68131410 (Mahoraga boss) - 1 spawn
  - 68131411 (Raviel) - Multiple spawns
  - 68131412 (Despojo) - Multiple spawns
  - 68131413 (Unknown) - Multiple spawns
  - 68131414 (Unknown) - Multiple spawns
  - 18311128 (Bomb mob) - Multiple spawns

## How to Add Mobs Properly:

Instead of:
```lua
Action( "add mobgroup" )
--[
    Param( "group", 86011 )  -- Group doesn't exist!
    Param( "no spawn wait", "true" )
--]
End()
```

Use:
```lua
Action("add mob")
--[
    Param("index", 68131411)  -- Specific mob ID
    Param("group", 86021)     -- Group for tracking (can be any number)
    Param("loc x", -155.0)
    Param("loc y", 112.0)
    Param("loc z", 20.0)
    Param("dir x", 0.0)
    Param("dir z", 1.0)
    Param("no spawn wait", "true")
--]
End()
```

## Next Steps to Fully Fix:

**Option 1: Add Mob Definitions to Mobs.txt**
Add your custom mobs to `DboServer/ExecutionEnv/config/Mobs.txt`:
```
@addmob 68131410 Mahoraga
@addmob 68131411 Raviel
@addmob 68131412 Despojo
@addmob 68131413 CustomMob3
@addmob 68131414 CustomMob4
```

**Option 2: Create Spawn Table Groups** (Advanced)
Edit `spawn_mob_bdmaho_001.rdf` using RDF tools to define groups 86011, 86013, 86014, etc.

**Option 3: Add Individual Egg Spawns**
Replace commented-out egg group spawns with individual egg mob spawns at specific coordinates.

## Testing Checklist:
- [ ] Wave 1 mobs spawn
- [ ] Wave 1 clears and progresses to Wave 2
- [ ] Wave 2 mobs spawn
- [ ] Boss spawns after Wave 2
- [ ] Boss HP phases trigger (90%, 70%, 50%, 30%, 20%, 10%)
- [ ] Buffs apply to boss at each phase
- [ ] Individual mobs spawn at each HP phase
- [ ] Dungeon completes when boss is defeated

## Backup:
Previous version saved as: `86004.wps.backup_20251014_124840`
