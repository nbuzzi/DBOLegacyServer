# Blood Palace (86004) - Progressive CustomDropEvent Difficulty System

## Problem
When CustomDropEvent is enabled, ALL modifiers are applied globally to mobs immediately. You want progressive difficulty scaling where mobs get stronger at each HP phase (90%, 70%, 50%).

## Solution: Phase-Specific Mob IDs

Instead of using the same mob IDs at all phases, we'll use **DIFFERENT mob IDs for each phase**, each with its own modifier entry in CustomDropEvent.cfg. This allows progressive scaling!

### Current Mob IDs Used (from your CustomDropEvent.cfg):
- 68131390 - Vampa Beetle
- 68131391 - SS Broly
- 68131392 - Mini-boss variant
- 68131393 - Broly variant
- 68131411 - Raviel
- 68131412 - Despojo
- 68131413 - Custom 3
- 68131414 - Custom 4

## Progressive Mob ID Strategy

### Create 3 Tiers of the Same Mobs with Different IDs:

**Tier 1 (90% HP) - BASIC MODIFIERS:**
- Use existing IDs: 68131411-68131414, 68131390-68131393
- Light modifiers (current state)

**Tier 2 (70% HP) - INTERMEDIATE MODIFIERS:**
- New IDs: 68131421-68131424 (copies of 68131411-68131414)
- Medium modifiers (more stats)

**Tier 3 (50% HP) - ADVANCED MODIFIERS:**
- New IDs: 68131431-68131434 (copies of 68131411-68131414)
- Heavy modifiers (max stats + crit)

## Implementation Steps

### Step 1: Update CustomDropEvent.cfg

Add new mob ID entries with progressive modifiers:

```cfg
# ============================================================
# BLOOD PALACE (86004) - PROGRESSIVE DIFFICULTY SCALING
# ============================================================

# TIER 1: 90% HP Phase - Basic Enhancement (+30% stats)
68131411 modifiers: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131412 modifiers: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131413 modifiers: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131414 modifiers: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131390 modifiers: hp=1.2 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1.1 runSpd=1.1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131391 modifiers: hp=1.4 physAtk=1.4 engAtk=1.4 physDef=1.2 engDef=1.2 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131393 modifiers: hp=1.4 physAtk=1.4 engAtk=1.4 physDef=1.2 engDef=1.2 atkSpd=1 runSpd=1 physCrit=1 engCrit=1 physCritDmg=1 engCritDmg=1 attackRate=1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1

# TIER 2: 70% HP Phase - Intermediate Enhancement (+60% stats, +20% speed/defense)
68131421 modifiers: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2 physCritDmg=1 engCritDmg=1 attackRate=1.1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131422 modifiers: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2 physCritDmg=1 engCritDmg=1 attackRate=1.1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131423 modifiers: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2 physCritDmg=1 engCritDmg=1 attackRate=1.1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1
68131424 modifiers: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2 physCritDmg=1 engCritDmg=1 attackRate=1.1 dodgeRate=1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1

# TIER 3: 50% HP Phase - Advanced Enhancement (+100% stats, +50% crit, fast attacks)
68131431 modifiers: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5 attackRate=1.2 dodgeRate=1.1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1.1
68131432 modifiers: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5 attackRate=1.2 dodgeRate=1.1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1.1
68131433 modifiers: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5 attackRate=1.2 dodgeRate=1.1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1.1
68131434 modifiers: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5 attackRate=1.2 dodgeRate=1.1 blockRate=1 blockDmg=1 guardRate=1 sizeRate=1.1

# Register spawns for these mobs (required for custom event)
68131411 spawn: 68131411@0
68131412 spawn: 68131412@0
68131413 spawn: 68131413@0
68131414 spawn: 68131414@0
68131421 spawn: 68131421@0
68131422 spawn: 68131422@0
68131423 spawn: 68131423@0
68131424 spawn: 68131424@0
68131431 spawn: 68131431@0
68131432 spawn: 68131432@0
68131433 spawn: 68131433@0
68131434 spawn: 68131434@0
```

### Step 2: Update 86004.wps to Use Progressive Mob IDs

Modify the elite mob spawns to use different mob IDs per phase:

**90% HP Phase:** Use IDs 68131411-68131414, 68131390-68131393 (TIER 1)
**70% HP Phase:** Use IDs 68131421-68131424, 68131390-68131393 (TIER 2)
**50% HP Phase:** Use IDs 68131431-68131434, 68131390-68131393 (TIER 3)

### Step 3: Register New Mob IDs in Mobs.txt

Add the new mob IDs so the server recognizes them:

```
@addmob 68131421 Raviel_T2
@addmob 68131422 Despojo_T2
@addmob 68131423 Custom3_T2
@addmob 68131424 Custom4_T2
@addmob 68131431 Raviel_T3
@addmob 68131432 Despojo_T3
@addmob 68131433 Custom3_T3
@addmob 68131434 Custom4_T3
```

## Modifier Explanation

### Attack Speed Modifier
- `atkSpd=0.85` means 15% FASTER attacks (0.85 = 85% of normal delay = faster)
- `atkSpd=0.65` means 35% FASTER attacks
- Lower = Faster attacks

### Size Modifier
- `sizeRate=1.1` = 110% of normal size (10% bigger)
- Can make mobs visually larger at later phases

### Crit Modifiers
- `physCrit=1.5` = 150% crit rate (50% more crits)
- `physCritDmg=1.5` = 150% crit damage (50% more damage on crits)

## Difficulty Progression Comparison

| Phase | HP | Phys Atk | Atk Speed | Crit Rate | Crit Dmg | Defense | Visual |
|-------|-----|----------|-----------|-----------|----------|---------|--------|
| 90%   | +30% | +30%     | Normal    | Normal    | Normal   | Normal  | Normal |
| 70%   | +60% | +60%     | +15% faster | +20%    | Normal   | +20%    | Normal |
| 50%   | +100% | +100%   | +35% faster | +50%    | +50%     | +50%    | +10% bigger |

## Alternative: Python Script to Generate Spawn Updates

Would you like me to generate a Python script that:
1. Reads your current 86004_ELITE_MOB_SPAWNS_READY.txt
2. Replaces mob IDs based on phase:
   - 90% spawns: Keep existing IDs (Tier 1)
   - 70% spawns: Replace with Tier 2 IDs (6813141X → 6813142X)
   - 50% spawns: Replace with Tier 3 IDs (6813141X → 6813143X)
3. Outputs updated spawn code ready to paste

## Testing

After implementation:

1. **Without CustomDropEvent enabled:**
   - All mobs spawn normally with base stats
   - No progressive difficulty

2. **With CustomDropEvent enabled:**
   - 90% HP: Mobs are moderately stronger (+30% stats)
   - 70% HP: Mobs are significantly stronger (+60% stats, faster)
   - 50% HP: Mobs are VERY strong (+100% stats, high crits, fast, bigger)

This gives you full control - enable CustomDropEvent for hardcore mode, disable for normal mode!

## Next Steps

Would you like me to:
1. Generate the full CustomDropEvent.cfg additions?
2. Create updated 86004_ELITE_MOB_SPAWNS_READY.txt with progressive mob IDs?
3. Generate the Mobs.txt entries?
4. All of the above?
