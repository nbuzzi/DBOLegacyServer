# Mob Appearance Overrides

This server-side feature lets you swap the model that clients see for specific mobs without rebuilding the client.  
The mapping is driven by a plain-text config so you can redirect one or many mob `tblidx` values (for example to make a mob appear as a human).

## Runtime Configuration
Place the config file next to the other channel configs at `.\\config\\MobAppearanceOverrides.cfg` inside your `ExecutionEnv`.  
A `config/MobAppearanceOverrides.cfg.sample` template is included here—copy it into the runtime directory and edit the mappings you need.
The GameServer loads it during startup and keeps it disabled when the file is missing or `Enabled=0`.

```
# Comments can start with #, //, or ;
settings: Enabled=1 Verbose=0

# Use a human mob template for a specific boss while keeping the original stats/drops
mob=6812101 replace=3210001 keepStats=1

# Global fallback (mob=0) that forces every mob to use tblidx 3210001 visuals and stats
mob=0 replace=3210001 useTargetStats=1
```

### Supported keys per line
- `mob=<tblidx>` identifies the source mob (use `0` for a global fallback).
- `replace=<tblidx>` / `target=<tblidx>` / `with=<tblidx>` picks the destination mob template to display.
- `keepStats=1` (or `preserveStats=1`) keeps the source stats/drops while only swapping the appearance.
- `useTargetStats=1` applies the full target template (stats, drops, AI); omit or set to `0` when you only want a cosmetic swap.
- `settings:` accepts `Enabled=0/1` and `Verbose=0/1` for global toggles.

When `keepStats=1`, the server still runs the original mob logic but tells clients to load the replacement model.  
When `useTargetStats=1`, the mob behaves exactly like the replacement template, including combat stats and drops.

> **Tip:** The replacement `tblidx` must already exist in the client data; a human-shaped mob entry is required on both the server and client for the swap to render correctly.
