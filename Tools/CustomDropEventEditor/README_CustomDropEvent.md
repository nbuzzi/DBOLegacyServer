# CustomDropEvent Configuration Guide / Guía de Configuración

This file explains how to configure `CustomDropEvent.cfg` used by GameServer. Below youll find English and Spanish sections.

## English

- Location: `DboServer/ExecutionEnv/config/CustomDropEvent.cfg`
- When it applies: Only while the CustomDropEvent is ON, in `GAMERULE_NORMAL` worlds. Spawn rules also require the killer vs mob level gap ≤ 10.

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
- `rate` is percent; `count` is how many mobs to spawn if the roll succeeds (default 1).  
- Example:  
  `all spawn: 46661101@5` → Any mob kill has a 5% chance to spawn 1 Devil King Piccolo (`46661101`).  
  `46661101 spawn: 16454101@50x2, 16454102@25` → Killing DKP has: 50% spawn 2× 16454101; 25% spawn 1× 16454102.

### Notes
- Item `count` stacks respect each items `byMax_Stack`: if exceeded, the drop is split into multiple stacks (with a safety cap of 10 stacks per kill).
- Items are validated via `IsValidSingleDropIdx`. Invalid IDs are ignored.
- Drops from this event are additional to normal drops.
- Special-case included: Killing `46661101` (Devil King Piccolo) also drops 100 Aztec Coins (`315`) stacked.

### Quick Examples

- Always drop 3 Health Capsules (`11160029`) from mob `16454101`:
  `16454101: 11160029@100x3`

- 20% chance to drop 10 Silver Coins (`315`) stacked from mob `16454102`:
  `16454102: 315@20x10`

- Make a mob tankier and bigger:
  `16454102 modifiers: hp=2.0 physDef=1.3 engDef=1.3 sizeRate=14`

- Global spawn chance for DKP:
  `all spawn: 46661101@5`

---

## Español

- Ubicación: `DboServer/ExecutionEnv/config/CustomDropEvent.cfg`
- Cuándo aplica: Solo mientras el CustomDropEvent está ACTIVO, en mundos `GAMERULE_NORMAL`. Las reglas de invocación (spawn) además requieren diferencia de nivel ≤ 10 entre jugador y mob muerto.

### Tipos de líneas

1) Drops por mob
- Formato: `mobId: item@tasaXcantidad, item@tasa, item`  
- `tasa` es porcentaje (0..100), por defecto 100 si se omite  
- `cantidad` es cuántos ítems soltar si la tirada acierta (se acumula en stacks si el ítem lo permite). Por defecto 1.  
- Ejemplo:  
  `16454102: 11160029@50, 315@25x5, 11160030`  
  → 50% 11160029 (uno), 25% 5× 315, y siempre 11160030.

2) Modificadores por mob
- Formato: `mobId modifiers: clave=valor clave=valor ...`  
- Claves soportadas: `hp, physAtk, engAtk, physDef, engDef, atkSpd, runSpd, attackRate, dodgeRate, blockRate, blockDmg, guardRate, physCrit, engCrit, physCritDmg, engCritDmg, sizeRate`  
- Los valores son multiplicadores, excepto `sizeRate` que es tamaño absoluto (rango típico cliente ~10..20).  
- Ejemplo:  
  `46661101 modifiers: hp=8.0 physAtk=3.0 engAtk=2.5 physDef=2.2 engDef=2.2 atkSpd=0.85 runSpd=1.35 physCrit=1.5 engCrit=1.5 sizeRate=100`

3) Spawns al matar
- Formato: `mobId spawn: mob@tasaXcantidad, mob@tasaXcantidad, ...`  
- También existe el comodín `all` para aplicar a todos los mobs: `all spawn: mob@tasaXcantidad, ...`  
- `tasa` es porcentaje; `cantidad` es cuántos mobs invocar si la tirada acierta (por defecto 1).  
- Ejemplos:  
  `all spawn: 46661101@5` → Cualquier mob muerto tiene 5% de invocar 1 Piccolo Rey Demonio (`46661101`).  
  `46661101 spawn: 16454101@50x2, 16454102@25` → Al matar a DKP: 50% invoca 2× 16454101; 25% invoca 1× 16454102.

### Notas
- La `cantidad` de ítems respeta `byMax_Stack`; si se excede, se divide en varios stacks (tope de 10 stacks por muerte para proteger rendimiento).
- Los ítems se validan con `IsValidSingleDropIdx`. IDs inválidos se ignoran.
- Los drops del evento se suman a los drops normales.
- Caso especial incluido: Al matar `46661101` (Piccolo Rey Demonio) también suelta 100 Monedas Aztecas (`315`) en stack.

### Ejemplos rápidos

- Siempre soltar 3 cápsulas de vida (`11160029`) del mob `16454101`:
  `16454101: 11160029@100x3`

- 20% de soltar 10 monedas de plata (`315`) en stack del mob `16454102`:
  `16454102: 315@20x10`

- Hacer un mob más tanque y grande:
  `16454102 modifiers: hp=2.0 physDef=1.3 engDef=1.3 sizeRate=14`

- Spawn global de DKP:
  `all spawn: 46661101@5`
