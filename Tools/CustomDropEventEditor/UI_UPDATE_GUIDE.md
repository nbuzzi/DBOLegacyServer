# UI Update Guide - Phase System Implementation

## Quick Implementation Guide

This guide shows exactly what to add to `MainForm.cs` to support the PHASE system.

---

## Step 1: Add New UI Controls

### In the class-level fields section (around line 76):

```csharp
// Existing fields...
private TextBox txtDrops, txtSpawns, txtMods, ...

// ADD THESE NEW FIELDS:
private ComboBox cboPhase;  // Phase selector dropdown
private Label lblPhase;      // Label for phase dropdown
private CheckBox chkRestoreHP;  // RestoreHP checkbox
private Label lblPhaseList;  // Label showing configured phases
private ListBox lstPhases;   // List of configured phases for current mob
private Label lblStatWarning;  // Warning label for stat overflow
```

---

## Step 2: Initialize New Controls

### In the `InitializeLayout()` or constructor method:

```csharp
// Phase selector setup
lblPhase = new Label
{
    Text = "Phase:",
    AutoSize = true,
    Location = new Point(20, 450)
};

cboPhase = new ComboBox
{
    DropDownStyle = ComboBoxStyle.DropDownList,
    Width = 120,
    Location = new Point(80, 448)
};
cboPhase.Items.AddRange(new object[]
{
    "None (Base)", "1", "2", "3", "4", "5"
});
cboPhase.SelectedIndex = 0;
cboPhase.SelectedIndexChanged += CboPhase_SelectedIndexChanged;

// RestoreHP checkbox
chkRestoreHP = new CheckBox
{
    Text = "Restore HP to 100% when this phase starts",
    AutoSize = true,
    Location = new Point(20, 850),
    Checked = false
};
chkRestoreHP.CheckedChanged += ChkRestoreHP_CheckedChanged;

// Phase list view
lblPhaseList = new Label
{
    Text = "Configured Phases:",
    AutoSize = true,
    Location = new Point(950, 450)
};

lstPhases = new ListBox
{
    Width = 350,
    Height = 200,
    Location = new Point(950, 475),
    IntegralHeight = false
};
lstPhases.DoubleClick += LstPhases_DoubleClick;

// Warning label
lblStatWarning = new Label
{
    Text = "",
    AutoSize = true,
    Location = new Point(20, 820),
    ForeColor = Color.Orange,
    Visible = false
};

// Add controls to form
Controls.Add(lblPhase);
Controls.Add(cboPhase);
Controls.Add(chkRestoreHP);
Controls.Add(lblPhaseList);
Controls.Add(lstPhases);
Controls.Add(lblStatWarning);
```

---

## Step 3: Add Event Handlers

```csharp
private byte GetSelectedPhase()
{
    return cboPhase.SelectedIndex == 0 ? (byte)0 : (byte)cboPhase.SelectedIndex;
}

private void CboPhase_SelectedIndexChanged(object sender, EventArgs e)
{
    // Load modifiers for the selected phase
    if (lstCfgMobs.SelectedItem is not ListViewItemWrapper w) return;

    var mobId = w.Id;
    var phase = GetSelectedPhase();

    if (phase == 0)
    {
        // Load regular modifiers
        if (_model.Mods.TryGetValue(mobId, out var mods))
        {
            LoadModifiersToUI(mods);
        }
        else
        {
            LoadDefaultModifiers();
        }
    }
    else
    {
        // Load phase-specific modifiers
        if (_model.ModsByPhase.TryGetValue(mobId, out var phaseDict) &&
            phaseDict.TryGetValue(phase, out var mods))
        {
            LoadModifiersToUI(mods);
        }
        else
        {
            LoadDefaultModifiers();
        }
    }
}

private void LoadModifiersToUI(Modifiers m)
{
    // Update all the modifier textboxes
    // You'll need to parse txtMods or use individual numeric controls
    // Example if using individual controls:
    numHP.Value = (decimal)m.Hp;
    numPhysAtk.Value = (decimal)m.PhysAtk;
    // ... etc for all stats

    chkRestoreHP.Checked = m.RestoreHP;

    // Validate for overflow
    ValidateStatOverflow(m);
}

private void LoadDefaultModifiers()
{
    // Load default values (all 1.0)
    var defaultMods = new Modifiers();
    LoadModifiersToUI(defaultMods);
}

private void SaveCurrentModifiers()
{
    if (lstCfgMobs.SelectedItem is not ListViewItemWrapper w) return;

    var mobId = w.Id;
    var phase = GetSelectedPhase();

    // Parse modifiers from UI
    var mods = ParseModifiersFromUI();
    mods.RestoreHP = chkRestoreHP.Checked;

    if (phase == 0)
    {
        // Save as regular modifier
        _model.Mods[mobId] = mods;
    }
    else
    {
        // Save as phase-specific modifier
        if (!_model.ModsByPhase.TryGetValue(mobId, out var phaseDict))
        {
            phaseDict = new Dictionary<byte, Modifiers>();
            _model.ModsByPhase[mobId] = phaseDict;
        }
        phaseDict[phase] = mods;
    }

    RefreshPhaseList(mobId);
}

private Modifiers ParseModifiersFromUI()
{
    // If using the txtMods textbox approach:
    return Modifiers.Parse(txtMods.Text);

    // OR if using individual numeric controls:
    // return new Modifiers
    // {
    //     Hp = (float)numHP.Value,
    //     PhysAtk = (float)numPhysAtk.Value,
    //     // ... etc
    // };
}

private void ChkRestoreHP_CheckedChanged(object sender, EventArgs e)
{
    // Auto-save when checkbox changes
    SaveCurrentModifiers();
}

private void RefreshPhaseList(uint mobId)
{
    lstPhases.Items.Clear();

    // Show regular modifiers
    if (_model.Mods.TryGetValue(mobId, out var baseMods))
    {
        lstPhases.Items.Add("Base: " + GetModifiersSummary(baseMods));
    }

    // Show phase modifiers
    if (_model.ModsByPhase.TryGetValue(mobId, out var phaseDict))
    {
        foreach (var (phase, mods) in phaseDict.OrderBy(p => p.Key))
        {
            var summary = GetModifiersSummary(mods);
            var restoreIcon = mods.RestoreHP ? " 🔄" : "";
            lstPhases.Items.Add($"Phase {phase}: {summary}{restoreIcon}");
        }
    }
}

private string GetModifiersSummary(Modifiers m)
{
    var parts = new List<string>();
    if (m.Hp != 1f) parts.Add($"HP×{m.Hp:F1}");
    if (m.PhysAtk != 1f) parts.Add($"PhysAtk×{m.PhysAtk:F1}");
    if (m.EngAtk != 1f) parts.Add($"EngAtk×{m.EngAtk:F1}");
    if (m.AtkSpd != 1f) parts.Add($"AtkSpd×{m.AtkSpd:F2}");
    if (parts.Count == 0) return "Default";
    return string.Join(", ", parts);
}

private void LstPhases_DoubleClick(object sender, EventArgs e)
{
    // Jump to selected phase when double-clicking in phase list
    if (lstPhases.SelectedItem is not string item) return;

    if (item.StartsWith("Base:"))
    {
        cboPhase.SelectedIndex = 0;
    }
    else if (item.StartsWith("Phase "))
    {
        var phaseStr = item.Substring(6, 1);
        if (byte.TryParse(phaseStr, out var phase) && phase <= 5)
        {
            cboPhase.SelectedIndex = phase;
        }
    }
}

private void ValidateStatOverflow(Modifiers m)
{
    // This requires knowing base stats - you can estimate or load from table
    // For now, show warning if multiplier is very high
    var warnings = new List<string>();

    if (m.PhysAtk > 20f) warnings.Add("physAtk");
    if (m.EngAtk > 20f) warnings.Add("engAtk");
    if (m.PhysDef > 20f) warnings.Add("physDef");
    if (m.EngDef > 20f) warnings.Add("engDef");
    if (m.AtkSpd > 20f) warnings.Add("atkSpd");

    if (warnings.Count > 0)
    {
        lblStatWarning.Text = $"⚠️ High multipliers may overflow: {string.Join(", ", warnings)}";
        lblStatWarning.Text += " (max 65,535)";
        lblStatWarning.Visible = true;
    }
    else
    {
        lblStatWarning.Visible = false;
    }
}
```

---

## Step 4: Update Existing Methods

### Update `lstCfgMobs_SelectedIndexChanged()`:

```csharp
private void lstCfgMobs_SelectedIndexChanged(object sender, EventArgs e)
{
    // ... existing code ...

    // ADD THIS:
    RefreshPhaseList(mobId);

    // Reset to base modifiers view
    cboPhase.SelectedIndex = 0;
}
```

### Update the Save button handler:

```csharp
private void btnSave_Click(object sender, EventArgs e)
{
    // Save current modifiers before saving file
    SaveCurrentModifiers();

    // ... existing save code ...
}
```

---

## Step 5: Apply Dark Theme to New Controls

### In your theme application method:

```csharp
private void ApplyDarkTheme()
{
    // ... existing theme code ...

    // ADD THESE:
    cboPhase.BackColor = Color.FromArgb(40, 41, 45);
    cboPhase.ForeColor = Color.Gainsboro;

    chkRestoreHP.BackColor = Color.FromArgb(32, 33, 36);
    chkRestoreHP.ForeColor = Color.Gainsboro;

    lstPhases.BackColor = Color.FromArgb(40, 41, 45);
    lstPhases.ForeColor = Color.Gainsboro;

    lblPhaseList.ForeColor = Color.Gainsboro;
    lblPhase.ForeColor = Color.Gainsboro;
}
```

---

## UI Layout Suggestion

```
┌─────────────────────────────────────────────────────────────────┐
│ Path: [________________] [Browse] [Load] [Save] [Help]          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│ Configured Mobs    │ Mob Details         │ Configured Phases    │
│ ┌────────────┐     │                     │ ┌──────────────────┐ │
│ │ 123        │     │ Phase: [None▼]     │ │ Base: Default    │ │
│ │ 456        │     │                     │ │ Phase 1: HP×1.3  │ │
│ │ 789        │     │ HP:      [1.0  ]   │ │ Phase 2: HP×1.6🔄│ │
│ └────────────┘     │ PhysAtk: [1.0  ]   │ │ Phase 3: HP×2.0🔄│ │
│                    │ EngAtk:  [1.0  ]   │ └──────────────────┘ │
│                    │ ...                 │                       │
│                    │                     │                       │
│                    │ ☐ Restore HP       │                       │
│                    │                     │                       │
│                    │ ⚠️ Warning: ...    │                       │
│                    │                     │                       │
│                    │ [Apply Modifiers]  │                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Testing Procedure

1. Load an existing config with phase modifiers
2. Select a mob → should show all phases in the phase list
3. Switch between phases → modifier values should change
4. Edit phase 2 modifiers → check the "Restore HP" checkbox
5. Save config → verify output contains `restoreHP=1`
6. Test high multipliers → should show overflow warning

---

## Minimal Implementation (Quick Version)

If you want to test quickly without full UI:

1. Just add `RestoreHP` support to the existing `txtMods` textbox
2. User manually types: `hp=2.0 physAtk=2.0 restoreHP=1`
3. Parser already handles it via `ConfigModel.cs` changes
4. Phase support already works via parsing `phase=N` in the line

**This requires NO UI changes** - just the `ConfigModel.cs` updates already done!

---

## Summary of Changes

### ✅ Already Done:
- `ConfigModel.cs` - Parser and save logic for phases and restoreHP
- `Modifiers` class - RestoreHP property
- Config file format support

### 📝 To Do (Optional UI Enhancements):
- Phase dropdown selector
- RestoreHP checkbox
- Phase list view
- Overflow warnings
- Help text updates

The backend is **fully functional** - UI updates are optional quality-of-life improvements!
