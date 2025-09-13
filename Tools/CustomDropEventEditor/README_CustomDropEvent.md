# CustomDropEvent Configuration Guide / Guía de Configuración

This file explains how to configure `CustomDropEvent.cfg` used by GameServer. Below you’ll find English and Spanish sections.

## English

- Location: `DboServer/ExecutionEnv/config/CustomDropEvent.cfg`
- When it applies: Only while the CustomDropEvent is ON, in `GAMERULE_NORMAL` worlds. Spawn rules also require the killer vs mob level gap ≤ 10 for sub‑100% chances.

### Line Types

1) Per-mob item drops
- Format: `mobId: item@ratexcount, item@rate, item`  
- `rate` is percent (0..100), default 100 if omitted  
- `count` is how many items to drop if the roll succeeds (stacked where possible). Default 1.
- Example:  
  `16454102: 11160029@50, 315@25x5, 11160030`  
  → 50% chance 11160029 (one), 25% chance 5× 315, and always 11160030.

2) Per-mob modifiers
- Format: `mobId modifiers: key=value key=value ...`  
- Supported keys: `hp, physAtk, engAtk, physDef, engDef, atkSpd, runSpd, attackRate, dodgeRate, blockRate, blockDmg, guardRate, physCrit, engCrit, physCritDmg, engCritDmg, sizeRate`
- Values are multipliers, except `sizeRate` which is an absolute size (typical client range ~10..20).  
- Example:  
  `46661101 modifiers: hp=8.0 physAtk=3.0 engAtk=2.5 physDef=2.2 engDef=2.2 atkSpd=0.85 runSpd=1.35 physCrit=1.5 engCrit=1.5 sizeRate=100`

3) On-kill spawns
- Format: `mobId spawn: mob@ratexcount, mob@ratexcount, ...`  
- You may also use wildcard `all` to apply to every mob: `all spawn: mob@ratexcount, ...`  
- You can place multiple `all spawn:` lines; all entries are appended (not overwritten).
- `rate` is percent; `count` is how many mobs to spawn if the roll succeeds (default 1).  
- Example:  
  `all spawn: 46661101@5` → Any mob kill has a 5% chance to spawn 1 Devil King Piccolo (`46661101`).  
  `46661101 spawn: 16454101@50x2, 16454102@25` → Killing DKP has: 50% spawn 2× 16454101; 25% spawn 1× 16454102.

4) Buffs (applied to spawned mob and normal spawns during the event)
- Format: `mobId buffs: skillTblidx@durationMs, skillTblidx, ...`  
- Global: `all buffs: skillTblidx@durationMs, ...` applies to all mobs.  
- You can place multiple `all buffs:` lines; all entries are appended (not overwritten).  
 - Duration is optional; omit to use default duration from tables. Unit is milliseconds (ms).  
- Examples:  
  `all buffs: 123@30000, 456` → Apply skill 123 for 30s and skill 456 with default duration to all mobs.  
  `46661101 buffs: 789@15000` → DKP receives buff 789 for 15s.

5) Totems (AoE buff/heal beacons)
- Format: `mobId totem: beaconMob@life@radius@interval: skill@duration[@period], ...`  
  - `beaconMob`: mob tblidx to spawn as the beacon
  - `life`: lifetime of the beacon (ms or with `s` suffix)
  - `radius`: application radius in meters
  - `interval`: default pulse period (ms)
  - For each buff: `skill@duration[@period]`  
    - `skill`: `SkillTable` tblidx (not SystemEffect)
    - `duration`: buff keep time (ms or `Xs`)
    - `period` (optional): how often this specific buff triggers; defaults to `interval`
- Effects:  
  - Direct heal skills are applied instantly each period (no buff icon).  
  - HoT/DoT and normal buffs are registered with the given duration.  
  - `healMul` multiplies both direct-heal and HoT magnitudes.
- Global settings: `all settings: radius=<m> interval=<ms> healMul=<x> duration=<ms>`  
  - `duration` here overrides totem buff durations globally (0 = use skill/default).
- Examples:
  - `all totem: 3131101@60s@120@1500: 1520834@3s@1500`  
  - `46661101 totem: 3131101@30000@100@2000: 1529996@3s, 1520951@10s@5s`

### Exceptions (global rules)
- Use `all <section> except: id1, id2` to skip specific mobs for that section:  
  - `all spawn except: 16454101, 16454102`  
  - Applies to: drops, modifiers, spawns, buffs, titles, visuals, totems

### GM Commands
- `@start_customdrop [hours]`: Starts the event for the given hours (0 or omitted uses default 3h; capped at 24h).
- `@stop_customdrop`: Stops the event immediately.
- `@reload_customdrop <name|path>`: Reloads config; bare name resolves to `.\\config\\<name>.cfg`.
- `@customdrop_chainspawns on|off`: Allow event-spawned mobs to trigger spawns/totems.
- `@customdrop_healmul <float>`: Sets heal multiplier for direct-heal/HoT (no arg prints current).
- `@customdrop_buffduration <ms>`: Overrides all totem buff durations (0 = reset; no arg prints current).
- `@buff BUFF_ID DURATION_SECONDS [RADIUS_METERS] [TARGET_NAME]`: Applies a buff.
  - If `RADIUS_METERS > 0`, applies to all PCs within radius of the target (or caster if no target).
  - If radius omitted or `0`, applies only to the target (or caster if target missing).
  - Direct-heal skills are applied instantly.

---

## Español

- Ubicación: `DboServer/ExecutionEnv/config/CustomDropEvent.cfg`
- Cuándo aplica: Solo mientras el evento está activo en mundos `GAMERULE_NORMAL`. Para probabilidades < 100% en spawns, se requiere diferencia de nivel ≤ 10.

### Tipos de líneas

1) Drops por mob
- Formato: `mobId: item@tasaXcantidad, item@tasa, item`
- `tasa` en % (0..100), por defecto 100 si se omite
- `cantidad` cuántos ítems se sueltan si acierta, apilando si es posible (por defecto 1).

2) Modificadores por mob
- Formato: `mobId modifiers: clave=valor ...` (ver lista en inglés)

3) Spawns al matar
- Formato: `mobId spawn: mob@tasaXcantidad, ...` y `all spawn: ...`

4) Buffs
- Formato: `mobId buffs: skillTblidx@duracion[, ...]` (duración en milisegundos)

5) Tótems (balizas de buff/curación AoE)
- Formato: `mobId totem: baliza@vida@radio@intervalo: skill@duracion[@periodo], ...`
  - `vida`/`duracion`/`periodo`: en ms o con sufijo `s` (segundos)
  - Sanación directa se aplica al instante; HoT/DoT se registran con duración
  - `healMul` multiplica sanaciones directa y por tiempo
- Ajustes globales: `all settings: radius=<m> interval=<ms> healMul=<x> duration=<ms>`
- Ejemplos:  
  `all totem: 3131101@60s@120@1500: 1520834@3s@1500`  
  `46661101 totem: 3131101@30000@100@2000: 1529996@3s, 1520951@10s@5s`

### Excepciones (reglas globales)
- `all <sección> except: id1, id2` (aplica a drops, modifiers, spawns, buffs, titles, visuals, totems)

### Comandos GM
- `@start_customdrop [hours]`: Inicia el evento por la cantidad de horas indicada (0 u omitido usa 3h por defecto; máximo 24h).
- `@stop_customdrop`: Detiene el evento inmediatamente.
- `@reload_customdrop <nombre|ruta>`: Recarga la configuración; nombre simple resuelve a `.\\config\\<nombre>.cfg`.
- `@customdrop_chainspawns on|off`: Permite que los mobs invocados por el evento activen spawns/tótems.
- `@customdrop_healmul <float>`: Establece el multiplicador de sanación (sin argumento muestra el valor actual).
- `@customdrop_buffduration <ms>`: Sobrescribe la duración de buffs de tótem (0 = reiniciar; sin argumento muestra el valor actual).
- `@buff BUFF_ID DURATION_SECONDS [RADIUS_METERS] [TARGET_NAME]`: Aplica un buff.
  - Si `RADIUS_METERS > 0`, aplica a todos los jugadores dentro del radio del objetivo (o del caster si no hay objetivo).
  - Si el radio se omite o es `0`, solo aplica al objetivo (o al caster si no hay objetivo).
  - Las curaciones directas se aplican de inmediato.
