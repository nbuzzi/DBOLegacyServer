# DBO Server Monitor (NET 8 WPF)

A lightweight UI to start/stop/restart and monitor DBO server processes. Supports multiple GameServer channels, ensures Dojo (9) channel, edits INI files, and optionally auto-restarts on crash while collecting logs.

## Features
- Start All in order: Master -> Query -> Auth -> Char -> Chat -> Game (0 and 9 by default)
- Per-process Start/Stop/Restart and Open Logs
- Auto-restart on crash (toggle)
- Start Next Channel (1..9), Ensure Dojo (9)
- INI editor with backups
- ExecutionEnv folder picker (auto-detected if opened from repo)

## Build
Open `Tools/Tools.sln` in Visual Studio 2022 or run a build from the solution. Ensure the project `ServerMonitor` is included in the solution.

## Run
- Start the app, set `ExecutionEnv` to `DboServer/ExecutionEnv`
- Click Start All to bring up the default stack
- Use per-row actions for individual control

## Notes
- Processes are matched by executable path in the selected `ExecutionEnv`
- Log collection copies files under `logs/_monitor/<server>_<timestamp>`
- Stopping attempts a graceful close, then kills after ~1.5s