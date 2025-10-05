# OpenDBO
DBO Client, Server and Tools software.

## 🤖 AI Assistant / Automation Guidelines (Summary)
For full rules see `.github/copilot-instructions.md`.
- Do not edit: `ExecutionEnv/`, `Database/migrations/`, `resource/`, `x64/`.
- Keep diffs minimal; no mass reformatting.
- New features should prefer a config key with a documented default.
- Preserve logging (`ERR_LOG`, `NTL_PRINT`, `EVENT_VLOG`). Add diagnostics on failures.
- EventManager is editable; mark any large refactors with a short rationale comment.

---

## Setting everything up
All required third party tools can be obtained from [our 3rd party repository](https://github.com/OpenDBO/OpenDBO-3rdParty/releases).
<details>
 <summary>Compiling the Client</summary>

1. **Download / Install the necessary files:**
    - DirectX9.
        - In case of encountering error S1023, execute the following commands as administrator:
            ```
            MsiExec.exe /passive /X{F0C3E5D1-1ADE-321E-8167-68EF0DE699A5}
            MsiExec.exe /passive /X{1D8E6291-B0D5-35EC-8441-6616F567A0F7}
            ```
    - 3rd party tools that you downloaded from [our 3rd party repository](https://github.com/OpenDBO/OpenDBO-3rdParty/releases).
    - Client files: Download them from the [Assets repository](https://github.com/OpenDBO/OpenDBO-Assets). First get the Base Client from the Releases section and then replace all needed files with the updated ones on the repo itself.
    - Download v142 build tools (C++ MFC) via Visual Studio Installer.

2. **Navigate to the following path:** `source\repos\OpenDBO-Core` and create a folder named `3rdParty`. Move the Xtreme Toolkit folder there.

3. **Move the GFx SDK 3.3 folder into:** `source\repos\OpenDBO-Core\DBOClient\Lib\NtlFlasher`

4. **Access the GFx SDK 3.3 folder you just moved:**
    - Navigate to `source\repos\OpenDBO-Core\DBOClient\Lib\NtlFlasher\GFx SDK 3.3\3rdParty\jpeg-6b\gfx_projects\Win32`
    - Rename the folder named "Msvc80" to "Msvc142."

5. **Enter the Msvc142 folder and open the project:** "libjpeg.sln"

6. **Ensure the solution configuration in VS 2019 is set to:** "Release and Win32." If not, adjust it.

7. **Configure the libjpeg project settings:**
    - Right-click on libjpeg, go to Properties, and under Windows SDK Version, set it to the latest (10.0).
    - Change the Platform Toolset to (v142).
    - Right-click on libjpeg -> Build.

8. **Navigate to folder:** `source\repos\OpenDBO-Core\DboClient\DragonBall` and extract the client's contents.
    - The downloaded RAR file should contain a folder named DragonBall; copy and paste its contents into the specified path.
    - This RAR file is from the "Client" link provided above.

9. **Go to folder:** `source\repos\OpenDBO-Core\DboClient` and open "DboClient.sln"

10. **In Solution Explorer, go to DBO\Client.vcxproj and repeat step 7.**

11. **In Solution Explorer, navigate to Tools\2DParticleEditor, right-click, and unload it.**

12. **Right-click on Client.vcxproj -> Build**
    - You should encounter only one error after compiling: 'libjpeg.lob'

13. **Right-click on Client.vcxproj then go to properties:**
    - Navigate to Linker -> General -> Additional Library Directories.
    - Update the paths to match your system: 
        - `$(SolutionDir)Lib\NtlFlasher\GFx SDK 3.3\Lib\$(PlatformName)\Msvc80\Release`
        - `$(SolutionDir)Lib\NTlFlasher\GFx SDK 3.3\3rdParty\jpeg-6b\lib\$(PlatformName)\Msvc142\Release`

14. **Right-click on Client.vcxproj -> Rebuild**

15. **The Client should have compiled successfully.**

</details>

<details>
 <summary>Setting up the Server</summary>

1. **Requirements:**
    - Windows 10
    - 8 GB of RAM

2. **Download and install Visual Studio 2019:**
    - [Visual Studio 2019](https://my.visualstudio.com/Downloads?q=visual%20studio%202019)
    - Select the "Community - Free Download" option to obtain the installer app.
    - Open the installer app and follow these steps:
        - Navigate to the "Workloads" section and install both "Desktop development with C++" and "Game development with C++."
        - In the "Individual components" section, install "C++ Clang-cl for v142 build tools (x64/x86)."
    - Click "Install while downloading" and wait for the process to complete.

3. **Download and extract the OpenDBO-Core Repository:**
    - [OpenDBO-Core Repository](https://github.com/OpenDBO/OpenDBO-Core). Click "Code" -> Download ZIP.
    - Extract the ZIP file to the main folder named "OpenDBO-Core."

4. **Download and install/extract additional programs: it's recommended to set up the Client repository now. Otherwise perform steps 1 and 2 from the Client guide (if you already did it you can skip this step)**

5. **Compile the private server using Visual Studio 2019:**
    - Navigate to the "OpenDBO-Core" main folder, then to the "Dboserver" subfolder.
    - Double-click "Dboserver.sln" to open it in Visual Studio 2019.
    - Ensure "Release" and "x64" are selected from the top bar.
    - In the "Solution Explorer," under "Server," right-click each server (AuthServer, CharServer, etc.) and click "Build" one by one.
    - Wait for the server executable files to finish compiling.

6. **Download and install XAMPP (you can also install the latest version of MariaDB directly, if so, you can skip this step and some steps of 7.):**
    - Deactivate User Account Control (UAC) by searching for "msconfig" in Windows, accessing the "tools" tab, and selecting "Disable UAC."
    - Download and install XAMPP from [XAMPP](https://www.apachefriends.org/index.html)
    - Navigate to "C:/xampp/apache/conf/httpd.conf" in Explorer.
    - Open "httpd.conf" in Notepad, change the port to "Listen 8080," and change "ServerName localhost:" to "ServerName localhost:8080."
    - In the XAMPP Control Panel, start "Apache" and "MySQL."
    - Ensure your antivirus accepts XAMPP.

7. **Open your browser and type: "localhost:8080/phpmyadmin/"**
    - Click "New" in the left column.
    - Add a database named "dbo_acc" and click "make."
    - Click "import" at the top, choose file, and select "dbo_acc.sql" from "OpenDBO-Core/DboServer/Database."
    - Click "Start" at the bottom.
    - Repeat this for "dbo_char" and "dbo_log."
    - Within "dbo_acc," find "accounts," click "insert," and fill in a username and password.
    - Generate an MD5 Hash for the password using [MD5 Hash Generator](https://www.md5hashgenerator.com), paste it next to "Password_hash," and click "Start" at the bottom.
    - If you want GM permissions make sure to set `admin` and `isgm` to `10` in the `accounts` table and `gamemaster` to `10` in the `characters` tabel from the `dbo_char` database.

8. **Change the server .ini files:**
    - Navigate to "OpenDBO-Core/DboServer/ExecutionEnv/Config."
    - Open "AuthServer.ini," "CharServer.ini," etc., with Notepad.
    - Ensure all IP addresses are "127.0.0.1" and change the password to "test."

9. **Run the server executables:**
    - Go to "OpenDBO-Core/DboServer/ExecutionEnv."
    - Run `start_master_server.bat`
    - Run `start_query_server.bat` and wait until its done (if you get missing `MSVCP120.dll` error you can fix it by installing Microsoft Visual C++ Redistributable 2013).
    - Run `start_char_server_0.bat`
    - Run `start_auth_server.bat`
    - Run `start_channel_0.bat` and wait until it's finished (can take a lot of time, might need to press ENTER). Then close it.
    - Run `start_chat_server.bat` and wait until its done.
    - Run `start_channel_0.bat` again, `start_channel_1.bat` (if you need 2 channels) and `start_channel_9.bat`

10. **Download and extract the DBO Client Files:**
    - Development Client/Server Access
    - In the main folder, locate "ConfigOptions.xml," change the IP to "127.0.0.1" on each line, and save.
    - Run the client, enter your username and password, and you’re ready to go.

</details>
Alternatively there's a video showcasing this process: https://www.youtube.com/watch?v=lWDffP81ACw

## Custom Drop Event (server)
- Config-driven drops, spawns, modifiers, buffs, titles (attributes), and visuals for mobs.

### Location
- Config: `DboServer/ExecutionEnv/config/CustomDropEvent.cfg`
- Optional levels sidecar: `DboServer/ExecutionEnv/config/CustomDropEvent.levels.json`

### Lifecycle
- Event can be started via server control path (internally managed). While active, all rules apply to normal spawns and to event-created spawns.

### Syntax overview (in `CustomDropEvent.cfg`)
- General format per line: `<id> [section]: <values>`
- `<id>`: numeric mob tblidx. Use `all` for global rules.
- Supported sections:
    - Drops (default when section omitted): `itemId@ratexcount` (rate in 0..100, lowercase `x`, count default 1)
    - `modifiers`: space-separated key/value multipliers (see keys below)
    - `spawn` or `spawns`: `mobId@ratexcount`
    - `buffs`: `skillTblidx[@durationMs]` (duration 0/omitted = skill default)
    - `titles`: `titleTblidx, ...` (attribute effects from CharTitleTable applied to mobs)
    - `visuals`: `systemEffectTblidx[@intervalMs]` (visual-only; interval reserved; use `@0`)

#### Modifiers keys
- `hp`, `physAtk`, `engAtk`, `physDef`, `engDef`, `atkSpd`, `runSpd`, `physCrit`, `engCrit`, `physCritDmg`, `engCritDmg`, `attackRate`, `dodgeRate`, `blockRate`, `blockDmg`, `guardRate`, `sizeRate`
- All are multiplicative except `sizeRate` which is an absolute rate (client typical default 10). Specific overrides global when set (>0).

### Merge and precedence rules
- You can have multiple lines per section. Values append (drops/spawns/buffs/titles/visuals) or multiply (modifiers).
- Global (`all`) and specific (`<id>`) rules combine:
    - Lists (drops/spawns/buffs/titles/visuals): global entries + specific entries.
    - Modifiers: multiply each field; `sizeRate` chosen from specific if set, else global if set.

### Spawns and scaling
- On mob kill, if event is active, each configured spawn entry rolls independently. `rate < 100` spawns are skipped if killer-target level gap > 10.
- Event-created mobs:
    - Use per-mob level if present in sidecar; otherwise base on killer mob level.
    - Scaled stats: HP +1.5%/level, ATK +1.0%/level, DEF +1.0%/level.
    - Apply configured modifiers, buffs, titles, and visuals.

### Titles (attributes-only)
- Mobs don’t have a `charTitle` visual field; titles here apply only attribute effects by reading `CharTitleTable` system effects and applying them to the mob’s attributes.

### Visuals (purely visual)
- Use `visuals:` to always broadcast SystemEffect visuals to clients when the mob spawns (normal or event-created). No stat change.
- For aura-like persistent visuals via buffs, use `buffs:` with long durations and choose skills that have keep visuals (the editor’s Visuals picker helps).

### Levels sidecar (`CustomDropEvent.levels.json`)
- Flat JSON map of mobId to spawn level (1..255). Example:
```json
{
    "3131102": 50,
    "46661101": 70
}
```

### Examples
```
# ===== Global (applies to all mobs) =====
all buffs: 700101@600000, 700202          # skillId@durationMs; 0/omitted = skill default
all visuals: 12345@0, 23456               # SystemEffect tblidx; @interval reserved (use 0)
all modifiers: hp=1.5 physAtk=1.2 runSpd=1.1 sizeRate=12
all titles: 1201, 1205                    # CharTitle tblidx (attributes only)
all spawn: 3131102@100x2, 3131103@25x1    # mobId@ratexcount (use lowercase x)

# ===== Per-mob rules (merge with globals) =====
46661101: 315@100x100                     # drops: itemId@ratexcount
46661101 spawn: 3131102@50x3
46661101 buffs: 701234@900000, 700777     # second uses skill keep time
46661101 visuals: 34567@0, 45678@0        # visuals broadcast on spawn
46661101 modifiers: hp=2.0 physDef=1.3 engDef=1.3
46661101 titles: 1302

# Another mob with only visuals/buffs
3131102 buffs: 700555@0
3131102 visuals: 56789@0
```

### Event Totems (AoE buffs on kill)
- Spawn a neutral beacon NPC on mob kill that periodically applies buffs in an area.
- Supports per-mob and global rules that merge like other sections.

#### Syntax
- `<id> totem: beaconMobTblidx@lifeMs@radius@intervalMs: skillTblidx[@durationMs], skillTblidx[@durationMs], ...`
- Omit `radius` or `intervalMs` to use global defaults (see settings below).
- Duration `@0` or omitted uses the skill’s default keep time.

#### Global Totem Settings
- `all settings: radius=<meters> interval=<ms> healMul=<float>`
- `radius`: default area if a totem rule omits radius (default 30.0).
- `interval`: default pulse interval if omitted (default 2000 ms).
- `healMul`: multiplier for Heal-over-Time and EP-over-Time magnitudes applied by totems (default 3.0 for strong heals).

#### Behavior
- On kill, each matching totem rule spawns the specified beacon at the mob’s position.
- While alive, the beacon pulses buffs to all players within `radius` every `intervalMs`.
- HoT and EP-over-Time effects are boosted by `healMul`.
- Beacons despawn automatically after `lifeMs` or when the event ends.

#### Examples
```
# Global defaults for big area and strong heals
all settings: radius=60 interval=2000 healMul=4

# Global speed zone + regen in huge area (good durations)
all totem: 16454201@45000@@: 300500@20000, 300510@20000, 300200@15000
# (beacon 16454201 lasts 45s, default 60m radius and 2s pulses; speed up, attack speed up, heal over time)

# Per-mob totem with explicit large radius and faster pulses
46661101 totem: 16454203@30000@80@1500: 300500@25000, 300200@15000
```

#### Tips
- Add any number of buffs to a totem rule. Use per-buff `@durationMs` to control lifetime; re-applies on each pulse for players who remain in range.
- For pure visuals, prefer `visuals:` or use long-duration buffs that keep visuals.
- Combine with `buffs:` and `modifiers:` on the beacon mob ID if you want the beacon to show effects or have altered stats (optional).

### Notes & tips
- “all …” lines append; use multiple lines as needed.
- `spawn` tokens are `mobId@ratexcount` with lowercase `x`.
- `visuals:` require SystemEffect tblidx values, not skill ids. For long-lived visuals via buffs, list the skill ids under `buffs:` with a long `@durationMs`.
- Safety caps exist internally to avoid spam (e.g., stacked drops). Keep values reasonable.

## Documentation

Comprehensive documentation for server features and tools is available in the [Documentation](DboServer/Documentation) folder:

- **[Event Manager](DboServer/Documentation/EventManager_README.md)** - Round-based event system with world rotation, minion spawning, and auto-restart
  - [Versión en Español](DboServer/Documentation/EventManager_README_ES.md)
- **[Custom Event Drop](DboServer/Documentation/Custom%20Event%20Drop.md)** - Config-driven drops, spawns, modifiers, buffs, titles, and visuals
- **[Server Monitor](DboServer/Documentation/Server%20Monitor.md)** - Real-time server monitoring and management tool
- **[RDF Table Editor](DboServer/Documentation/RDF%20Table%20Editor.md)** - Table editing tool for game data
- **[WPS Stage Gen](DboServer/Documentation/WPS%20Stage%20Gen.md)** - World path staging generation tool
  - [CLI Version](DboServer/Documentation/WPS%20Stage%20GEN%20CLI.md)

## Acknowledgements
All and any copyrighted material belongs to their respective owners, this is just a non-profit fan project aiming for game preservation. Thanks to DBOG for providing the base for this source code.
