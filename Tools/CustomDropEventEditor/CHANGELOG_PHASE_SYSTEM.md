# CustomDropEventEditor - PHASE System Update

## Overview
The CustomDropEvent system has been significantly enhanced with new features. This document describes the changes made to support these features in the editor tool.

## New Features Added

### 1. PHASE System Support
Mobs can now have different modifier sets for different difficulty phases.

**Syntax**:
```
<mobId> modifiers phase=<N>: <stats>
```

**Example**:
```
68131410 modifiers phase=1: hp=1.3 physAtk=1.3
68131410 modifiers phase=2: hp=1.6 physAtk=1.6 atkSpd=0.85
68131410 modifiers phase=3: hp=2.0 physAtk=2.0 atkSpd=0.65 restoreHP=1
```

### 2. HP Restoration Control (`restoreHP`)
By default, phase transitions NO LONGER restore mob HP to 100%. The mob keeps its current HP percentage.

**New Field**: `restoreHP=1`
- **Default**: `false` (HP preserved during phase change)
- **When set to 1**: Restores HP to 100% when this modifier is applied

**Example**:
```
# Phase 2 increases stats but DOES NOT restore HP
68131410 modifiers phase=2: hp=1.6 physAtk=1.6

# Phase 3 increases stats AND restores HP to 100%
68131410 modifiers phase=3: hp=2.0 physAtk=2.0 restoreHP=1
```

### 3. Stat Overflow Protection
Attack, defense, and other stats are capped at 65,535 (WORD max) to prevent overflow.

**Stats with caps (WORD = 65,535)**:
- `physAtk`, `engAtk` (attack values)
- `physDef`, `engDef` (defense values)
- `atkSpd` (attack speed rate)
- `physCrit`, `engCrit` (critical rates)
- `attackRate`, `dodgeRate`, `blockRate`, `guardRate`

**Stats with NO caps (float)**:
- `hp` (can be VERY high, e.g., hp=100.0)
- `runSpd`
- `physCritDmg`, `engCritDmg`

**Safe Multiplier Guidelines**:
| Base Attack | Max Safe Multiplier | Example |
|-------------|---------------------|---------|
| 1,000 | 65× | `physAtk=65.0` ✅ |
| 2,000 | 32× | `physAtk=32.0` ✅ |
| 3,000 | 21× | `physAtk=21.0` ✅ |
| 5,000 | 13× | `physAtk=13.0` ✅ |

**Recommended**: Use multipliers ≤ 20 for safety.

---

## Code Changes Made

### ConfigModel.cs

#### 1. Added Phase Support
```csharp
// NEW: Phase-specific modifiers dictionary
public Dictionary<uint, Dictionary<byte, Modifiers>> ModsByPhase { get; } = new();
```

#### 2. Enhanced Parser
- Detects `phase=N` in modifier lines
- Stores phase-specific modifiers separately from regular modifiers
- Example: `"123 modifiers phase=2"` → stored in `ModsByPhase[123][2]`

#### 3. Enhanced Save Method
- Outputs regular modifiers first
- Then outputs all phase-specific modifiers grouped by mob ID and phase

#### 4. Added `RestoreHP` Field to Modifiers
```csharp
public bool RestoreHP { get; set; } = false; // Default: no HP restoration
```

#### 5. Updated ToString() Method
- Only outputs `restoreHP=1` if the flag is true (keeps configs clean)

---

## UI Recommendations

### Suggested UI Enhancements

#### 1. Phase Modifier Editor
Add a phase selector to the modifiers panel:

**UI Elements**:
```
┌─────────────────────────────────────────┐
│ Phase: [ Dropdown: None/1/2/3/4/5 ]    │
│                                         │
│ HP Multiplier:     [1.0    ]           │
│ Phys Attack:       [1.0    ]           │
│ Energy Attack:     [1.0    ]           │
│ ...                                     │
│                                         │
│ ☑ Restore HP to 100% on phase change   │
│                                         │
│ [ Apply Phase Modifiers ]               │
└─────────────────────────────────────────┘
```

**Behavior**:
- When "Phase" is "None" → regular (non-phase) modifiers
- When "Phase" is 1-5 → phase-specific modifiers for that phase
- Show different modifier sets when switching between phases
- Checkbox for `restoreHP` (default unchecked)

#### 2. Stat Overflow Warnings
Add validation for stat caps:

```csharp
private void ValidateStatMultiplier(float baseValue, float multiplier, string statName)
{
    if (statName is "physAtk" or "engAtk" or "physDef" or "engDef" or "atkSpd")
    {
        float result = baseValue * multiplier;
        if (result > 65535f)
        {
            // Show warning tooltip or label
            lblWarning.Text = $"⚠️ {statName} will be capped at 65,535!";
            lblWarning.ForeColor = Color.Orange;
        }
    }
}
```

**Visual Indicators**:
- Yellow warning icon when multiplier would cause overflow
- Tool tip showing: "Will be capped at 65,535"
- Suggested safe max multiplier based on base stats

#### 3. Phase List View
Add a list showing all configured phases for the selected mob:

```
Configured Phases:
┌──────────────────────────────────────┐
│ Phase 1: HP×1.3, PhysAtk×1.3         │
│ Phase 2: HP×1.6, PhysAtk×1.6 🔄      │  ← 🔄 = restoreHP
│ Phase 3: HP×2.0, PhysAtk×2.0 🔄      │
└──────────────────────────────────────┘
```

#### 4. Help Text Updates
Update the help dialog with:
- PHASE system explanation
- `restoreHP` flag description
- Stat cap warnings and safe multiplier ranges
- Examples of phase-specific configurations

---

## Usage Examples for the UI

### Example 1: Blood Palace Boss
```
Mob ID: 68131410

Regular Modifiers (no phase):
  hp=1.0 physAtk=1.0 (base stats)

Phase 1 (at 90% HP):
  hp=1.3 physAtk=1.3
  ☐ Restore HP

Phase 2 (at 70% HP):
  hp=1.6 physAtk=1.6 atkSpd=0.85
  ☐ Restore HP

Phase 3 (at 50% HP):
  hp=2.0 physAtk=2.0 atkSpd=0.65 sizeRate=11
  ☑ Restore HP  ← Only restore HP at final phase
```

### Example 2: Validation Warning
```
Mob ID: 1234 (base physAtk: 5000)

Phase 1:
  physAtk=15.0  ← Result: 75,000

⚠️ Warning: physAtk will overflow!
Suggested: Use physAtk ≤ 13.0 for this mob
```

---

## Testing Checklist

When updating the UI, test:

1. ✅ Loading configs with phase modifiers
2. ✅ Loading configs without phase modifiers (backwards compatibility)
3. ✅ Saving configs with mixed regular + phase modifiers
4. ✅ `restoreHP` flag parsing and output
5. ✅ Phase dropdown functionality
6. ✅ Overflow warnings for attack/defense stats
7. ✅ Multiple phases for same mob
8. ✅ Deleting phase modifiers
9. ✅ Copying phase modifiers between phases

---

## Backend Changes Summary

### Files Modified (Server):
1. `CustomDropEvent.h` - Added `restoreHP` field to Modifiers struct
2. `CustomDropEvent.cpp`:
   - Parser for `phase=N` and `restoreHP=1`
   - Overflow protection for WORD stats
   - HP ratio preservation during phase changes
   - Tracking to prevent double-application of modifiers
3. `CustomDropEvent.cfg` - Updated documentation with new features
4. `PHASE_SYSTEM_QUICK_REFERENCE.md` - New feature documentation

### Files Modified (Editor Tool):
1. `ConfigModel.cs`:
   - Added `ModsByPhase` dictionary
   - Added `RestoreHP` property to Modifiers
   - Enhanced parser to detect `phase=N`
   - Enhanced Save() to output phase modifiers
   - Updated ToString() to include `restoreHP=1` when needed

---

## Migration Notes

### For Existing Configs
- Old configs without `restoreHP` will default to `restoreHP=false`
- This means **HP will be preserved** during phase changes (new behavior)
- To restore old behavior (HP reset), add `restoreHP=1` to your phase modifiers

### Backward Compatibility
- Regular (non-phase) modifiers work exactly as before
- Phase modifiers are optional - you can mix phase and non-phase modifiers
- The editor can load both old and new config formats

---

## Quick Reference

### Config Syntax
```
# Regular modifier (no phase)
<mobId> modifiers: hp=X physAtk=Y ...

# Phase-specific modifier
<mobId> modifiers phase=N: hp=X physAtk=Y ... [restoreHP=1]

# Example with multiple phases
123 modifiers: hp=1.0 physAtk=1.0
123 modifiers phase=1: hp=1.5 physAtk=1.5
123 modifiers phase=2: hp=2.0 physAtk=2.0 restoreHP=1
```

### GM Commands
```
@setphase <N>              # Set world difficulty phase (0-255)
@reload_customdrop [name]  # Reload config (existing mobs keep old stats until respawn)
```

---

## Next Steps for UI Development

1. **Priority 1**: Add phase dropdown to modifier panel
2. **Priority 2**: Add `restoreHP` checkbox
3. **Priority 3**: Add overflow warnings for capped stats
4. **Priority 4**: Add phase list view showing all configured phases
5. **Priority 5**: Update help dialog with new features

**Estimated Development Time**: 4-6 hours for full UI implementation

---

## Support

For questions or issues:
- Check server logs for overflow warnings: `[CustomDropEvent] PhysAtk overflow: mob X`
- Test configs in-game with `@reload_customdrop` command
- Verify phase transitions with `@setphase N` command
