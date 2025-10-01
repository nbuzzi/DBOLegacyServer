# Sistema de Gestor de Eventos - Guía Completa

## Tabla de Contenidos
- [Resumen General](#resumen-general)
- [Inicio Rápido](#inicio-rápido)
- [Archivo de Configuración](#archivo-de-configuración)
- [Configuración de Rondas](#configuración-de-rondas)
- [Grupos de Esbirros](#grupos-de-esbirros)
- [Rotación de Mundos](#rotación-de-mundos)
- [Auto-Reinicio](#auto-reinicio)
- [Comandos de Jugador](#comandos-de-jugador)
- [Ejemplos Avanzados](#ejemplos-avanzados)
- [Solución de Problemas](#solución-de-problemas)

---

## Resumen General

El **Gestor de Eventos** es un sistema flexible de eventos PvE basado en rondas para Dragon Ball Online. Te permite crear eventos automáticos o manuales donde los jugadores luchan contra oleadas de mobs a través de múltiples rondas, con recompensas por cada ronda y al completar el evento.

### Características Principales
- **Progresión por rondas**: Múltiples rondas con diferentes mobs y configuraciones
- **Sistema de Jefe + Esbirros**: Genera guardias esbirros alrededor de los jefes
- **Rotación de mundos**: Cada ronda puede usar un mundo/mapa diferente
- **Auto-reinicio**: Crea eventos en bucle infinito que se reinician automáticamente
- **Integración con CustomDropEvent**: Usa mobs modificados con stats/drops personalizados
- **Recompensas flexibles**: Ítems fijos, drops de botín en rango, o puntos Mudosa
- **Aislamiento por canal**: Solo se ejecuta en canales llamados "EVENTS"

---

## Inicio Rápido

### 1. Habilitar el Sistema de Eventos
Edita `config/Events.cfg`:
```ini
[Event]
Enabled = 1
ChannelNameContains = EVENTS
```

### 2. Configurar Rondas Básicas
```ini
# Evento simple de 3 rondas
# Formato: mobs:recompensa:cantidad:duración:mundo
Rounds = 3411,3412:11120220:2:0:180:1;3413,3414:RANGE:19916:10:240:500;3416:11120093:5:0:300:13000
```

### 3. Iniciar el Evento
Los jugadores en el canal EVENTS pueden escribir:
```
@participate
```

### 4. Flujo del Evento
1. **Inscripción** - Los jugadores se unen con el comando `@participate`
2. **Teletransporte** - Los jugadores son teletransportados al mundo del evento
3. **Inicio de Ronda** - Los mobs aparecen, el temporizador comienza
4. **Matar Todos los Mobs** - Los jugadores deben eliminar todos los mobs generados
5. **Ronda Completada** - Se distribuyen las recompensas
6. **Siguiente Ronda** - Se repiten los pasos 3-5 para cada ronda
7. **Evento Completado** - Todas las rondas terminadas, jugadores teletransportados de vuelta

---

## Archivo de Configuración

### Sección [Event]

```ini
[Event]
Enabled = 1                          # Habilitar/deshabilitar sistema de eventos
ChannelNameContains = EVENTS         # Solo ejecutar en canales con "EVENTS" en el nombre
EventWorldTblidx = 1                 # Mundo predeterminado para eventos
SpawnPosX = 5000.0                   # Posición de spawn predeterminada X
SpawnPosY = 0.0                      # Posición de spawn predeterminada Y
SpawnPosZ = 4000.0                   # Posición de spawn predeterminada Z
EnrollmentSeconds = 300              # Cuánto tiempo está abierta la inscripción (5 minutos)
RequireParticipateCommand = 1        # Requerir comando @participate (1=sí, 0=no)
StartDelaySeconds = 10               # Retraso antes de la primera ronda después del teletransporte
MobSpawnRadius = 50.0                # Radio para posicionamiento aleatorio de mobs
RandomMobPositions = 1               # Aleatorizar posiciones (1=sí, 0=no)
VerboseLogs = 1                      # Habilitar registro detallado (1=sí, 0=no)
```

### Sección [AutoEvent]

```ini
[AutoEvent]
Enabled = 1                          # Habilitar programación automática de eventos
IntervalSeconds = 1800               # Tiempo entre eventos (30 minutos)
InitialDelaySeconds = 600            # Retraso antes del primer evento después del inicio del servidor
RestartOnComplete = 1                # Auto-reiniciar después de completar (bucle infinito)
RestartDelaySeconds = 300            # Retraso antes de reiniciar (5 minutos)
```

### Sección [WorldRotation]

```ini
[WorldRotation]
Enabled = 1                          # Habilitar rotación de mundos por ronda
WorldList = 1,500,13000,43000        # IDs de mundos separados por comas para rotar
RandomizeWorlds = 0                  # 0=secuencial, 1=selección aleatoria
```

### Configuración de Recompensas

```ini
MudosaPerRound = 1000                # Puntos Mudosa por completar ronda
MudosaEventComplete = 5000           # Bono Mudosa por completar todas las rondas

# Teletransporte post-evento
PostEventTeleport = 1
PostEventWorldTblidx = 1
PostEventPosX = 4975.609863
PostEventPosY = -48.869999
PostEventPosZ = 4012.609863
PostEventTeleportDelayMs = 3000
```

---

## Configuración de Rondas

### Formato Básico

```
mobs:tipo_recompensa:param1:param2:duración:mundo:esbirros
```

### Parámetros Explicados

| Parámetro | Descripción | Ejemplo |
|-----------|-------------|---------|
| `mobs` | IDs de mobs separados por comas | `3411,3412,3413` |
| `tipo_recompensa` | `RANGE` para drop de botín o ID de ítem | `RANGE` o `11120220` |
| `param1` | Cantidad de ítems o ID de ítem de botín | `5` o `19916` |
| `param2` | 0 para fijo, o cantidad de botín | `0` o `10` |
| `duración` | Duración de la ronda en segundos | `180` |
| `mundo` | ID del mundo (0=usar rotación) | `1` o `500` |
| `esbirros` | Configuración de esbirros (opcional) | `3410,3411\|15\|5` |

### Ejemplo de Recompensas Fijas

```ini
# Generar mobs 3411,3412 → Dar ítem 11120220 x2 → 180 segundos → Mundo 1
Rounds = 3411,3412:11120220:2:0:180:1
```

### Ejemplo de Rango de Botín

```ini
# Generar mobs 3413,3414 → Esparcir ítem 19916 x10 → 240 segundos → Mundo 500
Rounds = 3413,3414:RANGE:19916:10:240:500
```

### Múltiples Rondas

Separa las rondas con punto y coma (`;`):
```ini
Rounds = 3411:11120220:2:0:180:1;3412:RANGE:19916:10:240:500;3413:11120093:5:0:300:13000
```

---

## Grupos de Esbirros

### ¿Qué son los Esbirros?

Los esbirros son mobs adicionales que se generan alrededor de **cada** mob jefe. Esto crea encuentros más desafiantes con mecánicas de jefe + adds.

### Formato de Esbirros

```
esbirro1,esbirro2,esbirro3|radio|cantidad
```

| Parte | Descripción | Predeterminado |
|------|-------------|----------------|
| `esbirro1,esbirro2` | IDs de mobs para generar como esbirros | Requerido |
| `radio` | Radio de spawn alrededor del jefe (metros) | 15.0 |
| `cantidad` | Total de esbirros a generar (0=uno de cada) | 0 |

### Ejemplos

#### Ejemplo 1: Jefe con 5 Esbirros Aleatorios
```ini
# Jefe 3416 + 5 esbirros (mezcla aleatoria de 3411,3412) dentro de 15m
Rounds = 3416:RANGE:19916:10:300:1:3411,3412|15|5
```
**Resultado**: Genera el jefe 3416, luego genera 5 esbirros elegidos aleatoriamente de [3411, 3412] en un radio de 15m alrededor del jefe.

#### Ejemplo 2: Jefe con Uno de Cada Tipo de Esbirro
```ini
# Jefe 3420 + uno de cada tipo de esbirro dentro de 20m
Rounds = 3420:11120093:5:0:240:500:3415,3416,3417|20|0
```
**Resultado**: Genera el jefe 3420, luego genera exactamente 3 esbirros (uno 3415, uno 3416, uno 3417) en un radio de 20m.

#### Ejemplo 3: Múltiples Jefes con Esbirros
```ini
# 2 jefes, CADA UNO recibe esbirros
Rounds = 3416,3417:RANGE:19916:10:300:1:3411,3412|15|5
```
**Resultado**:
- Jefe 3416 con 5 esbirros a su alrededor
- Jefe 3417 con 5 esbirros a su alrededor

#### Ejemplo 4: Rush de Jefes con Esbirros Escalados
```ini
# Ronda 1: Jefe + 3 esbirros
# Ronda 2: Jefe + 5 esbirros
# Ronda 3: Jefe + 8 esbirros
Rounds = 3416:RANGE:19916:10:180:1:3411,3412|15|3;3416:RANGE:19916:10:180:1:3411,3412,3413|18|5;3416:RANGE:19916:10:180:1:3411,3412,3413,3414|22|8
```

---

## Rotación de Mundos

### Cómo Funciona

La rotación de mundos permite que cada ronda use un mapa/mundo diferente. Hay **3 niveles de prioridad**:

1. **Mundo específico de ronda** - Definido en la configuración de la ronda
2. **Lista de rotación de mundos** - Desde la configuración `WorldList`
3. **Mundo predeterminado** - Desde `EventWorldTblidx`

### Rotación Secuencial

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000
RandomizeWorlds = 0

# Las rondas sin mundo especificado usan rotación
Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:0;3413:RANGE:19916:10:180:0
```
**Resultado**:
- Ronda 1 → Mundo 1
- Ronda 2 → Mundo 500
- Ronda 3 → Mundo 13000

### Selección Aleatoria de Mundo

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000,43000,44000
RandomizeWorlds = 1

Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:0;3413:RANGE:19916:10:180:0
```
**Resultado**: Cada ronda elige un mundo aleatorio de la lista.

### Modo Mixto

```ini
[WorldRotation]
Enabled = 1
WorldList = 1,500,13000

# Ronda 1: Usa rotación (Mundo 1)
# Ronda 2: Fuerza Mundo 43000
# Ronda 3: Usa rotación (Mundo 500)
Rounds = 3411:RANGE:19916:10:180:0;3412:RANGE:19916:10:180:43000;3413:RANGE:19916:10:180:0
```

---

## Auto-Reinicio

### Modo de Bucle Infinito

Crea eventos que se reinician automáticamente después de completarse:

```ini
[AutoEvent]
Enabled = 1
RestartOnComplete = 1        # Habilitar bucle infinito
RestartDelaySeconds = 300    # Esperar 5 minutos antes de reiniciar
```

### Cómo Funciona

1. El evento ejecuta todas las rondas hasta completarse
2. Los jugadores son teletransportados de vuelta
3. El sistema espera `RestartDelaySeconds`
4. La inscripción se vuelve a abrir automáticamente
5. El proceso se repite indefinidamente

### Casos de Uso

- **Eventos de farmeo 24/7**: Los jugadores pueden unirse en cualquier momento
- **Bucles de rush de jefes**: Encuentros continuos con jefes
- **Áreas de entrenamiento**: Zona de práctica siempre disponible

### Ejemplo de Configuración

```ini
[Event]
EnrollmentSeconds = 120          # Ventana de inscripción de 2 minutos

[AutoEvent]
Enabled = 1
RestartOnComplete = 1
RestartDelaySeconds = 180        # Descanso de 3 minutos entre bucles

[WorldRotation]
Enabled = 1
WorldList = 1,500,13000
RandomizeWorlds = 1              # Mundo diferente cada bucle

# 3 rondas con dificultad escalada
Rounds = 3411:RANGE:19916:5:120:0:3410|10|3;3412:RANGE:19916:10:150:0:3410,3411|15|5;3413:RANGE:19916:15:180:0:3410,3411,3412|20|8
```

---

## Comandos de Jugador

### @participate

Unirse al evento durante la fase de inscripción.

```
@participate
```

**Requisitos**:
- Debe estar en un canal con "EVENTS" en el nombre
- El evento debe estar en estado ENROLLMENT (INSCRIPCIÓN)
- No estar ya inscrito

---

## Ejemplos Avanzados

### Ejemplo 1: Rush de Jefes con Dificultad Creciente

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

# 5 rondas, cada una más difícil que la anterior
# Ronda 1: Jefe fácil + 3 esbirros
# Ronda 2: Jefe medio + 5 esbirros
# Ronda 3: Jefe difícil + 8 esbirros
# Ronda 4: Jefe muy difícil + 12 esbirros
# Ronda 5: Jefe final + 15 esbirros
Rounds = 3411:RANGE:19916:5:120:0:3410|12|3;3412:RANGE:19916:8:150:0:3410,3411|15|5;3413:RANGE:19916:12:180:0:3410,3411,3412|18|8;3414:RANGE:19916:18:240:0:3410,3411,3412,3413|22|12;3415:RANGE:19916:25:300:0:3410,3411,3412,3413,3414|25|15

MudosaPerRound = 1500
MudosaEventComplete = 10000
```

### Ejemplo 2: Encuentros Multi-Jefe

```ini
# Cada ronda genera múltiples jefes con sus propios esbirros
# Ronda 1: 2 jefes con esbirros
# Ronda 2: 3 jefes con esbirros
# Ronda 3: 4 jefes con esbirros

Rounds = 3416,3417:11120093:5:0:240:1:3411,3412|12|4;3416,3417,3418:11120093:8:0:300:500:3411,3412,3413|15|5;3416,3417,3418,3419:11120093:12:0:360:13000:3411,3412,3413,3414|18|6
```

### Ejemplo 3: Tour Temático de Mundos

```ini
# Cada ronda en una ubicación temática diferente
[WorldRotation]
Enabled = 0  # Deshabilitado, usando mundos por ronda

# Ronda 1: Bosque (Mundo 1)
# Ronda 2: Desierto (Mundo 500)
# Ronda 3: Hielo (Mundo 13000)
# Ronda 4: Volcán (Mundo 43000)
# Ronda 5: Espacio (Mundo 44000)

Rounds = 3411:RANGE:19916:10:180:1:3410,3411|15|5;3412:RANGE:19916:12:180:500:3410,3412|15|6;3413:RANGE:19916:15:180:13000:3410,3413|15|7;3414:RANGE:19916:18:180:43000:3410,3414|15|8;3415:RANGE:19916:20:240:44000:3410,3411,3412,3413,3414|20|10
```

### Ejemplo 4: Defensa de Oleadas

```ini
# Defiende contra oleadas de enemigos sin jefe
[Event]
MobSpawnRadius = 30.0

# 10 oleadas de mobs regulares (sin jefe, solo patrón de esbirros)
Rounds = 3411:RANGE:19916:5:120:1:3410,3411,3412|25|8;3412:RANGE:19916:5:120:1:3410,3411,3412,3413|25|10;3413:RANGE:19916:5:120:1:3410,3411,3412,3413,3414|25|12;3414:RANGE:19916:5:120:1:3410,3411,3412,3413,3414,3415|25|15;3415:RANGE:19916:10:150:1:3410,3411,3412,3413,3414,3415,3416|30|20
```

---

## Solución de Problemas

### El Evento No Inicia

**Problema**: El evento no comienza después de escribir `@participate`

**Soluciones**:
1. Verifica que el nombre del canal contenga "EVENTS"
2. Verifica `Enabled = 1` en la sección `[Event]`
3. Comprueba que el estado del evento sea ENROLLMENT (usa logs detallados)
4. Asegúrate de que `RequireParticipateCommand = 1` si es unión manual

### Los Esbirros No Se Generan

**Problema**: Los jefes aparecen pero no los esbirros

**Soluciones**:
1. Verifica el formato de esbirros: `mobId1,mobId2|radio|cantidad`
2. Verifica que los IDs de mobs esbirros sean válidos
3. Revisa los logs detallados para errores de spawn
4. Asegúrate de que la sección de esbirros use separador pipe `|`, no dos puntos

### La Rotación de Mundos No Funciona

**Problema**: Siempre genera en el mismo mundo

**Soluciones**:
1. Verifica `Enabled = 1` en `[WorldRotation]`
2. Comprueba que `WorldList` tenga múltiples IDs de mundo válidos
3. Asegúrate de que las rondas no especifiquen mundo (usa `:0:` u omítelo)
4. Revisa los logs detallados para la selección de mundo

### El Auto-Reinicio No Funciona

**Problema**: El evento termina pero no se reinicia

**Soluciones**:
1. Verifica `RestartOnComplete = 1` en `[AutoEvent]`
2. Comprueba `Enabled = 1` en `[AutoEvent]`
3. Espera a que `RestartDelaySeconds` se complete
4. Revisa los logs detallados para transiciones de estado de reinicio

### Los Mobs No Mueren / La Ronda No Se Completa

**Problema**: Mataste todos los mobs pero la ronda no se completa

**Soluciones**:
1. Busca mobs atascados en paredes/terreno
2. Verifica que todos los mobs generados estén rastreados en el sistema
3. Usa logs detallados para ver el seguimiento de muertes
4. Comprueba que `MobSpawnRadius` no sea demasiado grande

---

## Referencia de Configuración

### Plantilla de Configuración Completa

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

Rounds = TuConfiguraciónDeRondasAquí

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

## Soporte

Para problemas o preguntas:
1. Activa logs detallados: `VerboseLogs = 1`
2. Revisa la sintaxis de configuración
3. Prueba primero con una configuración simple
4. Verifica la consola del servidor para mensajes de error

---

**Versión**: 1.0
**Última Actualización**: 2025-01-10
