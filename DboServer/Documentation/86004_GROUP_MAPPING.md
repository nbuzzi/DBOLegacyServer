# 86004.wps - Mob Group Mapping Fix

## Problem
The original 86004.wps used custom mob groups (86500, 86600, 86012, etc.) that **do not exist** in the spawn table file `spawn_mob_bdmaho_001.rdf`. This caused:
- Only 3 individual mobs spawning (68131410, 68131411, 68131412)
- No egg spawns
- No group-based mob spawns
- Buffs not working (targeting mobs that never spawned)

## Solution
Replaced all non-existent groups with **existing groups from BID4 (60040.wps)** that are defined in the spawn table.

## Group Mapping

| Old Group (Non-existent) | New Group (BID4) | Purpose |
|-------------------------|------------------|---------|
| 86500 | 102 | Wave 1 mobs |
| 86600 | 105 | Wave 2 mobs |
| 86012 | 101 | Boss group (Mahoraga) |
| 86011 | 103 | Eggs |
| 86003 | 86006 | Initial engagement mobs |
| 86013 | 86001 | Mini-boss (70% HP phase) |
| 86014 | 86051 | Mini-boss (50% HP phase) |
| 86020 | 106 | Bombs |
| 86021 | 86077 | Additional support mobs |
| 86006 | 86006 | Support mobs (unchanged) |

## Groups Now Used (All Valid)
- **101** - Boss group
- **102** - Wave 1
- **103** - Eggs
- **105** - Wave 2
- **106** - Bombs
- **86001** - Mini-boss 1
- **86006** - Support mobs
- **86051** - Mini-boss 2
- **86077** - Additional support

## Expected Behavior Now

### Wave 1
- Spawns group 102 mobs (from BID4 spawn table)
- Spawns eggs (group 103)

### Wave 2
- Spawns group 105 mobs (from BID4 spawn table)
- Spawns eggs (group 103)

### Boss Phases
- **90% HP**: Boss gets buffs (2462 x2, 2669, 2632) + eggs + bombs + support mobs
- **70% HP**: Boss INVINCIBLE + buffs + mini-boss (group 86001) + eggs + support
- **50% HP**: Boss INVINCIBLE + buffs + mini-boss (group 86051) + eggs + support
- **30% HP**: Boss gets buffs (2462 x5, 2632 x3, 2669 x3) + eggs + support
- **20% HP**: Boss gets buffs (2462 x7, 2632 x7, 2669 x5) + eggs + support
- **10% HP**: Boss gets MASSIVE buffs (2462 x10, 2668 x7, 2669 x7, 2632 x7, 2393 x5) + eggs + support

## What Needs to Happen Next

**CRITICAL**: The spawn table file must match the dungeon number!

If your dungeon is accessed as **86004**, the game will look for:
- `spawn_mob_bd86004_001.rdf` or similar naming

Currently using groups from BID4, which means the dungeon will load the **spawn table for BID4/Kraken dungeon**.

### To make this work properly, you have TWO options:

**Option A: Keep using BID4 groups (Current)**
- Pros: Works immediately, no table editing needed
- Cons: You'll spawn BID4's mobs (Kraken dungeon mobs), not custom Mahoraga mobs

**Option B: Create proper spawn table (Recommended for production)**
- Copy BID4's spawn table structure
- Replace mob IDs with your custom mobs (68131410-68131414)
- Create groups 101, 102, 103, 105, 106, 86001, 86006, 86051, 86077 in the new table
- This requires RDF editing tools

## Backup
Backup created: `86004.wps.backup_YYYYMMDD_HHMMSS`

## Changes Applied
- 9 group replacements
- All groups now use existing BID4 spawn table groups
- File structure unchanged
- All mechanics preserved
