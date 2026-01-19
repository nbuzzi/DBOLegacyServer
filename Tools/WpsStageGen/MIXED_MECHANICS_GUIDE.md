# Mixed Boss Mechanics Guide

## Overview

The Enhanced Dungeon Profile system now supports **per-boss customization**, allowing you to mix different boss mechanics within a single dungeon. Each boss can have:
- Unique mechanics template (simple, enrage, multi-phase, or custom)
- Different arena/world
- Custom reward items
- Boss-specific variables
- Individual mob group IDs

## Why Use Mixed Mechanics?

**Progressive Difficulty**: Start with simple bosses and gradually introduce complex mechanics
**Variety**: Keep players engaged with different boss patterns
**Themed Encounters**: Match mechanics to arena themes (fire boss = enrage, ice boss = phases)
**Testing**: Mix mechanics to find what works best for your server
**Multi-Instance Safe**: Each dungeon has unique WPS ID, safe for multiple server instances

## Key Concepts

### 1. WPS ID Management

**CRITICAL**: Each dungeon needs a **unique WPS ID**.

```json
{
  "wpsId": 83001,  // Must be unique across ALL server instances
  "name": "My Custom Dungeon"
}
```

**WPS ID Rules:**
- 83000: Reserved for original CCBD
- 83001+: Available for custom dungeons
- Must be unique even across multiple game server instances
- All instances share the same WPS folder

**Helper Methods:**
```csharp
// Find next available WPS ID
int nextId = DungeonProfile.FindNextAvailableWpsId(wpsDirectory);

// Check if WPS ID is available
bool available = DungeonProfile.IsWpsIdAvailable(83005, wpsDirectory);

// Get all existing WPS IDs
List<int> existingIds = DungeonProfile.GetExistingWpsIds(wpsDirectory);
```

### 2. Global vs Individual Boss Configuration

**Global Configuration** (applies to all bosses by default):
```json
{
  "bossConfig": {
    "bossGroupBase": 9999,
    "mechanicsTemplate": "templates/boss_phases.wps",
    "arenaRotation": ["FIRE", "ICE", "LIGHTNING"],
    "rewardItems": [7000002, 7000003],
    "incrementBossGroup": true
  }
}
```

**Individual Boss Override** (overrides global for specific floors):
```json
{
  "bossConfig": {
    "individualBosses": [
      {
        "floor": 10,
        "bossGroup": 9101,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_FIRE",
        "rewardItem": 7000003,
        "variables": {
          "ENRAGE_BUFF": "1900555"
        },
        "description": "Fire boss with enrage"
      }
    ]
  }
}
```

### 3. Boss Mechanics Templates

**Available Templates:**

| Template | Complexity | Description |
|----------|------------|-------------|
| `null` (none) | Simple | Basic boss spawn, no mechanics |
| `boss_simple_enrage.wps` | Medium | Enrage at configurable HP threshold |
| `boss_phases_91_71_61_41_25_20.wps` | Complex | 6 HP-triggered phases with adds |
| Custom | Variable | Your own template file |

## Complete Example: Elemental Gauntlet

Create a 50-floor dungeon with 10 bosses, each with unique mechanics:

```json
{
  "name": "Elemental Gauntlet",
  "description": "50 floors with themed elemental bosses",
  "wpsId": 83002,
  "baseWpsFile": "83000.wps",
  "floorCount": 50,
  "bossInterval": 5,

  "waveConfig": {
    "patternList": "(1, 40%), (2, 40%), (3, 20%)",
    "autoLevel": 50
  },

  "bossConfig": {
    "bossGroupBase": 9200,
    "incrementBossGroup": true,
    "mechanicsTemplate": null,
    "rewardItems": [7000010, 7000011, 7000012],

    "individualBosses": [
      {
        "floor": 5,
        "bossGroup": 9200,
        "mechanicsTemplate": null,
        "arena": "TRAINING_GROUNDS",
        "rewardItem": 7000010,
        "description": "Tutorial boss - simple mechanics"
      },
      {
        "floor": 10,
        "bossGroup": 9201,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_FIRE",
        "rewardItem": 7000010,
        "variables": {
          "ENRAGE_BUFF": "1900301",
          "FIRE_DOT_BUFF": "1900302",
          "ENRAGE_HP_THRESHOLD": "35"
        },
        "description": "Fire Elemental - enrages at 35%, applies burn DoT"
      },
      {
        "floor": 15,
        "bossGroup": 9202,
        "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps",
        "arena": "ARENA_ICE",
        "rewardItem": 7000011,
        "variables": {
          "INVINCIBLE_BUFF": "1900101",
          "PHASE91_GROUP": "401",
          "PHASE71_GROUP": "402",
          "PHASE61_GROUP": "403",
          "PHASE41_GROUP": "404",
          "PHASE25_GROUP": "405",
          "PHASE20_GROUP": "406",
          "ICE_FREEZE_DEBUFF": "1800303"
        },
        "description": "Ice Titan - 6-phase mechanics, spawns ice minions, freeze debuff"
      },
      {
        "floor": 20,
        "bossGroup": 9203,
        "mechanicsTemplate": null,
        "arena": "ARENA_LIGHTNING",
        "rewardItem": 7000011,
        "variables": {
          "LIGHTNING_STUN_CHANCE": "25"
        },
        "description": "Lightning Djinn - fast attacks with stun chance"
      },
      {
        "floor": 25,
        "bossGroup": 9204,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_EARTH",
        "rewardItem": 7000011,
        "variables": {
          "ENRAGE_BUFF": "1900304",
          "EARTH_ROOT_DEBUFF": "1800305",
          "ENRAGE_HP_THRESHOLD": "30"
        },
        "description": "Earth Golem - heavy armor, enrages at 30%, roots players"
      },
      {
        "floor": 30,
        "bossGroup": 9205,
        "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps",
        "arena": "ARENA_VOID",
        "rewardItem": 7000012,
        "variables": {
          "INVINCIBLE_BUFF": "1900101",
          "PHASE91_GROUP": "411",
          "PHASE71_GROUP": "412",
          "PHASE61_GROUP": "413",
          "PHASE41_GROUP": "414",
          "PHASE25_GROUP": "415",
          "PHASE20_GROUP": "416",
          "VOID_DAMAGE_AMP": "1800306"
        },
        "description": "Void Entity - mid-boss with reality-warping mechanics"
      },
      {
        "floor": 35,
        "bossGroup": 9206,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_FIRE",
        "rewardItem": 7000012,
        "variables": {
          "ENRAGE_BUFF": "1900307",
          "ENRAGE_HP_THRESHOLD": "25"
        },
        "description": "Inferno Drake - fire damage, hard enrage at 25%"
      },
      {
        "floor": 40,
        "bossGroup": 9207,
        "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps",
        "arena": "ARENA_ICE",
        "rewardItem": 7000012,
        "variables": {
          "INVINCIBLE_BUFF": "1900101",
          "PHASE91_GROUP": "421",
          "PHASE71_GROUP": "422",
          "PHASE61_GROUP": "423",
          "PHASE41_GROUP": "424",
          "PHASE25_GROUP": "425",
          "PHASE20_GROUP": "426"
        },
        "description": "Frost Wyrm - advanced phase mechanics with aerial attacks"
      },
      {
        "floor": 45,
        "bossGroup": 9208,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_LIGHTNING",
        "rewardItem": 7000012,
        "variables": {
          "ENRAGE_BUFF": "1900308",
          "ENRAGE_HP_THRESHOLD": "20"
        },
        "description": "Storm Leviathan - fast, aggressive, early enrage"
      },
      {
        "floor": 50,
        "bossGroup": 9209,
        "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps",
        "arena": "FINAL_BOSS_ARENA",
        "rewardItem": 7000013,
        "variables": {
          "INVINCIBLE_BUFF": "1900101",
          "PHASE91_GROUP": "431",
          "PHASE71_GROUP": "432",
          "PHASE61_GROUP": "433",
          "PHASE41_GROUP": "434",
          "PHASE25_GROUP": "435",
          "PHASE20_GROUP": "436",
          "FINAL_BOSS_ENRAGE": "1900309",
          "PROTECT_PLATFORM_GROUP": "450"
        },
        "description": "Primordial Chaos - final boss with all mechanics combined"
      }
    ]
  },

  "variables": {
    "variables": {},
    "varsFilePath": null
  }
}
```

## Usage Workflow

### Step 1: Create Profile

```csharp
var profile = new DungeonProfile
{
    Name = "My Dungeon",
    WpsId = 83003,  // Important: unique ID
    BaseWpsFile = "83000.wps",
    FloorCount = 50,
    BossInterval = 5
};
```

### Step 2: Configure Global Boss Settings

```csharp
profile.BossConfig.BossGroupBase = 9300;
profile.BossConfig.IncrementBossGroup = true;
profile.BossConfig.RewardItems = new List<int> { 7000002, 7000003 };
```

### Step 3: Add Individual Boss Configurations

```csharp
// Floor 5: Simple boss
profile.SetBossConfigForFloor(5, new IndividualBossConfig
{
    Floor = 5,
    BossGroup = 9300,
    MechanicsTemplate = null,
    Description = "Intro boss"
});

// Floor 10: Enrage boss
profile.SetBossConfigForFloor(10, new IndividualBossConfig
{
    Floor = 10,
    BossGroup = 9301,
    MechanicsTemplate = "templates/boss_simple_enrage.wps",
    Arena = "ARENA_FIRE",
    Variables = new Dictionary<string, string>
    {
        ["ENRAGE_BUFF"] = "1900555",
        ["ENRAGE_HP_THRESHOLD"] = "30"
    },
    Description = "Fire boss with enrage"
});

// Floor 15: Multi-phase boss
profile.SetBossConfigForFloor(15, new IndividualBossConfig
{
    Floor = 15,
    BossGroup = 9302,
    MechanicsTemplate = "templates/boss_phases_91_71_61_41_25_20.wps",
    Arena = "ARENA_ICE",
    Variables = new Dictionary<string, string>
    {
        ["INVINCIBLE_BUFF"] = "1900101",
        ["PHASE91_GROUP"] = "501",
        ["PHASE71_GROUP"] = "502"
        // ... more phase groups
    },
    Description = "Ice boss with phases"
});
```

### Step 4: Validate and Save

```csharp
// Validate configuration
var errors = profile.Validate();
if (errors.Count > 0)
{
    foreach (var error in errors)
    {
        Console.WriteLine($"Error: {error}");
    }
    return;
}

// Save profile
profile.SaveToFile("profiles/my_dungeon.json");
```

### Step 5: Generate WPS File

```bash
cd Tools/WpsStageGen
dotnet run -- profile="profiles/my_dungeon.json" out="../../DboServer/ExecutionEnv/resource/server_data/wps/wps/83003.wps"
```

### Step 6: Enter Dungeon (GM Command)

```
@world 83003
```

## Multi-Instance Server Considerations

### Why WPS ID Management Matters

**Scenario**: You run 3 game server instances (channels) pointing to same WPS folder:
- GameServer Instance 1 (Channel 1)
- GameServer Instance 2 (Channel 2)
- GameServer Instance 3 (Channel 3)

**All instances load WPS files from**: `resource/server_data/wps/wps/`

**Problem**: If two dungeons use the same WPS ID, only one will work correctly.

**Solution**: Use unique WPS IDs for each dungeon:
- 83001.wps → Fire Temple
- 83002.wps → Ice Cavern
- 83003.wps → Lightning Spire
- 83004.wps → Earth Sanctum
- etc.

### Best Practices

1. **Centralized WPS ID Registry**
   - Keep a list of all used WPS IDs
   - Document which dungeon uses which ID
   - Use `GetExistingWpsIds()` to check

2. **Sequential Assignment**
   - Start at 83001 and increment
   - Use `FindNextAvailableWpsId()` for automation

3. **Profile Naming Convention**
   ```
   83001_fire_temple.json
   83002_ice_cavern.json
   83003_lightning_spire.json
   ```

4. **Validation Before Generation**
   ```csharp
   string wpsDir = "../../DboServer/ExecutionEnv/resource/server_data/wps/wps/";

   if (!DungeonProfile.IsWpsIdAvailable(profile.WpsId, wpsDir))
   {
       Console.WriteLine($"WPS ID {profile.WpsId} is already in use!");
       int nextId = DungeonProfile.FindNextAvailableWpsId(wpsDir);
       Console.WriteLine($"Next available ID: {nextId}");
       return;
   }
   ```

## Advanced Techniques

### Progressive Difficulty Curve

Mix mechanics to create difficulty progression:

1. **Floors 1-10**: Simple bosses (no template)
2. **Floors 11-20**: Introduce enrage mechanics
3. **Floors 21-30**: Mix enrage + simple
4. **Floors 31-40**: Introduce multi-phase
5. **Floors 41-50**: All multi-phase bosses

### Themed Dungeons

**Example: Elemental Tower**
- Fire bosses (5, 25, 45): Enrage mechanics
- Ice bosses (10, 30, 50): Multi-phase with adds
- Lightning bosses (15, 35): Simple but high damage
- Earth bosses (20, 40): Tank-and-spank with healing

### Boss Rush Mode

Create 10 bosses in 50 floors (boss every 5):
- Configure each with unique mechanics
- Escalating rewards
- Mix all template types

## Troubleshooting

### Individual Boss Not Using Custom Config

**Problem**: Boss at floor 15 not using specified template

**Solution**: Check boss interval. Floor 15 must be divisible by boss interval.
```json
{
  "bossInterval": 5,  // 15 % 5 == 0 ✓
  "individualBosses": [
    {
      "floor": 15,  // Valid boss floor
      "mechanicsTemplate": "..."
    }
  ]
}
```

### WPS ID Conflict

**Problem**: Dungeon not loading, error in server logs

**Solution**: Check for duplicate WPS IDs
```csharp
var existingIds = DungeonProfile.GetExistingWpsIds(wpsDirectory);
Console.WriteLine($"Existing WPS IDs: {string.Join(", ", existingIds)}");
```

### Variables Not Replaced in Template

**Problem**: `{{ENRAGE_BUFF}}` appears literally in WPS file

**Solution**: Ensure variables are in `individualBosses[].variables`, not global `variables`
```json
{
  "individualBosses": [
    {
      "floor": 10,
      "variables": {
        "ENRAGE_BUFF": "1900555"  // ✓ Correct location
      }
    }
  ]
}
```

### Boss Uses Wrong Arena

**Problem**: Boss spawns in default arena, not specified arena

**Solution**: Verify arena string matches CCBD arena definitions
```json
{
  "arena": "ARENA_FIRE"  // Must match arena world ID in ServerConfigTable
}
```

## API Reference

### DungeonProfile Methods

```csharp
// WPS ID Management
string GetOutputWpsPath(string wpsDirectory = "")
static bool IsWpsIdAvailable(int wpsId, string wpsDirectory)
static int FindNextAvailableWpsId(string wpsDirectory, int startFrom = 83001)
static List<int> GetExistingWpsIds(string wpsDirectory)

// Boss Configuration
IndividualBossConfig? GetBossConfigForFloor(int floor)
void SetBossConfigForFloor(int floor, IndividualBossConfig bossConfig)
void RemoveBossConfigForFloor(int floor)
List<int> GetBossFloors()

// Validation
List<string> Validate()

// Serialization
void SaveToFile(string path)
static DungeonProfile LoadFromFile(string path)
string ToCommandLineArgs()

// Factory Methods
static DungeonProfile CreateDefault(string baseWpsFile = "")
static DungeonProfile CreateHardModeTemplate(string baseWpsFile = "")
static DungeonProfile CreateMixedMechanicsTemplate(string baseWpsFile = "", int wpsId = 83001)
```

## Examples

See `profiles/` directory for complete examples:
- `mixed_mechanics_gauntlet.json` - Full 50-floor example with 6 unique bosses
- `default_extension.json` - Simple extension
- `extreme_challenge.json` - All multi-phase bosses
- `training_mode.json` - All simple bosses

---

**Version:** 1.1
**Last Updated:** 2025-01-16
**Compatibility:** WpsStageGen v4.0+ with enhanced profile system
