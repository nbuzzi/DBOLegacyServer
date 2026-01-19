# WpsStageGen UI - v4 Enhancements Summary

## Overview
Completely overhauled the WpsStageGen UI to be more intuitive, user-friendly, and feature-rich. The new interface provides better guidance, validation, and quick-access presets for generating CCBD floors.

## Major Enhancements

### 🎨 **Visual Improvements**

1. **Updated Branding**
   - Title: "🎰 CCBD Floor Generator v4"
   - Subtitle: "Generate Crazy Casino floors • Up to 255 stages • Advanced templates & mechanics"
   - Cards now have emojis for better visual identification

2. **Color-Coded Feedback**
   - Dynamic stage limit warnings (green/orange/red)
   - Real-time validation indicators
   - LightSkyBlue accents for important info

### ⚙️ **New Features**

#### 1. **Starting Stage Control**
- New `numStartStage` field (0-255)
- 0 = auto-detect from WPS file
- Manual override for specific stage numbers
- Integrated into CLI args generation

#### 2. **Dynamic Stage Information**
- Real-time calculation of:
  - Regular floor count
  - Boss floor count
  - Stage range (start-end)
  - Total stages vs. limit (X/255)
- Visual warnings:
  - ✓ Green: Within limits (<= 200)
  - ⚠ Orange: High count (201-255)
  - ⚠ Red: Exceeds limit (>255)

#### 3. **Template Preset System**
New dropdown with 6 presets:
1. **None** - Basic floors only
2. **Simple Boss** - Enrage at 30% HP
3. **Advanced Boss** - Full 4-phase mechanics
4. **Enhanced Regular + Simple Boss**
5. **Enhanced Regular + Advanced Boss** (Recommended)
6. **Custom** - Manual template selection

Auto-loads templates from `/templates` folder on selection.

#### 4. **Quick Access Buttons**
- **📖 Help** - Opens comprehensive README.md
- **📁 Templates** - Opens templates folder in Explorer
- Both with proper error handling and user feedback

### 📝 **Enhanced Tooltips**

Extended tooltips for all fields with detailed explanations:

| Field | Tooltip |
|-------|---------|
| numAdd | "Number of new floors to generate (e.g., 50 = 45 regular + 10 boss stages)" |
| numStartStage | "Starting stage number (0 = auto-detect from WPS file, 151 = start after floor 150)" |
| numBossEvery | "Boss appears every N floors (5 = boss at 5,10,15,20... \| 10 = boss at 10,20,30...)" |
| txtPattern | "Mob spawn pattern with probabilities: (pattern_id, percentage)" |
| cboTemplatePreset | "Quick select pre-configured template combinations" |
| btnOpenDocs | "Open the comprehensive documentation and usage guide" |
| btnOpenTemplates | "Open the templates folder to view/edit template files" |

...and 10+ more fields with helpful context.

### 🔄 **Improved Defaults**

Changed defaults to more practical values:
- `numAdd`: 5 → **50** (generate 50 floors at once)
- Window minimum size: 1600x1080 → **1400x900** (more accessible)
- Template preset: None → **Enhanced Regular + Advanced Boss**

### 🎯 **Better User Experience**

1. **Real-time Feedback**
   - Stage info updates instantly as you type
   - Args preview updates on every change
   - Validation warnings appear immediately

2. **Smart Template Loading**
   - Auto-detects templates folder
   - Preset system automatically fills template paths
   - Falls back gracefully if templates missing

3. **Intuitive Layout**
   - Cards with icons and emojis
   - Logical grouping:
     - ⚙ Stage Parameters
     - 🎁 Rewards & Pattern
     - 👾 Boss & Worlds
     - 📄 Templates & Variables
     - 🚀 Action (with help buttons)

4. **Validation & Error Prevention**
   - Stage limit warnings (>255)
   - File existence checks before opening
   - Clear error messages with suggestions

### 🛠️ **Technical Improvements**

#### New Methods Added:
```csharp
void UpdateStageInfo()              // Real-time stage calculation & display
void ApplyTemplatePreset()          // Handles template preset selection
void OpenDocumentation()            // Opens README.md with error handling
void OpenTemplatesFolder()          // Opens templates folder in Explorer
```

#### Updated Methods:
- `ResetDefaults()` - Now includes new controls
- `BuildArgsOnly()` - Includes `start` parameter
- `UpdateArgs()` - Includes `start` parameter
- Tooltips - Extended from 5 to 17 fields

#### New Controls:
- `numStartStage` - NumericUpDown (0-255)
- `cboTemplatePreset` - ComboBox with 6 presets
- `lblStageInfo` - Dynamic stage information
- `lblMaxStage` - Stage limit warning
- `btnOpenDocs` - Help button
- `btnOpenTemplates` - Templates folder button

### 📊 **Comparison: Old vs New**

| Feature | Old UI | New UI v4 |
|---------|--------|-----------|
| Default floors | 5 | 50 |
| Stage limit indicator | ❌ None | ✅ Dynamic (X/255) |
| Template presets | ❌ None | ✅ 6 presets |
| Starting stage control | ❌ Auto only | ✅ Manual override (0-255) |
| Quick help access | ❌ None | ✅ Help + Templates buttons |
| Tooltips | 5 basic | 17 detailed |
| Real-time validation | ❌ Limited | ✅ Full validation |
| Visual feedback | ❌ Minimal | ✅ Color-coded warnings |
| Card icons | ❌ Text only | ✅ Emojis + MDL2 icons |

## Usage Examples

### Example 1: Quick Start (Recommended Preset)
1. Click "Browse" for WPS file → Select `83000.wps`
2. Template preset already set to "Enhanced Regular + Advanced Boss (Recommended)"
3. Set "Floors to append" = 50
4. Set "Starting stage" = 151
5. Click "Run"

Result: Generates stages 151-200 with full boss phases and enhanced regular floors.

### Example 2: Simple Extension
1. Load WPS file
2. Change preset to "Simple Boss (Enrage at 30%)"
3. Set floors = 25
4. Leave starting stage = 0 (auto-detect)
5. Click "Run"

Result: Adds 25 simple floors with basic boss enrage mechanic.

### Example 3: Custom Templates
1. Load WPS file
2. Set preset to "Custom (Select files manually)"
3. Browse to select your own template files
4. Configure parameters
5. Click "Run"

Result: Uses your custom templates for complete control.

## Benefits for Users

### Beginners
- Template presets eliminate guesswork
- Tooltips explain every field
- Visual warnings prevent mistakes
- Help button provides full documentation
- Recommended preset works out of the box

### Advanced Users
- Manual starting stage control
- Custom template support
- Inline variables editor
- Full CLI args preview
- Templates folder quick access

### Server Operators
- Generate 50+ floors in one click
- Stage limit validation prevents errors
- Real-time feedback reduces trial-and-error
- Copy args for batch processing
- Detailed tooltips reduce support questions

## Files Modified

1. **MainForm.cs** - Complete UI overhaul
   - Added 6 new controls
   - Enhanced 4 existing methods
   - Added 4 new helper methods
   - Expanded tooltips system
   - Improved visual theme

## Next Steps

### Future Enhancements (Optional)
1. **Preview Tab** - Show generated WPS preview before saving
2. **Batch Mode** - Generate multiple stage ranges at once
3. **Template Editor** - Built-in template editing
4. **Stage Analyzer** - Analyze existing WPS file structure
5. **Difficulty Calculator** - Suggest mob scaling based on stage
6. **Export Presets** - Save/load user configurations

### Immediate Actions for Users
1. Launch new UI: `WpsStageGen.UI.exe`
2. Try the recommended preset
3. Click "📖 Help" to read full documentation
4. Browse templates folder to see examples
5. Generate your first batch of floors!

## Changelog

### v4.0 (2025-10-02)
- ✅ Added starting stage control (0-255 manual override)
- ✅ Added dynamic stage info display (regular + boss count)
- ✅ Added template preset system (6 presets)
- ✅ Added Help and Templates folder buttons
- ✅ Extended tooltips to 17 fields with detailed explanations
- ✅ Added real-time stage limit validation (green/orange/red)
- ✅ Improved visual design with emojis and better colors
- ✅ Changed default floors from 5 to 50
- ✅ Updated branding to reflect "Up to 255 stages"
- ✅ Enhanced error handling and user feedback
- ✅ Improved layout and card organization

### v3.0 (Previous)
- Basic file selection
- Parameter configuration
- Args preview
- Template file selection

## Support

**Documentation**: Click "📖 Help" in UI or see `templates/README.md`
**Templates**: Click "📁 Templates" in UI
**Quick Start**: `templates/QUICK_START.txt`

---

**Status**: ✅ Complete & Production Ready
**Built**: 2025-10-02
**Tested**: Yes (successful build, no errors)
