# CustomDropEvent - Wave-Based Progressive Difficulty System

## Overview
Add support for applying **different modifier tiers based on dungeon wave/phase progression** without changing mob IDs. The server will track which "wave/phase" the dungeon is in and apply progressively stronger modifiers as phases advance.

## Design Goals
1. **Single mob ID** - Same mob IDs throughout (68131411-68131414, etc.)
2. **Progressive scaling** - Mobs get stronger as phases progress (90% → 70% → 50%)
3. **Dynamic application** - Modifiers change based on current phase
4. **Minimal WPS changes** - WPS only needs to signal phase transitions
5. **Backward compatible** - Doesn't break existing CustomDropEvent functionality

## Implementation Strategy

### Option 1: World-State Phase Tracking (Recommended)
Track the current "difficulty phase" per-world in the server, and apply modifiers based on that phase when mobs spawn.

### Option 2: Mob-Group-Based Phase Detection
Detect which phase a mob belongs to based on its spawn group (86025) and apply modifiers accordingly.

### Option 3: Time-Based Progressive Scaling
Apply progressively stronger modifiers based on how long the dungeon has been active.

## Recommended Implementation: Option 1 - World Phase Tracking

### Step 1: Add Phase Tracking to World

**File: `CWorld.h`** (or similar world class)
```cpp
// Add to CWorld or equivalent class
private:
    BYTE m_byDifficultyPhase; // 0=none, 1=phase1(90%), 2=phase2(70%), 3=phase3(50%), etc.

public:
    void SetDifficultyPhase(BYTE phase) { m_byDifficultyPhase = phase; }
    BYTE GetDifficultyPhase() const { return m_byDifficultyPhase; }
```

### Step 2: Extend CustomDropEvent.cfg Format

Add new syntax for **phase-specific modifiers**:

```cfg
# Phase-based modifiers for Blood Palace (world 86004)
# Format: mobId modifiers phase=N: <modifiers>

# Tier 1: Phase 1 (90% HP) - Basic Enhancement
68131411 modifiers phase=1: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1
68131412 modifiers phase=1: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1
68131413 modifiers phase=1: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1
68131414 modifiers phase=1: hp=1.3 physAtk=1.3 engAtk=1.3 physDef=1 engDef=1 atkSpd=1 runSpd=1

# Tier 2: Phase 2 (70% HP) - Intermediate Enhancement
68131411 modifiers phase=2: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2
68131412 modifiers phase=2: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2
68131413 modifiers phase=2: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2
68131414 modifiers phase=2: hp=1.6 physAtk=1.6 engAtk=1.6 physDef=1.2 engDef=1.2 atkSpd=0.85 runSpd=1 physCrit=1.2 engCrit=1.2

# Tier 3: Phase 3 (50% HP) - Advanced Enhancement
68131411 modifiers phase=3: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5
68131412 modifiers phase=3: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5
68131413 modifiers phase=3: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5
68131414 modifiers phase=3: hp=2.0 physAtk=2.0 engAtk=2.0 physDef=1.5 engDef=1.5 atkSpd=0.65 runSpd=1 physCrit=1.5 engCrit=1.5 physCritDmg=1.5 engCritDmg=1.5
```

### Step 3: Modify CustomDropEvent.h

```cpp
// Add phase-aware modifiers storage
class CCustomDropEvent : public CNtlSingleton<CCustomDropEvent>
{
    // ... existing code ...

private:
    // NEW: Store modifiers per mob per phase
    // Map: mobTblidx -> phase -> Modifiers
    std::unordered_map<unsigned int, std::unordered_map<BYTE, Modifiers>> m_mobModsByPhase;

public:
    // NEW: Apply modifiers with phase awareness
    void ApplyModifiersWithPhase(CMonster *pMob, BYTE byPhase);
};
```

### Step 4: Modify CustomDropEvent.cpp Config Loader

```cpp
// In LoadConfigInternal(), detect "phase=N" in modifiers line:

if (isMods)
{
    // Check for optional phase specifier
    BYTE phase = 0; // 0 = no phase (original behavior)

    // Look for "phase=N" keyword before colon
    char* phaseKey = strstr(p, "phase=");
    if (phaseKey && phaseKey < colon)
    {
        phase = (BYTE)atoi(phaseKey + 6); // Extract phase number
    }

    // Parse modifiers (existing code)
    Modifiers m;
    // ... existing modifier parsing ...

    // Store based on whether phase is specified
    if (phase > 0)
    {
        // Phase-specific modifier
        m_mobModsByPhase[mobId][phase] = m;
    }
    else
    {
        // Global/default modifier (existing behavior)
        m_mobMods[mobId] = m;
    }
}
```

### Step 5: Modify ApplyModifiers() to Use Phase

```cpp
void CCustomDropEvent::ApplyModifiersWithPhase(CMonster *pMob, BYTE byPhase)
{
    if (!m_bOn)
        return;

    Modifiers m; // start with identity

    // Priority order:
    // 1. Phase-specific modifier for this mob
    // 2. Phase-specific global modifier (mobId=0)
    // 3. Non-phase-specific modifier (original behavior)

    auto itPhase = m_mobModsByPhase.find(pMob->GetTblidx());
    if (itPhase != m_mobModsByPhase.end())
    {
        auto itPhaseData = itPhase->second.find(byPhase);
        if (itPhaseData != itPhase->second.end())
        {
            // Use phase-specific modifier
            const Modifiers& pm = itPhaseData->second;
            m.hp *= pm.hp;
            m.physAtk *= pm.physAtk;
            // ... apply all modifiers ...
        }
    }

    // Fall back to original non-phase-aware modifiers if no phase match
    if (m.IsIdentity())
    {
        // Use existing ApplyModifiers() logic
        ApplyModifiers(pMob);
        return;
    }

    // ... rest of existing modifier application code ...
}
```

### Step 6: GM Command to Set World Phase

Add a new GM command to set the current difficulty phase:

```cpp
// In GM command handler
void CGMCommandHandler::HandleSetDifficultyPhase(const char* args)
{
    BYTE phase = (BYTE)atoi(args);
    if (phase > 5)
        phase = 5; // cap at phase 5

    // Get current world
    CWorld* pWorld = pPlayer->GetCurWorld();
    if (!pWorld)
        return;

    pWorld->SetDifficultyPhase(phase);

    // Notify player
    SendSystemMessage(pPlayer, "Difficulty phase set to %u", phase);

    // Log for debugging
    ERR_LOG(LOG_GENERAL, "[DifficultyPhase] World %u phase set to %u",
            pWorld->GetWorldTblidx(), phase);
}
```

**GM Command:** `@setphase 2` or `@difficulty 2`

### Step 7: WPS Integration via SendEventToWPS

**In 86004.wps**, use custom WPS events to signal phase transitions:

```lua
-- At 90% HP phase
Action( "send event to wps" )
--[
    Param( "type", "SET_DIFFICULTY_PHASE" )
    Param( "value", 1 )  -- Phase 1
--]
End()

-- At 70% HP phase
Action( "send event to wps" )
--[
    Param( "type", "SET_DIFFICULTY_PHASE" )
    Param( "value", 2 )  -- Phase 2
--]
End()

-- At 50% HP phase
Action( "send event to wps" )
--[
    Param( "type", "SET_DIFFICULTY_PHASE" )
    Param( "value", 3 )  -- Phase 3
--]
End()
```

## Alternative: Simpler Approach Without Code Changes

### Use Existing Group-Based Detection

If you don't want to modify C++ code, use a **simpler approach**:

1. **Use different spawn groups per phase:**
   - 90% HP: Spawn mobs to group **86025**
   - 70% HP: Spawn mobs to group **86026**
   - 50% HP: Spawn mobs to group **86027**

2. **Configure CustomDropEvent.cfg with world-specific modifiers:**

```cfg
# Use world+group combination to detect phase
# When mobs spawn in world 86004 (Blood Palace) in group 86025, apply Tier 1
# This requires checking spawn group in ApplyModifiers()

# Alternatively, use existing mob-level system:
# Just configure DIFFERENT base modifiers per mob ID when spawning

# Phase-aware config without code changes:
# Use "spawn" feature to replace mobs dynamically
68131411 spawn: 68131421@100  # At phase 2, spawn tier-2 variant
68131412 spawn: 68131422@100
68131413 spawn: 68131423@100
68131414 spawn: 68131424@100
```

## Testing Plan

1. **Without CustomDropEvent enabled:**
   - Mobs spawn with base stats
   - No progressive scaling

2. **With CustomDropEvent enabled + Phase 1:**
   - Mobs get +30% stats (moderate difficulty)
   - Use: `@setphase 1`

3. **Transition to Phase 2:**
   - Trigger 70% HP threshold
   - New spawns get +60% stats, faster attacks
   - OLD mobs keep Phase 1 stats (unless retroactively updated)

4. **Transition to Phase 3:**
   - Trigger 50% HP threshold
   - New spawns get +100% stats, high crits
   - Combat becomes very difficult

## Difficulty Progression Table

| Phase | When | HP | Phys Atk | Atk Speed | Crit | Defense | Visual Size |
|-------|------|----|-----------|-----------| -----|---------|-------------|
| 0     | Default | +0% | +0% | Normal | Normal | Normal | Normal |
| 1     | 90% HP | +30% | +30% | Normal | Normal | Normal | Normal |
| 2     | 70% HP | +60% | +60% | 15% faster | +20% | +20% | Normal |
| 3     | 50% HP | +100% | +100% | 35% faster | +50% | +50% | +10% bigger |
| 4     | 30% HP | +150% | +150% | 50% faster | +80% | +70% | +15% bigger |
| 5     | 10% HP | +200% | +200% | 70% faster | +100% | +100% | +20% bigger |

## Benefits

1. **No mob ID changes needed** - Same mobs throughout
2. **Dynamic difficulty** - Scales with dungeon progression
3. **Server-side control** - No client patches needed
4. **Flexible** - Easy to adjust modifiers via config
5. **Extensible** - Can add more phases easily

## Next Steps

Would you like me to:
1. **Implement the full C++ code changes** (Phase tracking + GM command)?
2. **Create the updated CustomDropEvent.cfg** with phase-specific modifiers?
3. **Generate the WPS event triggers** to set phases at HP thresholds?
4. **Use the simpler group-based approach** (no C++ changes)?
5. **All of the above**?
