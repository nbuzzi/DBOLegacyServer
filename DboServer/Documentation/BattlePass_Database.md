# Battle Pass Database Integration

This document defines the database schema, utility views, and integration patterns for persisting and exposing Battle Pass progression data to the website (leaderboards, player profile pages, reward claims, etc.).

> NOTE: The `Database/migrations/` path is protected. Apply the SQL below manually (or via your approved migration system) rather than committing an automated patch there.

## 1. Core Tables

### 1.1 `battle_pass_progress`
Stores the current state of each character for a specific Battle Pass season. One row per (character_id, season_id).

```sql
CREATE TABLE IF NOT EXISTS battle_pass_progress (
  character_id           INT UNSIGNED NOT NULL,
  season_id              INT UNSIGNED NOT NULL,
  level                  INT UNSIGNED NOT NULL DEFAULT 0,
  xp                     INT UNSIGNED NOT NULL DEFAULT 0,          -- XP towards next level
  total_xp               BIGINT UNSIGNED NOT NULL DEFAULT 0,       -- Lifetime cumulative XP (analytics)
  mob_kills              INT UNSIGNED NOT NULL DEFAULT 0,
  dungeon_stages         INT UNSIGNED NOT NULL DEFAULT 0,
  dungeon_clears         INT UNSIGNED NOT NULL DEFAULT 0,
  rank_battles           INT UNSIGNED NOT NULL DEFAULT 0,
  rank_wins              INT UNSIGNED NOT NULL DEFAULT 0,
  budokai_entries        INT UNSIGNED NOT NULL DEFAULT 0,
  budokai_wins           INT UNSIGNED NOT NULL DEFAULT 0,
  deaths                 INT UNSIGNED NOT NULL DEFAULT 0,
  last_daily_reset_day   INT UNSIGNED NOT NULL DEFAULT 0,          -- YYYYMMDD UTC snapshot
  updated_at             TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (character_id, season_id),
  KEY idx_season_level (season_id, level DESC, total_xp DESC),
  KEY idx_season_totalxp (season_id, total_xp DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

Rationale:
* Composite PK enforces uniqueness per season.
* Separate season + (level, total_xp) index accelerates leaderboards.
* `total_xp` index can support alternative ranking (tie-breakers if level equal).

### 1.2 (Optional) `battle_pass_events`
Use only if you need granular analytics or anti‑cheat auditing. High write volume; keep disabled unless required.

```sql
CREATE TABLE IF NOT EXISTS battle_pass_events (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  character_id  INT UNSIGNED NOT NULL,
  season_id     INT UNSIGNED NOT NULL,
  action_code   TINYINT UNSIGNED NOT NULL,  -- Enum mapping to CBattlePassManager::Action
  xp_awarded    INT UNSIGNED NOT NULL DEFAULT 0,
  level_before  INT UNSIGNED NOT NULL,
  level_after   INT UNSIGNED NOT NULL,
  created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_char_season (character_id, season_id),
  KEY idx_season_created (season_id, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 1.3 (Optional) `battle_pass_rewards_claimed`
If you plan web-based reward claiming.

```sql
CREATE TABLE IF NOT EXISTS battle_pass_rewards_claimed (
  character_id  INT UNSIGNED NOT NULL,
  season_id     INT UNSIGNED NOT NULL,
  reward_tier   INT UNSIGNED NOT NULL,      -- Tier / level at which reward becomes available
  claimed_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (character_id, season_id, reward_tier)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

## 2. Upsert Pattern (Server Flush Batch)

The server batches dirty rows and issues a multi‑row UPSERT. MySQL example:

```sql
INSERT INTO battle_pass_progress
 (character_id, season_id, level, xp, total_xp, mob_kills, dungeon_stages, dungeon_clears,
  rank_battles, rank_wins, budokai_entries, budokai_wins, deaths, last_daily_reset_day)
VALUES
  -- rows injected by server batch
  (?,?, ?,?,?, ?,?,?, ?,?,?, ?,?, ?),
  (... more ...)
ON DUPLICATE KEY UPDATE
  level=VALUES(level),
  xp=VALUES(xp),
  total_xp=VALUES(total_xp),
  mob_kills=VALUES(mob_kills),
  dungeon_stages=VALUES(dungeon_stages),
  dungeon_clears=VALUES(dungeon_clears),
  rank_battles=VALUES(rank_battles),
  rank_wins=VALUES(rank_wins),
  budokai_entries=VALUES(budokai_entries),
  budokai_wins=VALUES(budokai_wins),
  deaths=VALUES(deaths),
  last_daily_reset_day=VALUES(last_daily_reset_day);
```

If you enable the `battle_pass_events` table, append one INSERT per updated row (or bulk insert all events separately). Keep it optional to avoid heavy disk churn.

## 3. Website Consumption

### 3.1 Leaderboard Query (Top N by Level then Total XP)
```sql
SELECT p.character_id, p.level, p.xp, p.total_xp, c.CharName
FROM battle_pass_progress p
JOIN characters c ON c.CharacterID = p.character_id
WHERE p.season_id = ?
ORDER BY p.level DESC, p.total_xp DESC
LIMIT 100;
```

### 3.2 Single Player Progress
```sql
SELECT level, xp, total_xp, mob_kills, dungeon_stages, dungeon_clears,
       rank_battles, rank_wins, budokai_entries, budokai_wins, deaths,
       last_daily_reset_day, updated_at
FROM battle_pass_progress
WHERE character_id = ? AND season_id = ?;
```

### 3.3 Player Reward Eligibility (if using rewards table)
List unclaimed rewards up to current level:
```sql
SELECT r.reward_tier
FROM battle_pass_rewards_reference r          -- hypothetical static config table
LEFT JOIN battle_pass_rewards_claimed c
  ON c.character_id=? AND c.season_id=? AND c.reward_tier=r.reward_tier
JOIN battle_pass_progress p
  ON p.character_id=? AND p.season_id=?
WHERE r.season_id=?
  AND r.reward_tier <= p.level
  AND c.reward_tier IS NULL;
```

### 3.4 Lightweight API JSON Shape
```json
{
  "characterId": 12345,
  "seasonId": 1,
  "level": 7,
  "xp": 420,
  "xpNeeded": 1000,         // compute client/server side using same formula
  "totalXp": 6420,
  "counters": {
    "mobKills": 1320,
    "dungeonStages": 18,
    "dungeonClears": 3,
    "rankBattles": 12,
    "rankWins": 5,
    "budokaiEntries": 1,
    "budokaiWins": 0,
    "deaths": 9
  },
  "lastDailyResetDay": 20251006,
  "updatedAt": "2025-10-06T19:55:23Z"
}
```

## 4. Security & Data Integrity
| Concern | Mitigation |
|---------|------------|
| Tampering via API | All authoritative writes originate from game server batch. Website is read-only except reward claim endpoint. |
| Double reward claims | Primary key constraint on `battle_pass_rewards_claimed` + transactional insert. |
| Season rollover | Server resets `season_id` in-memory; new row INSERT on first flush. |
| Partial flush crash | Batching occurs atomically per INSERT ... ON DUPLICATE statement. Repeat is idempotent. |

## 5. Adding Premium / Entitlement
Add column `pass_tier TINYINT` to `battle_pass_progress` if premium vs. free is needed, or put tiers in a separate entitlement table keyed by account_id. The server’s `PlayerHasBattlePass` placeholder can then check that table before awarding XP.

```sql
ALTER TABLE battle_pass_progress ADD COLUMN pass_tier TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER season_id;
CREATE INDEX idx_tier ON battle_pass_progress (season_id, pass_tier, level DESC);
```

## 6. Optional View for Leaderboard (simplifies queries)
```sql
CREATE OR REPLACE VIEW vw_battle_pass_leaderboard AS
SELECT p.season_id, p.character_id, p.level, p.xp, p.total_xp
FROM battle_pass_progress p;
```

## 7. Flush & Batch Parameters (Server Config Mapping)
| Config Key | Purpose | Suggested Default |
|------------|---------|-------------------|
| UseDatabase | Toggle DB mode | 1 (when ready) |
| FlushSeconds | Interval between batched UPSERTs | 30 |
| MinDeltaXp | Minimum `total_xp` delta to include row in flush | 50 |

## 8. Testing Checklist
1. Apply schema to dev database.
2. Start server with `UseDatabase=1` & verify no `.dat` file writes (log should show DB flush attempts if verbose). 
3. Gain XP (mob kills, etc.) – confirm row appears after flush interval.
4. Simulate crash before flush (kill process) – ensure only last interval’s progress is lost (acceptable window). 
5. Increase level; verify `level` and `xp` persisted properly (no negative or leftover large xp).
6. (Optional) Enable events table and confirm write volume is acceptable.

## 9. Future Extensions
* Seasonal reward schedule & claim logic.
* Historical leaderboard archiving post-season (copy rows to `battle_pass_progress_archive`).
* Partition large events table by season for pruning.
* Add Redis caching for top N leaderboard if traffic spikes.

## 10. Rollback Strategy
If DB mode causes issues:
1. Set `UseDatabase=0` in `BattlePass.cfg`.
2. Optionally restore file autosave by setting `AutosaveMinutes>0`.
3. No schema rollback needed; unused tables stay dormant.

---
**End of Battle Pass DB Integration Guide**
