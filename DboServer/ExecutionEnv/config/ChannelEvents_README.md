# Channel-Based Event Auto-Start Configuration

This document explains how to configure events to automatically enable/disable based on channel name or number when the game server starts.

## Overview

All three event systems now support channel-based configuration and "always-on" modes:

1. **PlayerModifiers** - Player stat multipliers
2. **CustomDropEvent** - Enhanced mob drops and spawns
3. **EventManager** - Automated boss events

## Configuration Methods

### Method 1: Individual Event Configs (Simple)

Each event system has its own config file with channel filtering:

#### PlayerModifiers.cfg
```ini
# Enable/disable globally
enabled = 0

# Auto-schedule settings
AutoScheduleEnabled = 1
AutoScheduleDurationHours = 3
AutoScheduleIntervalHours = 56
AutoScheduleInitialDelayMinutes = 30

# Channel filtering
ChannelFilterEnabled = 1
AllowedChannels = 2,3  # Only run on channels 2 and 3
# OR use name matching:
# ChannelNameContains = MODS  # Run on any channel with "MODS" in name

# Always-on mode (24/7, no scheduling)
AlwaysOn = 1  # Set to 1 for permanent enable on specified channels

# Stat multipliers
all modifiers: hp=1.5 physAtk=1.2 engAtk=1.2
```

#### CustomDropEvent.cfg
```ini
[Settings section]
all settings: autoStart=1 autoStartHours=0 autoStartChannels=1 alwaysOn=1

# autoStart = 1          : Enable auto-start at server boot
# autoStartHours = 0     : Infinite duration (when combined with alwaysOn=1)
# autoStartChannels = 1  : Only channel 1, or use "all" for all channels
# alwaysOn = 1           : Never expire (24/7 mode)
```

#### Events.cfg (EventManager)
```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS  # Only run on channels with "EVENTS" in name

[AutoEvent]
Enabled = 1
RestartOnComplete = 1         # Always-on: restart immediately after completion
RestartDelaySeconds = 5       # Short delay between runs (quasi-24/7)
```

---

## Use Case Examples

### Use Case 1: HARDCORE Channel - Always Custom Drops
**Scenario:** Channel 1 named "HARDCORE" should have custom drops 24/7

**CustomDropEvent.cfg:**
```ini
all settings: autoStart=1 autoStartHours=0 autoStartChannels=1 alwaysOn=1
```

---

### Use Case 2: MODS Channel - Always Player Modifiers
**Scenario:** Channel 2 named "MODS" should have player stat boosts 24/7

**PlayerModifiers.cfg:**
```ini
enabled = 0  # Disable by default
ChannelFilterEnabled = 1
AllowedChannels = 2
AlwaysOn = 1

all modifiers: hp=2.0 physAtk=1.5 engAtk=1.5 runSpd=1.3
```

---

### Use Case 3: EVENTS Channel - Continuous Boss Events
**Scenario:** Channel 0 named "EVENTS" should run boss events continuously

**Events.cfg:**
```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS

[AutoEvent]
Enabled = 1
RestartOnComplete = 1
RestartDelaySeconds = 300  # 5 minutes between events
```

---

## Channel Matching Rules

### Channel Number Matching
- Uses `app->m_config.byChannel` (channel index)
- Example: `AllowedChannels = 0,1,2`
- Channels are numbered 0, 1, 2, 3, etc.

### Channel Name Matching
- Uses `app->m_config.ServerName` (contains channel name)
- Case-insensitive substring search
- Example: `ChannelNameContains = EVENTS` matches "EVENTS", "Events Channel", "TEST-EVENTS"

### Filter Logic
- If **ChannelFilterEnabled = 0**: Allowed on all channels
- If **ChannelFilterEnabled = 1**:
  - Check `AllowedChannels` list (if not empty)
  - Check `ChannelNameContains` string (if set)
  - If either matches → Allowed
  - If both filters exist but none match → Not allowed

---

## Always-On Mode vs Scheduled Mode

### Scheduled Mode (Default)
```ini
AlwaysOn = 0
AutoScheduleEnabled = 1
AutoScheduleDurationHours = 3      # Run for 3 hours
AutoScheduleIntervalHours = 56     # Wait 56 hours before next run
```
**Result:** Event runs for 3 hours, then waits 56 hours, repeating forever.

### Always-On Mode (24/7)
```ini
AlwaysOn = 1
# Scheduling settings ignored when AlwaysOn = 1
```
**Result:** Event is permanently enabled on matching channels.

---

## Configuration Priority

1. **Always-On** takes highest priority (overrides all scheduling)
2. **Channel filtering** is checked before starting scheduled sessions
3. **Auto-start** runs once at server boot (if channel matches)

---

## Testing Your Configuration

### Step 1: Check Current Channel
In game, check your channel number/name.

### Step 2: Set Up Test Config
For channel 0, set PlayerModifiers:
```ini
ChannelFilterEnabled = 1
AllowedChannels = 0
AlwaysOn = 1
all modifiers: hp=3.0  # Triple HP for easy testing
```

### Step 3: Restart Server
Watch server logs for:
```
[PlayerModifiers] Always-on mode enabled
```

### Step 4: Login and Verify
Check your character stats - HP should be 3x normal.

---

## Common Patterns

### Pattern 1: Different Events Per Channel
- Channel 0: EventManager (boss events)
- Channel 1: CustomDropEvent (enhanced drops)
- Channel 2: PlayerModifiers (stat boosts)

### Pattern 2: Multiple Events on One Channel
Enable multiple events on the same channel (e.g., "HARDCORE"):
- CustomDropEvent: `autoStartChannels=1 alwaysOn=1`
- PlayerModifiers: `AllowedChannels=1 AlwaysOn=1`

### Pattern 3: Name-Based Matching
Use channel names for clarity:
- "EVENTS" channel → EventManager only
- "MODS" channel → PlayerModifiers only
- "HARDCORE" channel → CustomDropEvent only

---

## Troubleshooting

### Event Not Starting
1. Check channel number/name matches configuration
2. Verify `ChannelFilterEnabled = 1` if using filters
3. Check server logs for filtering messages

### Event Starting on Wrong Channel
1. Verify channel number in `AllowedChannels`
2. Check `ChannelNameContains` substring matching
3. Disable `ChannelFilterEnabled` temporarily to test

### Always-On Not Working
1. Confirm `AlwaysOn = 1` in config
2. Check channel filtering allows current channel
3. Verify no conflicting `AutoScheduleEnabled` logic

---

## Summary

✅ **PlayerModifiers**: Channel filtering + AlwaysOn support
✅ **CustomDropEvent**: Channel filtering + AlwaysOn support
✅ **EventManager**: Channel name filtering + RestartOnComplete (quasi-always-on)

All systems now support per-channel configuration for maximum flexibility!
