# Enhanced Dungeon Profile System - Summary

## 🎉 What's New (Phase 1 Enhanced)

Based on your feedback about multi-instance servers and boss mechanics mixing, we've significantly enhanced the Dungeon Profile system!

### Key Enhancements

#### 1. **WPS ID Management**
✅ Unique dungeon identification
✅ Automatic WPS ID discovery
✅ Conflict detection
✅ Multi-instance safe

#### 2. **Per-Boss Customization**
✅ Mix different mechanics in same dungeon
✅ Boss-specific templates
✅ Individual arena assignments
✅ Custom variables per boss
✅ Progressive difficulty curves

#### 3. **Multi-Instance Server Awareness**
✅ WPS ID uniqueness validation
✅ Shared WPS folder support
✅ No conflicts between server instances

---

## 🚀 Quick Start Examples

### Example 1: Simple Extension (All Bosses Same)

```json
{
  "name": "Standard Extension",
  "wpsId": 83001,
  "baseWpsFile": "83000.wps",
  "floorCount": 50,
  "bossInterval": 5,
  "bossConfig": {
    "bossGroupBase": 9999,
    "mechanicsTemplate": null,
    "rewardItems": [7000002]
  }
}
```

**Generate:**
```bash
dotnet run -- profile="profiles/standard.json" out="../../DboServer/ExecutionEnv/resource/server_data/wps/wps/83001.wps"
```

### Example 2: Mixed Mechanics (Different Bosses)

```json
{
  "name": "Mixed Gauntlet",
  "wpsId": 83002,
  "baseWpsFile": "83000.wps",
  "floorCount": 30,
  "bossInterval": 5,
  "bossConfig": {
    "bossGroupBase": 9100,
    "increment BossGroup": true,
    "individualBosses": [
      {
        "floor": 5,
        "mechanicsTemplate": null,
        "description": "Simple warm-up boss"
      },
      {
        "floor": 10,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_FIRE",
        "variables": {
          "ENRAGE_BUFF": "1900555"
        },
        "description": "Fire boss with enrage"
      },
      {
        "floor": 15,
        "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps",
        "arena": "ARENA_ICE",
        "variables": {
          "INVINCIBLE_BUFF": "1900101",
          "PHASE91_GROUP": "301"
        },
        "description": "Ice boss with multi-phase"
      }
    ]
  }
}
```

### Example 3: WPS ID Management (C#)

```csharp
string wpsDir = "../../DboServer/ExecutionEnv/resource/server_data/wps/wps/";

// Find next available WPS ID
int nextId = DungeonProfile.FindNextAvailableWpsId(wpsDir);
Console.WriteLine($"Next available WPS ID: {nextId}");

// Create profile with unique ID
var profile = new DungeonProfile
{
    Name = "Auto-ID Dungeon",
    WpsId = nextId,
    BaseWpsFile = Path.Combine(wpsDir, "83000.wps"),
    FloorCount = 50,
    BossInterval = 5
};

// Validate before generating
var errors = profile.Validate();
if (errors.Count == 0)
{
    string outputPath = profile.GetOutputWpsPath(wpsDir);
    // Generate WPS...
}
```

---

## 📊 Feature Comparison

| Feature | Before | After (Enhanced) |
|---------|--------|------------------|
| Boss Mechanics | Same template for all bosses | Mix different templates per boss |
| WPS ID | Manual management | Automatic detection & validation |
| Multi-Instance | No awareness | Full support with conflict detection |
| Boss Configuration | Global only | Global + per-boss overrides |
| Arena Assignment | Rotation only | Rotation + per-boss override |
| Variables | Global only | Global + per-boss specific |
| Validation | None | Full profile validation with errors |

---

## 🎯 Use Cases

### 1. Progressive Difficulty Dungeon
Start easy, get harder:
- Floors 1-10: Simple bosses
- Floors 11-20: Add enrage mechanics
- Floors 21-30: Introduce multi-phase
- Floors 31-40: Complex combinations
- Floors 41-50: Raid-level bosses

### 2. Themed Elemental Tower
Match mechanics to elements:
- Fire bosses → Enrage (rage = fire)
- Ice bosses → Multi-phase with control
- Lightning bosses → Fast, simple
- Earth bosses → Tank with healing

### 3. Boss Rush Event
10 unique bosses in 50 floors:
- Each boss completely unique
- Custom mechanics per boss
- Escalating rewards
- Mix all available templates

### 4. Testing/Training Dungeon
Safe environment:
- First boss: Simple (learn mechanics)
- Second boss: Enrage (learn positioning)
- Third boss: Phases (learn coordination)
- Remaining: Practice specific mechanics

---

## 🏗️ Architecture

### Profile System Flow

```
1. Create/Load Profile (JSON)
   ↓
2. Validate Configuration
   - WPS ID unique?
   - Files exist?
   - Boss floors valid?
   ↓
3. Generate Command-Line Args
   - Base parameters
   - Per-boss overrides
   ↓
4. Execute WpsStageGen
   - Parse base WPS
   - Generate floors
   - Inject templates
   - Apply variables
   ↓
5. Output WPS File
   - Unique WPS ID (e.g., 83002.wps)
   - Deploy to server
   ↓
6. Server Startup
   - Load WPS files
   - Available via @world command
```

### Multi-Instance Safety

```
Server Instance 1 (Channel 1)  ┐
Server Instance 2 (Channel 2)  ├─ All point to same WPS folder
Server Instance 3 (Channel 3)  ┘

WPS Folder:
├── 83000.wps (Original CCBD)
├── 83001.wps (Fire Temple)      ← Unique ID
├── 83002.wps (Ice Cavern)       ← Unique ID
├── 83003.wps (Lightning Spire)  ← Unique ID
└── ...

Each WPS ID is unique → No conflicts!
```

---

## 📁 Files Created/Modified

### New Files

```
Tools/WpsStageGen/
├── DungeonProfile.cs (ENHANCED)
│   ├── IndividualBossConfig class (NEW)
│   ├── WPS ID management methods (NEW)
│   ├── Per-boss configuration helpers (NEW)
│   └── Validation system (NEW)
│
├── MIXED_MECHANICS_GUIDE.md (NEW)
│   └── Complete guide for mixed mechanics
│
├── ENHANCED_FEATURES_SUMMARY.md (NEW - THIS FILE)
│   └── Quick reference for new features
│
└── profiles/ (ENHANCED)
    ├── default_extension.json
    ├── extreme_challenge.json
    ├── training_mode.json
    └── mixed_mechanics_gauntlet.json (NEW)
        └── Example with 6 unique bosses
```

### Enhanced Features in DungeonProfile.cs

**New Classes:**
- `IndividualBossConfig` - Per-boss configuration
- Enhanced `BossConfig` with `individualBosses` list

**New Properties:**
- `DungeonProfile.WpsId` - Unique dungeon ID

**New Methods:**
```csharp
// WPS ID Management
GetOutputWpsPath(wpsDirectory)
IsWpsIdAvailable(wpsId, wpsDirectory)
FindNextAvailableWpsId(wpsDirectory, startFrom)
GetExistingWpsIds(wpsDirectory)

// Boss Configuration
GetBossConfigForFloor(floor)
SetBossConfigForFloor(floor, bossConfig)
RemoveBossConfigForFloor(floor)
GetBossFloors()

// Validation
Validate()

// Factory
CreateMixedMechanicsTemplate(baseWpsFile, wpsId)
```

---

## 🎓 Learning Path

### Beginner
1. Start with `default_extension.json`
2. Modify floor count and boss interval
3. Generate and test

### Intermediate
1. Use `extreme_challenge.json` as template
2. Change arena rotation
3. Modify reward progression
4. Add global boss template

### Advanced
1. Study `mixed_mechanics_gauntlet.json`
2. Create per-boss configurations
3. Mix different templates
4. Use custom variables per boss
5. Build themed dungeons

### Expert
1. Programmatically generate profiles
2. Create dungeon series (83001-83010)
3. Build custom templates
4. Implement complex mechanics
5. Create season-based dungeons

---

## 🔧 Common Tasks

### Task 1: Check Available WPS IDs

```csharp
using WpsStageGen;

string wpsDir = "path/to/wps/folder";
var existingIds = DungeonProfile.GetExistingWpsIds(wpsDir);

Console.WriteLine("Existing WPS IDs:");
foreach (var id in existingIds)
{
    Console.WriteLine($"  - {id}.wps");
}

Console.WriteLine($"\nNext available: {DungeonProfile.FindNextAvailableWpsId(wpsDir)}");
```

### Task 2: Create Profile with Mixed Bosses

```csharp
var profile = DungeonProfile.CreateDefault("83000.wps");
profile.Name = "My Mixed Dungeon";
profile.WpsId = 83005;

// Simple boss at floor 5
profile.SetBossConfigForFloor(5, new IndividualBossConfig
{
    Floor = 5,
    BossGroup = 9500,
    MechanicsTemplate = null
});

// Enrage boss at floor 10
profile.SetBossConfigForFloor(10, new IndividualBossConfig
{
    Floor = 10,
    BossGroup = 9501,
    MechanicsTemplate = "templates/boss_simple_enrage.wps",
    Variables = new Dictionary<string, string>
    {
        ["ENRAGE_BUFF"] = "1900555"
    }
});

profile.SaveToFile("profiles/my_mixed.json");
```

### Task 3: Validate Before Generation

```csharp
var profile = DungeonProfile.LoadFromFile("profiles/my_dungeon.json");

var errors = profile.Validate();
if (errors.Count > 0)
{
    Console.WriteLine("Validation Errors:");
    foreach (var error in errors)
    {
        Console.WriteLine($"  ❌ {error}");
    }
    return;
}

Console.WriteLine("✅ Profile is valid!");
string args = profile.ToCommandLineArgs();
Console.WriteLine($"Args: {args}");
```

### Task 4: Generate Multiple Dungeons

```csharp
string wpsDir = "path/to/wps/folder";
var dungeons = new[] { "fire", "ice", "lightning" };

foreach (var dungeon in dungeons)
{
    int wpsId = DungeonProfile.FindNextAvailableWpsId(wpsDir);

    var profile = DungeonProfile.CreateMixedMechanicsTemplate("83000.wps", wpsId);
    profile.Name = $"{char.ToUpper(dungeon[0])}{dungeon.Substring(1)} Temple";

    // Customize based on dungeon type...

    string profilePath = $"profiles/{wpsId}_{dungeon}_temple.json";
    profile.SaveToFile(profilePath);

    Console.WriteLine($"Created: {profilePath} (WPS ID: {wpsId})");
}
```

---

## 📋 Checklist for New Dungeon

- [ ] Choose unique WPS ID (check existing)
- [ ] Create profile JSON or programmatically
- [ ] Define floor count and boss interval
- [ ] Configure wave patterns
- [ ] Set up global boss settings
- [ ] Add individual boss configurations (if mixing)
- [ ] Define template variables
- [ ] Validate profile
- [ ] Generate WPS file
- [ ] Deploy to server WPS folder
- [ ] Restart GameServer
- [ ] Test with `@world <wpsId>`
- [ ] Document WPS ID usage

---

## 🐛 Troubleshooting

### Issue: WPS ID Conflict

**Symptoms:** Dungeon not loading, server log errors
**Solution:**
```csharp
var existingIds = DungeonProfile.GetExistingWpsIds(wpsDir);
int nextId = DungeonProfile.FindNextAvailableWpsId(wpsDir);
profile.WpsId = nextId;
```

### Issue: Individual Boss Not Working

**Symptoms:** Boss uses default mechanics instead of custom
**Solution:** Verify boss floor is divisible by boss interval
```json
{
  "bossInterval": 5,
  "individualBosses": [
    {
      "floor": 10  // ✓ 10 % 5 == 0 (valid boss floor)
    }
  ]
}
```

### Issue: Variables Not Replaced

**Symptoms:** {{VARIABLE}} appears in WPS literally
**Solution:** Put variables in `individualBosses[].variables`, not global `variables`

### Issue: Template File Not Found

**Symptoms:** Validation error about missing template
**Solution:** Use relative paths from WpsStageGen directory
```json
{
  "mechanicsTemplate": "templates/boss_phases_91_71_61_41_25_20.wps"
}
```

---

## 🎯 Next Steps

### Immediate Use (No Code Required)
1. Use existing profiles in `profiles/` directory
2. Modify JSON files for your needs
3. Generate WPS files with `dotnet run`
4. Deploy and test

### Phase 2 (Server Integration - Optional)
If you want server-side profile management:
1. C++ DungeonProfile classes
2. DungeonProfiles.cfg configuration
3. GM commands (@dungeon_profile)
4. Runtime profile loading

**Current State:** Phase 1 is 100% functional without Phase 2!

---

## 📞 Support

**Documentation:**
- `README.md` - Main WpsStageGen documentation
- `MIXED_MECHANICS_GUIDE.md` - Per-boss customization guide
- `profiles/README.md` - Profile schema reference
- `ENHANCED_FEATURES_SUMMARY.md` - This file

**Examples:**
- `profiles/default_extension.json` - Simple
- `profiles/extreme_challenge.json` - Advanced
- `profiles/mixed_mechanics_gauntlet.json` - Mixed bosses

---

**Status:** ✅ Phase 1 Enhanced - Fully Functional
**Version:** 1.1
**Last Updated:** 2025-01-16
**Ready for:** Production Use with Multi-Instance Support

You now have a powerful system to create unlimited custom dungeons with mixed boss mechanics, safe for multiple server instances! 🎉
