# AI / Automation Operating Rules

These guidelines tell AI assistants (and contributors) what they MAY and MAY NOT do.

## 🚫 Do NOT
- Edit or commit binaries / generated artifacts (`*.exe`, `*.dll`, `*.pdb`, `ExecutionEnv/`, `x64/`, `Tools/publish/`).
- Remove or silence logging macros (`ERR_LOG`, `NTL_PRINT`, `EVENT_VLOG`) without equivalent diagnostics.
- Introduce new third‑party dependencies.
- Reformat entire files or mass-change whitespace; keep diffs minimal.

## ✅ Allowed / Encouraged
- Add new features behind a config key (default OFF or safe default) and document the key.
- Use existing helper functions for world creation, teleport, and spawning.
- Add focused tests or harness comments when changing combat, spawn, or teleport logic.
- Extend config parsing in `LoadConfigFromIniPath` instead of creating duplicate parsing code.

## 🔐 Protected Files / Paths
```
DboServer/ExecutionEnv/
Database/migrations/
resource/
x64/
```

## 🧱 Region Protection Markers
Use these marker comments to denote critical code (AI should not modify the contents unless explicitly asked):
```cpp
// AI-NO-EDIT START: <reason>
// ...critical logic...
// AI-NO-EDIT END
```

## 🧪 Feature Introduction Checklist
1. Define config key with clear name (e.g. `EnableXYZ`).
2. Add default in `Config` struct (EventManager.h).
3. Parse in `LoadConfigFromIniPath` with safe fallback.
4. Guard usage: `if (m_cfg.enableXyz) { ... }`.
5. Add at least one `EVENT_VLOG` (when enabled) and one `ERR_LOG` on failure.
6. Update `Documentation/EventManager_README.md` or relevant guide.

## 📝 Logging Rules
- Use wide string literals for system/player messages: `L"[EVENT] ..."`.
- When replacing `_T("%S")` patterns, prefer wide literals or existing macros.
- Keep severity: do not downgrade `ERR_LOG` to `NTL_PRINT`.

## ⚖️ Change Size Limit
Pull requests or automated patches should avoid exceeding ~400 changed lines unless performing a documented migration.

## 🧪 Testing Expectations
For logic that changes progression (round timers, teleport gating, spawn distribution):
- Add a comment block scenario walkthrough.
- Prefer not to depend on real time > 5s in any test harness.

## ♻️ Rollback Strategy
Wrap optional features or experimental changes in clearly delimited blocks:
```cpp
// FEATURE:INTERMISSION (BEGIN)
// ... code ...
// FEATURE:INTERMISSION (END)
```
Allows quick reverse or isolation.

## 🆘 If Uncertain
AI assistants should respond with a clarification request instead of guessing when:
- A symbol appears in a protected region and change would alter control flow.
- A config key usage is ambiguous (missing default or documentation).

## 📄 Reserved Identifiers
`ERR_LOG`, `NTL_PRINT`, `EVENT_VLOG`, `StartTeleport`, `ScheduleAutoAfterTermination`, `TeleportParticipantsToWorld`.

## 🔄 Post-Event Automation
When altering event termination logic, ensure `ScheduleAutoAfterTermination` remains invoked on:
- Manual Stop
- Enrollment cancellation (no participants)

## 🧩 File Patterns To Skip
```
*.sql
*.pdb
*.exe
*.dll
*.bat
*.ps1
```
(Unless the explicit user request is to modify that file.)

## ✅ EventManager Editing
`EventManager.*` is now fully editable. Keep changes purposeful; large structural rewrites should still explain rationale in comments and update docs.

---
_Last updated: 2025-10-04_
