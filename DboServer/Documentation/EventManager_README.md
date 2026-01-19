# Event Manager System - Complete Guide

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Configuration File](#configuration-file)
- [Round Configuration](#round-configuration)
- [Minion Groups](#minion-groups)
- [World Rotation](#world-rotation)
- [Auto-Restart](#auto-restart)
- [Player Commands](#player-commands)
- [Advanced Examples](#advanced-examples)
- [Troubleshooting](#troubleshooting)

---

## Overview

The **Event Manager** is a flexible, round-based PvE event system for Dragon Ball Online. It allows you to create automated or manual events where players fight waves of mobs across multiple rounds, with rewards for each round and completion.

### Key Features
- **Round-based progression**: Multiple rounds with different mobs and configurations
- **Boss + Minion system**: Spawn minion guards around boss mobs
- **World rotation**: Each round can use a different world/map
- **Auto-restart**: Create infinite loop events that restart automatically
- **CustomDropEvent integration**: Use modified mobs with custom stats/drops
- **Flexible rewards**: Fixed items, loot drops in range, or Mudosa points
- **Channel isolation**: Only runs on channels named "EVENTS"

---

## Quick Start

### 1. Enable the Event System
Edit `config/Events.cfg`:
```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS
```

### 2. Configure Basic Rounds
```ini
# Simple 3-round event
# Format: mobs:reward:count:duration:world
Rounds = 3411,3412:11120220:2:0:180:1;3413,3414:RANGE:19916:10:240:500;3416:11120093:5:0:300:13000
```

### 3. Start the Event
Players on EVENTS channel can type:
```
@participate
```

### 4. Event Flow
1. **Enrollment** - Players join with `@participate` command
2. **Teleport** - Players are teleported to event world
3. **Round Start** - Mobs spawn, timer starts
4. **Kill All Mobs** - Players must kill all spawned mobs
5. **Round Complete** - Rewards distributed
6. **Next Round** - Repeat steps 3-5 for each round
7. **Event Complete** - All rounds done, players teleported back

---

## Configuration File

### [Event] Section

```ini
[Event]
Enabled = 1                          # Enable/disable event system
ChannelNameContains = EVENTS         # Only run on channels with "EVENTS" in name
EventWorldTblidx = 1                 # Default world for events
SpawnPosX = 5000.0                   # Default spawn position X
SpawnPosY = 0.0                      # Default spawn position Y
SpawnPosZ = 4000.0                   # Default spawn position Z
EnrollmentSeconds = 300              # How long enrollment is open (5 minutes)
RequireParticipateCommand = 1        # Require @participate command (1=yes, 0=no)
StartDelaySeconds = 10               # Delay before first round after teleport
MobSpawnRadius = 50.0                # Radius for random mob positioning
RandomMobPositions = 1               # Randomize positions (1=yes, 0=no)
VerboseLogs = 1                      # Enable detailed logging (1=yes, 0=no)
```

### [AutoEvent] Section

```ini
[AutoEvent]
Enabled = 1                          # Enable automatic event scheduling
IntervalSeconds = 1800               # Time between events (30 minutes)
InitialDelaySeconds = 600            # Delay before first event after server start
RestartOnComplete = 1                # Auto-restart after completion (infinite loop)
RestartDelaySeconds = 300            # Delay before restart (5 minutes)
```

### [WorldRotation] Section

```ini
[WorldRotation]
Enabled = 1                          # Enable world rotation per round
WorldList = 1,500,13000,43000        # Comma-separated world IDs to rotate
RandomizeWorlds = 0                  # 0=sequential, 1=random selection
```

### Rewards Configuration

```ini
MudosaPerRound = 1000                # Mudosa points per round completion
MudosaEventComplete = 5000           # Bonus Mudosa for completing all rounds

# Post-event teleport
PostEventTeleport = 1
PostEventWorldTblidx = 1
PostEventPosX = 4975.609863
PostEventPosY = -48.869999
PostEventPosZ = 4012.609863
PostEventTeleportDelayMs = 3000
```

---

## Round Configuration

### Basic Format

```
mobs:reward_type:param1:param2:duration:world:minions
```

### Parameters Explained

| Parameter | Description | Example |
|-----------|-------------|---------|
| `mobs` | Comma-separated mob IDs | `3411,3412,3413` |
| `reward_type` | `RANGE` for loot drop or item ID | `RANGE` or `11120220` |
| `param1` | Item count or loot item ID | `5` or `19916` |
| `param2` | 0 for fixed, or loot count | `0` or `10` |
| `duration` | Round duration in seconds | `180` |
| `world` | World ID (0=use rotation) | `1` or `500` |
| `minions` | Minion configuration (optional) | `3410,3411\|15\|5` |

### Fixed Rewards Example

```ini
# Spawn mobs 3411,3412 → Give item 11120220 x2 → 180 seconds → World 1
Rounds = 3411,3412:11120220:2:0:180:1
```

### Loot Range Example

```ini
# Spawn mobs 3413,3414 → Scatter item 19916 x10 → 240 seconds → World 500
Rounds = 3413,3414:RANGE:19916:10:240:500
```

### Multiple Rounds

Separate rounds with semicolons (`;`):
```ini
Rounds = 3411:11120220:2:0:180:1;3412:RANGE:19916:10:240:500;3413:11120093:5:0:300:13000
```

---

## Minion Groups

### What are Minions?

Minions are additional mobs that spawn around **each** boss mob. This creates more challenging encounters with boss + adds mechanics.

### Minion Format

```
minion1,minion2,minion3|radius|count
```

| Part | Description | Default |
|------|-------------|---------|
| `minion1,minion2` | Mob IDs to spawn as minions | Required |
| `radius` | Spawn radius around boss (meters) | 15.0 |
| `count` | Total minions to spawn (0=one each) | 0 |

### Examples

#### Example 1: Boss with 5 Random Minions
```ini
# Boss 3416 + 5 minions (random mix of 3411,3412) within 15m
Rounds = 3416:RANGE:19916:10:300:1:3411,3412|15|5
```
**Result**: Spawns boss 3416, then spawns 5 minions randomly chosen from [3411, 3412] in a 15m radius around the boss.

#### Example 2: Boss with One of Each Minion
```ini
# Boss 3420 + one of each minion type within 20m
Rounds = 3420:11120093:5:0:240:500:3415,3416,3417|20|0
```
**Result**: Spawns boss 3420, then spawns exactly 3 minions (one 3415, one 3416, one 3417) in a 20m radius.

#### Example 3: Multiple Bosses with Minions
```ini
# 2 bosses, EACH gets minions
Rounds = 3416,3417:RANGE:19916:10:300:1:3411,3412|15|5
```
**Result**:
- Boss 3416 with 5 minions around it
- Boss 3417 with 5 minions around it

#### Example 4: Boss Rush with Escalating Minions
```ini
# Round 1: Boss + 3 minions
# Round 2: Boss + 5 minions
# Round 3: Boss + 8 minions
Rounds = 3416:RANGE:19916:10:180:1:3411,3412|15|3;3416:RANGE:19916:10:180:1:3411,3412,3413|18|5;3416:RANGE:19916:10:180:1:3411,3412,3413,3414|22|8
```

---

## World Rotation

### How It Works

World rotation allows each round to use a different map/world. There are **3 priority levels**:

1. **Round-specific world** - Defined in round config
2. **World rotation list** - From `WorldList` setting
3. **Default world** - From `EventWorldTblidx`

### Sequential Rotation

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000
RandomizeWorlds = 0

# Rounds without world specified use rotation
Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:0;3413:RANGE:19916:10:180:0
```
**Result**:
- Round 1 → World 1
- Round 2 → World 500
- Round 3 → World 13000

### Random World Selection

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000,44000
RandomizeWorlds = 1

Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:0;3413:RANGE:19916:10:180:0
```
**Result**: Each round picks a random world from the list.

### Mixed Mode

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000

# Round 1: Uses rotation (World 1)
# Round 2: Forces World 43000
# Round 3: Uses rotation (World 500)
Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:43000;3413:RANGE:19916:10:180:0
```

---

## Auto-Restart

### Infinite Loop Mode

Create events that restart automatically after completion:

```ini
[AutoEvent]
Enabled = 1
RestartOnComplete = 1        # Enable infinite loop
RestartDelaySeconds = 300    # Wait 5 minutes before restart
```

### How It Works

1. Event runs all rounds to completion
2. Players teleported back
3. System waits `RestartDelaySeconds`
4. Enrollment reopens automatically
5. Process repeats indefinitely

### Use Cases

- **24/7 farming events**: Players can join anytime
- **Boss rush loops**: Continuous boss encounters
- **Training grounds**: Always-available practice area

### Example Configuration

```ini
[Event]
EnrollmentSeconds = 120          # 2 minute enrollment window

[AutoEvent]
Enabled = 1
RestartOnComplete = 1
RestartDelaySeconds = 180        # 3 minute break between loops

[WorldRotation]
Enabled = 1
WorldList = 1,500,13000
RandomizeWorlds = 1              # Different world each loop

# 3 rounds with escalating difficulty
Rounds = 3411:RANGE:19916:5:120:0:3410|10|3;3412:RANGE:19916:10:150:0:3410,3411|15|5;3413:RANGE:19916:15:180:0:3410,3411,3412|20|8
```

---

## Player Commands

### @participate

Join the event during enrollment phase.

```
@participate
```

**Requirements**:
- Must be on a channel with "EVENTS" in the name
- Event must be in ENROLLMENT state
- Not already enrolled

---

## Advanced Examples

### Example 1: Boss Rush with Increasing Difficulty

```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS
EnrollmentSeconds = 180

[AutoEvent]
Enabled = 1
RestartOnComplete = 1
RestartDelaySeconds = 240

[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000,44000
RandomizeWorlds = 0

# 5 rounds, each harder than the last
# Round 1: Easy boss + 3 minions
# Round 2: Medium boss + 5 minions
# Round 3: Hard boss + 8 minions
# Round 4: Very Hard boss + 12 minions
# Round 5: Final boss + 15 minions
Rounds = 3411:RANGE:19916:5:120:0:3410|12|3;3412:RANGE:19916:8:150:0:3410,3411|15|5;3413:RANGE:19916:12:180:0:3410,3411,3412|18|8;3414:RANGE:19916:18:240:0:3410,3411,3412,3413|22|12;3415:RANGE:19916:25:300:0:3410,3411,3412,3413,3414|25|15

MudosaPerRound = 1500
MudosaEventComplete = 10000
```

### Example 2: Multi-Boss Encounters

```ini
# Each round spawns multiple bosses with their own minions
# Round 1: 2 bosses with minions
# Round 2: 3 bosses with minions
# Round 3: 4 bosses with minions

Rounds = 3416,3417:11120093:5:0:240:1:3411,3412|12|4;3416,3417,3418:11120093:8:0:300:500:3411,3412,3413|15|5;3416,3417,3418,3419:11120093:12:0:360:13000:3411,3412,3413,3414|18|6
```

### Example 3: Themed World Tour

```ini
# Each round in a different themed location
[WorldRotation]
Enabled = 0  # Disabled, using per-round worlds

# Round 1: Forest (World 1)
# Round 2: Desert (World 500)
# Round 3: Ice (World 13000)
# Round 4: Volcano (World 43000)
# Round 5: Space (World 44000)

Rounds = 3411:RANGE:19916:10:180:1:3410,3411|15|5;3412:RANGE:19916:12:180:500:3410,3412|15|6;3413:RANGE:19916:15:180:13000:3410,3413|15|7;3414:RANGE:19916:18:180:43000:3410,3414|15|8;3415:RANGE:19916:20:240:44000:3410,3411,3412,3413,3414|20|10
```

### Example 4: Wave Defense

```ini
# Defend against waves of enemies with no boss
[Event]
MobSpawnRadius = 30.0

# 10 waves of regular mobs (no boss, just minions pattern)
Rounds = 3411:RANGE:19916:5:120:1:3410,3411,3412|25|8;3412:RANGE:19916:5:120:1:3410,3411,3412,3413|25|10;3413:RANGE:19916:5:120:1:3410,3411,3412,3413,3414|25|12;3414:RANGE:19916:5:120:1:3410,3411,3412,3413,3414,3415|25|15;3415:RANGE:19916:10:150:1:3410,3411,3412,3413,3414,3415,3416|30|20
```

---

## Troubleshooting

### Event Won't Start

**Problem**: Event doesn't start after typing `@participate`

**Solutions**:
1. Check channel name contains "EVENTS"
2. Verify `Enabled = 1` in `[Event]` section
3. Check event state is ENROLLMENT (use verbose logs)
4. Ensure `RequireParticipateCommand = 1` if manual join

### Minions Not Spawning

**Problem**: Bosses spawn but no minions appear

**Solutions**:
1. Check minion format: `mobId1,mobId2|radius|count`
2. Verify minion mob IDs are valid
3. Check verbose logs for spawn errors
4. Ensure minion section uses pipe `|` separator, not colon

### World Rotation Not Working

**Problem**: Always spawns in same world

**Solutions**:
1. Verify `Enabled = 1` in `[WorldRotation]`
2. Check `WorldList` has multiple valid world IDs
3. Ensure rounds don't specify world (use `:0:` or omit)
4. Check verbose logs for world selection

### Auto-Restart Not Working

**Problem**: Event ends but doesn't restart

**Solutions**:
1. Verify `RestartOnComplete = 1` in `[AutoEvent]`
2. Check `Enabled = 1` in `[AutoEvent]`
3. Wait for `RestartDelaySeconds` to complete
4. Check verbose logs for restart state transitions

### Mobs Not Dying / Round Not Completing

**Problem**: Killed all mobs but round doesn't complete

**Solutions**:
1. Check for mobs stuck in walls/terrain
2. Verify all spawned mobs are tracked in system
3. Use verbose logs to see kill tracking
4. Check `MobSpawnRadius` isn't too large

---

## Configuration Reference

### Complete Configuration Template

```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS
EventWorldTblidx = 1
SpawnPosX = 5000.0
SpawnPosY = 0.0
SpawnPosZ = 4000.0
MaxTickCount = 0
TickIntervalMs = 1000
EnrollmentSeconds = 300
RequireParticipateCommand = 1
StartDelaySeconds = 10
TeleportPosX = 5000.0
TeleportPosY = 0.0
TeleportPosZ = 4000.0
PostEventTeleport = 1
PostEventWorldTblidx = 1
PostEventPosX = 4975.609863
PostEventPosY = -48.869999
PostEventPosZ = 4012.609863
PostEventTeleportDelayMs = 3000
MudosaPerRound = 1000
MudosaEventComplete = 5000
MobSpawnRadius = 50.0
RandomMobPositions = 1
SpectatorsEnabled = 0
VerboseLogs = 1

Rounds = YourRoundsConfigurationHere

[AutoEvent]
Enabled = 0
IntervalSeconds = 1800
InitialDelaySeconds = 600
RestartOnComplete = 1
RestartDelaySeconds = 300

[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000
RandomizeWorlds = 0
```

---

## Support

For issues or questions:
1. Check verbose logs: `VerboseLogs = 1`
2. Review configuration syntax
3. Test with simple configuration first
4. Check server console for error messages

---

**Version**: 1.0
**Last Updated**: 2025-01-10
