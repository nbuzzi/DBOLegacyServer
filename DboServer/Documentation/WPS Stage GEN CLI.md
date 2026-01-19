# WpsStageGen

A tiny .NET 8 console tool to append CCBD stages to a `.wps` file.

- Detects highest existing `Action("CCBD stage")` `Param("stage", N)`
- Clears any existing `CCBD reward` `Param("last stage", "true")`
- Appends N new stages with:
  - Regular floors: `CCBD exec pattern` using your `pattern` list
  - Boss floors: `direct play=false`, spawn `add mobgroup group=<bossGroup>`, stage clear, wait, and `CCBD reward`
  - Last appended boss floor gets `CCBD reward` `Param("last stage", "true")`
  - Optional boss arena cycle comment `-- Boss arena: WORLD` per boss when `bossWorlds` is provided

## Build

Ensure .NET 8 SDK is installed.

```powershell
cd d:\projects\dbo-legacy\OpenDBO-Core\Tools\WpsStageGen
 dotnet build
```

## Usage

```powershell
# Append 5 stages after the current max stage, boss every 5
 dotnet run -- in="d:\projects\dbo-legacy\OpenDBO-Core\DboServer\ExecutionEnv\resource\server_data\wps\wps\83000.wps" out="d:\\temp\\83000-gen.wps" add=5 bossEvery=5 bossGroup=9999 rewardItem=7000002 pattern="(1,35%), (2,35%), (3,10%), (4,10%), (6,10%)" bossWorlds=ARENA_A,ARENA_B,ARENA_C bossTemplate="d:\\projects\\boss150-snippet.wps"
```

### Parameters
- `in`: Path to WPS file
- `add`: Number of stages to add (default 5)
- `bossEvery`: Every Nth stage is a boss (default 5)
- `start`: First stage number to generate (default max+1)
- `bossGroup`: Mob group id used for boss stages
- `rewardItem`: CCBD reward item tblidx (default 7000002)
- `pattern`: Pattern list string for regular floors (e.g. "(1,35%), (2,35%), (3,10%), (4,10%), (6,10%)")
- `bossWorlds`: Comma-separated world names to cycle per boss (optional, comment only)
 - `out`: Output path (optional). If not set, overwrites the input file.
 - `bossTemplate`: Path to a WPS snippet injected inside each generated boss stage (optional)
 - `regularTemplate`: Path to a WPS snippet injected inside each generated regular stage (optional)

### Template placeholders
You can include placeholders in your template files and they will be replaced:
- `{{STAGE}}` → current stage number
- `{{IS_BOSS}}` → true/false
- `{{BOSS_GROUP}}` → boss group id
- `{{REWARD_ITEM}}` → reward item tblidx
- `{{ARENA_WORLD}}` → current arena name from bossWorlds (empty if none)
- `{{BOSS_EVERY}}`, `{{START_STAGE}}`, `{{END_STAGE}}`

## Notes
- The tool does not attempt to validate group ids or world names against server data.
- Make a backup of your `.wps` before running on production files.
