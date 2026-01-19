# CCBD WPS Stage Generator - Template Guide

## Overview
This guide explains how to use the WPS Stage Generator to create new Crazy Casino floors beyond stage 150, with support for custom boss mechanics, patterns, and special floor effects.

## Recent Updates (v4)
- **Backend now supports up to 255 stages** (was limited to 150)
- **Dynamic boss detection** - no more hardcoded boss stages
- **Arena cycling** - boss arenas automatically cycle if you exceed configured arenas
- **Flexible template system** - easily add custom mechanics

## Quick Start

### Basic Usage - Generate Simple Floors
```bash
WpsStageGen in="83000.wps" out="83000_extended.wps" add=50 start=151 bossEvery=5
```

### Advanced Usage - With Templates and Custom Mechanics
```bash
WpsStageGen in="83000.wps" out="83000_extended.wps" add=50 start=151 bossEvery=5 bossGroup=9999 rewardItem=7000002 pattern="(1,35%),(2,35%),(3,10%),(4,10%),(6,10%)" bossWorlds=ARENA_A,ARENA_B,ARENA_C bossTemplate="templates/boss_phases.wps" regularTemplate="templates/regular_enhanced.wps" varsFile="templates/sample_vars.ini"
```

## Parameters

| Parameter | Description | Default | Example |
|-----------|-------------|---------|---------|
| `in` | Input WPS file path | *Required* | `in="83000.wps"` |
| `out` | Output WPS file path | Same as input | `out="83000_new.wps"` |
| `add` | Number of stages to add | 5 | `add=50` |
| `start` | Starting stage number | Auto (max+1) | `start=151` |
| `bossEvery` | Boss appears every N floors | 5 | `bossEvery=5` |
| `bossGroup` | Boss mob group ID | 9999 | `bossGroup=9999` |
| `rewardItem` | Reward item table ID | 7000002 | `rewardItem=7000003` |
| `pattern` | Mob spawn pattern list | See default | `pattern="(1,40%),(2,40%),(3,20%)"` |
| `bossWorlds` | Boss arena rotation | None | `bossWorlds=A,B,C` |
| `bossTemplate` | Boss mechanics template | None | `bossTemplate="boss_phases.wps"` |
| `regularTemplate` | Regular floor template | None | `regularTemplate="regular_enhanced.wps"` |
| `varsFile` | Custom variables file | None | `varsFile="sample_vars.ini"` |

## Template System

### Built-in Variables
All templates support these placeholders:
- `{{STAGE}}` - Current stage number
- `{{BOSS_GROUP}}` - Boss mob group ID
- `{{REWARD_ITEM}}` - Reward item ID
- `{{ARENA_WORLD}}` - Boss arena world ID
- `{{START_STAGE}}` - First stage in generation
- `{{END_STAGE}}` - Last stage in generation
- `{{BOSS_EVERY}}` - Boss interval
- `{{IS_BOSS}}` - "true" or "false"
- `{{MARK_LAST_STAGE}}` - "true" or "false"

### Custom Variables
Define in `varsFile` (INI format):
```ini
INVINCIBLE_BUFF=1900101
ENRAGE_BUFF=1900201
PHASE2_HP_THRESHOLD=75
```

Or pass inline:
```bash
var.CUSTOM_VALUE=123 var.ANOTHER_VAR="some text"
```

## Example Templates

### Boss Template - Multi-Phase Boss (`boss_phases.wps`)
This template adds 4-phase boss mechanics:
- **Phase 1 (100%-75%)**: Normal difficulty
- **Phase 2 (75%-50%)**: Boss becomes invincible briefly, spawns adds
- **Phase 3 (50%-25%)**: Enrage mode, increased damage
- **Phase 4 (<25%)**: Final phase, massive add spawns

Features:
- HP-triggered phase transitions
- Invincibility periods during transitions
- Reinforcement spawns
- System messages for each phase
- Enrage mechanics

### Regular Floor Template (`regular_enhanced.wps`)
Adds enhanced mechanics to regular floors:
- **Elite mob chance** (10% after clearing first wave)
- **Time tracking** for performance bonuses
- **Death counter** for perfect clear rewards
- **Fast clear bonus** (under 60 seconds)
- **Environmental hazards** (optional, commented out)

### Variables File (`sample_vars.ini`)
Pre-configured buff IDs, mob IDs, message IDs, and thresholds for easy customization.

## Creating Custom Boss Mechanics

### Example: Boss with Shield Phase
Create `boss_shield.wps`:
```lua
-- Shield phase at 50% HP
Action( "while" )
--[
    Condition( "check LP" )
    --[
        Param( "target", "mob" )
        Param( "mobindex", {{BOSS_GROUP}} )
        Param( "condition", 2 )
        Param( "value", 50 )
    --]
    End()

    Action( "then" )
    --[
        Action( "system message" )
        --[
            Param( "index", 9810 )
        --]
        End()

        -- Add shield mobs that must be killed
        Action( "add mobgroup" )
        --[
            Param( "group", {{BOSS_GROUP}} + 10 )
            Param( "no spawn wait", "true" )
        --]
        End()

        -- Boss invincible until shields down
        Action( "register buff" )
        --[
            Param( "index", {{BOSS_GROUP}} )
            Param( "tblidx", {{INVINCIBLE_BUFF}} )
            Param( "keep time", -1 ) -- Permanent
        --]
        End()

        -- Wait for shield mobs to die
        Action( "wait" )
        --[
            Condition( "check mobgroup" )
            --[
                Param( "type", 2 )
                Param( "group", {{BOSS_GROUP}} + 10 )
            --]
            End()
        --]
        End()

        -- Remove invincibility
        Action( "unregister buff" )
        --[
            Param( "index", {{BOSS_GROUP}} )
            Param( "tblidx", {{INVINCIBLE_BUFF}} )
        --]
        End()
    --]
    End()
--]
End()
```

Use it:
```bash
WpsStageGen in="83000.wps" add=10 start=151 bossTemplate="boss_shield.wps" var.INVINCIBLE_BUFF=1900101
```

## Pattern Lists

Pattern lists define mob spawns for regular floors using format: `(pattern_id, probability%)`

Example patterns from 83000.wps:
- **Pattern 1-4**: Standard mob groups (4 mobs per spawn)
- **Pattern 5**: Boss-tier single mob
- **Pattern 6**: Elite mix

Default: `"(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)"`

Custom example:
```bash
pattern="(1, 50%), (3, 30%), (6, 20%)"
```

## Boss Arena Rotation

Specify arena worlds to cycle through for boss stages:
```bash
bossWorlds=ARENA_A,ARENA_B,ARENA_C
```

The generator will:
1. Cycle through arenas for each boss stage
2. Add a comment in WPS: `-- Boss arena: ARENA_A`
3. You can use `{{ARENA_WORLD}}` in templates for teleport logic

**Note**: The backend now supports arena cycling automatically if you exceed 51 configured arenas.

## Workflow: Adding Floors 151-200

### Step 1: Prepare
```bash
cd Tools/WpsStageGen/bin/Release/net8.0/win-x64
```

### Step 2: Generate
```bash
WpsStageGen.exe in="path/to/83000.wps" out="path/to/83000_v2.wps" add=50 start=151 bossEvery=5 bossGroup=9999 rewardItem=7000002 pattern="(1,35%),(2,35%),(3,10%),(4,10%),(6,10%)" bossTemplate="templates/boss_phases.wps" regularTemplate="templates/regular_enhanced.wps" varsFile="templates/sample_vars.ini"
```

### Step 3: Review Output
- Check `83000_v2.wps` for generated stages
- Verify last stage has `Param( "last stage", "true" )`
- Test in game

### Step 4: Iterate
- Modify templates for better mechanics
- Adjust mob groups in patterns
- Tune HP thresholds in variables file
- Re-generate and test

## Advanced Techniques

### Progressive Difficulty
Generate in batches with increasing rewards:
```bash
# Stages 151-175
WpsStageGen in="83000.wps" add=25 start=151 rewardItem=7000003

# Stages 176-200 (harder)
WpsStageGen in="83000.wps" add=25 start=176 rewardItem=7000004 var.DAMAGE_MULTIPLIER=150
```

### Special Boss Every 10 Floors
```bash
WpsStageGen in="83000.wps" add=20 start=151 bossEvery=10
```

### Custom Mob Groups Per Stage Range
Create multiple templates and generate in segments with different patterns.

## Troubleshooting

### "No CCBD stages found"
- Input file must contain existing CCBD stages
- Ensure file path is correct

### Boss doesn't spawn
- Check `bossGroup` ID exists in mob tables
- Verify mob group is configured correctly

### Template variables not replaced
- Ensure proper syntax: `{{VAR_NAME}}` (double curly braces)
- Check variable is defined in varsFile or command line

### Arena teleport fails
- Verify ServerConfigTable has sufficient boss arena entries
- Backend now cycles arenas automatically, but check arena configurations exist

## Backend Changes Summary

### Modified Files
1. **NtlCCBD.h** - `CCBD_MAX_STAGE` increased from 100 to 255
2. **NtlCCBD.cpp** - `IsCCBDBossStage()` now uses modulo calculation (dynamic)
3. **ServerConfigTable.h** - `ENTER_BOSS_STATE_LOC_COUNT` increased from 30 to 51
4. **WpsScriptAlgoAction_CCBD_stage.cpp** - Added arena cycling logic

### Server Restart Required
After modifying WPS files, restart the GameServer to load new stages.

## Tips & Best Practices

1. **Backup original WPS** before generating
2. **Test incrementally** - add 10-20 floors at a time
3. **Use templates** for consistency across floors
4. **Balance difficulty** - increase HP/damage gradually
5. **Provide visual feedback** - use system messages for events
6. **Version your WPS files** - e.g., `83000_v1.wps`, `83000_v2.wps`

## Support

For issues or questions:
1. Check this README
2. Review example templates
3. Test with minimal parameters first
4. Gradually add complexity

## Credits
- Original CCBD system by DBO Legacy team
- WPS Generator tool by server developers
- Template system expansion for v4 update
