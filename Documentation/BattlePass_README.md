# Battle Pass System (Server-Side Skeleton)

This introduces a minimal Battle Pass progression manager on the GameServer. It is currently **in-memory only** (no database persistence yet) and is safe to keep disabled until schema changes are added manually.

## Goals
* Provide a unified place to award Battle Pass XP for gameplay actions: mob kills, dungeon progress, rank battle, Budokai.
* Allow website / external services to later query persistent progress (future phase).
* Keep initial diff small and isolated; no changes to protected paths or DB migrations.

## Files
* `Server/GameServer/BattlePassManager.h/.cpp` – Manager singleton and logic.
* `Monster.cpp` – Added hook after `EventManager` mob kill notification.

## Config (`.\\config\\BattlePass.cfg` – create manually)
```
[BATTLEPASS]
Enabled=1
SeasonId=1
StartUnix=0
EndUnix=0
Verbose=0
BaseXpPerLevel=1000
LevelXpGrowthPercent=15
  AutosaveMinutes=5
  MudosaPerLevel=0
MobKillXp=5
DungeonStageXp=40
DungeonClearXp=250
RankBattleParticipationXp=60
RankBattleWinXp=140
BudokaiParticipationXp=300
BudokaiWinXp=800
DeathXp=0
DailyResetHour=0
NotifyOnXpGain=1
BroadcastLevelUp=1
WelcomeMessageEnabled=1
WelcomeMessageCooldownSec=300
```

Place this file beside `Events.cfg`. The manager will log a notice and stay disabled if the file is missing.
  A flat-file snapshot of progress is stored at `BattlePassProgress.dat` (same folder) and auto-saved every `AutosaveMinutes` (0 disables autosave).

## XP & Level Formula
`RequiredXP(level) = BaseXpPerLevel * (1 + GrowthPercent/100) ^ level` (integer truncated). XP overflow after level-up carries into the next level.

## Player Progress Fields
Tracked per character (in-memory):
```
level, xp (current), totalXp,
mobKills, dungeonStages, dungeonClears,
rankBattles, rankWins, budokaiEntries, budokaiWins, deaths
```

## Event Hooks Implemented
* Mob kill: `Monster.cpp` → `OnMobKill`.
* Player death: `char.cpp` faint state → `OnPlayerDeath`.
* Rank Battle: `RankBattle.cpp` winner decision → `OnRankBattleParticipation(win)` (participation for all, win for winner).
* Budokai Major & Final matches: `BudokaiManager.cpp` inside `UpdateMajorMatchScore` / `UpdateFinalMatchScore`:
  * First stage (before stage increment) awards participation XP once.
  * When match score reaches max (winner decided) awards win XP to winning team/individual.
* CCBD (Battle Dungeon) Stage Clear: `WpsScriptAlgoAction_CCBD_stage_clear.cpp` awards `OnDungeonStageComplete` to every player in the dungeon world.
* CCBD Final Clear / Exit: `Party.cpp` (inside reward exit logic when `m_bLastStage` true) awards `OnDungeonClear` to each exiting player.
* Login / First World Entry: `CPlayer::OnEnterWorldComplete()` sends a Battle Pass welcome message if enabled (rate limited).

## Remaining Hooks (Planned)
* Time Quest (TMQ) stage / completion (call `OnDungeonStageComplete` / `OnDungeonClear`).
* Time Leap Dungeon (TLQ) stage / completion.
* Ultimate Dungeon (UD) stage / completion.

## Persistence (Current & Future)
Currently progress is saved to `.\\config\\BattlePassProgress.dat` as CSV on autosave (every `AutosaveMinutes`) and can be forced via GM command (if implemented later). This flat-file persistence is intentionally simple and will be replaced by DB storage after tuning.

## Welcome Message Feature
Config keys:
```
WelcomeMessageEnabled=1            ; Master toggle
WelcomeMessageCooldownSec=300      ; Min seconds between welcome messages per character
```
Behavior:
* Fired in `CPlayer::OnEnterWorldComplete()` when Battle Pass is enabled.
* Rate limited per character (process memory; resets on server restart).
* Template currently hard-coded; future enhancement: configurable `WelcomeMessageTemplate` and dynamic tier name.
* Localization: Placeholder hook present—once translation/DeepL adapter is exposed server-side, replace the direct send with translated string.

### Database Persistence (Future)
Add a table (suggested schema):
```
CREATE TABLE battle_pass_progress (
  character_id INT PRIMARY KEY,
  season_id INT NOT NULL,
  level INT NOT NULL,
  xp INT NOT NULL,
  total_xp BIGINT NOT NULL,
  mob_kills INT NOT NULL,
  dungeon_stages INT NOT NULL,
  dungeon_clears INT NOT NULL,
  rank_battles INT NOT NULL,
  rank_wins INT NOT NULL,
  budokai_entries INT NOT NULL,
  budokai_wins INT NOT NULL,
  deaths INT NOT NULL,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```
Then implement load/save (QueryServer RPC) outside protected migration paths.

## Logging
Verbose mode (`Verbose=1`) prints XP gains & daily resets to the server log (may be noisy).

## Rollback
All changes are isolated; remove the two new source files and the small block in `Monster.cpp` to fully revert.

## Next Steps
1. Confirm config & basic kill XP accumulation with `Verbose=1`.
2. Wire remaining hooks: dungeon stage/clear (rank battle & Budokai already integrated).
3. (Optional) Reward tables per level (items/currency) & website sync.
4. Implement daily / weekly challenges (extend `PlayerProgress`).
5. Replace flat-file persistence with DB once migrations allowed.

---
Initial version: skeleton (no persistence) – October 2025.
