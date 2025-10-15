# Proposed Changes for 86004.wps Boss Enhancement

## Current Issues Identified:
1. **Boss has invincibility buff but using WRONG target index** (using group 86012 instead of mob index 68131410)
2. **No attack/damage buffs** applied to boss to make it hit harder
3. **Boss only single-target attacks** - needs AOE or multi-target capabilities
4. **Buffs not properly targeted** - all "target index" parameters use group ID instead of mob ID

## Proposed Enhancements:

### 1. Fix All Buff Target Indices
- Change all `Param( "target index", 86012 )` to `Param( "target index", 68131410 )`
- This will ensure buffs actually apply to the boss mob

### 2. Add Progressive Damage Buffs at Each HP Phase:
- **90% HP**: Add Attack Power buff (buff index 6620 or similar)
- **70% HP**: Keep invincibility + Add Physical/Energy Attack buff
- **50% HP**: Keep invincibility + Add Speed buff (faster attacks)
- **30% HP**: Add Critical buff + Damage buff
- **10% HP ENRAGE**: Stack multiple buffs (ATK + DEF + SPD + CRIT)

### 3. Boss Mechanics to Add:
- Periodic AOE attacks (if supported by mob AI)
- Enrage timer buffs that stack over time
- Remove some buffs when mini-bosses die (creates strategy)

### 4. Common Buff Indices (from other WPS files):
- 2385: Invincibility
- 6620: Attack Power Up
- 6621: Defense Up
- 6622: Speed Up
- Other offensive buffs: 6623-6630 range

Would you like me to implement these changes?
