# Phase System Quick Reference Guide

## 🎮 GM Commands

### Set Difficulty Phase
```
@setphase <0-5>
```

**Examples**:
```
@setphase 0    # Reset to default (no modifiers)
@setphase 1    # Phase 1: +30% HP/Attack
@setphase 2    # Phase 2: +60% HP/Attack, +15% attack speed
@setphase 3    # Phase 3: +100% HP/Attack, +35% attack speed, +50% crit
```

### Reload Config
```
@reload_customdrop
```

---

## 📊 Phase Stats Overview

| Phase | HP | Attack | Atk Speed | Crit | Defense | Description |
|-------|----|----|-----------|------|---------|-------------|
| **0** | +0% | +0% | Normal | Normal | Normal | Default (no modifiers) |
| **1** | +30% | +30% | Normal | Normal | Normal | Basic enhancement |
| **2** | +60% | +60% | +15% | +20% | +20% | Intermediate |
| **3** | +100% | +100% | +35% | +50% | +50% | Advanced |
| **4** | +150% | +150% | +50% | +80% | +70% | Reserved |
| **5** | +200% | +200% | +70% | +100% | +100% | Reserved |

---

## 🔧 Config Syntax

### Phase-Specific Modifiers
```cfg
<mobId> modifiers phase=<N>: <stat>=<value> <stat>=<value> ...
```

**Example**:
```cfg
68131411 modifiers phase=1: hp=1.3 physAtk=1.3
68131411 modifiers phase=2: hp=1.6 physAtk=1.6 atkSpd=0.85
68131411 modifiers phase=3: hp=2.0 physAtk=2.0 atkSpd=0.65 sizeRate=11
```

### Global Phase Modifiers
Apply to all mobs by using ID `0`:
```cfg
0 modifiers phase=1: hp=1.5 physAtk=1.5
```

---

## 🎯 Blood Palace (86004) Usage

### Boss HP Thresholds

| Boss HP | Phase | Command | Mods Applied |
|---------|-------|---------|--------------|
| 100% → 90% | Default | `@setphase 0` | No modifiers |
| 90% | Phase 1 | `@setphase 1` | +30% HP/Atk |
| 70% | Phase 2 | `@setphase 2` | +60% HP/Atk, faster |
| 50% | Phase 3 | `@setphase 3` | +100% HP/Atk, very fast |

### Affected Mobs
- **Elite Mobs**: 68131411 (Raviel), 68131412 (Despojo), 68131413, 68131414
- **Support Mobs**: 68131390 (Vampa Beetle), 68131391 (SS Broly), 68131393 (Broly)

---

## ⚠️ Important Notes

1. **Phase applies to NEW spawns only**
   - Existing mobs keep their current stats
   - Kill/respawn or wait for natural respawn to see changes

2. **Phase is per-world instance**
   - Each dungeon instance has its own phase
   - Phase resets when instance resets

3. **Attack Speed is inverted**
   - `atkSpd=0.85` = 15% faster
   - `atkSpd=0.65` = 35% faster
   - Lower values = faster attacks

4. **Size Rate**
   - `sizeRate=0` = no change (use mob default)
   - `sizeRate=10` = normal size (100%)
   - `sizeRate=11` = 110% size
   - `sizeRate=15` = 150% size

---

## 🔍 Debugging

### Check Current Phase
No built-in command yet. Ask the GM who set it, or check server logs:
```
[DifficultyPhase] World 86004 phase set to 2 by AdminName
```

### Verify Config Loaded
Check server startup logs for:
```
[CustomDropEvent] Loaded phase=1 modifiers for mob 68131411
[CustomDropEvent] Loaded phase=2 modifiers for mob 68131411
[CustomDropEvent] Loaded phase=3 modifiers for mob 68131411
```

### Test Modifiers
1. Set phase: `@setphase 3`
2. Spawn test mob in same world
3. Check mob HP/stats in-game
4. Expected: Phase 3 = 2x HP, 2x Attack, fast attacks

---

## 📁 Key Files

| File | Purpose |
|------|---------|
| `CustomDropEvent.cfg` | Phase modifier definitions |
| `86004.wps` | Blood Palace script (has phase comments) |
| `World.h` / `World.cpp` | Phase tracking per world |
| `CustomDropEvent.h` / `CustomDropEvent.cpp` | Phase-aware modifier system |
| `Monster.cpp` | Applies phase modifiers on spawn |
| `gm.cpp` | GM command `@setphase` |

---

## 🚀 Quick Start

1. **Start server** and enter Blood Palace
2. **Enable CustomDropEvent**: `@start_customdrop 3` (3 hours)
3. **Set phase**: `@setphase 1`
4. **Trigger mob spawns** (boss HP threshold or manual)
5. **Observe**: New mobs spawn with enhanced stats
6. **Increase phase**: `@setphase 2` → `@setphase 3`
7. **Reset**: `@setphase 0` when done

---

## 📞 Support

For issues or questions, check:
- `86004_WAVE_DIFFICULTY_IMPLEMENTATION_COMPLETE.md` (full technical reference)
- Server logs in `D:\projects\dbo-legacy\OpenDBO-Core\DboServer\ExecutionEnv\log\`
- GitHub issues: https://github.com/OpenDBO-Core/issues

---

**Last Updated**: 2025-10-14
**System Version**: OpenDBO-Core v4
