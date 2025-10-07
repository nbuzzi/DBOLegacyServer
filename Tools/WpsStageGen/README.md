# WpsStageGen - CCBD Floor Generator

![Version](https://img.shields.io/badge/version-4.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-.NET%208.0-purple.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**WpsStageGen** is a powerful tool suite for generating new Crazy Casino (CCBD) dungeon floors in DBO Legacy. It consists of a command-line generator and a feature-rich Windows GUI that makes creating complex multi-floor dungeons with boss mechanics, phased encounters, and custom rewards effortless.

---

## 📋 Table of Contents

- [Features](#-features)
- [What Can You Create?](#-what-can-you-create)
- [Installation](#-installation)
- [Quick Start](#-quick-start)
- [GUI Application](#-gui-application)
- [Command-Line Usage](#-command-line-usage)
- [Templates System](#-templates-system)
- [Variables & Placeholders](#-variables--placeholders)
- [Advanced Examples](#-advanced-examples)
- [How It Works](#-how-it-works)
- [File Structure](#-file-structure)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)

---

## ✨ Features

### Core Features
- **🎰 Automated Floor Generation** - Generate up to 255 CCBD floors in seconds
- **🎯 Flexible Boss Intervals** - Set boss stages every N floors (e.g., 5, 10, 15)
- **📊 Smart Pattern System** - Configure mob spawn patterns with weighted probabilities
- **🎁 Custom Rewards** - Define reward items for boss stage completion
- **🔄 Arena Rotation** - Cycle through different boss arenas automatically
- **📝 Template Support** - Inject complex boss mechanics via reusable templates
- **🔧 Variable System** - Use placeholders for dynamic content generation
- **✅ Auto-Detection** - Automatically detects highest existing stage number
- **🛡️ Safety Features** - Prevents duplicate "last stage" flags and stage overflow

### Advanced Features
- **Multi-Phase Boss Mechanics** - Create bosses with HP-triggered phases (91%, 71%, 61%, 41%, 25%, 20%)
- **Protective Platforms** - Add invincibility mechanics tied to destructible objects
- **Buff & Debuff Systems** - Apply status effects at specific phases
- **Add/Remove Mob Groups** - Spawn additional enemies during boss phases
- **Stage Clear Logic** - Automatic stage completion and reward distribution
- **Custom Variables** - Override any template placeholder with your own values

---

## 🎮 What Can You Create?

### Example Use Cases

**1. Extended Crazy Casino Tower (50 new floors)**
- 40 regular floors with randomized mob patterns
- 10 boss floors with custom mechanics
- Progressive difficulty with arena rotation

**2. Challenge Mode Dungeon**
- Boss every 3 floors for intense difficulty
- Multi-phase boss fights with add spawns
- Special reward items for each boss clear

**3. Event Dungeon**
- 100 floors with increasing rewards
- Unique boss arenas with environmental themes
- Custom mechanics per boss phase

**4. Training Mode**
- Simple regular floors for practice
- Boss every 10 floors to test DPS
- Fixed patterns without templates

---

## 📦 Installation

### Prerequisites
- **.NET 8.0 SDK or Runtime** - [Download here](https://dotnet.microsoft.com/download)
- **Windows 10/11** (for GUI application)
- **DBO Legacy Server** with existing CCBD WPS file (typically `83000.wps`)

### Option 1: Download Pre-Built Releases
```bash
# Download the latest release from GitHub
# Extract to Tools/WpsStageGen
# Run WpsStageGen.UI.exe (GUI) or WpsStageGen.exe (CLI)
```

### Option 2: Build from Source
```bash
# Navigate to the Tools directory
cd Tools/WpsStageGen

# Build the CLI tool
dotnet build -c Release

# Build the GUI tool
cd ../WpsStageGen.UI
dotnet build -c Release

# Run the tools
dotnet run --project ../WpsStageGen
# OR
dotnet run --project ../WpsStageGen.UI
```

### Option 3: Publish Self-Contained Executables
```bash
# CLI tool
cd Tools/WpsStageGen
dotnet publish -c Release -r win-x64 --self-contained

# GUI tool
cd ../WpsStageGen.UI
dotnet publish -c Release -r win-x64 --self-contained
```

---

## 🚀 Quick Start

### Using the GUI (Recommended for Beginners)

1. **Launch the application**
   ```bash
   cd Tools/WpsStageGen.UI/bin/Release/net8.0-windows
   ./WpsStageGen.UI.exe
   ```

2. **Select your source WPS file**
   - Click "Browse..." next to "WPS source file"
   - Select your `83000.wps` (typically in `DboServer/ExecutionEnv/script/wps/`)

3. **Choose output location**
   - Click "Browse..." next to "Output .wps"
   - Save as `83000_extended.wps` (or any name you prefer)

4. **Configure basic settings**
   - **Floors to append**: 50 (generates 45 regular + 10 boss stages)
   - **Boss every N floors**: 5 (boss at floors 5, 10, 15, 20, etc.)
   - **Starting stage**: 0 (auto-detect from file)

5. **Select template preset**
   - Choose "Enhanced Regular + Advanced Boss (Recommended)"
   - This includes multi-phase boss mechanics and enhanced regular floors

6. **Click "Run"**
   - The generator will create your new floors
   - Check the "Output" section for results

7. **Deploy the new file**
   - Copy `83000_extended.wps` to your server's WPS folder
   - Rename it to `83000.wps` (backup the original first!)
   - Restart the GameServer

### Using the Command Line

```bash
cd Tools/WpsStageGen

# Basic: Add 50 floors to existing WPS
dotnet run -- in="D:/path/to/83000.wps" out="D:/path/to/83000_extended.wps" add=50

# Advanced: Boss every 5 floors with custom group and rewards
dotnet run -- in="D:/path/to/83000.wps" out="output.wps" add=50 bossEvery=5 bossGroup=9999 rewardItem=7000002

# With templates: Add boss mechanics
dotnet run -- in="input.wps" out="output.wps" add=50 bossEvery=5 \
  bossTemplate="templates/boss_phases_91_71_61_41_25_20.wps" \
  varsFile="templates/sample_vars.ini"

# Advanced with arena rotation
dotnet run -- in="input.wps" out="output.wps" add=100 bossEvery=10 \
  bossTemplate="templates/boss_phases.wps" \
  bossWorlds="ARENA_FIRE,ARENA_ICE,ARENA_LIGHTNING" \
  varsFile="templates/sample_vars.ini"
```

---

## 🖥️ GUI Application

The **WpsStageGen.UI** provides a modern, intuitive interface for floor generation.

### Interface Overview

#### 1. Generator Section
- **WPS source file**: Input file path (your existing 83000.wps)
- **Generator folder**: Location of WpsStageGen.csproj (auto-detected)
- **Output .wps**: Where to save the generated file (recommended: use a new file)

#### 2. Stage Parameters
- **Floors to append**: How many new floors to create (1-1000)
- **Starting stage**: First stage number (0 = auto-detect, e.g., 151 to start after floor 150)
- **Boss every N floors**: Interval for boss stages (typically 5 or 10)
- **Presets**: Quick-select buttons for common intervals (3, 5, 10, 15)
- **Template preset**: Pre-configured template combinations

#### 3. Rewards & Pattern
- **CCBD reward item (tblidx)**: Item ID given after boss clear (default: 7000002)
- **Regular floor pattern list**: Weighted spawn patterns
  - Format: `(pattern_id, percentage), (pattern_id, percentage), ...`
  - Example: `(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)`
  - Pattern IDs must exist in your WPS file

#### 4. Boss & Worlds
- **Boss mob group**: Group ID for boss spawns (default: 9999)
  - Must exist in your mob group tables
- **Boss arenas rotation**: Comma-separated arena names for cycling
  - Example: `ARENA_A,ARENA_B,ARENA_C`
  - Cycles through arenas for consecutive boss stages

#### 5. Templates & Variables
- **Regular template**: WPS snippet injected into regular floors
- **Boss template**: WPS snippet injected into boss floors
- **Vars file**: INI/TXT file with NAME=VALUE pairs
- **Inline vars**: Variables entered directly (one per line)

#### 6. Action Section
- **Args preview**: See the command-line arguments being generated
- **Copy**: Copy arguments to clipboard
- **Reset**: Restore default values
- **📁 Templates**: Open templates folder
- **📖 Help**: Open this documentation
- **Run**: Execute the generator

#### 7. Output Section
- Real-time console output
- Success/error messages
- Generation statistics

### Template Presets Explained

| Preset | Description | Use Case |
|--------|-------------|----------|
| **None (Basic floors only)** | No templates, just basic mob spawns | Simple floor extensions |
| **Simple Boss (Enrage at 30%)** | Basic boss with single enrage phase | Quick boss encounters |
| **Advanced Boss (4 phases)** | Multi-phase boss with add spawns | Complex boss fights |
| **Enhanced Regular + Simple Boss** | Enhanced regular floors + simple boss | Balanced difficulty |
| **Enhanced Regular + Advanced Boss** | Full featured (recommended) | Production-ready dungeons |
| **Custom** | Manual template selection | Advanced users |

---

## 💻 Command-Line Usage

### Syntax

```bash
WpsStageGen in=PATH [options]
```

### Required Parameters

- `in=PATH` - Path to source WPS file

### Optional Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `out=PATH` | string | (overwrites input) | Output file path |
| `add=N` | int | 5 | Number of floors to append |
| `start=N` | int | 0 | Starting stage number (0 = auto-detect) |
| `bossEvery=N` | int | 5 | Boss appears every N floors |
| `bossGroup=N` | int | 9999 | Boss mob group ID |
| `rewardItem=N` | int | 7000002 | CCBD reward item tblidx |
| `pattern="..."` | string | (1,35%),(2,35%),... | Regular floor pattern list |
| `bossWorlds=A,B,C` | string | (none) | Comma-separated arena rotation |
| `bossTemplate=PATH` | string | (none) | Boss template file |
| `regularTemplate=PATH` | string | (none) | Regular floor template |
| `varsFile=PATH` | string | (none) | Variables file (.ini or .json) |
| `var.NAME=VALUE` | string | (none) | Inline variable override |

### Examples

#### Example 1: Basic Extension
```bash
# Add 20 floors with default settings
dotnet run -- in="83000.wps" out="83000_new.wps" add=20
```

#### Example 2: Custom Boss Interval
```bash
# Boss every 10 floors instead of 5
dotnet run -- in="83000.wps" out="output.wps" add=50 bossEvery=10
```

#### Example 3: Custom Rewards and Boss Group
```bash
# Use custom boss group and reward item
dotnet run -- in="83000.wps" out="output.wps" add=30 \
  bossGroup=12345 rewardItem=8000001
```

#### Example 4: Arena Rotation
```bash
# Cycle through 3 different boss arenas
dotnet run -- in="83000.wps" out="output.wps" add=60 bossEvery=5 \
  bossWorlds="FIRE_ARENA,ICE_CAVE,THUNDER_DOME"
```

#### Example 5: Advanced Boss Mechanics
```bash
# Multi-phase boss with custom variables
dotnet run -- in="83000.wps" out="output.wps" add=50 \
  bossTemplate="templates/boss_phases_91_71_61_41_25_20.wps" \
  varsFile="templates/sample_vars.ini" \
  var.INVINCIBLE_BUFF=1900101 \
  var.PHASE91_GROUP=301
```

#### Example 6: Complete Configuration
```bash
# Full-featured dungeon with all options
dotnet run -- in="83000.wps" out="ccbd_extreme.wps" \
  add=100 \
  start=151 \
  bossEvery=10 \
  bossGroup=9999 \
  rewardItem=7000002 \
  pattern="(1,30%),(2,30%),(3,15%),(4,15%),(6,10%)" \
  bossWorlds="ARENA_FIRE,ARENA_ICE,ARENA_LIGHTNING,ARENA_EARTH" \
  bossTemplate="templates/boss_phases_91_71_61_41_25_20.wps" \
  regularTemplate="templates/regular_basic_pattern.wps" \
  varsFile="templates/sample_vars.ini" \
  var.PHASE91_GROUP=301 \
  var.PHASE71_GROUP=302 \
  var.INVINCIBLE_BUFF=1900101
```

---

## 📝 Templates System

Templates are reusable WPS code snippets that get injected into generated floors. They support placeholders for dynamic content.

### Included Templates

#### 1. `boss_phases_91_71_61_41_25_20.wps`
**Multi-phase boss with add spawns and protective mechanics**

Features:
- Boss engages in combat
- Spawns adds at 91%, 71%, 61%, 41%, 25%, 20% HP
- Protective platform mechanic (boss invincible until platform destroyed)
- Removes invincibility when platform dies
- Cleans up add groups on boss death

Use case: Complex raid-style bosses

#### 2. `regular_basic_pattern.wps`
**Enhanced regular floor with optional timed events**

Features:
- Uses CCBD exec pattern for standard mob spawns
- Optional: Timed add group spawns (commented out by default)
- Placeholder support for custom groups

Use case: Regular floors with light mechanics

### Creating Custom Templates

Templates are plain text files containing WPS script fragments with `{{PLACEHOLDER}}` markers.

**Example: Simple Boss Enrage**
```lua
-- templates/boss_simple_enrage.wps

Action( "function" )
--[
    Condition( "child" )
    --[
        Action( "while" )
        --[
            Action( "loop" )
            --[
                -- Wait until boss is in combat
                Action( "wait" )
                --[
                    Condition( "check battle" )
                    --[
                        Param( "type", "mob" )
                        Param( "group", {{BOSS_GROUP}} )
                        Param( "is battle", "true" )
                    --]
                    End()
                --]
                End()

                -- Enrage at 30% HP
                Action( "wait" )
                --[
                    Condition( "check lp" )
                    --[
                        Param( "type", "mob" )
                        Param( "group", {{BOSS_GROUP}} )
                        Param( "lp", 30 )
                    --]
                    End()
                --]
                End()

                -- Apply enrage buff
                Action( "register buff" )
                --[
                    Param( "target type", "mob" )
                    Param( "target index", {{BOSS_GROUP}} )
                    Param( "buff index", {{ENRAGE_BUFF}} )
                --]
                End()

                -- Exit loop when boss dies
                Action( "wait" )
                --[
                    Condition( "check battle" )
                    --[
                        Param( "type", "mob" )
                        Param( "group", {{BOSS_GROUP}} )
                        Param( "is battle", "false" )
                    --]
                    End()
                --]
                End()
            --]
            End()
        --]
        End()
    --]
    End()
--]
End()
```

**Usage:**
```bash
dotnet run -- in="83000.wps" out="output.wps" add=20 \
  bossTemplate="templates/boss_simple_enrage.wps" \
  var.ENRAGE_BUFF=1900555
```

### Template Best Practices

1. **Use descriptive placeholders** - `{{PHASE91_GROUP}}` is better than `{{G1}}`
2. **Comment your code** - Explain what each section does
3. **Test incrementally** - Start with simple mechanics, add complexity gradually
4. **Follow WPS syntax** - Ensure proper indentation and `Action`/`End()` pairs
5. **Document variables** - List required placeholders at the top of your template

---

## 🔧 Variables & Placeholders

### Built-in Placeholders

These are automatically available in all templates:

| Placeholder | Description | Example Value |
|-------------|-------------|---------------|
| `{{STAGE}}` | Current stage number | 151, 152, 153, ... |
| `{{IS_BOSS}}` | Whether this is a boss stage | "true" or "false" |
| `{{BOSS_GROUP}}` | Boss mob group ID | 9999 |
| `{{REWARD_ITEM}}` | Reward item tblidx | 7000002 |
| `{{ARENA_WORLD}}` | Current boss arena (if using rotation) | "ARENA_FIRE" |
| `{{MARK_LAST_STAGE}}` | Whether this is the final boss | "true" or "false" |
| `{{BOSS_EVERY}}` | Boss interval setting | 5, 10, etc. |
| `{{START_STAGE}}` | First generated stage number | 151 |
| `{{END_STAGE}}` | Last generated stage number | 200 |

### Custom Variables

#### Method 1: Variables File (.ini or .txt)

**templates/sample_vars.ini:**
```ini
# Boss buffs
INVINCIBLE_BUFF=1900101
ENRAGE_BUFF=1900102
PLAYER_DEBUFF=1800203

# Phase groups
PHASE91_GROUP=301
PHASE71_GROUP=302
PHASE61_GROUP=303
PHASE41_GROUP=304
PHASE25_GROUP=305
PHASE20_GROUP=306

# Protection mechanics
PROTECT_PLATFORM_GROUP=350
PROTECT_ADDONS_GROUP=351

# Regular floor extras
ADD_GROUP=210
```

**Usage:**
```bash
dotnet run -- in="input.wps" out="output.wps" add=50 \
  varsFile="templates/sample_vars.ini"
```

#### Method 2: JSON Variables File

**templates/vars.json:**
```json
{
  "INVINCIBLE_BUFF": "1900101",
  "PHASE91_GROUP": "301",
  "PHASE71_GROUP": "302",
  "BOSS_SPAWN_DELAY": "3",
  "PLATFORM_HP": "100000"
}
```

**Usage:**
```bash
dotnet run -- in="input.wps" out="output.wps" add=50 \
  varsFile="templates/vars.json"
```

#### Method 3: Inline Variables (Command-Line)

```bash
dotnet run -- in="input.wps" out="output.wps" add=50 \
  var.INVINCIBLE_BUFF=1900101 \
  var.PHASE91_GROUP=301 \
  var.PHASE71_GROUP=302 \
  var.CUSTOM_TIMER=60
```

#### Method 4: GUI Inline Variables

In the GUI, use the "Inline vars" textbox:
```
INVINCIBLE_BUFF=1900101
PHASE91_GROUP=301
PHASE71_GROUP=302
CUSTOM_TIMER=60
```

### Variable Precedence (Highest to Lowest)

1. **Command-line inline** (`var.NAME=VALUE`)
2. **GUI inline vars**
3. **Variables file** (`varsFile=...`)
4. **Built-in placeholders** (automatic)

---

## 📚 Advanced Examples

### Example 1: Progressive Difficulty Tower

**Goal:** 200 floors with increasing boss frequency

```bash
# Floors 1-50: Boss every 10 floors
dotnet run -- in="83000.wps" out="output.wps" add=50 bossEvery=10

# Floors 51-100: Boss every 7 floors
dotnet run -- in="output.wps" out="output.wps" add=50 start=51 bossEvery=7

# Floors 101-150: Boss every 5 floors
dotnet run -- in="output.wps" out="output.wps" add=50 start=101 bossEvery=5

# Floors 151-200: Boss every 3 floors
dotnet run -- in="output.wps" out="output.wps" add=50 start=151 bossEvery=3
```

### Example 2: Themed Boss Arena Rotation

```bash
# 4 elemental arenas with matching boss groups
dotnet run -- in="83000.wps" out="elemental_tower.wps" \
  add=80 \
  bossEvery=5 \
  bossWorlds="FIRE_TEMPLE,ICE_CAVERN,THUNDER_SPIRE,EARTH_SANCTUARY" \
  bossTemplate="templates/boss_phases.wps" \
  varsFile="templates/elemental_vars.ini"
```

**elemental_vars.ini:**
```ini
# Fire boss
FIRE_BOSS_GROUP=9001
FIRE_BURN_BUFF=1900201

# Ice boss
ICE_BOSS_GROUP=9002
ICE_FREEZE_BUFF=1900202

# Thunder boss
THUNDER_BOSS_GROUP=9003
THUNDER_STUN_BUFF=1900203

# Earth boss
EARTH_BOSS_GROUP=9004
EARTH_ROOT_BUFF=1900204
```

### Example 3: Challenge Mode with Tight Mechanics

```bash
# Boss every 3 floors, tight phases, high rewards
dotnet run -- in="83000.wps" out="challenge_mode.wps" \
  add=30 \
  bossEvery=3 \
  bossGroup=8888 \
  rewardItem=9000001 \
  bossTemplate="templates/boss_tight_phases.wps" \
  var.PHASE85_GROUP=401 \
  var.PHASE70_GROUP=402 \
  var.PHASE50_GROUP=403 \
  var.PHASE30_GROUP=404 \
  var.PHASE15_GROUP=405
```

### Example 4: Practice Tower (Simple)

```bash
# 100 floors, boss every 10, no complex mechanics
dotnet run -- in="83000.wps" out="practice_tower.wps" \
  add=100 \
  bossEvery=10 \
  bossGroup=9999 \
  rewardItem=7000002 \
  pattern="(1,50%),(2,50%)"
# No templates = basic spawns only
```

### Example 5: Event Dungeon with Custom Pattern Mix

```bash
# Halloween event: ghost-themed patterns
dotnet run -- in="83000.wps" out="halloween_event.wps" \
  add=50 \
  bossEvery=5 \
  pattern="(10,40%),(11,30%),(12,20%),(13,10%)" \
  bossGroup=6666 \
  rewardItem=8500001 \
  bossWorlds="HAUNTED_MANSION,GRAVEYARD,CRYPT" \
  bossTemplate="templates/boss_halloween.wps" \
  var.GHOST_ADD_GROUP=710 \
  var.SKELETON_ADD_GROUP=711
```

---

## ⚙️ How It Works

### Generation Process

1. **Parse Source File**
   - Scans input WPS for existing `Action( "CCBD stage" )` blocks
   - Finds highest stage number (e.g., 150)
   - Clears any existing `Param( "last stage", "true" )` flags

2. **Determine Start Stage**
   - If `start=0`: Auto-detect from file (max + 1)
   - If `start=N`: Use N as first new stage

3. **Calculate Boss Stages**
   - Determine which stages are bosses based on `bossEvery` interval
   - Example: `bossEvery=5` → stages 5, 10, 15, 20, etc.

4. **Generate Stages Loop**
   - For each stage from START to START+ADD:
     - **If Regular Floor:**
       - Add `CCBD exec pattern` with specified pattern list
       - Inject `regularTemplate` (if provided)
       - Apply variable replacements
     - **If Boss Floor:**
       - Add `add mobgroup` for boss spawn
       - Inject `bossTemplate` (if provided)
       - Add `CCBD stage clear` action
       - Add 10-second wait
       - Add `CCBD reward` with item tblidx
       - Mark as "last stage" if it's the final boss

5. **Write Output**
   - Append all generated stages to source content
   - Write to output file (or overwrite input if no output specified)

### WPS Structure Generated

**Regular Floor:**
```lua
-----------------------------------
-- Stage 151
-----------------------------------
Action( "CCBD stage" )
--[
    Param( "stage", 151 )

    Action( "CCBD exec pattern" )
    --[
        Param( "pattern list", "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)" )
    --]
    End()

    -- [Optional: Injected template content here]
--]
End()
    --- end Action( "CCBD stage" )
```

**Boss Floor:**
```lua
-----------------------------------
-- Stage 155
-----------------------------------
Action( "CCBD stage" )
--[
    Param( "stage", 155 )
    Param( "direct play", "false" )
    -- Boss arena: ARENA_FIRE

    Action( "add mobgroup" )
    --[
        Param( "group", 9999 )
        Param( "no spawn wait", "true" )
    --]
    End()

    -- [Optional: Injected boss template mechanics here]

    Action( "CCBD stage clear" )
    --[
        -- Tell the client that the stage has ended.
    --]
    End()

    Action( "wait" )
    --[
        Condition( "check time" )
        --[
            Param( "time", 10 )
        --]
        End()
    --]
    End()

    Action( "CCBD reward" )
    --[
        Param( "item tblidx", 7000002 )
        Param( "last stage", "true" )  -- Only on final boss
    --]
    End()
--]
End()
    --- end Action( "CCBD stage" )
```

---

## 📁 File Structure

```
Tools/
├── WpsStageGen/                      # CLI tool
│   ├── Program.cs                    # Main generator logic
│   ├── WpsStageGen.csproj           # Project file
│   ├── README.md                     # This file
│   └── templates/                    # Template library
│       ├── boss_phases_91_71_61_41_25_20.wps
│       ├── regular_basic_pattern.wps
│       └── sample_vars.ini
│
└── WpsStageGen.UI/                   # GUI tool
    ├── Program.cs                    # Entry point
    ├── MainForm.cs                   # Main UI implementation
    ├── WpsStageGen.UI.csproj        # Project file
    └── Properties/                   # Build settings
        └── PublishProfiles/
            └── FolderProfile.pubxml  # Publish configuration
```

---

## 🐛 Troubleshooting

### Common Issues

#### 1. "No CCBD stages found in file"
**Cause:** Input file doesn't contain any `Action( "CCBD stage" )` blocks.

**Solution:**
- Verify you're using the correct WPS file (should be `83000.wps` for CCBD)
- Check file encoding (should be UTF-8)
- Ensure file isn't corrupted

#### 2. "Could not locate WpsStageGen (exe/dll/csproj)"
**Cause:** GUI can't find the generator executable.

**Solution:**
- Set "Generator folder" to the path containing `WpsStageGen.csproj`
- Or publish both tools to the same directory
- Ensure .NET 8.0 runtime is installed

#### 3. Generated Stages Don't Appear In-Game
**Cause:** Server not reading the updated WPS file.

**Solution:**
- Ensure you replaced the correct file in `DboServer/ExecutionEnv/script/wps/`
- Restart GameServer completely (not just reload)
- Check server logs for WPS parsing errors
- Verify file permissions

#### 4. Boss Doesn't Spawn
**Cause:** Invalid boss group ID.

**Solution:**
- Verify `bossGroup` value exists in your mob tables
- Check `add mobgroup` syntax in generated file
- Ensure mob spawn tables are correctly configured

#### 5. Template Variables Not Replaced
**Cause:** Variable name mismatch or missing vars file.

**Solution:**
- Check placeholder spelling: `{{PHASE91_GROUP}}` (case-sensitive)
- Verify vars file path is correct
- Use inline vars in GUI to test: `PHASE91_GROUP=301`
- Ensure no extra spaces in placeholder: `{{ WRONG }}` vs `{{RIGHT}}`

#### 6. "Exceeds max 255 stages" Warning
**Cause:** Total stage count > 255.

**Solution:**
- Reduce `add` value
- Split into multiple files
- Check `start` parameter calculation

#### 7. Pattern List Not Working
**Cause:** Invalid pattern syntax or non-existent pattern IDs.

**Solution:**
- Verify format: `(id, percentage), (id, percentage)`
- Ensure percentages are valid (don't need to sum to 100%)
- Check pattern IDs exist in your WPS file
- Example: `(1, 35%), (2, 35%), (3, 30%)`

### Debug Tips

1. **Use Small Test Batches**
   ```bash
   # Generate just 5 floors to test
   dotnet run -- in="83000.wps" out="test.wps" add=5
   ```

2. **Check Generated Output**
   - Open `test.wps` in a text editor
   - Look for your new stages at the bottom
   - Verify stage numbers are sequential
   - Check template content was injected

3. **Test Without Templates First**
   ```bash
   # Basic generation without complexity
   dotnet run -- in="83000.wps" out="basic_test.wps" add=10 bossEvery=5
   ```

4. **Enable Verbose Logging**
   - Check GUI "Output" section for detailed messages
   - CLI shows stage count and final stage number on success

5. **Validate WPS Syntax**
   - Ensure all `Action()` blocks have matching `End()`
   - Check indentation consistency (uses spaces, not tabs)
   - Verify quotes are properly closed

---

## 🤝 Contributing

We welcome contributions! Here's how you can help:

### Reporting Bugs
- Use GitHub Issues
- Include steps to reproduce
- Provide sample WPS files (if possible)
- Attach error messages and logs

### Suggesting Features
- Describe your use case
- Explain expected behavior
- Provide examples

### Submitting Templates
- Create useful, reusable mechanics
- Document all placeholders
- Include sample vars file
- Add usage examples

### Code Contributions
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Template Contribution Guidelines
- Use descriptive names: `boss_enrage_triple_phase.wps`
- Comment complex logic
- List required variables at the top
- Test with multiple stage counts
- Provide vars file example

---

## 📜 License

MIT License - See LICENSE file for details

---

## 🎯 Credits

**Developed by:** DBO Legacy Team
**Version:** 4.0
**Last Updated:** 2025

**Special Thanks:**
- DBO Legacy community for testing and feedback
- Contributors who created templates and improvements

---

## 📞 Support

- **Discord:** [DBO Legacy Discord](#)
- **Forum:** https://dbolegacy.com
- **GitHub Issues:** [Report bugs here](#)
- **Documentation:** This README and in-app help

---

## 🔮 Future Enhancements

- [ ] Visual template editor
- [ ] Stage preview system
- [ ] Mob group validation
- [ ] Pattern ID autocomplete
- [ ] Real-time WPS syntax highlighting
- [ ] Template marketplace/library
- [ ] Multi-file batch processing
- [ ] Stage difficulty calculator
- [ ] Reward progression system
- [ ] Boss mechanics wizard

---

**Happy floor generating! 🎰**
