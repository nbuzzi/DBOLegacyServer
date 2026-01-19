# CustomDropEventEditor - Version 2.0 Update

## What's New

The CustomDropEvent system has been updated with powerful new features! The editor tool's **backend (ConfigModel.cs) has been fully updated** to support these features. UI enhancements are optional.

---

## ✅ Features Now Supported

### 1. **PHASE System**
Different modifier sets for different difficulty phases.

**Example**:
```
68131410 modifiers phase=1: hp=1.3 physAtk=1.3
68131410 modifiers phase=2: hp=1.6 physAtk=1.6
68131410 modifiers phase=3: hp=2.0 physAtk=2.0
```

### 2. **HP Restoration Control**
Control whether HP resets to 100% during phase changes.

**Default behavior**: HP is **preserved** (no restoration)

**To restore HP**: Add `restoreHP=1`
```
68131410 modifiers phase=3: hp=2.0 physAtk=2.0 restoreHP=1
```

### 3. **Stat Overflow Protection**
Automatic capping at 65,535 for WORD stats (attack, defense, speed, etc.)

**Stats with caps**: physAtk, engAtk, physDef, engDef, atkSpd, crit rates, etc.
**Stats without caps**: hp, runSpd, critDmg rates

---

## 📦 Files Included

| File | Purpose |
|------|---------|
| `ConfigModel.cs` | ✅ **UPDATED** - Full PHASE & restoreHP support |
| `CHANGELOG_PHASE_SYSTEM.md` | Complete feature documentation |
| `UI_UPDATE_GUIDE.md` | Step-by-step UI implementation guide |
| `README_UPDATE.md` | This file - quick start guide |

---

## 🚀 Quick Start (No UI Changes Needed)

The tool **already works** with all new features! You can use it right now:

### Manual Usage (Current Method):

1. **Open** your CustomDropEvent.cfg file
2. **Edit** mob modifiers with new syntax:
   ```
   # Regular modifiers (no phase)
   123 modifiers: hp=1.5 physAtk=1.5

   # Phase-specific modifiers
   123 modifiers phase=1: hp=2.0 physAtk=2.0
   123 modifiers phase=2: hp=3.0 physAtk=3.0 restoreHP=1
   ```
3. **Save** the file
4. **Load** in the editor to verify parsing works
5. **Save** from the editor to verify output is correct

### What Works Right Now:
- ✅ Loading configs with phase modifiers
- ✅ Loading configs with `restoreHP` flag
- ✅ Saving configs preserves all phase data
- ✅ Editing via the modifier textbox (`txtMods`)

### Limitations (Without UI Updates):
- ⚠️ No dropdown to switch between phases
- ⚠️ No checkbox for `restoreHP` (must type manually)
- ⚠️ No overflow warnings (manual checking needed)

---

## 🎨 Optional UI Enhancements

For better user experience, implement the UI controls described in:
- **`UI_UPDATE_GUIDE.md`** - Step-by-step implementation
- **Estimated time**: 4-6 hours
- **Benefit**: Visual phase selector, restoreHP checkbox, overflow warnings

---

## 📖 Examples

### Example 1: Blood Palace Boss with Phases

```
# Base stats (no modifiers at phase 0)
68131410 modifiers: hp=1.0 physAtk=1.0

# Phase 1 at 90% HP
68131410 modifiers phase=1: hp=1.3 physAtk=1.3

# Phase 2 at 70% HP
68131410 modifiers phase=2: hp=1.6 physAtk=1.6 atkSpd=0.85

# Phase 3 at 50% HP - restore HP to 100%
68131410 modifiers phase=3: hp=2.0 physAtk=2.0 atkSpd=0.65 restoreHP=1
```

### Example 2: Simple Event Mob (No Phases)

```
# Regular modifier - no phase needed
12345 modifiers: hp=5.0 physAtk=3.0 physDef=2.0
```

### Example 3: Global All Mobs

```
# Apply to all mobs
0 modifiers: hp=2.0 physAtk=1.5

# Phase 1 for all mobs
0 modifiers phase=1: hp=3.0 physAtk=2.0
```

---

## 🔍 Testing Your Config

### In-Game Commands:
```
@reload_customdrop          # Reload the config
@setphase 1                 # Set world to phase 1
@setphase 2                 # Set world to phase 2
@setphase 0                 # Reset to base phase
```

### Server Log Messages:
```
[CustomDropEvent] PhysAtk overflow: mob 123 base=5000 mult=30.0 result=150000 (capped at 65535)
```
If you see overflow warnings, reduce the multiplier!

---

## 📊 Stat Multiplier Guidelines

### Safe Multipliers (Won't Overflow)

| Stat Type | Base Value | Max Safe Multiplier |
|-----------|------------|---------------------|
| Attack (physAtk, engAtk) | 1,000 | 65× |
| Attack | 2,000 | 32× |
| Attack | 3,000 | 21× |
| Attack | 5,000 | 13× |
| **HP** | Any | **Unlimited!** |

### Recommended Approach:
- Use **moderate multipliers** (≤20) for attack/defense
- Use **very high multipliers** for HP (100+ is fine!)
- Test in-game and check server logs

---

## 🐛 Known Issues

### Reload Not Affecting Existing Mobs
**Issue**: After `@reload_customdrop`, existing mobs keep old stats.

**Solution**: Mobs must respawn for new modifiers to apply.
- Kill the mob and wait for respawn
- OR restart the server

This is by design - modifiers only apply during mob creation.

---

## 🔧 Configuration Format Reference

### Complete Syntax:
```
# Drops
<mobId>: <itemId>@<rate>x<count>, ...

# Modifiers (regular)
<mobId> modifiers: hp=X physAtk=Y ... [restoreHP=1]

# Modifiers (phase-specific)
<mobId> modifiers phase=N: hp=X physAtk=Y ... [restoreHP=1]

# Spawns
<mobId> spawn: <spawnMobId>@<rate>x<count>, ...

# Buffs
<mobId> buffs: <skillId>@<durationMs>, ...

# Titles
<mobId> titles: <titleId>, ...

# Visuals
<mobId> visuals: <effectId>@<intervalMs>, ...
```

### All Available Modifier Stats:
```
hp, physAtk, engAtk, physDef, engDef, atkSpd, runSpd,
physCrit, engCrit, physCritDmg, engCritDmg,
attackRate, dodgeRate, blockRate, blockDmg, guardRate,
sizeRate, restoreHP
```

---

## 📚 Documentation Files

1. **CHANGELOG_PHASE_SYSTEM.md** - Complete technical documentation
   - All new features explained
   - Code changes summary
   - Testing checklist

2. **UI_UPDATE_GUIDE.md** - UI implementation guide
   - Code snippets for all new controls
   - Event handlers
   - Layout suggestions

3. **README_UPDATE.md** - This file
   - Quick start guide
   - Examples
   - Testing procedures

---

## 🎯 Next Steps

### For Immediate Use:
1. ✅ Use the editor as-is (manual phase editing)
2. ✅ Test loading/saving configs with phases
3. ✅ Verify in-game with `@reload_customdrop` and `@setphase`

### For Enhanced Experience:
1. 📝 Read `UI_UPDATE_GUIDE.md`
2. 🎨 Implement phase dropdown and restoreHP checkbox
3. ⚠️ Add overflow warnings
4. 📋 Add phase list view

---

## ✨ Summary

| Feature | Backend | UI | Status |
|---------|---------|-----|--------|
| PHASE modifiers | ✅ Done | ⏳ Optional | Working |
| restoreHP flag | ✅ Done | ⏳ Optional | Working |
| Overflow protection | ✅ Done | ⏳ Optional | Working |
| Load/Save | ✅ Done | ✅ Done | Working |

**Bottom line**: The tool is **fully functional** with all new features. UI enhancements would make it easier to use, but are not required.

---

## 📞 Support

- Check server logs for overflow warnings
- Test configs with `@reload_customdrop` command
- Use `@setphase N` to test phase transitions in-game
- Verify HP behavior during phase changes

Enjoy the new PHASE system! 🎉
