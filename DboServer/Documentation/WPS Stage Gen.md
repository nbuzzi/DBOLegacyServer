# WpsStageGen.UI v4

A modern WinForms UI for running the WpsStageGen tool with friendly inputs, template presets, and advanced features.

> **✨ New in v4:** Template preset system, starting stage control, real-time validation, enhanced tooltips, and built-in help system. See [WPS_Stage_Gen_UI_v4.md](WPS_Stage_Gen_UI_v4.md) for details.

## Prerequisites
- .NET 8 SDK on Windows
- Existing WpsStageGen console project (e.g. `D:\projects\dbo-legacy\OpenDBO-Core\Tools\WpsStageGen`)

## Build and Run

```powershell
# From the UI project folder
cd d:\projects\dbo-legacy\OpenDBO-Core\DboServer\ExecutionEnv\resource\server_data\wps\WpsStageGen.UI

# Restore & run (debug)
dotnet run

# Run the published EXE (self-contained)
./publish/YYYYMMDD-HHMMSS/WpsStageGen.UI.exe

# Or framework-dependent
./publish-fd/YYYYMMDD-HHMMSS/WpsStageGen.UI.exe
```

### Publish EXEs

Option A — Use the helper script (recommended):
```powershell
./publish.ps1            # Release, win-x64, both builds
./publish.ps1 -VersionStamp  # Adds git/date version info to metadata
```
Outputs:
- Self-contained: `publish/WpsStageGen.UI.exe` (no runtime needed)
- Framework-dependent: `publish-fd/WpsStageGen.UI.exe` (requires .NET 8 Desktop Runtime)

Option B — Manual commands:
```powershell
# Self-contained (single file)
dotnet publish WpsStageGen.UI.csproj -c Release -r win-x64 -p:PublishSingleFile=true -p:SelfContained=true -p:IncludeNativeLibrariesForSelfExtract=true -p:PublishTrimmed=false -o ./publish

# Framework-dependent (single file)
dotnet publish WpsStageGen.UI.csproj -c Release -r win-x64 -p:PublishSingleFile=true -p:SelfContained=false -p:PublishTrimmed=false -o ./publish-fd
```

## Quick Start (v4)

1. **Launch UI**: `WpsStageGen.UI.exe`
2. **Browse** for your WPS file (e.g., `83000.wps`)
3. **Set parameters**:
   - Floors to append: `50` (generates 50 new floors)
   - Starting stage: `151` (or `0` for auto-detect)
   - Template preset: **Enhanced Regular + Advanced Boss (Recommended)**
4. **Click Run** - Done! ✨

### New Features in v4

- **Template Presets**: 6 pre-configured options (Simple Boss, Advanced Boss, etc.)
- **Starting Stage Control**: Manual override or auto-detect
- **Real-time Validation**: Visual warnings when approaching 255 stage limit
- **Dynamic Stage Info**: Shows "💡 Stages 151-200 (45 regular + 10 boss)"
- **Help Buttons**: 📖 Opens README, 📁 Opens templates folder
- **Enhanced Tooltips**: 17 detailed tooltips with examples

See [WPS_Stage_Gen_UI_v4.md](WPS_Stage_Gen_UI_v4.md) for complete UI feature documentation.

## Advanced Usage

In the UI:
- Set `WPS file` to your `83000.wps` file
- Set `Generator path` (optional, auto-detected if bundled)
- Use **Template Preset** dropdown for quick configuration
- Or manually set: `Boss template`, `Regular template`, `Vars file`
- Configure `Boss every`, `Boss group`, `Reward item`, `Pattern list`
- Set `Boss worlds cycle` for rotating boss arenas
- Click `Run`

Display/Scaling:
- The app is Per-Monitor V2 DPI aware and uses Segoe UI; labels and inputs scale cleanly at 125–150%.

## Notes
- The UI simply shells out to `dotnet run --` in the generator folder with the composed arguments.
