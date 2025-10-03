# Feature Flags System

## Overview

The Feature Flags system allows you to enable or disable specific server features at runtime through a simple configuration file. This is useful for testing, gradual rollouts, or temporarily disabling problematic features.

## Configuration File

Location: `DboServer/ExecutionEnv/config/FeatureFlags.cfg`

## Available Feature Flags

| Flag | Description | Default |
|------|-------------|---------|
| `EnableVirtualTransformations` | Virtual transformation system (custom transformations) | 1 (Enabled) |
| `EnableBudokai` | Budokai Tournament System | 1 (Enabled) |
| `EnableDojo` | Dojo System | 1 (Enabled) |
| `EnableRankBattle` | Rank Battle System | 1 (Enabled) |
| `EnableDWC` | Dragon Ball World Championship | 1 (Enabled) |
| `EnableTMQ` | Time Machine Quest System | 1 (Enabled) |
| `EnableQuickSlot` | Quick Slot System | 1 (Enabled) |
| `EnableItemUpgrade` | Item Upgrade System | 1 (Enabled) |
| `EnableItemExchange` | Item Exchange/Trade System | 1 (Enabled) |
| `EnablePartyMatchmaking` | Party Matchmaking System | 1 (Enabled) |

## Usage

### Enabling/Disabling Features

Edit the `FeatureFlags.cfg` file and set the value to:
- `1` to enable the feature
- `0` to disable the feature

Example:
```ini
[Features]
# Disable virtual transformations
EnableVirtualTransformations=0

# Keep budokai enabled
EnableBudokai=1
```

### Applying Changes

Changes require a server restart to take effect.

## Implementation Details

### For Developers

#### Using Feature Flags in Code

Include the header:
```cpp
#include "FeatureFlags.h"
```

Check a feature flag:
```cpp
if (!g_pFeatureFlags->IsVirtualTransformationsEnabled())
{
    // Feature is disabled, handle accordingly
    ERR_LOG(LOG_GENERAL, "Virtual transformations are disabled");
    return false;
}
```

#### Available Methods

```cpp
bool IsVirtualTransformationsEnabled() const;
bool IsBudokaiEnabled() const;
bool IsDojoEnabled() const;
bool IsRankBattleEnabled() const;
bool IsDWCEnabled() const;
bool IsTMQEnabled() const;
bool IsQuickSlotEnabled() const;
bool IsItemUpgradeEnabled() const;
bool IsItemExchangeEnabled() const;
bool IsPartyMatchmakingEnabled() const;
```

#### Runtime Modification (for testing)

```cpp
// Disable virtual transformations at runtime
g_pFeatureFlags->SetVirtualTransformationsEnabled(false);
```

## Example: Virtual Transformations

When `EnableVirtualTransformations=0`, the virtual transformation system will reject all transformation requests with a log message:

```
[VTRANSFORM] Virtual transformations are disabled by feature flag
```

Players attempting to use the `@vtransform_start` GM command will see the transformation fail.

## Adding New Feature Flags

1. Add the flag to `FeatureFlags.cfg` under `[Features]`
2. Add a private member to `CFeatureFlags` class (`FeatureFlags.h`)
3. Add getter/setter methods in the header
4. Initialize the flag in `CFeatureFlags::Init()` (`FeatureFlags.cpp`)
5. Load the flag in `CFeatureFlags::LoadFromFile()` (`FeatureFlags.cpp`)
6. Add logging for the flag in `LoadFromFile()`
7. Use the flag in relevant code

## Notes

- If the configuration file is missing, all features default to **enabled**
- Invalid values in the config file will default to enabled
- The system logs all loaded feature flag values on server startup
- Feature flags are loaded early in the server initialization process
