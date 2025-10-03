# CCBD v4 - Documentation Index

## 📚 Complete CCBD v4 Documentation

This index provides quick access to all CCBD (Crazy Casino Battle Dungeon) v4 documentation.

---

## 🚀 Quick Start

**New to CCBD floor generation?** Start here:

1. **[WPS_Quick_Start.txt](WPS_Quick_Start.txt)** - Quick reference card (3 minutes)
2. **[WPS Stage Gen.md](WPS%20Stage%20Gen.md)** - UI basics (5 minutes)
3. **[WPS_Templates_Guide.md](WPS_Templates_Guide.md)** - Full template system guide (15 minutes)

---

## 📖 User Documentation

### GUI Tool
- **[WPS Stage Gen.md](WPS%20Stage%20Gen.md)** - WpsStageGen.UI basics and quick start
- **[WPS_Stage_Gen_UI_v4.md](WPS_Stage_Gen_UI_v4.md)** - Complete v4 UI enhancements and features

### CLI Tool
- **[WPS Stage GEN CLI.md](WPS%20Stage%20GEN%20CLI.md)** - Command-line interface usage
- **[WPS_Quick_Start.txt](WPS_Quick_Start.txt)** - Quick reference with examples

### Templates
- **[WPS_Templates_Guide.md](WPS_Templates_Guide.md)** - Comprehensive template system guide
  - Parameter reference
  - Built-in variables
  - Custom mechanics examples
  - Pattern lists
  - Troubleshooting

---

## 🔧 Technical Documentation

### Backend Implementation
- **[CCBD_Expansion_Backend_v4.md](CCBD_Expansion_Backend_v4.md)** - Backend changes and implementation
  - C++ modifications (NtlCCBD.h, NtlCCBD.cpp, etc.)
  - Dynamic boss detection algorithm
  - Arena cycling logic
  - Migration guide

### Complete Reference
- **[CCBD_v4_Complete_Guide.md](CCBD_v4_Complete_Guide.md)** - Everything in one place
  - Backend changes
  - Tool enhancements
  - UI improvements
  - Testing results
  - Usage examples

---

## 🎯 By Task

### "I want to add more floors"
1. Read: [WPS_Quick_Start.txt](WPS_Quick_Start.txt)
2. Use: GUI ([WPS Stage Gen.md](WPS%20Stage%20Gen.md)) or CLI ([WPS Stage GEN CLI.md](WPS%20Stage%20GEN%20CLI.md))

### "I want to customize boss mechanics"
1. Read: [WPS_Templates_Guide.md](WPS_Templates_Guide.md) - Creating Custom Boss Mechanics
2. See: Template examples in `Tools/WpsStageGen/bin/Release/net8.0/win-x64/templates/`
3. Use: boss_phases.wps or boss_simple.wps as starting points

### "I want to understand the backend"
1. Read: [CCBD_Expansion_Backend_v4.md](CCBD_Expansion_Backend_v4.md)
2. Review: Modified C++ files
3. See: Migration guide for deployment steps

### "I want to know what changed in v4"
1. Read: [CCBD_v4_Complete_Guide.md](CCBD_v4_Complete_Guide.md) - Before & After Comparison
2. See: UI enhancements in [WPS_Stage_Gen_UI_v4.md](WPS_Stage_Gen_UI_v4.md)

---

## 📂 Template Files

Located in: `Tools/WpsStageGen/bin/Release/net8.0/win-x64/templates/`

- **boss_phases.wps** - Advanced 4-phase boss mechanics
- **boss_simple.wps** - Basic enrage mechanic (beginner-friendly)
- **regular_enhanced.wps** - Enhanced regular floor mechanics
- **sample_vars.ini** - Pre-configured variables

See [WPS_Templates_Guide.md](WPS_Templates_Guide.md) for detailed template documentation.

---

## 🎰 Related Documentation

### Other Tools
- **[Custom Event Drop.md](Custom%20Event%20Drop.md)** - Custom drop event system
- **[EventManager_README.md](EventManager_README.md)** - Event management system
- **[RDF Table Editor.md](RDF%20Table%20Editor.md)** - Table editing tool
- **[Server Monitor.md](Server%20Monitor.md)** - Server monitoring tool

### Game Systems
- **[Budokai_Player_Guide.md](Budokai_Player_Guide.md)** - Budokai tournament system

---

## 🔗 Quick Links

### Tools
- **UI**: `Tools/WpsStageGen.UI/bin/Release/net8.0-windows/WpsStageGen.UI.exe`
- **CLI**: `Tools/WpsStageGen/bin/Release/net8.0/win-x64/WpsStageGen.exe`
- **Templates**: `Tools/WpsStageGen/bin/Release/net8.0/win-x64/templates/`

### Documentation
- **This Folder**: `DboServer/Documentation/`
- **Main README**: [CCBD_v4_Complete_Guide.md](CCBD_v4_Complete_Guide.md)
- **Quick Start**: [WPS_Quick_Start.txt](WPS_Quick_Start.txt)

---

## 📊 Documentation Structure

```
DboServer/Documentation/
├── CCBD_Documentation_Index.md          (This file)
├── CCBD_v4_Complete_Guide.md            (Everything)
├── CCBD_Expansion_Backend_v4.md         (Backend technical)
├── WPS_Stage_Gen_UI_v4.md               (UI features)
├── WPS_Templates_Guide.md               (Template system)
├── WPS_Quick_Start.txt                  (Quick reference)
├── WPS Stage Gen.md                     (UI basics)
└── WPS Stage GEN CLI.md                 (CLI usage)

Tools/WpsStageGen/bin/Release/net8.0/win-x64/templates/
├── boss_phases.wps                      (Advanced boss)
├── boss_simple.wps                      (Simple boss)
├── regular_enhanced.wps                 (Enhanced regular)
├── sample_vars.ini                      (Variables)
├── README.md                            (Linked to WPS_Templates_Guide.md)
└── QUICK_START.txt                      (Linked to WPS_Quick_Start.txt)
```

---

## 💡 Tips

- **Beginners**: Start with UI and recommended preset
- **Advanced**: Use CLI for batch operations
- **Developers**: Read backend documentation first
- **Template Creators**: Study existing templates, use placeholders

---

## ✨ What's New in v4

- **Max Stages**: 150 → **255**
- **Boss Detection**: Hardcoded → **Dynamic**
- **Arena Handling**: Fixed → **Auto-cycling**
- **Templates**: None → **4 templates + vars system**
- **UI**: Basic → **Advanced with presets, validation, help**
- **Documentation**: Minimal → **Comprehensive**

See [CCBD_v4_Complete_Guide.md](CCBD_v4_Complete_Guide.md) for complete changelog.

---

**Version**: v4.0
**Updated**: 2025-10-02
**Status**: ✅ Production Ready
