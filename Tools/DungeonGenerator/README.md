# Dungeon Generator - WinForms UI

A Windows desktop application for creating custom CCBD-like dungeons for OpenDBO server using an intuitive graphical interface.

## Features

✨ **Visual Dungeon Designer**
- Configure all dungeon parameters via form inputs
- Real-time WPS ID availability checking
- Auto-find next available WPS ID
- Tab-based organization (Basic Settings, Individual Bosses, Output)

🎮 **Full Dungeon Configuration**
- **Basic Settings**: Name, description, WPS ID
- **Floor Settings**: Total floors (1-255), boss interval, start floor
- **Boss Defaults**: Boss group base, mechanics template, increment options
- **Arena & Rewards**: Custom arena rotation, reward item progression

👹 **Individual Boss Customization** (Mix Mechanics!)
- Add custom configuration per boss floor
- Mix different boss mechanics in the same dungeon:
  - Simple bosses (no mechanics)
  - Enrage bosses (rage at low HP)
  - Multi-phase bosses (6 phases with adds)
- Per-boss arenas and rewards
- Boss descriptions for documentation

📤 **Export & Generation**
- Generate JSON profile files
- Save/load profile templates
- View generated command line
- Copy command to clipboard for execution
- Real-time validation with error messages

## How to Use

### 1. Launch the Application

```bash
cd Tools/DungeonGenerator
dotnet run
```

Or build and run the executable:

```bash
dotnet build
cd bin/Debug/net8.0-windows
DungeonGenerator.exe
```

### 2. Configure Your Dungeon

**Tab 1: Basic Configuration**

1. **Basic Information**
   - Enter dungeon name
   - Set a unique WPS ID (use "Find Next Available" button)
   - Add description (optional)

2. **Floor Settings**
   - Total Floors: How many floors in your dungeon (1-255)
   - Boss Every N Floors: Boss appears every N floors (e.g., 5 = bosses on floors 5, 10, 15...)
   - Start Floor: Usually 0 (auto)

3. **Default Boss Configuration**
   - Boss Group Base: Starting mob group ID for bosses
   - Mechanics Template: Default mechanics for all bosses
     - None = Simple boss (just spawns)
     - Simple Enrage = Boss enrages at 30% HP
     - Multi-Phase = 6-phase boss with adds
   - Increment Boss Group ID: Auto-increment for each boss

4. **Arena & Rewards**
   - Arena Rotation: Comma-separated arena IDs (e.g., `ARENA_FIRE,ARENA_ICE,ARENA_LIGHTNING`)
   - Reward Items: Comma-separated item IDs (e.g., `7000002,7000003,7000004`)

**Tab 2: Individual Bosses**

Mix different boss mechanics by adding custom configurations:

1. Click **"Add Boss"**
2. Select boss floor from dropdown (only shows valid boss floors)
3. Configure:
   - Boss Group ID (unique mob group)
   - Mechanics Template (overrides default)
   - Arena World ID (overrides rotation)
   - Reward Item (overrides default)
   - Description (for documentation)
4. Click **"OK"**

Example mixed dungeon:
- Floor 5: Simple boss (tutorial)
- Floor 10: Enrage boss (introduce mechanic)
- Floor 15: Simple boss (break)
- Floor 20: Multi-phase boss (challenge)
- Floor 25: Enrage boss with different arena

**Tab 3: Generate & Export**

1. Click **"Generate Profile"** to validate and preview
2. Review output:
   - Profile summary
   - JSON configuration
   - Command line to execute
3. Click **"Save JSON"** to save profile to file
4. Click **"Copy Command"** to copy command to clipboard
5. Execute command in terminal to generate WPS file

### 3. Generate WPS File

After saving/generating your profile, use the command shown in the output:

```bash
cd Tools
dotnet run --project WpsStageGen -- profile="profiles/your_dungeon.json" out="path/to/output.wps"
```

Or copy the full command using the **"Copy Command"** button.

### 4. Deploy to Server

1. Copy the generated WPS file to: `DboServer/ExecutionEnv/resource/server_data/wps/wps/`
2. Restart GameServer
3. Enter dungeon in-game: `@world <your_wps_id>`

## Example Workflows

### Quick Dungeon (Default Settings)

1. Open application
2. Set WPS ID (use "Find Next Available")
3. Change name and floor count
4. Click "Generate Profile"
5. Click "Save JSON"
6. Done! Generate WPS using command

### Mixed Mechanics Dungeon

1. Configure basic settings (50 floors, boss every 5)
2. Set default mechanics to "None"
3. Go to "Individual Bosses" tab
4. Add custom bosses:
   - Floor 5: Simple
   - Floor 10: Enrage with ARENA_FIRE
   - Floor 15: Simple
   - Floor 20: Multi-Phase with ARENA_ICE
   - Floor 25: Enrage with ARENA_LIGHTNING
5. Generate and save!

### Load Existing Template

1. Click "Load Template"
2. Browse to `WpsStageGen/profiles/`
3. Select a profile (e.g., `mixed_mechanics_gauntlet.json`)
4. Modify as needed
5. Save with new WPS ID

## UI Features

### Status Bar

Bottom status bar shows:
- WPS directory location (auto-detected)
- WPS ID availability status
- Operation results (saved, loaded, etc.)

### WPS ID Validation

When you change the WPS ID:
- ✓ Shows green checkmark if available
- ⚠ Shows warning if WPS file already exists

### Load Templates

Application looks for profiles in: `Tools/WpsStageGen/profiles/`

Built-in templates:
- `default_extension.json` - Simple 50-floor dungeon
- `extreme_challenge.json` - 100-floor raid dungeon
- `training_mode.json` - Practice dungeon
- `mixed_mechanics_gauntlet.json` - Mixed mechanics example

## Technical Details

### Architecture

**Form Components:**
- `Form1.cs` - Main form with tab control
- `Form1.Designer.cs` - UI layout (auto-generated)
- `BossConfigDialog.cs` - Dialog for individual boss configuration

**Integration:**
- References `WpsStageGen` project
- Uses `DungeonProfile` class for all operations
- Auto-detects WPS directory relative to application

**Dependencies:**
- .NET 8.0 Windows Forms
- WpsStageGen (project reference)
- System.Text.Json (for JSON serialization)

### Profile Storage

Profiles are saved as JSON files compatible with the command-line tool:

```json
{
  "name": "My Dungeon",
  "wpsId": 83005,
  "baseWpsFile": "83000.wps",
  "floorCount": 50,
  "bossInterval": 5,
  "bossConfig": {
    "bossGroupBase": 9100,
    "mechanicsTemplate": null,
    "individualBosses": [
      {
        "floor": 10,
        "bossGroup": 9101,
        "mechanicsTemplate": "templates/boss_simple_enrage.wps",
        "arena": "ARENA_FIRE",
        "description": "Fire enrage boss"
      }
    ]
  }
}
```

## Troubleshooting

### WPS Directory Not Found

If status bar shows "Warning: WPS directory not found":

**Solution:** The app tries to auto-detect the WPS directory. If it fails:
- Ensure you're running from Tools/DungeonGenerator
- Or use absolute paths when saving profiles

### Invalid WPS ID

If you get validation error about WPS ID:

**Solution:**
- WPS ID must be >= 83001 (83000 is reserved for original CCBD)
- Use "Find Next Available" to get a guaranteed available ID
- Check for conflicts with existing WPS files

### Individual Boss Not Working

If custom boss isn't shown in list:

**Solution:**
- Ensure the floor number is a valid boss floor
- Boss floors must be multiples of "Boss Interval"
- Example: If interval is 5, valid floors are 5, 10, 15, 20, etc.

### Validation Errors

If "Generate Profile" shows validation errors:

**Common issues:**
- WPS ID out of range or duplicate
- Floor count exceeds 255
- Boss interval is 0
- Template file paths incorrect

**Solution:** Fix the highlighted issues and try again

## Development

### Project Structure

```
DungeonGenerator/
├── Form1.cs                    # Main form code-behind
├── Form1.Designer.cs           # UI designer code
├── BossConfigDialog.cs         # Boss config dialog
├── Program.cs                  # Application entry point
├── DungeonGenerator.csproj     # Project file (.NET 8 WinForms)
└── README.md                   # This file
```

### Building

```bash
cd Tools/DungeonGenerator
dotnet build
```

### Running

```bash
dotnet run
```

### Publishing

```bash
dotnet publish -c Release -r win-x64 --self-contained
```

Output: `bin/Release/net8.0-windows/win-x64/publish/`

## Integration with Tools Solution

The Dungeon Generator is part of the main Tools solution:

```
Tools/
├── CustomDropEventEditor/      # Drop event editor
├── ServerMonitor/              # Server monitoring tool
├── WpsStageGen/                # WPS generator (CLI)
├── WpsStageGen.UI/             # WPS generator (GUI)
├── DungeonGenerator/           # Dungeon profile generator (this tool)
└── Tools.sln                   # Solution file
```

All tools can be built together:

```bash
cd Tools
dotnet build Tools.sln
```

## Credits

**Created for:** OpenDBO-Core
**Version:** 1.0
**Framework:** .NET 8.0 Windows Forms
**License:** Same as OpenDBO-Core project

## See Also

- [DungeonProfile.cs](../WpsStageGen/DungeonProfile.cs) - Core profile system
- [MIXED_MECHANICS_GUIDE.md](../WpsStageGen/MIXED_MECHANICS_GUIDE.md) - Comprehensive guide
- [ENHANCED_FEATURES_SUMMARY.md](../WpsStageGen/ENHANCED_FEATURES_SUMMARY.md) - Feature overview
- [Session Summary](../../Sessions/SESSION_SUMMARY_CC_DUNGEONS.md) - Development session notes
