# WpsStageGen

A tiny .NET 8 console tool to append CCBD stages to a `.wps` file.

- Detects highest existing `Action("CCBD stage")` `Param("stage", N)`
- Sets any existing `LastStage` to false
- Appends N new stages with:
  - `add mobgroup` auto-incrementing group ids
  - `drop item` and `drop item amount` with linear progression
  - Boss stage every K floors: marks `LastStage=true` and sets `tp world` if provided

## Build

Ensure .NET 8 SDK is installed.

```powershell
cd d:\projects\dbo-legacy\OpenDBO-Core\Tools\WpsStageGen
 dotnet build
```

## Usage

```powershell
# Append 5 stages after the current max stage, boss every 5
 dotnet run -- in="d:\projects\dbo-legacy\OpenDBO-Core\DboServer\ExecutionEnv\resource\server_data\wps\wps\83000.wps" add=5 bossEvery=5 dropItem=7000014 dropBase=2 dropStep=1 mobBase=9101 bossGroup=9999 bossWorld=CCBD_BOSS_WORLD
```

### Parameters
- `in`: Path to WPS file
- `add`: Number of stages to add (default 5)
- `bossEvery`: Every Nth stage is a boss (default 5)
- `start`: First stage number to generate (default max+1)
- `dropItem`: Item id used by `drop item` param
- `dropBase`: Base amount used by `drop item amount`
- `dropStep`: Increment per stage
- `mobBase`: Base mob group for regular stages (group=mobBase+i)
- `bossGroup`: Mob group id used for boss stages
- `bossWorld`: Teleport world string for boss stages (optional)

## Notes
- The tool does not attempt to validate group ids or world names against server data.
- Make a backup of your `.wps` before running on production files.
