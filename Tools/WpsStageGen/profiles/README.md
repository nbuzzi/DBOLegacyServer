# Dungeon Profiles

This directory contains pre-configured dungeon profile templates that can be used with WpsStageGen to quickly generate custom CCBD dungeons.

## What are Dungeon Profiles?

Dungeon Profiles are JSON configuration files that define all parameters needed to generate a complete dungeon:
- Floor count and boss intervals
- Wave patterns and mob configurations
- Boss mechanics templates
- Arena rotation
- Reward progression
- Custom variables for templates

## Available Profiles

### 1. **default_extension.json**
- **Floors:** 50 (45 regular + 10 boss)
- **Boss Interval:** Every 5 floors
- **Difficulty:** Standard
- **Boss Mechanics:** None (simple boss spawn)
- **Use Case:** Quick extension of existing CCBD with standard difficulty

### 2. **extreme_challenge.json**
- **Floors:** 100 (90 regular + 10 boss)
- **Boss Interval:** Every 10 floors
- **Difficulty:** Extreme
- **Boss Mechanics:** Multi-phase (91%, 71%, 61%, 41%, 25%, 20%)
- **Arena Rotation:** FIRE, ICE, LIGHTNING
- **Reward Progression:** Increasing quality rewards
- **Use Case:** End-game challenge dungeon with complex boss fights

### 3. **training_mode.json**
- **Floors:** 50 (45 regular + 5 boss)
- **Boss Interval:** Every 10 floors
- **Difficulty:** Easy
- **Boss Mechanics:** None (simple boss spawn)
- **Use Case:** Practice dungeon for testing builds and mechanics

## How to Use Profiles

### Method 1: Command Line

```bash
# Navigate to WpsStageGen directory
cd Tools/WpsStageGen

# Generate dungeon from profile
dotnet run -- profile="profiles/extreme_challenge.json" out="83001.wps"
```

### Method 2: Programmatic Loading (C#)

```csharp
using WpsStageGen;

// Load profile from JSON file
var profile = DungeonProfile.LoadFromFile("profiles/extreme_challenge.json");

// Modify if needed
profile.FloorCount = 150;
profile.BossInterval = 5;

// Convert to command-line arguments
string args = profile.ToCommandLineArgs();

// Save modified profile
profile.SaveToFile("profiles/my_custom_dungeon.json");
```

### Method 3: Using WpsStageGen.UI (Future)

The GUI tool will have a "Load Profile" button that allows you to:
1. Browse for .json profile files
2. Load all settings into the UI
3. Modify settings visually
4. Save as new profile or generate WPS directly

## Creating Custom Profiles

### Option 1: Start from Template

```bash
# Copy an existing profile
cp profiles/default_extension.json profiles/my_dungeon.json

# Edit with your favorite editor
# Modify floor count, boss intervals, templates, etc.
```

### Option 2: Create Programmatically

```csharp
var myProfile = new DungeonProfile
{
    Name = "My Custom Dungeon",
    Description = "200 floors with elemental bosses",
    BaseWpsFile = "path/to/83000.wps",
    FloorCount = 200,
    BossInterval = 5,
    WaveConfig = new WaveConfig
    {
        PatternList = "(1, 40%), (2, 40%), (3, 20%)",
        AutoLevel = 60
    },
    BossConfig = new BossConfig
    {
        BossGroupBase = 9100,
        MechanicsTemplate = "templates/boss_phases_91_71_61_41_25_20.wps",
        ArenaRotation = new List<string> { "FIRE", "ICE", "LIGHTNING", "EARTH" },
        RewardItems = new List<int> { 7000005, 7000006, 7000007 },
        IncrementBossGroup = true
    },
    Variables = new VariableConfig
    {
        Variables = new Dictionary<string, string>
        {
            ["INVINCIBLE_BUFF"] = "1900101",
            ["PHASE91_GROUP"] = "401",
            ["PHASE71_GROUP"] = "402"
        }
    }
};

myProfile.SaveToFile("profiles/my_dungeon.json");
```

## Profile Schema

### Root Object
| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Display name for the dungeon |
| `description` | string | Detailed description |
| `baseWpsFile` | string | Path to source WPS file (usually 83000.wps) |
| `floorCount` | int | Total floors to generate |
| `bossInterval` | int | Boss appears every N floors (5, 10, etc.) |
| `startFloor` | int | Starting floor number (0 = auto-detect) |
| `waveConfig` | WaveConfig | Configuration for regular floors |
| `bossConfig` | BossConfig | Configuration for boss floors |
| `variables` | VariableConfig | Template variables |

### WaveConfig Object
| Field | Type | Description |
|-------|------|-------------|
| `patternList` | string | Mob spawn patterns with probabilities |
| `autoLevel` | int | Auto-scaling level (0 = use default) |
| `mobGroupOverride` | int | Override mob group (0 = use pattern) |
| `customTemplate` | string | Path to custom wave template |

**Pattern List Format:**
```
"(pattern_id, percentage), (pattern_id, percentage), ..."
```

**Example:**
```json
"patternList": "(1, 35%), (2, 35%), (3, 15%), (4, 15%)"
```

### BossConfig Object
| Field | Type | Description |
|-------|------|-------------|
| `bossGroupBase` | int | Base boss mob group ID |
| `mechanicsTemplate` | string | Path to boss mechanics template |
| `arenaRotation` | string[] | Array of arena world IDs |
| `rewardItems` | int[] | Array of reward item table IDs |
| `incrementBossGroup` | bool | Auto-increment boss group per stage |

**Boss Mechanics Templates:**
- `null` or empty: Simple boss spawn (no mechanics)
- `"templates/boss_phases_91_71_61_41_25_20.wps"`: Multi-phase raid boss
- Custom path: Your own template file

**Arena Rotation Example:**
```json
"arenaRotation": [
  "ARENA_FIRE",
  "ARENA_ICE",
  "ARENA_LIGHTNING",
  "ARENA_EARTH"
]
```

Bosses will cycle through arenas: Boss 1 → FIRE, Boss 2 → ICE, Boss 3 → LIGHTNING, Boss 4 → EARTH, Boss 5 → FIRE (repeat)

**Reward Progression Example:**
```json
"rewardItems": [7000002, 7000003, 7000004]
```

First boss tier gets 7000002, second tier gets 7000003, third tier gets 7000004, then cycles.

### VariableConfig Object
| Field | Type | Description |
|-------|------|-------------|
| `variables` | object | Key-value pairs for template substitution |
| `varsFilePath` | string | Path to external variables file |

**Variables Example:**
```json
"variables": {
  "INVINCIBLE_BUFF": "1900101",
  "PHASE91_GROUP": "301",
  "PHASE71_GROUP": "302",
  "ENRAGE_BUFF": "1900555"
}
```

These replace `{{VARIABLE_NAME}}` placeholders in template files.

## Template Variables

### Built-in Variables (Auto-generated)
| Variable | Description | Example |
|----------|-------------|---------|
| `{{STAGE}}` | Current stage number | 151 |
| `{{IS_BOSS}}` | Is boss stage | true/false |
| `{{BOSS_GROUP}}` | Boss mob group ID | 9999 |
| `{{REWARD_ITEM}}` | Reward item tblidx | 7000002 |
| `{{ARENA_WORLD}}` | Current boss arena | ARENA_FIRE |
| `{{MARK_LAST_STAGE}}` | Is final boss | true/false |
| `{{BOSS_EVERY}}` | Boss interval setting | 5 |
| `{{START_STAGE}}` | First generated stage | 151 |
| `{{END_STAGE}}` | Last generated stage | 200 |

### Custom Variables
You can define any custom variables in the profile's `variables` section. Common examples:

**Boss Mechanics:**
- `INVINCIBLE_BUFF` - Buff ID for invincibility
- `ENRAGE_BUFF` - Buff ID for enrage
- `PHASE91_GROUP`, `PHASE71_GROUP`, etc. - Mob groups for phases

**Protection Mechanics:**
- `PROTECT_PLATFORM_GROUP` - Platform/pillar mob group
- `PROTECT_ADDONS_GROUP` - Supporting mobs

**Custom Effects:**
- `PLAYER_DEBUFF` - Debuff to apply to players
- `BOSS_HEAL_AMOUNT` - Boss healing amount
- `TIME_LIMIT` - Custom time limit in seconds

## Best Practices

### 1. Boss Intervals
- **5 floors:** Standard CCBD rhythm, frequent boss encounters
- **10 floors:** Slower pace, more regular floor progression
- **3 floors:** Intense, boss-heavy challenge mode

### 2. Floor Counts
- **50 floors:** Quick extension, moderate playtime
- **100 floors:** Full dungeon experience
- **150+ floors:** Epic marathon dungeon
- **Max:** 255 (BYTE limit)

### 3. Boss Mechanics
- **None:** Fast, simple boss fights (training mode)
- **Single Phase:** Enrage at X% HP
- **Multi-Phase:** Complex raid boss with adds

### 4. Reward Progression
Use increasing quality items for motivation:
```json
"rewardItems": [
  7000002,  // Tier 1: Floors 5-15
  7000003,  // Tier 2: Floors 20-30
  7000004,  // Tier 3: Floors 35-45
  7000005   // Tier 4: Floors 50+
]
```

### 5. Arena Rotation
Match arena themes to boss types:
- FIRE arena → Fire-element boss (use fire buff variables)
- ICE arena → Ice-element boss (use freeze debuffs)
- LIGHTNING arena → Thunder boss (use stun mechanics)

## Troubleshooting

### Profile Won't Load
- **Check JSON syntax:** Use a JSON validator (jsonlint.com)
- **Verify file paths:** baseWpsFile and template paths must exist
- **Check quotes:** All strings must use double quotes `""`

### Generated Dungeon Has Errors
- **Boss group not found:** Verify bossGroupBase exists in mob tables
- **Template variables missing:** Check all {{PLACEHOLDERS}} have values
- **Pattern list invalid:** Verify pattern IDs exist in base WPS

### Bosses Don't Spawn
- **Mob group ID:** Must exist in spawn tables
- **Template syntax:** Check boss template WPS syntax
- **Teleport issues:** Verify arena world IDs are valid

## Examples

### Quick 50-Floor Extension
```bash
dotnet run -- profile="profiles/default_extension.json" out="83000_v2.wps"
```

### Custom 200-Floor Marathon
```json
{
  "name": "Marathon Tower",
  "baseWpsFile": "83000.wps",
  "floorCount": 200,
  "bossInterval": 5,
  "bossConfig": {
    "bossGroupBase": 9000,
    "incrementBossGroup": true,
    "rewardItems": [7000002, 7000003, 7000004, 7000005]
  }
}
```

### Elemental Challenge
```json
{
  "name": "Elemental Gauntlet",
  "floorCount": 80,
  "bossInterval": 5,
  "bossConfig": {
    "bossGroupBase": 9100,
    "arenaRotation": ["FIRE", "ICE", "LIGHTNING", "EARTH"],
    "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps"
  },
  "variables": {
    "variables": {
      "FIRE_BURN_BUFF": "1900201",
      "ICE_FREEZE_BUFF": "1900202",
      "LIGHTNING_STUN_BUFF": "1900203",
      "EARTH_ROOT_BUFF": "1900204"
    }
  }
}
```

## Future Enhancements

- **GUI Profile Editor:** Visual profile creation tool
- **Profile Library:** Community-shared profiles
- **Validation:** Pre-generation profile validation
- **Templates:** More boss mechanics templates
- **Dynamic Difficulty:** Auto-scaling based on player count

## Support

For issues or questions:
1. Check this README
2. Review WpsStageGen main README
3. Examine sample profiles in this directory
4. Check WPS template documentation

---

**Version:** 1.0
**Last Updated:** 2025-01-16
**Compatibility:** WpsStageGen v4.0+
