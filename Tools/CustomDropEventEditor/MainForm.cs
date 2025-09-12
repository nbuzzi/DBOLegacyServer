using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace CustomDropEventEditor
{
    public class MainForm : Form
    {
        using System;
        using System.Collections.Generic;
        using System.Drawing;
        using System.IO;
        using System.Linq;
        using System.Windows.Forms;

        namespace CustomDropEventEditor
        {
            public class MainForm : Form
            {
                private sealed class ListViewItemWrapper
                {
                    public uint Id { get; }
                    private readonly string _display;
                    public ListViewItemWrapper(uint id, string display)
                    {
                        Id = id;
                        _display = display;
                    }
                    public override string ToString() => _display;
                }

                private TextBox txtPath;
                private Button btnBrowse, btnLoad, btnSave;
                private ListBox lstMobs;
                private TextBox txtMobFilter;
                private Label lblMobFilter;
                private Button btnClearFilter;

                private TextBox txtDrops, txtSpawns, txtMods, txtGlobalSpawns;
                private Label lblDrops, lblSpawns, lblMods, lblGlobalSpawns;
                private Button btnImportMobs, btnImportItems;

                private Label _lblStatus;

                private ListBox lstDrops, lstSpawns;
                private TextBox txtDropItemId, txtSpawnMobId;
                private NumericUpDown numDropRate, numSpawnRate, numSpawnCount;
                private Button btnAddDrop, btnRemoveDrop, btnAddSpawn, btnRemoveSpawn;

                private TextBox txtNewMobId;
                private Button btnAddMob;
                private Label lblAddMob, lblDropItem, lblDropRate, lblSpawnMob, lblSpawnRate, lblSpawnCount;

                private ConfigModel _model = new();
                private Dictionary<uint, string> _mobNames = new();
                private Dictionary<uint, string> _itemNames = new();

                public MainForm()
                {
                    Text = "Custom Drop Event Editor";
                    Width = 1100;
                    Height = 720;
                    AutoScaleMode = AutoScaleMode.Dpi;
                    Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point);
                    BackColor = Color.FromArgb(32, 33, 36);
                    ForeColor = Color.Gainsboro;

                    txtPath = new TextBox { Left = 10, Top = 10, Width = 800, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
                    btnBrowse = new Button { Left = 820, Top = 8, Width = 80, Height = 28, Text = "Browse" };
                    btnLoad = new Button { Left = 910, Top = 8, Width = 80, Height = 28, Text = "Load" };
                    btnSave = new Button { Left = 1000, Top = 8, Width = 80, Height = 28, Text = "Save" };

                    btnImportMobs = new Button { Left = 820, Top = 40, Width = 130, Height = 28, Text = "Import Mobs..." };
                    btnImportItems = new Button { Left = 960, Top = 40, Width = 120, Height = 28, Text = "Import Items..." };

                    lblMobFilter = new Label { Left = 10, Top = 70, AutoSize = true, Text = "Filter" };
                    txtMobFilter = new TextBox { Left = 60, Top = 68, Width = 180, Height = 24 };
                    btnClearFilter = new Button { Left = 245, Top = 68, Width = 65, Height = 24, Text = "Clear" };

                    lstMobs = new ListBox { Left = 10, Top = 98, Width = 300, Height = 572, Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left, IntegralHeight = false };

                    lblGlobalSpawns = new Label { Left = 320, Top = 70, AutoSize = true, Text = "Global Spawns (all spawn: mob@ratexcount, ...)" };
                    txtGlobalSpawns = new TextBox { Left = 320, Top = 90, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

                    lblDrops = new Label { Left = 320, Top = 130, AutoSize = true, Text = "Drops (item@rate, ...)" };
                    txtDrops = new TextBox { Left = 320, Top = 150, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

                    lstDrops = new ListBox { Left = 320, Top = 182, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
                    lblDropItem = new Label { Left = 320, Top = 286, AutoSize = true, Text = "Item ID" };
                    txtDropItemId = new TextBox { Left = 320, Top = 306, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
                    lblDropRate = new Label { Left = 470, Top = 286, AutoSize = true, Text = "Rate" };
                    numDropRate = new NumericUpDown { Left = 470, Top = 306, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
                    btnAddDrop = new Button { Left = 580, Top = 306, Width = 90, Height = 28, Text = "Add" };
                    btnRemoveDrop = new Button { Left = 680, Top = 306, Width = 90, Height = 28, Text = "Remove" };

                    lstSpawns = new ListBox { Left = 320, Top = 398, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
                    lblSpawns = new Label { Left = 320, Top = 370, AutoSize = true, Text = "Spawns (mob@ratexcount, ...)" };
                    lblSpawnMob = new Label { Left = 320, Top = 502, AutoSize = true, Text = "Mob ID" };
                    txtSpawnMobId = new TextBox { Left = 320, Top = 522, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
                    lblSpawnRate = new Label { Left = 470, Top = 502, AutoSize = true, Text = "Rate" };
                    numSpawnRate = new NumericUpDown { Left = 470, Top = 522, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
                    lblSpawnCount = new Label { Left = 580, Top = 502, AutoSize = true, Text = "Count" };
                    numSpawnCount = new NumericUpDown { Left = 580, Top = 522, Width = 80, Height = 28, DecimalPlaces = 0, Minimum = 1, Maximum = 255, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
                    btnAddSpawn = new Button { Left = 670, Top = 522, Width = 90, Height = 28, Text = "Add" };
                    btnRemoveSpawn = new Button { Left = 770, Top = 522, Width = 90, Height = 28, Text = "Remove" };

                    lblMods = new Label { Left = 320, Top = 562, AutoSize = true, Text = "Modifiers (hp= engAtk= physAtk= ... sizeRate=)" };
                    txtMods = new TextBox { Left = 320, Top = 582, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

                    lblAddMob = new Label { Left = 10, Top = 40, AutoSize = true, Text = "Add Mob ID:" };
                    txtNewMobId = new TextBox { Left = 100, Top = 38, Width = 140, Height = 28 };
                    btnAddMob = new Button { Left = 250, Top = 38, Width = 60, Height = 28, Text = "Add" };

                    txtSpawns = new TextBox { Left = 320, Top = 620, Width = 760, Height = 24, Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right, Visible = false };

                    Controls.AddRange(new Control[]
                    {
                        txtPath, btnBrowse, btnLoad, btnSave,
                        btnImportMobs, btnImportItems,
                        lblMobFilter, txtMobFilter, btnClearFilter,
                        lstMobs,
                        lblGlobalSpawns, txtGlobalSpawns,
                        lblDrops, txtDrops, lstDrops, lblDropItem, txtDropItemId, lblDropRate, numDropRate, btnAddDrop, btnRemoveDrop,
                        lblSpawns, lstSpawns, lblSpawnMob, txtSpawnMobId, lblSpawnRate, numSpawnRate, lblSpawnCount, numSpawnCount, btnAddSpawn, btnRemoveSpawn,
                        lblMods, txtMods,
                        lblAddMob, txtNewMobId, btnAddMob,
                        txtSpawns
                    });

                    btnBrowse.Click += (s, e) => BrowsePath();
                    btnLoad.Click += (s, e) => LoadCfg();
                    btnSave.Click += (s, e) => SaveCfg();
                    btnImportMobs.Click += (s, e) => ImportMobs();
                    btnImportItems.Click += (s, e) => ImportItems();
                    lstMobs.SelectedIndexChanged += (s, e) => LoadMobIntoEditors();
                    txtDrops.Leave += (s, e) => SaveMobFromEditors();
                    txtSpawns.Leave += (s, e) => SaveMobFromEditors();
                    txtMods.Leave += (s, e) => SaveMobFromEditors();
                    txtGlobalSpawns.Leave += (s, e) => SaveGlobalFromEditors();

                    btnAddDrop.Click += (s, e) => AddDropFromInputs();
                    btnRemoveDrop.Click += (s, e) => RemoveSelectedDrop();
                    btnAddSpawn.Click += (s, e) => AddSpawnFromInputs();
                    btnRemoveSpawn.Click += (s, e) => RemoveSelectedSpawn();
                    btnAddMob.Click += (s, e) => AddMob();

                    txtMobFilter.TextChanged += (s, e) => RefreshMobList(txtMobFilter.Text);
                    btnClearFilter.Click += (s, e) => { txtMobFilter.Text = string.Empty; RefreshMobList(); };
                    lstDrops.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedDrop(); e.Handled = true; } };
                    lstSpawns.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedSpawn(); e.Handled = true; } };
                    txtDropItemId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
                    txtSpawnMobId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
                    numDropRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
                    numSpawnRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
                    numSpawnCount.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };

                    KeyPreview = true;
                    KeyDown += (s, e) =>
                    {
                        if (e.Control && e.KeyCode == Keys.S) { btnSave.PerformClick(); e.Handled = true; }
                        else if (e.Control && e.KeyCode == Keys.O) { btnLoad.PerformClick(); e.Handled = true; }
                    };

                    ApplyFieldTheme(txtPath);
                    ApplyFieldTheme(txtGlobalSpawns);
                    ApplyFieldTheme(txtDrops);
                    ApplyFieldTheme(txtSpawns);
                    ApplyFieldTheme(txtMods);
                    ApplyFieldTheme(txtDropItemId);
                    ApplyFieldTheme(txtSpawnMobId);
                    ApplyFieldTheme(txtNewMobId);
                    ApplyFieldTheme(txtMobFilter);
                    ApplyLabelTheme(lblMobFilter);
                    ApplyNumericTheme(numDropRate);
                    ApplyNumericTheme(numSpawnRate);
                    ApplyNumericTheme(numSpawnCount);
                    ApplyListTheme(lstMobs);
                    ApplyListTheme(lstDrops);
                    ApplyListTheme(lstSpawns);
                    StyleButton(btnBrowse);
                    StyleButton(btnLoad);
                    StyleButton(btnSave, primary: true);
                    StyleButton(btnImportMobs);
                    StyleButton(btnImportItems);
                    StyleButton(btnAddDrop);
                    StyleButton(btnRemoveDrop);
                    StyleButton(btnAddSpawn);
                    StyleButton(btnRemoveSpawn);
                    StyleButton(btnAddMob);
                    StyleButton(btnClearFilter);

                    var __status = new Panel { Height = 26, Dock = DockStyle.Bottom, BackColor = Color.FromArgb(45, 47, 51), Padding = new Padding(8, 0, 8, 0) };
                    _lblStatus = new Label { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, ForeColor = Color.Gainsboro, Text = "Ready" };
                    __status.Controls.Add(_lblStatus);
                    Controls.Add(__status);

                    var defaultPath = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "DboServer", "ExecutionEnv", "config", "CustomDropEvent.cfg");
                    txtPath.Text = Path.GetFullPath(defaultPath);
                }

                private void ImportMobs()
                {
                    using var ofd = new OpenFileDialog();
                    ofd.Title = "Import Mob List";
                    ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
                    if (ofd.ShowDialog(this) == DialogResult.OK)
                    {
                        _mobNames = MobsItemsCatalog.ParseMobs(ofd.FileName);
                        RefreshMobList(txtMobFilter?.Text);
                        MessageBox.Show(this, $"Imported {_mobNames.Count} mobs.");
                    }
                }

                private void ImportItems()
                {
                    using var ofd = new OpenFileDialog();
                    ofd.Title = "Import Item List";
                    ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
                    if (ofd.ShowDialog(this) == DialogResult.OK)
                    {
                        _itemNames = MobsItemsCatalog.ParseItems(ofd.FileName);
                        MessageBox.Show(this, $"Imported {_itemNames.Count} items.");
                    }
                }

                private void BrowsePath()
                {
                    using var ofd = new OpenFileDialog();
                    ofd.Filter = "Config files (*.cfg)|*.cfg|All files (*.*)|*.*";
                    if (File.Exists(txtPath.Text))
                        ofd.FileName = txtPath.Text;
                    if (ofd.ShowDialog(this) == DialogResult.OK)
                        txtPath.Text = ofd.FileName;
                }

                private void LoadCfg()
                {
                    try
                    {
                        _model = ConfigModel.Load(txtPath.Text);
                        RefreshMobList(txtMobFilter?.Text);
                        LoadGlobalIntoEditors();
                        if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
                        var mobCount = lstMobs.Items.Count;
                        var dropCount = _model.Drops.Sum(kv => kv.Value.Count);
                        var spawnKeys = _model.Spawns.Keys.Count;
                        var modKeys = _model.Mods.Keys.Count;
                        _lblStatus.Text = $"Loaded: mobs={mobCount} drops={dropCount} spawnKeys={spawnKeys} modKeys={modKeys}";
                        MessageBox.Show(this, "Config loaded.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        if (_lblStatus != null) _lblStatus.Text = $"Error: {ex.Message}";
                    }
                }

                private void SaveCfg()
                {
                    try
                    {
                        SaveGlobalFromEditors();
                        SaveMobFromEditors();
                        _model.Save(txtPath.Text);
                        MessageBox.Show(this, "Config saved.");
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }

                private void RefreshMobList(string? filter = null)
                {
                    var ids = new HashSet<uint>();
                    foreach (var k in _model.Drops.Keys) ids.Add(k);
                    foreach (var k in _model.Spawns.Keys) if (k != 0) ids.Add(k);
                    foreach (var k in _model.Mods.Keys) if (k != 0) ids.Add(k);
                    var sorted = ids.OrderBy(x => x).ToList();
                    lstMobs.Items.Clear();
                    foreach (var id in sorted)
                    {
                        var hasName = _mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name);
                        var display = hasName ? $"{id} - {name}" : id.ToString();
                        if (!string.IsNullOrWhiteSpace(filter))
                        {
                            var f = filter.Trim();
                            var match = display.IndexOf(f, StringComparison.OrdinalIgnoreCase) >= 0;
                            if (!match) continue;
                        }
                        lstMobs.Items.Add(new ListViewItemWrapper(id, display));
                    }
                    if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
                }

                private void LoadGlobalIntoEditors()
                {
                    if (_model.Spawns.TryGetValue(0, out var global))
                        txtGlobalSpawns.Text = string.Join(
                            ", ",
                            global.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
                    else
                        txtGlobalSpawns.Text = string.Empty;
                }

                private void SaveGlobalFromEditors()
                {
                    var text = txtGlobalSpawns.Text.Trim();
                    if (string.IsNullOrEmpty(text))
                    {
                        _model.Spawns.Remove(0);
                        return;
                    }
                    var tmpModel = new ConfigModel();
                    tmpModel.Spawns[0] = new List<SpawnEntry>();
                    foreach (var tok in text.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint mob = 0; float rate = 100f; byte count = 1;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                            var rx = t[(at + 1)..].Trim();
                            var x = rx.IndexOf('x');
                            if (x >= 0)
                            {
                                float.TryParse(rx[..x], out rate);
                                byte.TryParse(rx[(x + 1)..], out count);
                            }
                            else
                            {
                                float.TryParse(rx, out rate);
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out mob)) continue;
                        }
                        tmpModel.Spawns[0].Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                    }
                    if (tmpModel.Spawns[0].Count > 0)
                        _model.Spawns[0] = tmpModel.Spawns[0];
                    else
                        _model.Spawns.Remove(0);
                }

                private void LoadMobIntoEditors()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) { txtDrops.Text = txtSpawns.Text = txtMods.Text = string.Empty; lstDrops.Items.Clear(); lstSpawns.Items.Clear(); return; }
                    var id = wrap.Id;
                    if (_model.Drops.TryGetValue(id, out var drops))
                    {
                        txtDrops.Text = string.Join(
                            ", ",
                            drops.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
                        lstDrops.Items.Clear();
                        foreach (var d in drops)
                            lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
                    }
                    else { txtDrops.Text = string.Empty; lstDrops.Items.Clear(); }

                    if (_model.Spawns.TryGetValue(id, out var spawns))
                    {
                        txtSpawns.Text = string.Join(
                            ", ",
                            spawns.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
                        lstSpawns.Items.Clear();
                        foreach (var s in spawns)
                            lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
                    }
                    else { txtSpawns.Text = string.Empty; lstSpawns.Items.Clear(); }

                    if (_model.Mods.TryGetValue(id, out var mods))
                        txtMods.Text = mods.ToString();
                    else txtMods.Text = string.Empty;
                }

                private void SaveMobFromEditors()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
                    var id = wrap.Id;

                    if (lstDrops.Items.Count > 0)
                    {
                        var list = new List<DropEntry>();
                        foreach (var it in lstDrops.Items.Cast<string>())
                        {
                            var t = it.Trim();
                            if (t.Length == 0) continue;
                            var at = t.IndexOf('@');
                            uint item = 0; float rate = 100f;
                            if (at >= 0)
                            {
                                if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                                var rv = t[(at + 1)..].Trim();
                                float.TryParse(rv, out rate);
                            }
                            else
                            {
                                if (!uint.TryParse(t, out item)) continue;
                            }
                            list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                        }
                        if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
                    }
                    else
                    {
                        var dText = txtDrops.Text.Trim();
                        if (!string.IsNullOrEmpty(dText))
                        {
                            var list = new List<DropEntry>();
                            foreach (var tok in dText.Split(','))
                            {
                                var t = tok.Trim();
                                if (string.IsNullOrEmpty(t)) continue;
                                var at = t.IndexOf('@');
                                uint item = 0; float rate = 100f;
                                if (at >= 0)
                                {
                                    if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                                    var rv = t[(at + 1)..].Trim();
                                    float.TryParse(rv, out rate);
                                }
                                else
                                {
                                    if (!uint.TryParse(t, out item)) continue;
                                }
                                list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                            }
                            if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
                        }
                        else _model.Drops.Remove(id);
                    }

                    if (lstSpawns.Items.Count > 0)
                    {
                        var list = new List<SpawnEntry>();
                        foreach (var it in lstSpawns.Items.Cast<string>())
                        {
                            var t = it.Trim();
                            if (t.Length == 0) continue;
                            var at = t.IndexOf('@');
                            uint mob = 0; float rate = 100f; byte count = 1;
                            if (at >= 0)
                            {
                                if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                                var rx = t[(at + 1)..].Trim();
                                var x = rx.IndexOf('x');
                                if (x >= 0)
                                {
                                    float.TryParse(rx[..x], out rate);
                                    byte.TryParse(rx[(x + 1)..], out count);
                                }
                                else
                                {
                                    float.TryParse(rx, out rate);
                                }
                            }
                            else
                            {
                                if (!uint.TryParse(t, out mob)) continue;
                            }
                            list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                        }
                        if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
                    }
                    else
                    {
                        var sText = txtSpawns.Text.Trim();
                        if (!string.IsNullOrEmpty(sText))
                        {
                            var list = new List<SpawnEntry>();
                            foreach (var tok in sText.Split(','))
                            {
                                var t = tok.Trim();
                                if (string.IsNullOrEmpty(t)) continue;
                                var at = t.IndexOf('@');
                                uint mob = 0; float rate = 100f; byte count = 1;
                                if (at >= 0)
                                {
                                    if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                                    var rx = t[(at + 1)..].Trim();
                                    var x = rx.IndexOf('x');
                                    if (x >= 0)
                                    {
                                        float.TryParse(rx[..x], out rate);
                                        byte.TryParse(rx[(x + 1)..], out count);
                                    }
                                    else
                                    {
                                        float.TryParse(rx, out rate);
                                    }
                                }
                                else
                                {
                                    if (!uint.TryParse(t, out mob)) continue;
                                }
                                list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                            }
                            if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
                        }
                        else _model.Spawns.Remove(id);
                    }

                    var mText = txtMods.Text.Trim();
                    if (!string.IsNullOrEmpty(mText)) _model.Mods[id] = Modifiers.Parse(mText); else _model.Mods.Remove(id);

                    LoadMobIntoEditors();
                }

                private void AddDropFromInputs()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
                    if (!uint.TryParse(txtDropItemId.Text.Trim(), out var itemId)) { MessageBox.Show(this, "Invalid Item ID"); return; }
                    var rate = (float)numDropRate.Value;
                    var id = wrap.Id;
                    if (!_model.Drops.TryGetValue(id, out var list)) { list = new List<DropEntry>(); _model.Drops[id] = list; }
                    list.Add(new DropEntry { ItemTblidx = itemId, Rate = rate });
                    RefreshDropsUI(list);
                }

                private void RemoveSelectedDrop()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
                    var id = wrap.Id;
                    if (!_model.Drops.TryGetValue(id, out var list)) return;
                    var idx = lstDrops.SelectedIndex;
                    if (idx < 0 || idx >= list.Count) return;
                    list.RemoveAt(idx);
                    if (list.Count == 0) _model.Drops.Remove(id);
                    RefreshDropsUI(_model.Drops.TryGetValue(id, out var l2) ? l2 : null);
                }

                private void RefreshDropsUI(List<DropEntry> list)
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper) return;
                    lstDrops.Items.Clear();
                    if (list != null)
                    {
                        foreach (var d in list)
                            lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
                        txtDrops.Text = string.Join(
                            ", ",
                            list.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
                    }
                    else
                    {
                        txtDrops.Text = string.Empty;
                    }
                }

                private void AddSpawnFromInputs()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
                    if (!uint.TryParse(txtSpawnMobId.Text.Trim(), out var mobId)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
                    var rate = (float)numSpawnRate.Value;
                    var count = (byte)numSpawnCount.Value;
                    var id = wrap.Id;
                    if (!_model.Spawns.TryGetValue(id, out var list)) { list = new List<SpawnEntry>(); _model.Spawns[id] = list; }
                    list.Add(new SpawnEntry { MobTblidx = mobId, Rate = rate, Count = count });
                    RefreshSpawnsUI(list);
                }

                private void RemoveSelectedSpawn()
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
                    var id = wrap.Id;
                    if (!_model.Spawns.TryGetValue(id, out var list)) return;
                    var idx = lstSpawns.SelectedIndex;
                    if (idx < 0 || idx >= list.Count) return;
                    list.RemoveAt(idx);
                    if (list.Count == 0) _model.Spawns.Remove(id);
                    RefreshSpawnsUI(_model.Spawns.TryGetValue(id, out var l2) ? l2 : null);
                }

                private void RefreshSpawnsUI(List<SpawnEntry> list)
                {
                    if (lstMobs.SelectedItem is not ListViewItemWrapper) return;
                    lstSpawns.Items.Clear();
                    if (list != null)
                    {
                        foreach (var s in list)
                            lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
                        txtSpawns.Text = string.Join(
                            ", ",
                            list.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
                    }
                    else
                    {
                        txtSpawns.Text = string.Empty;
                    }
                }

                private void ApplyFieldTheme(TextBox tb)
                {
                    tb.BorderStyle = BorderStyle.FixedSingle;
                    tb.BackColor = Color.FromArgb(40, 41, 45);
                    tb.ForeColor = Color.Gainsboro;
                }

                private void ApplyNumericTheme(NumericUpDown nd)
                {
                    nd.BorderStyle = BorderStyle.FixedSingle;
                    nd.BackColor = Color.FromArgb(40, 41, 45);
                    nd.ForeColor = Color.Gainsboro;
                }

                private void ApplyListTheme(ListBox lb)
                {
                    lb.BorderStyle = BorderStyle.FixedSingle;
                    lb.BackColor = Color.FromArgb(40, 41, 45);
                    lb.ForeColor = Color.Gainsboro;
                }

                private void StyleButton(Button btn, bool primary = false)
                {
                    btn.FlatStyle = FlatStyle.Flat;
                    btn.FlatAppearance.BorderColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(70, 72, 78);
                    btn.FlatAppearance.BorderSize = 1;
                    btn.ForeColor = Color.WhiteSmoke;
                    btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
                    btn.Cursor = Cursors.Hand;
                    btn.MouseEnter += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 105, 190) : Color.FromArgb(75, 77, 83);
                    btn.MouseLeave += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
                }

                private void ApplyLabelTheme(Label lbl)
                {
                    lbl.ForeColor = Color.Gainsboro;
                }

                private void AddMob()
                {
                    if (!uint.TryParse(txtNewMobId.Text.Trim(), out var id)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
                    var existingIds = new HashSet<uint>();
                    foreach (var k in _model.Drops.Keys) existingIds.Add(k);
                    foreach (var k in _model.Spawns.Keys) if (k != 0) existingIds.Add(k);
                    foreach (var k in _model.Mods.Keys) if (k != 0) existingIds.Add(k);
                    if (!existingIds.Contains(id))
                    {
                        _model.Mods[id] = new Modifiers();
                    }
                    RefreshMobList(txtMobFilter?.Text);
                    for (int i = 0; i < lstMobs.Items.Count; i++)
                    {
                        if (lstMobs.Items[i] is ListViewItemWrapper w && w.Id == id)
                        {
                            lstMobs.SelectedIndex = i;
                            break;
                        }
                    }
                    _lblStatus.Text = $"Mob {id} ready. Add drops/spawns on the right.";
                }
            }
        }
                if (lstMobs.Items[i] is ListViewItemWrapper w && w.Id == id)
                {
                    lstMobs.SelectedIndex = i;
                    break;
                }
            }
            _lblStatus.Text = $"Mob {id} ready. Add drops/spawns on the right.";
        }
    }
}

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace CustomDropEventEditor
{
    public class MainForm : Form
    {
        private TextBox txtPath;
        private Button btnBrowse, btnLoad, btnSave;
    private ListBox lstMobs;
    private TextBox txtMobFilter;
    private Label lblMobFilter;
    private Button btnClearFilter;
        private TextBox txtDrops, txtSpawns, txtMods, txtGlobalSpawns;
        private Label lblDrops, lblSpawns, lblMods, lblGlobalSpawns;
        private Button btnImportMobs, btnImportItems;

        private Label _lblStatus;

        // New UI for list-based editing
        private ListBox lstDrops, lstSpawns;
        private TextBox txtDropItemId, txtSpawnMobId;
        private NumericUpDown numDropRate, numSpawnRate, numSpawnCount;
        private Button btnAddDrop, btnRemoveDrop, btnAddSpawn, btnRemoveSpawn;

        // New: Add Mob controls and labels
        private TextBox txtNewMobId;
        private Button btnAddMob;
        private Label lblAddMob, lblDropItem, lblDropRate, lblSpawnMob, lblSpawnRate, lblSpawnCount;

        private ConfigModel _model = new();
        private Dictionary<uint, string> _mobNames = new();
        private Dictionary<uint, string> _itemNames = new();

        public MainForm()
        {
            Text = "Custom Drop Event Editor";
            Width = 1100;
            Height = 720;
            AutoScaleMode = AutoScaleMode.Dpi;
            Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point);
            BackColor = Color.FromArgb(32, 33, 36);
            ForeColor = Color.Gainsboro;

            txtPath = new TextBox { Left = 10, Top = 10, Width = 800, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
            btnBrowse = new Button { Left = 820, Top = 8, Width = 80, Height = 28, Text = "Browse" };
            btnLoad = new Button { Left = 910, Top = 8, Width = 80, Height = 28, Text = "Load" };
            btnSave = new Button { Left = 1000, Top = 8, Width = 80, Height = 28, Text = "Save" };

            btnImportMobs = new Button { Left = 820, Top = 40, Width = 130, Height = 28, Text = "Import Mobs..." };
            btnImportItems = new Button { Left = 960, Top = 40, Width = 120, Height = 28, Text = "Import Items..." };

            // Mob filter above the mob list
            lblMobFilter = new Label { Left = 10, Top = 70, AutoSize = true, Text = "Filter" };
            txtMobFilter = new TextBox { Left = 60, Top = 68, Width = 180, Height = 24 };
            btnClearFilter = new Button { Left = 245, Top = 68, Width = 65, Height = 24, Text = "Clear" };
            // Mob list below filter
            lstMobs = new ListBox { Left = 10, Top = 98, Width = 300, Height = 572, Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left, IntegralHeight = false };
            lblGlobalSpawns = new Label { Left = 320, Top = 70, AutoSize = true, Text = "Global Spawns (all spawn: mob@ratexcount, ...)" };
            txtGlobalSpawns = new TextBox { Left = 320, Top = 90, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lblDrops = new Label { Left = 320, Top = 130, AutoSize = true, Text = "Drops (item@rate, ...)" };
            // keep the text field for serialization/quick paste, but de-emphasize visually
            txtDrops = new TextBox { Left = 320, Top = 150, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            // Drops list and controls
            lstDrops = new ListBox { Left = 320, Top = 182, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            // Labels for drop inputs
            lblDropItem = new Label { Left = 320, Top = 286, AutoSize = true, Text = "Item ID" };
            txtDropItemId = new TextBox { Left = 320, Top = 306, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblDropRate = new Label { Left = 470, Top = 286, AutoSize = true, Text = "Rate" };
            numDropRate = new NumericUpDown { Left = 470, Top = 306, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddDrop = new Button { Left = 580, Top = 306, Width = 90, Height = 28, Text = "Add" };
            btnRemoveDrop = new Button { Left = 680, Top = 306, Width = 90, Height = 28, Text = "Remove" };

            // Spawns list and controls
            lstSpawns = new ListBox { Left = 320, Top = 398, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            // Labels for spawn inputs
            lblSpawnMob = new Label { Left = 320, Top = 502, AutoSize = true, Text = "Mob ID" };
            txtSpawnMobId = new TextBox { Left = 320, Top = 522, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnRate = new Label { Left = 470, Top = 502, AutoSize = true, Text = "Rate" };
            numSpawnRate = new NumericUpDown { Left = 470, Top = 522, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnCount = new Label { Left = 580, Top = 502, AutoSize = true, Text = "Count" };
            numSpawnCount = new NumericUpDown { Left = 580, Top = 522, Width = 80, Height = 28, DecimalPlaces = 0, Minimum = 1, Maximum = 255, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddSpawn = new Button { Left = 670, Top = 522, Width = 90, Height = 28, Text = "Add" };
            btnRemoveSpawn = new Button { Left = 770, Top = 522, Width = 90, Height = 28, Text = "Remove" };

            lblMods = new Label { Left = 320, Top = 562, AutoSize = true, Text = "Modifiers (hp= engAtk= physAtk= ... sizeRate=)" };
            txtMods = new TextBox { Left = 320, Top = 582, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            // Place Add Mob controls above the mob list on the left
            lblAddMob = new Label { Left = 10, Top = 40, AutoSize = true, Text = "Add Mob ID:" };
            txtNewMobId = new TextBox { Left = 100, Top = 38, Width = 140, Height = 28 };
            btnAddMob = new Button { Left = 250, Top = 38, Width = 60, Height = 28, Text = "Add" };

            Controls.AddRange(new Control[] { txtPath, btnBrowse, btnLoad, btnSave, btnImportMobs, btnImportItems, lblMobFilter, txtMobFilter, btnClearFilter, lstMobs, lblGlobalSpawns, txtGlobalSpawns, lblDrops, txtDrops, lstDrops, txtDropItemId, numDropRate, btnAddDrop, btnRemoveDrop, lblSpawns, txtSpawns, lstSpawns, txtSpawnMobId, numSpawnRate, numSpawnCount, btnAddSpawn, btnRemoveSpawn, lblMods, txtMods, lblAddMob, txtNewMobId, btnAddMob });

            // Wire events
            btnBrowse.Click += (s, e) => BrowsePath();
            btnLoad.Click += (s, e) => LoadCfg();
            btnSave.Click += (s, e) => SaveCfg();
            btnImportMobs.Click += (s, e) => ImportMobs();
            btnImportItems.Click += (s, e) => ImportItems();
            lstMobs.SelectedIndexChanged += (s, e) => LoadMobIntoEditors();
            txtDrops.Leave += (s, e) => SaveMobFromEditors();
            txtSpawns.Leave += (s, e) => SaveMobFromEditors();
            txtMods.Leave += (s, e) => SaveMobFromEditors();
            txtGlobalSpawns.Leave += (s, e) => SaveGlobalFromEditors();

            // New list buttons
            btnAddDrop.Click += (s, e) => AddDropFromInputs();
            btnRemoveDrop.Click += (s, e) => RemoveSelectedDrop();
            btnAddSpawn.Click += (s, e) => AddSpawnFromInputs();
            btnRemoveSpawn.Click += (s, e) => RemoveSelectedSpawn();
            btnAddMob.Click += (s, e) => AddMob();
            txtMobFilter.TextChanged += (s, e) => RefreshMobList(txtMobFilter.Text);
            btnClearFilter.Click += (s, e) => { txtMobFilter.Text = string.Empty; RefreshMobList(); };
            lstDrops.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedDrop(); e.Handled = true; } };
            lstSpawns.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedSpawn(); e.Handled = true; } };
            txtDropItemId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
            txtSpawnMobId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
            numDropRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
            numSpawnRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
            numSpawnCount.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };

            // Apply dark theme styles
            ApplyFieldTheme(txtPath);
            ApplyFieldTheme(txtGlobalSpawns);
            ApplyFieldTheme(txtDrops);
            ApplyFieldTheme(txtSpawns);
            ApplyFieldTheme(txtMods);
            ApplyFieldTheme(txtDropItemId);
            ApplyFieldTheme(txtSpawnMobId);
            ApplyFieldTheme(txtNewMobId);
            ApplyFieldTheme(txtMobFilter);
            ApplyLabelTheme(lblMobFilter);
            ApplyNumericTheme(numDropRate);
            ApplyNumericTheme(numSpawnRate);
            ApplyNumericTheme(numSpawnCount);
            ApplyListTheme(lstMobs);
            ApplyListTheme(lstDrops);
            ApplyListTheme(lstSpawns);
            StyleButton(btnBrowse);
            StyleButton(btnLoad);
            StyleButton(btnSave, primary: true);
            StyleButton(btnImportMobs);
            StyleButton(btnImportItems);
            StyleButton(btnAddDrop);
            StyleButton(btnRemoveDrop);
            StyleButton(btnAddSpawn);
            StyleButton(btnRemoveSpawn);
            StyleButton(btnAddMob);
            StyleButton(btnClearFilter);

            // Keyboard shortcuts
            KeyPreview = true;
            KeyDown += (s, e) =>
            {
                if (e.Control && e.KeyCode == Keys.S) { btnSave.PerformClick(); e.Handled = true; }
                else if (e.Control && e.KeyCode == Keys.O) { btnLoad.PerformClick(); e.Handled = true; }
            };

            // Status bar
            var __status = new Panel { Height = 26, Dock = DockStyle.Bottom, BackColor = Color.FromArgb(45, 47, 51), Padding = new Padding(8, 0, 8, 0) };
            _lblStatus = new Label { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, ForeColor = Color.Gainsboro, Text = "Ready" };
            __status.Controls.Add(_lblStatus);
            Controls.Add(__status);

            // Default path remains
            var defaultPath = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "DboServer", "ExecutionEnv", "config", "CustomDropEvent.cfg");
            txtPath.Text = Path.GetFullPath(defaultPath);
        }

        private void ImportMobs()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Mob List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _mobNames = MobsItemsCatalog.ParseMobs(ofd.FileName);
                RefreshMobList();
                MessageBox.Show(this, $"Imported {_mobNames.Count} mobs.");
            }
        }

        private void ImportItems()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Item List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _itemNames = MobsItemsCatalog.ParseItems(ofd.FileName);
                MessageBox.Show(this, $"Imported {_itemNames.Count} items.");
            }
        }

        private void BrowsePath()
        {
            using var ofd = new OpenFileDialog();
            ofd.Filter = "Config files (*.cfg)|*.cfg|All files (*.*)|*.*";
            if (File.Exists(txtPath.Text))
                ofd.FileName = txtPath.Text;
            if (ofd.ShowDialog(this) == DialogResult.OK)
                txtPath.Text = ofd.FileName;
        }

        private void LoadCfg()
        {
            try
            {
                _model = ConfigModel.Load(txtPath.Text);
                RefreshMobList();
                LoadGlobalIntoEditors();
                if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
                var mobCount = lstMobs.Items.Count;
                var dropCount = _model.Drops.Sum(kv => kv.Value.Count);
                var spawnKeys = _model.Spawns.Keys.Count;
                var modKeys = _model.Mods.Keys.Count;
                _lblStatus.Text = $"Loaded: mobs={mobCount} drops={dropCount} spawnKeys={spawnKeys} modKeys={modKeys}";
                MessageBox.Show(this, "Config loaded.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                if (_lblStatus != null) _lblStatus.Text = $"Error: {ex.Message}";
            }
        }

        private void SaveCfg()
        {
            try
            {
                SaveGlobalFromEditors();
                SaveMobFromEditors();
                _model.Save(txtPath.Text);
                MessageBox.Show(this, "Config saved.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void RefreshMobList(string? filter = null)
        {
            var ids = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) ids.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) ids.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) ids.Add(k);
            var sorted = ids.OrderBy(x => x).ToList();
            lstMobs.Items.Clear();
            foreach (var id in sorted)
            {
                var hasName = _mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name);
                var display = hasName ? $"{id} - {name}" : id.ToString();
                if (!string.IsNullOrWhiteSpace(filter))
                {
                    var f = filter.Trim();
                    var match = display.IndexOf(f, StringComparison.OrdinalIgnoreCase) >= 0;
                    if (!match) continue;
                }
                lstMobs.Items.Add(new ListViewItemWrapper(id, display));
            }
            if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
        }

        private void LoadGlobalIntoEditors()
        {
            if (_model.Spawns.TryGetValue(0, out var global))
                txtGlobalSpawns.Text = string.Join(
                    ", ",
                    global.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
            else
                txtGlobalSpawns.Text = string.Empty;
        }

        private void SaveGlobalFromEditors()
        {
            var text = txtGlobalSpawns.Text.Trim();
            if (string.IsNullOrEmpty(text))
            {
                _model.Spawns.Remove(0);
                return;
            }
            var tmpModel = new ConfigModel();
            tmpModel.Spawns[0] = new List<SpawnEntry>();
            foreach (var tok in text.Split(','))
            {
                var t = tok.Trim();
                if (string.IsNullOrEmpty(t)) continue;
                var at = t.IndexOf('@');
                uint mob = 0; float rate = 100f; byte count = 1;
                if (at >= 0)
                {
                    if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                    var rx = t[(at + 1)..].Trim();
                    var x = rx.IndexOf('x');
                    if (x >= 0)
                    {
                        float.TryParse(rx[..x], out rate);
                        byte.TryParse(rx[(x + 1)..], out count);
                    }
                    else
                    {
                        float.TryParse(rx, out rate);
                    }
                }
                else
                {
                    if (!uint.TryParse(t, out mob)) continue;
                }
                tmpModel.Spawns[0].Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
            }
            if (tmpModel.Spawns[0].Count > 0)
                _model.Spawns[0] = tmpModel.Spawns[0];
            else
                _model.Spawns.Remove(0);
        }

        private void LoadMobIntoEditors()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) { txtDrops.Text = txtSpawns.Text = txtMods.Text = string.Empty; lstDrops.Items.Clear(); lstSpawns.Items.Clear(); return; }
            var id = wrap.Id;
            if (_model.Drops.TryGetValue(id, out var drops))
            {
                txtDrops.Text = string.Join(
                    ", ",
                    drops.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
                lstDrops.Items.Clear();
                foreach (var d in drops)
                    lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
            }
            else { txtDrops.Text = string.Empty; lstDrops.Items.Clear(); }

            if (_model.Spawns.TryGetValue(id, out var spawns))
            {
                txtSpawns.Text = string.Join(
                    ", ",
                    spawns.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
                lstSpawns.Items.Clear();
                foreach (var s in spawns)
                    lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
            }
            else { txtSpawns.Text = string.Empty; lstSpawns.Items.Clear(); }

            if (_model.Mods.TryGetValue(id, out var mods))
                txtMods.Text = mods.ToString();
            else txtMods.Text = string.Empty;
        }

        private void SaveMobFromEditors()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;

            // Prefer list content if available; otherwise parse from text fields
            if (lstDrops.Items.Count > 0)
            {
                var list = new List<DropEntry>();
                foreach (var it in lstDrops.Items.Cast<string>())
                {
                    var t = it.Trim();
                    if (t.Length == 0) continue;
                    var at = t.IndexOf('@');
                    uint item = 0; float rate = 100f;
                    if (at >= 0)
                    {
                        if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                        var rv = t[(at + 1)..].Trim();
                        float.TryParse(rv, out rate);
                    }
                    else
                    {
                        if (!uint.TryParse(t, out item)) continue;
                    }
                    list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                }
                if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
            }
            else
            {
                // fallback to text parsing
                var dText = txtDrops.Text.Trim();
                if (!string.IsNullOrEmpty(dText))
                {
                    var list = new List<DropEntry>();
                    foreach (var tok in dText.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint item = 0; float rate = 100f;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                            var rv = t[(at + 1)..].Trim();
                            float.TryParse(rv, out rate);
                        }
                        else
                        {
                            if (!uint.TryParse(t, out item)) continue;
                        }
                        list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                    }
                    if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
                }
                else _model.Drops.Remove(id);
            }

            if (lstSpawns.Items.Count > 0)
            {
                var list = new List<SpawnEntry>();
                foreach (var it in lstSpawns.Items.Cast<string>())
                {
                    var t = it.Trim();
                    if (t.Length == 0) continue;
                    var at = t.IndexOf('@');
                    uint mob = 0; float rate = 100f; byte count = 1;
                    if (at >= 0)
                    {
                        if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                        var rx = t[(at + 1)..].Trim();
                        var x = rx.IndexOf('x');
                        if (x >= 0)
                        {
                            float.TryParse(rx[..x], out rate);
                            byte.TryParse(rx[(x + 1)..], out count);
                        }
                        else
                        {
                            float.TryParse(rx, out rate);
                        }
                    }
                    else
                    {
                        if (!uint.TryParse(t, out mob)) continue;
                    }
                    list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                }
                if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
            }
            else
            {
                // fallback to text parsing
                var sText = txtSpawns.Text.Trim();
                if (!string.IsNullOrEmpty(sText))
                {
                    var list = new List<SpawnEntry>();
                    foreach (var tok in sText.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint mob = 0; float rate = 100f; byte count = 1;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                            var rx = t[(at + 1)..].Trim();
                            var x = rx.IndexOf('x');
                            if (x >= 0)
                            {
                                float.TryParse(rx[..x], out rate);
                                byte.TryParse(rx[(x + 1)..], out count);
                            }
                            else
                            {
                                float.TryParse(rx, out rate);
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out mob)) continue;
                        }
                        list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                    }
                    if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
                }
                else _model.Spawns.Remove(id);
            }

            var mText = txtMods.Text.Trim();
            if (!string.IsNullOrEmpty(mText)) _model.Mods[id] = Modifiers.Parse(mText); else _model.Mods.Remove(id);

            // keep text fields in sync for convenience
            LoadMobIntoEditors();
        }

        private void AddDropFromInputs()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            if (!uint.TryParse(txtDropItemId.Text.Trim(), out var itemId)) { MessageBox.Show(this, "Invalid Item ID"); return; }
            var rate = (float)numDropRate.Value;
            var id = wrap.Id;
            if (!_model.Drops.TryGetValue(id, out var list)) { list = new List<DropEntry>(); _model.Drops[id] = list; }
            list.Add(new DropEntry { ItemTblidx = itemId, Rate = rate });
            RefreshDropsUI(list);
        }

        private void RemoveSelectedDrop()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;
            if (!_model.Drops.TryGetValue(id, out var list)) return;
            var idx = lstDrops.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Drops.Remove(id);
            RefreshDropsUI(_model.Drops.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshDropsUI(List<DropEntry> list)
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            lstDrops.Items.Clear();
            if (list != null)
            {
                foreach (var d in list)
                    lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
                txtDrops.Text = string.Join(
                    ", ",
                    list.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
            }
            else
            {
                txtDrops.Text = string.Empty;
            }
        }

        private void AddSpawnFromInputs()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            if (!uint.TryParse(txtSpawnMobId.Text.Trim(), out var mobId)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
            var rate = (float)numSpawnRate.Value;
            var count = (byte)numSpawnCount.Value;
            var id = wrap.Id;
            if (!_model.Spawns.TryGetValue(id, out var list)) { list = new List<SpawnEntry>(); _model.Spawns[id] = list; }
            list.Add(new SpawnEntry { MobTblidx = mobId, Rate = rate, Count = count });
            RefreshSpawnsUI(list);
        }

        private void RemoveSelectedSpawn()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;
            if (!_model.Spawns.TryGetValue(id, out var list)) return;
            var idx = lstSpawns.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Spawns.Remove(id);
            RefreshSpawnsUI(_model.Spawns.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshSpawnsUI(List<SpawnEntry> list)
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            lstSpawns.Items.Clear();
            if (list != null)
            {
                foreach (var s in list)
                    lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
                txtSpawns.Text = string.Join(
                    ", ",
                    list.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
            }
            else
            {
                txtSpawns.Text = string.Empty;
            }
        }

        private void ApplyFieldTheme(TextBox tb)
        {
            tb.BorderStyle = BorderStyle.FixedSingle;
            tb.BackColor = Color.FromArgb(40, 41, 45);
            tb.ForeColor = Color.Gainsboro;
        }

        private void ApplyNumericTheme(NumericUpDown nd)
        {
            nd.BorderStyle = BorderStyle.FixedSingle;
            nd.BackColor = Color.FromArgb(40, 41, 45);
            nd.ForeColor = Color.Gainsboro;
        }

        private void ApplyListTheme(ListBox lb)
        {
            lb.BorderStyle = BorderStyle.FixedSingle;
            lb.BackColor = Color.FromArgb(40, 41, 45);
            lb.ForeColor = Color.Gainsboro;
        }

        private void StyleButton(Button btn, bool primary = false)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(70, 72, 78);
            btn.FlatAppearance.BorderSize = 1;
            btn.ForeColor = Color.WhiteSmoke;
            btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
            btn.Cursor = Cursors.Hand;
            btn.MouseEnter += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 105, 190) : Color.FromArgb(75, 77, 83);
            btn.MouseLeave += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
        }

        private void ApplyLabelTheme(Label lbl)
        {
            lbl.ForeColor = Color.Gainsboro;
        }

        private void AddMob()
        {
            if (!uint.TryParse(txtNewMobId.Text.Trim(), out var id)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
            // If mob already present, just select it
            var existingIds = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) existingIds.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) existingIds.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) existingIds.Add(k);
            if (!existingIds.Contains(id))
            {
                // Create a default modifiers entry so it appears in the list
                _model.Mods[id] = new Modifiers();
            }
            RefreshMobList();
            // Select the newly added mob
            for (int i = 0; i < lstMobs.Items.Count; i++)
            {
                if (lstMobs.Items[i] is ListViewItemWrapper w && w.Id == id)
                {
                    lstMobs.SelectedIndex = i;
                    break;
                }
            }
            _lblStatus.Text = $"Mob {id} ready. Add drops/spawns on the right.";
        }
    }
}

        private void ImportMobs()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Mob List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _mobNames = MobsItemsCatalog.ParseMobs(ofd.FileName);
                RefreshMobList();
                MessageBox.Show(this, $"Imported {_mobNames.Count} mobs.");
            }
        }

        private void ImportItems()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Item List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _itemNames = MobsItemsCatalog.ParseItems(ofd.FileName);
                MessageBox.Show(this, $"Imported {_itemNames.Count} items.");
            }
        }

        private void BrowsePath()
        {
            using var ofd = new OpenFileDialog();
            ofd.Filter = "Config files (*.cfg)|*.cfg|All files (*.*)|*.*";
            if (File.Exists(txtPath.Text))
                ofd.FileName = txtPath.Text;
            if (ofd.ShowDialog(this) == DialogResult.OK)
                txtPath.Text = ofd.FileName;
        }

        private void LoadCfg()
        {
            try
            {
                _model = ConfigModel.Load(txtPath.Text);
                RefreshMobList();
                LoadGlobalIntoEditors();
                if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
                var mobCount = lstMobs.Items.Count;
                var dropCount = _model.Drops.Sum(kv => kv.Value.Count);
                var spawnKeys = _model.Spawns.Keys.Count;
                var modKeys = _model.Mods.Keys.Count;
                _lblStatus.Text = $"Loaded: mobs={mobCount} drops={dropCount} spawnKeys={spawnKeys} modKeys={modKeys}";
                MessageBox.Show(this, "Config loaded.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                if (_lblStatus != null) _lblStatus.Text = $"Error: {ex.Message}";
            }
        }

        private void SaveCfg()
        {
            try
            {
                SaveGlobalFromEditors();
                SaveMobFromEditors();
                _model.Save(txtPath.Text);
                MessageBox.Show(this, "Config saved.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void RefreshMobList()
        {
            var ids = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) ids.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) ids.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) ids.Add(k);
            var sorted = ids.OrderBy(x => x).ToList();
            lstMobs.Items.Clear();
            foreach (var id in sorted)
            {
                if (_mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name))
                    lstMobs.Items.Add(new ListViewItemWrapper(id, $"{id} - {name}"));
                else
                    lstMobs.Items.Add(new ListViewItemWrapper(id, id.ToString()));
            }
            if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
        }

        private void LoadGlobalIntoEditors()
        {
            if (_model.Spawns.TryGetValue(0, out var global))
                txtGlobalSpawns.Text = string.Join(
                    ", ",
                    global.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
            else
                txtGlobalSpawns.Text = string.Empty;
        }

        private void SaveGlobalFromEditors()
        {
            var text = txtGlobalSpawns.Text.Trim();
            if (string.IsNullOrEmpty(text))
            {
                _model.Spawns.Remove(0);
                return;
            }
            var tmpModel = new ConfigModel();
            tmpModel.Spawns[0] = new List<SpawnEntry>();
            foreach (var tok in text.Split(','))
            {
                var t = tok.Trim();
                if (string.IsNullOrEmpty(t)) continue;
                var at = t.IndexOf('@');
                uint mob = 0; float rate = 100f; byte count = 1;
                if (at >= 0)
                {
                    if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                    var rx = t[(at + 1)..].Trim();
                    var x = rx.IndexOf('x');
                    if (x >= 0)
                    {
                        float.TryParse(rx[..x], out rate);
                        byte.TryParse(rx[(x + 1)..], out count);
                    }
                    else
                    {
                        float.TryParse(rx, out rate);
                    }
                }
                else
                {
                    if (!uint.TryParse(t, out mob)) continue;
                }
                tmpModel.Spawns[0].Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
            }
            if (tmpModel.Spawns[0].Count > 0)
                _model.Spawns[0] = tmpModel.Spawns[0];
            else
                _model.Spawns.Remove(0);
        }

        private void LoadMobIntoEditors()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) { txtDrops.Text = txtSpawns.Text = txtMods.Text = string.Empty; lstDrops.Items.Clear(); lstSpawns.Items.Clear(); return; }
            var id = wrap.Id;
            if (_model.Drops.TryGetValue(id, out var drops))
            {
                txtDrops.Text = string.Join(
                    ", ",
                    drops.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
                lstDrops.Items.Clear();
                foreach (var d in drops)
                    lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
            }
            else { txtDrops.Text = string.Empty; lstDrops.Items.Clear(); }

            if (_model.Spawns.TryGetValue(id, out var spawns))
            {
                txtSpawns.Text = string.Join(
                    ", ",
                    spawns.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
                lstSpawns.Items.Clear();
                foreach (var s in spawns)
                    lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
            }
            else { txtSpawns.Text = string.Empty; lstSpawns.Items.Clear(); }

            if (_model.Mods.TryGetValue(id, out var mods))
                txtMods.Text = mods.ToString();
            else txtMods.Text = string.Empty;
        }

        private void SaveMobFromEditors()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;

            // Prefer list content if available; otherwise parse from text fields
            if (lstDrops.Items.Count > 0)
            {
                var list = new List<DropEntry>();
                foreach (var it in lstDrops.Items.Cast<string>())
                {
                    var t = it.Trim();
                    if (t.Length == 0) continue;
                    var at = t.IndexOf('@');
                    uint item = 0; float rate = 100f;
                    if (at >= 0)
                    {
                        if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                        var rv = t[(at + 1)..].Trim();
                        float.TryParse(rv, out rate);
                    }
                    else
                    {
                        if (!uint.TryParse(t, out item)) continue;
                    }
                    list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                }
                if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
            }
            else
            {
                // fallback to text parsing
                var dText = txtDrops.Text.Trim();
                if (!string.IsNullOrEmpty(dText))
                {
                    var list = new List<DropEntry>();
                    foreach (var tok in dText.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint item = 0; float rate = 100f;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                            var rv = t[(at + 1)..].Trim();
                            float.TryParse(rv, out rate);
                        }
                        else
                        {
                            if (!uint.TryParse(t, out item)) continue;
                        }
                        list.Add(new DropEntry { ItemTblidx = item, Rate = rate });
                    }
                    if (list.Count > 0) _model.Drops[id] = list; else _model.Drops.Remove(id);
                }
                else _model.Drops.Remove(id);
            }

            if (lstSpawns.Items.Count > 0)
            {
                var list = new List<SpawnEntry>();
                foreach (var it in lstSpawns.Items.Cast<string>())
                {
                    var t = it.Trim();
                    if (t.Length == 0) continue;
                    var at = t.IndexOf('@');
                    uint mob = 0; float rate = 100f; byte count = 1;
                    if (at >= 0)
                    {
                        if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                        var rx = t[(at + 1)..].Trim();
                        var x = rx.IndexOf('x');
                        if (x >= 0)
                        {
                            float.TryParse(rx[..x], out rate);
                            byte.TryParse(rx[(x + 1)..], out count);
                        }
                        else
                        {
                            float.TryParse(rx, out rate);
                        }
                    }
                    else
                    {
                        if (!uint.TryParse(t, out mob)) continue;
                    }
                    list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                }
                if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
            }
            else
            {
                // fallback to text parsing
                var sText = txtSpawns.Text.Trim();
                if (!string.IsNullOrEmpty(sText))
                {
                    var list = new List<SpawnEntry>();
                    foreach (var tok in sText.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint mob = 0; float rate = 100f; byte count = 1;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                            var rx = t[(at + 1)..].Trim();
                            var x = rx.IndexOf('x');
                            if (x >= 0)
                            {
                                float.TryParse(rx[..x], out rate);
                                byte.TryParse(rx[(x + 1)..], out count);
                            }
                            else
                            {
                                float.TryParse(rx, out rate);
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out mob)) continue;
                        }
                        list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                    }
                    if (list.Count > 0) _model.Spawns[id] = list; else _model.Spawns.Remove(id);
                }
                else _model.Spawns.Remove(id);
            }

            var mText = txtMods.Text.Trim();
            if (!string.IsNullOrEmpty(mText)) _model.Mods[id] = Modifiers.Parse(mText); else _model.Mods.Remove(id);

            // keep text fields in sync for convenience
            LoadMobIntoEditors();
        }

        private void AddDropFromInputs()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            if (!uint.TryParse(txtDropItemId.Text.Trim(), out var itemId)) { MessageBox.Show(this, "Invalid Item ID"); return; }
            var rate = (float)numDropRate.Value;
            var id = wrap.Id;
            if (!_model.Drops.TryGetValue(id, out var list)) { list = new List<DropEntry>(); _model.Drops[id] = list; }
            list.Add(new DropEntry { ItemTblidx = itemId, Rate = rate });
            RefreshDropsUI(list);
        }

        private void RemoveSelectedDrop()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;
            if (!_model.Drops.TryGetValue(id, out var list)) return;
            var idx = lstDrops.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Drops.Remove(id);
            RefreshDropsUI(_model.Drops.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshDropsUI(List<DropEntry> list)
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            lstDrops.Items.Clear();
            if (list != null)
            {
                foreach (var d in list)
                    lstDrops.Items.Add(d.Rate >= 100f ? $"{d.ItemTblidx}" : $"{d.ItemTblidx}@{d.Rate}");
                txtDrops.Text = string.Join(
                    ", ",
                    list.Select(e => e.Rate >= 100f ? $"{e.ItemTblidx}" : $"{e.ItemTblidx}@{e.Rate}"));
            }
            else
            {
                txtDrops.Text = string.Empty;
            }
        }

        private void AddSpawnFromInputs()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            if (!uint.TryParse(txtSpawnMobId.Text.Trim(), out var mobId)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
            var rate = (float)numSpawnRate.Value;
            var count = (byte)numSpawnCount.Value;
            var id = wrap.Id;
            if (!_model.Spawns.TryGetValue(id, out var list)) { list = new List<SpawnEntry>(); _model.Spawns[id] = list; }
            list.Add(new SpawnEntry { MobTblidx = mobId, Rate = rate, Count = count });
            RefreshSpawnsUI(list);
        }

        private void RemoveSelectedSpawn()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            var id = wrap.Id;
            if (!_model.Spawns.TryGetValue(id, out var list)) return;
            var idx = lstSpawns.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Spawns.Remove(id);
            RefreshSpawnsUI(_model.Spawns.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshSpawnsUI(List<SpawnEntry> list)
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) return;
            lstSpawns.Items.Clear();
            if (list != null)
            {
                foreach (var s in list)
                    lstSpawns.Items.Add(s.Rate >= 100f && s.Count <= 1 ? $"{s.MobTblidx}" : $"{s.MobTblidx}@{s.Rate}x{s.Count}");
                txtSpawns.Text = string.Join(
                    ", ",
                    list.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
            }
            else
            {
                txtSpawns.Text = string.Empty;
            }
        }

        private void ApplyFieldTheme(TextBox tb)
        {
            tb.BorderStyle = BorderStyle.FixedSingle;
            tb.BackColor = Color.FromArgb(40, 41, 45);
            tb.ForeColor = Color.Gainsboro;
        }

        private void ApplyNumericTheme(NumericUpDown nd)
        {
            nd.BorderStyle = BorderStyle.FixedSingle;
            nd.BackColor = Color.FromArgb(40, 41, 45);
            nd.ForeColor = Color.Gainsboro;
        }

        private void ApplyListTheme(ListBox lb)
        {
            lb.BorderStyle = BorderStyle.FixedSingle;
            lb.BackColor = Color.FromArgb(40, 41, 45);
            lb.ForeColor = Color.Gainsboro;
        }

        private void StyleButton(Button btn, bool primary = false)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(70, 72, 78);
            btn.FlatAppearance.BorderSize = 1;
            btn.ForeColor = Color.WhiteSmoke;
            btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
            btn.Cursor = Cursors.Hand;
            btn.MouseEnter += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 105, 190) : Color.FromArgb(75, 77, 83);
            btn.MouseLeave += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
        }

        private void ApplyLabelTheme(Label lbl)
        {
            lbl.ForeColor = Color.Gainsboro;
        }

        private void AddMob()
        {
            if (!uint.TryParse(txtNewMobId.Text.Trim(), out var id)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
            // If mob already present, just select it
            var existingIds = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) existingIds.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) existingIds.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) existingIds.Add(k);
            if (!existingIds.Contains(id))
            {
                // Create a default modifiers entry so it appears in the list
                _model.Mods[id] = new Modifiers();
            }
            RefreshMobList();
            // Select the newly added mob
            for (int i = 0; i < lstMobs.Items.Count; i++)
            {
                if (lstMobs.Items[i] is ListViewItemWrapper w && w.Id == id)
                {
                    lstMobs.SelectedIndex = i;
                    break;
                }
            }
            _lblStatus.Text = $"Mob {id} ready. Add drops/spawns on the right.";
        }

        public MainForm()
        {
            Text = "Custom Drop Event Editor";
            Width = 1100;
            Height = 720;
            AutoScaleMode = AutoScaleMode.Dpi;
            Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point);
            BackColor = Color.FromArgb(32, 33, 36);
            ForeColor = Color.Gainsboro;

            txtPath = new TextBox { Left = 10, Top = 10, Width = 800, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
            btnBrowse = new Button { Left = 820, Top = 8, Width = 80, Height = 28, Text = "Browse" };
            btnLoad = new Button { Left = 910, Top = 8, Width = 80, Height = 28, Text = "Load" };
            btnSave = new Button { Left = 1000, Top = 8, Width = 80, Height = 28, Text = "Save" };

            btnImportMobs = new Button { Left = 820, Top = 40, Width = 130, Height = 28, Text = "Import Mobs..." };
            btnImportItems = new Button { Left = 960, Top = 40, Width = 120, Height = 28, Text = "Import Items..." };

            lstMobs = new ListBox { Left = 10, Top = 70, Width = 300, Height = 600, Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left, IntegralHeight = false };
            lblGlobalSpawns = new Label { Left = 320, Top = 70, AutoSize = true, Text = "Global Spawns (all spawn: mob@ratexcount, ...)" };
            txtGlobalSpawns = new TextBox { Left = 320, Top = 90, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lblDrops = new Label { Left = 320, Top = 130, AutoSize = true, Text = "Drops (item@rate, ...)" };
            // keep the text field for serialization/quick paste, but de-emphasize visually
            txtDrops = new TextBox { Left = 320, Top = 150, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            // Drops list and controls
            lstDrops = new ListBox { Left = 320, Top = 182, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            // Labels for drop inputs
            lblDropItem = new Label { Left = 320, Top = 286, AutoSize = true, Text = "Item ID" };
            txtDropItemId = new TextBox { Left = 320, Top = 306, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblDropRate = new Label { Left = 470, Top = 286, AutoSize = true, Text = "Rate" };
            numDropRate = new NumericUpDown { Left = 470, Top = 306, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddDrop = new Button { Left = 580, Top = 306, Width = 90, Height = 28, Text = "Add" };
            btnRemoveDrop = new Button { Left = 680, Top = 306, Width = 90, Height = 28, Text = "Remove" };

            // Spawns list and controls
            lstSpawns = new ListBox { Left = 320, Top = 398, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            // Labels for spawn inputs
            lblSpawnMob = new Label { Left = 320, Top = 502, AutoSize = true, Text = "Mob ID" };
            txtSpawnMobId = new TextBox { Left = 320, Top = 522, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnRate = new Label { Left = 470, Top = 502, AutoSize = true, Text = "Rate" };
            numSpawnRate = new NumericUpDown { Left = 470, Top = 522, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnCount = new Label { Left = 580, Top = 502, AutoSize = true, Text = "Count" };
            numSpawnCount = new NumericUpDown { Left = 580, Top = 522, Width = 80, Height = 28, DecimalPlaces = 0, Minimum = 1, Maximum = 255, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddSpawn = new Button { Left = 670, Top = 522, Width = 90, Height = 28, Text = "Add" };
            btnRemoveSpawn = new Button { Left = 770, Top = 522, Width = 90, Height = 28, Text = "Remove" };

            lblMods = new Label { Left = 320, Top = 562, AutoSize = true, Text = "Modifiers (hp= engAtk= physAtk= ... sizeRate=)" };
            txtMods = new TextBox { Left = 320, Top = 582, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            // Place Add Mob controls above the mob list on the left
            lblAddMob = new Label { Left = 10, Top = 40, AutoSize = true, Text = "Add Mob ID:" };
            txtNewMobId = new TextBox { Left = 100, Top = 38, Width = 140, Height = 28 };
            btnAddMob = new Button { Left = 250, Top = 38, Width = 60, Height = 28, Text = "Add" };

            Controls.AddRange(new Control[] { txtPath, btnBrowse, btnLoad, btnSave, btnImportMobs, btnImportItems, lstMobs, lblGlobalSpawns, txtGlobalSpawns, lblDrops, txtDrops, lstDrops, txtDropItemId, numDropRate, btnAddDrop, btnRemoveDrop, lblSpawns, txtSpawns, lstSpawns, txtSpawnMobId, numSpawnRate, numSpawnCount, btnAddSpawn, btnRemoveSpawn, lblMods, txtMods, lblAddMob, txtNewMobId, btnAddMob });

            // Wire events
            btnBrowse.Click += (s, e) => BrowsePath();
            btnLoad.Click += (s, e) => LoadCfg();
            btnSave.Click += (s, e) => SaveCfg();
            btnImportMobs.Click += (s, e) => ImportMobs();
            btnImportItems.Click += (s, e) => ImportItems();
            lstMobs.SelectedIndexChanged += (s, e) => LoadMobIntoEditors();
            txtDrops.Leave += (s, e) => SaveMobFromEditors();
            txtSpawns.Leave += (s, e) => SaveMobFromEditors();
            txtMods.Leave += (s, e) => SaveMobFromEditors();
            txtGlobalSpawns.Leave += (s, e) => SaveGlobalFromEditors();

            // New list buttons
            btnAddDrop.Click += (s, e) => AddDropFromInputs();
            btnRemoveDrop.Click += (s, e) => RemoveSelectedDrop();
            btnAddSpawn.Click += (s, e) => AddSpawnFromInputs();
            btnRemoveSpawn.Click += (s, e) => RemoveSelectedSpawn();
            btnAddMob.Click += (s, e) => AddMob();

            // Apply dark theme styles
            ApplyFieldTheme(txtPath);
            ApplyFieldTheme(txtGlobalSpawns);
            ApplyFieldTheme(txtDrops);
            ApplyFieldTheme(txtSpawns);
            ApplyFieldTheme(txtMods);
            ApplyFieldTheme(txtDropItemId);
            ApplyFieldTheme(txtSpawnMobId);
            ApplyFieldTheme(txtNewMobId);
            ApplyNumericTheme(numDropRate);
            ApplyNumericTheme(numSpawnRate);
            ApplyNumericTheme(numSpawnCount);
            ApplyListTheme(lstMobs);
            ApplyListTheme(lstDrops);
            ApplyListTheme(lstSpawns);
            StyleButton(btnBrowse);
            StyleButton(btnLoad);
            StyleButton(btnSave, primary: true);
            StyleButton(btnImportMobs);
            StyleButton(btnImportItems);
            StyleButton(btnAddDrop);
            StyleButton(btnRemoveDrop);
            StyleButton(btnAddSpawn);
            StyleButton(btnRemoveSpawn);
            StyleButton(btnAddMob);

            // Status bar
            var __status = new Panel { Height = 26, Dock = DockStyle.Bottom, BackColor = Color.FromArgb(45, 47, 51), Padding = new Padding(8, 0, 8, 0) };
            _lblStatus = new Label { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, ForeColor = Color.Gainsboro, Text = "Ready" };
            __status.Controls.Add(_lblStatus);
            Controls.Add(__status);

            // Default path remains
            var defaultPath = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "DboServer", "ExecutionEnv", "config", "CustomDropEvent.cfg");
            txtPath.Text = Path.GetFullPath(defaultPath);
        }

        private void ImportMobs()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Mob List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _mobNames = MobsItemsCatalog.ParseMobs(ofd.FileName);
                RefreshMobList();
                MessageBox.Show(this, $"Imported {_mobNames.Count} mobs.");
            }
        }

        private void ImportItems()
        {
            using var ofd = new OpenFileDialog();
            ofd.Title = "Import Item List";
            ofd.Filter = "Text files (*.txt;*.cfg)|*.txt;*.cfg|All files (*.*)|*.*";
            if (ofd.ShowDialog(this) == DialogResult.OK)
            {
                _itemNames = MobsItemsCatalog.ParseItems(ofd.FileName);
                MessageBox.Show(this, $"Imported {_itemNames.Count} items.");
            }
        }

        private void BrowsePath()
        {
            using var ofd = new OpenFileDialog();
            ofd.Filter = "Config files (*.cfg)|*.cfg|All files (*.*)|*.*";
            if (File.Exists(txtPath.Text))
                ofd.FileName = txtPath.Text;
            if (ofd.ShowDialog(this) == DialogResult.OK)
                txtPath.Text = ofd.FileName;
        }

        private void LoadCfg()
        {
            try
            {
                _model = ConfigModel.Load(txtPath.Text);
                RefreshMobList();
                LoadGlobalIntoEditors();
                if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
                var mobCount = lstMobs.Items.Count;
                var dropCount = _model.Drops.Sum(kv => kv.Value.Count);
                var spawnKeys = _model.Spawns.Keys.Count;
                var modKeys = _model.Mods.Keys.Count;
                _lblStatus.Text = $"Loaded: mobs={mobCount} drops={dropCount} spawnKeys={spawnKeys} modKeys={modKeys}";
                MessageBox.Show(this, "Config loaded.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                if (_lblStatus != null) _lblStatus.Text = $"Error: {ex.Message}";
            }
        }

        private void SaveCfg()
        {
            try
            {
                SaveGlobalFromEditors();
                SaveMobFromEditors();
                _model.Save(txtPath.Text);
                MessageBox.Show(this, "Config saved.");
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void RefreshMobList()
        {
            var ids = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) ids.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) ids.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) ids.Add(k);
            var sorted = ids.OrderBy(x => x).ToList();
            lstMobs.Items.Clear();
            foreach (var id in sorted)
            {
                if (_mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name))
                    lstMobs.Items.Add(new ListViewItemWrapper(id, $"{id} - {name}"));
                else
                    lstMobs.Items.Add(new ListViewItemWrapper(id, id.ToString()));
            }
            if (lstMobs.Items.Count > 0) lstMobs.SelectedIndex = 0;
        }

        private void LoadGlobalIntoEditors()
        {
            if (_model.Spawns.TryGetValue(0, out var global))
                txtGlobalSpawns.Text = string.Join(
                    ", ",
                    global.Select(e => e.Rate >= 100f && e.Count <= 1 ? $"{e.MobTblidx}" : $"{e.MobTblidx}@{e.Rate}x{e.Count}"));
            else
                txtGlobalSpawns.Text = string.Empty;
        }

        private void SaveGlobalFromEditors()
        {
            var text = txtGlobalSpawns.Text.Trim();
            if (string.IsNullOrEmpty(text))
            {
                _model.Spawns.Remove(0);
                return;
            }
            var tmpModel = new ConfigModel();
            tmpModel.Spawns[0] = new List<SpawnEntry>();
            foreach (var tok in text.Split(','))
            {
                var t = tok.Trim();
                if (string.IsNullOrEmpty(t)) continue;
                var at = t.IndexOf('@');
                uint mob = 0; float rate = 100f; byte count = 1;
                if (at >= 0)
                {
                    if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                    var rx = t[(at + 1)..].Trim();
                    var x = rx.IndexOf('x');
                    if (x >= 0)
                    {
                        float.TryParse(rx[..x], out rate);
                        byte.TryParse(rx[(x + 1)..], out count);
                    }
                    else
                    {
                        float.TryParse(rx, out rate);
                    }
                }
                else
                {
                    if (!uint.TryParse(t, out mob)) continue;
                }
                tmpModel.Spawns[0].Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
            }
            if (tmpModel.Spawns[0].Count > 0)
                _model.Spawns[0] = tmpModel.Spawns[0];
            else
                _model.Spawns.Remove(0);
        }

        private void LoadMobIntoEditors()
        {
            if (lstMobs.SelectedItem is not ListViewItemWrapper wrap) { txtDrops.Text = txtSpawns.Text = txtMods.Text = string.Empty; lstDrops.Items.Clear(); lstSpawns.Items.Clear(); return; }
            var id = wrap.Id;
            if