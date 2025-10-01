# WpsStageGen.UI

A lightweight WinForms UI for running the WpsStageGen tool with friendly inputs.

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

In the UI:
- Set `WPS file` to your `83000.wps` file.
- Set `Generator path` to the folder with the WpsStageGen `.csproj` (for example: `D:\projects\dbo-legacy\OpenDBO-Core\Tools\WpsStageGen`).
- Set `Boss every`, `Boss group`, `Reward item`, `Pattern list`.
- Optionally set:
	- `Boss worlds cycle` as a comma-separated list to annotate varying arenas per boss
	- You can run with `out=PATH` and `bossTemplate=PATH` by copying the Args preview into a terminal
- Click `Run`.

Display/Scaling:
- The app is Per-Monitor V2 DPI aware and uses Segoe UI; labels and inputs scale cleanly at 125–150%.

## Notes
- The UI simply shells out to `dotnet run --` in the generator folder with the composed arguments.
