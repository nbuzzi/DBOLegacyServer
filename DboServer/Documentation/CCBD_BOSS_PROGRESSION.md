# CCBD Boss Floor Progression Feature

## Overview
Implemented progressive boss floor system for CCBD (Continuous Challenge Battle Dungeon) when Boss-Only Mode is enabled. Players now advance through boss floors (5 → 10 → 15 → 20 → 25...) upon re-entry instead of always starting at floor 5.

## Implementation Details

### Files Modified

#### 1. `CPlayer.h` (Lines 167-169, 274-276)
**Added Member Variable:**
```cpp
// CCBD Boss-Only Mode progression tracking
BYTE m_byCCBDLastBossStageCleared; // Last boss floor cleared (5, 10, 15, 20...)
```

**Added Accessors:**
```cpp
// CCBD Boss-Only Mode progression
inline void SetCCBDLastBossStageCleared(BYTE byStage) { m_byCCBDLastBossStageCleared = byStage; }
inline BYTE GetCCBDLastBossStageCleared() const { return m_byCCBDLastBossStageCleared; }
```

#### 2. `CPlayer.cpp` (Line 797)
**Initialized in Constructor:**
```cpp
// CCBD Boss-Only Mode progression tracking
m_byCCBDLastBossStageCleared = 0; // 0 = not yet cleared any boss floor
```

#### 3. `PacketGameServer.cpp` (Lines 11800-11807) - **OPTIMIZED**
**Modified Entry Logic:**
```cpp
// CCBD Boss-Only Mode: Progressive boss floor system (5→10→15→20...)
if (g_pDungeonConfig->IsCCBDBossOnlyModeEnabled())
{
    // Get saved progress from player (stage_clear saves next boss floor)
    BYTE byNextBossFloor = cPlayer->GetCCBDLastBossStageCleared();
    byBeginStage = (byNextBossFloor == 0) ? 5 : byNextBossFloor;
}
```
**Note:** Stage clear saves `clearedStage + 5` (next boss floor) directly, eliminating the need to add 5 here.

#### 4. `WpsScriptAlgoAction_CCBD_stage_clear.cpp` (Lines 7, 40-63)
**Added Include:**
```cpp
#include "DungeonConfig.h" // for boss-only mode check
```

**Added Tracking Logic in OnUpdate():**
```cpp
// Update CCBD Boss-Only Mode progression tracking
if (g_pDungeonConfig && g_pDungeonConfig->IsCCBDBossOnlyModeEnabled())
{
    BYTE byClearedStage = GetOwner()->GetCCBDStage();
    // Only track boss floors (stages 5, 10, 15, 20, 25...)
    if (byClearedStage % 5 == 0 && byClearedStage >= 5)
    {
        // Update all players' last cleared boss stage
        // Calculate and save NEXT boss floor (clearedStage + 5)
        BYTE byNextBossFloor = byClearedStage + 5; // 5→10, 10→15, 15→20...
        
        CPlayer* pPlayer = GetOwner()->GetPlayersFirst();
        while (pPlayer)
        {
            if (pPlayer->GetWorldID() == GetOwner()->GetWorld()->GetID())
            {
                // Save next boss floor directly (optimization!)
                pPlayer->SetCCBDLastBossStageCleared(byNextBossFloor);
                NTL_PRINT(PRINT_APP, L"[CCBD_BOSS_MODE] Player %u cleared floor %u, next: %u", 
                    pPlayer->GetCharID(), byClearedStage, byNextBossFloor);
            }
            pPlayer = GetOwner()->GetPlayersNext();
        }
        
        // Players will exit normally and re-enter at next boss floor
    }
}
```

## How It Works

### Session-Based Tracking
- **Scope:** Tracks progression during the game session (in-memory only)
- **Persistence:** Resets when player logs out
- **Initial State:** `m_byCCBDLastBossStageCleared = 0` on login

### Entry Point Calculation **[OPTIMIZED]**
When player presses "CHALLENGE" at the CCBD NPC with Boss-Only Mode enabled:
1. **First entry:** `byNextBossFloor = 0` → Start at floor 5
2. **After clearing floor 5:** `byNextBossFloor = 10` (saved by stage_clear) → Next entry starts at floor 10
3. **After clearing floor 10:** `byNextBossFloor = 15` (saved by stage_clear) → Next entry starts at floor 15

**Optimization:** Stage clear now saves `clearedStage + 5` directly, so the entry point doesn't need to calculate anything!
4. **Pattern continues:** Floor = `LastCleared + 5`

### Stage Clear Detection
When `Action( "CCBD stage clear" )` executes in WPS script:
1. Checks if Boss-Only Mode is enabled
2. Gets current stage from `GetOwner()->GetCCBDStage()`
3. Validates it's a boss floor (`stage % 5 == 0 && stage >= 5`)
4. Calculates **next boss floor** (`clearedStage + 5`)
5. Saves next boss floor to all party members in the dungeon
6. **CRITICAL FIX:** Forces stage to `nextBossFloor - 1` to prevent race condition

**Race Condition Fix:**
When player presses "CHALLENGE" after killing a boss, the system does an **internal teleport** to continue. Without the stage jump fix, there's a race condition:
- Party system triggers teleport to "normal stage location"
- WPS advances sequentially (5→6)
- Player gets teleported to floor 6 location (wrong!)
- `OnEnter()` tries to adjust stage 6→10 (too late, teleport already happened)
- Client shows teleport error, player becomes stuck

**Solution:**
Force the stage to `nextBossFloor - 1` immediately after boss clear:
```cpp
// Cleared floor 5 → Next should be 10
BYTE byJumpStage = byNextBossFloor - 1; // = 9
GetOwner()->SetCCBDStage(byJumpStage);
// When WPS advances: 9→10 (correct boss floor!)
```

This ensures the teleport happens to the correct boss location.

## Configuration Dependency
Feature activates only when:
```cpp
g_pDungeonConfig->IsCCBDBossOnlyModeEnabled() == true
```

This config key must be set in `DungeonConfig` (typically loaded from an INI file).

## Logging
When boss floor is cleared:
```
ERR_LOG(LOG_BOTAI, "[CCBD Boss Mode] Player %u cleared boss floor %u (current stage: %u)", 
        charId, clearedBossFloor, currentStage);
```
Example: `[CCBD Boss Mode] Player 12345 cleared boss floor 5 (current stage: 6)`

## Limitations & Future Enhancements

### Current Limitations
1. **No Database Persistence:** Progress resets on logout/disconnect
2. **Session-Only:** Cannot track lifetime progression across days/weeks
3. **No Cap:** Floors can theoretically go infinitely (5, 10, 15, 20... 995, 1000...)

### Potential Enhancements
If database persistence is desired:
1. Add column to `characters` table: `CCBDBossLastStage TINYINT UNSIGNED DEFAULT 0`
2. Save/load in player serialization code
3. Optionally add weekly/monthly reset logic
4. Add max floor cap (e.g., floor 100) with reward at peak

## Testing Checklist
- [ ] Boss-Only Mode flag enabled in config
- [ ] First entry starts at floor 5
- [ ] After clearing floor 5, re-entry starts at floor 10
- [ ] After clearing floor 10, re-entry starts at floor 15
- [ ] Progression continues correctly through floors 20, 25, 30...
- [ ] All party members receive tracking update
- [ ] EVENT_VLOG logs appear for each boss clear
- [ ] Normal (non-boss) stages don't update tracking
- [ ] Tracking resets to 0 on logout/login

## Related Files
- `DungeonConfig.h/cpp` - Boss-Only Mode flag configuration
- `WpsAlgoObject.h` - CCBD stage tracking (m_byCCBDStage, m_byCCBDStartStage)
- `BattleDungeon.h/cpp` - Dungeon instance management
- `83000.wps` - CCBD WPS script with stage actions

## Code Markers
No `AI-NO-EDIT` markers applied - this is new feature code, not protected legacy logic.

---
_Feature added: 2025-01-XX_  
_Last updated: 2025-01-XX_
