# Repository Guidelines

## Project Structure & Module Organization
- `DboServer/`: C++ server code and solution (`DboServer.sln`).
  - `Server/`: Auth, Char, Chat, Game, Master, Query servers.
  - `ExecutionEnv/`: Runtime folder with configs and binaries (do not edit in PRs).
  - `Database/`: SQL schema and `migrations/` (treat as read‑only unless migration is requested).
- `DboShared/`, `NtlLib/`: Shared C++ libraries and third‑party code.
- `Tools/`: .NET utilities (e.g., `WpsStageGen`, `ServerMonitor`, editors).
- `Documentation/`, `DboServer/Documentation/`: Feature and tool guides.

## Build, Test, and Development Commands
- Visual Studio build (recommended): open `DboServer/DboServer.sln` → `Release4Server|x64`.
- MSBuild quick path: `RebuildAndDeploy_GameServer.bat` (rebuilds GameServer and copies to `ExecutionEnv`).
- Tools (.NET): from tool folder, e.g. `dotnet publish WpsStageGen.UI.csproj -c Release -r win-x64 -p:PublishSingleFile=true -p:SelfContained=true -o ./publish`.
- Table editor example: `dotnet run --project Tools/TableEditor/Tools/RdfTableEditor/RdfTableEditor.csproj`.
- Smoke tests: launch from `DboServer/ExecutionEnv` using the provided `start_*.bat` scripts.

## Coding Style & Naming Conventions
- C++: follow `.editorconfig` — tabs, width 4, CRLF; keep existing class/file naming (PascalCase types/files, camelCase members). Avoid mass reformatting.
- Markdown: 2‑space indentation; keep line endings consistent.
- Preserve logging macros: `ERR_LOG`, `NTL_PRINT`, `EVENT_VLOG`. Prefer adding diagnostics to removing logs.

## Testing Guidelines
- No centralized unit test runner; prioritize targeted smoke tests in `ExecutionEnv` and scenario walkthrough comments for gameplay logic.
- When changing combat/spawn/teleport, include a short test plan in the PR description.

## Commit & Pull Request Guidelines
- Commits: imperative and scoped (e.g., `[GameServer] Fix respawn gating`). Keep diffs focused (< ~400 changed lines when possible).
- PRs must include: clear description, affected servers/tools, any config keys added (with defaults), and doc links/updates when relevant.
- Do not commit binaries or generated files (`*.exe`, `*.dll`, `*.pdb`, `ExecutionEnv/`, `x64/`, `Tools/publish/`).

## Agent‑Specific Instructions & Safety
- Respect protected paths: `DboServer/ExecutionEnv/`, `Database/migrations/`, `resource/`, `x64/`.
- Prefer adding features behind config parsed in `LoadConfigFromIniPath`; document keys in docs.
- If a change touches critical flow or protected markers, request clarification before proceeding. See `.github/copilot-instructions.md` for full rules.

## PowerShell Commands & Approvals
- Pre‑approved: run PowerShell for read/build/test/dev within this repo (e.g., `rg`, `Get-ChildItem`, `dotnet publish`, MSBuild, running unit/smoke scripts, local tooling under `Tools/`). No per‑command approval needed.
- Allowed writes: commands that build artifacts inside the workspace (bin/obj/publish folders under Tools) and typical temp files. Do not write into protected paths or commit binaries.
- Destructive ops: avoid unless explicitly requested (e.g., `rm -r`, database mutations, `git reset --hard`). If needed, state intent and scope first.
- Network installs: avoid introducing new dependencies; prefer existing SDKs. If a download is necessary, call it out.
- Note: This document grants repository‑level approval. If the CLI still prompts, adjust the CLI approval policy to a non‑interactive mode (e.g., on‑failure/never) outside of the repo.
