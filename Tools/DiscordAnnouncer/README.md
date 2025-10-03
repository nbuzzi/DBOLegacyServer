# Discord Announcer Tool for DBO Legacy

## Overview
CLI tool to post announcements and patch notes to DBO Legacy Discord channels in 4 languages.

## Quick Start

### Using the tool
```bash
# For patch notes (completed changes)
dotnet run --project Tools/DiscordAnnouncer -- patch "Your message here"

# For announcements (upcoming changes)
dotnet run --project Tools/DiscordAnnouncer -- announcement "Your message here"

# Using a message file (recommended for long messages)
dotnet run --project Tools/DiscordAnnouncer -- patch "@message.txt"
```

## Configuration

All announcement formatting rules and templates are stored in:
- **`announcement_template.json`** - Complete format specification, emojis, translations, and structure

### Key Configuration Elements

1. **Project Name**: DBO Legacy
2. **Always Include**: @everyone, emojis, 4 languages, separators
3. **Languages**: English 🇺🇸, Spanish 🇪🇸, Portuguese 🇧🇷, Chinese 🇨🇳
4. **Sections**: New Features, Changes, Bug Fixes (translated)

### Channel Types

- **Patch Notes**: Completed changes, live updates
- **Announcements**: Upcoming features, planned changes

## Standard Message Structure

```
@everyone

🎮 **DBO Legacy - [Type]** 🎮

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🆕 **NEW FEATURES**
💰 **Feature Name** - Description here.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🇪🇸 **ESPAÑOL**

🆕 **NUEVAS FUNCIONES**
💰 **Nombre de Función** - Descripción aquí.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🇧🇷 **PORTUGUÊS**

🆕 **NOVOS RECURSOS**
💰 **Nome do Recurso** - Descrição aqui.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🇨🇳 **中文**

🆕 **新功能**
💰 **功能名称** - 功能描述。

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

🔥 **Update live now!** | **¡Actualización disponible!** | **Atualização disponível!** | **更新已上线！** 🔥
```

## Recommended Emojis

See `announcement_template.json` for full list of recommended emojis per category:
- 🎮 Headers
- 🆕 New Features
- 🔧 Changes
- 🐛 Bug Fixes
- 💰 Money/Cash features
- 🎰 Roulette/Gambling
- ✨ UI Improvements
- ⚔️ Combat
- 🎉 Events
- 🔥 Closing emphasis

## Examples

### Example 1: Patch Notes
```bash
dotnet run -- patch "@patch_message.txt"
```

### Example 2: Announcement
```bash
dotnet run -- announcement "🎉 New event starting this weekend! Check Discord for details."
```

## Important Notes

1. Always use `@everyone` to notify all members
2. Maintain consistent formatting across all announcements
3. Keep all 4 language versions synchronized
4. Use emojis to enhance visual appeal
5. Test messages in a test channel first if unsure

## Recovery Instructions

If context is lost, read `announcement_template.json` to recover:
- Standard format structure
- Translation templates
- Emoji recommendations
- Channel usage guidelines
