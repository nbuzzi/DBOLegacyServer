using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace CustomDropEventEditor
{
    internal sealed class ItemsDialog : Form
    {
        private readonly ListBox _list = new() { Dock = DockStyle.Fill, IntegralHeight = false };
        private readonly TextBox _filter = new() { Dock = DockStyle.Top, Height = 28 };
        private readonly Button _ok = new() { Text = "OK", Dock = DockStyle.Bottom, Height = 32 };
        private readonly Button _cancel = new() { Text = "Cancel", Dock = DockStyle.Bottom, Height = 32 };
        private List<(uint id, string name)> _data = new();
        public uint? SelectedId { get; private set; }
        public ItemsDialog(Dictionary<uint, string> items)
        {
            Text = "Items"; Width = 520; Height = 600; StartPosition = FormStartPosition.CenterParent;
            BackColor = Color.FromArgb(32,33,36); ForeColor = Color.Gainsboro; Font = new Font("Segoe UI", 10);
            var panelButtons = new Panel { Dock = DockStyle.Bottom, Height = 36 };
            panelButtons.Controls.Add(_ok); panelButtons.Controls.Add(_cancel);
            _ok.Width = 100; _cancel.Width = 100; _ok.Left = 520 - 220; _cancel.Left = 520 - 110;
            Controls.Add(_list); Controls.Add(panelButtons); Controls.Add(_filter);
            _data = items.Select(kv => (kv.Key, kv.Value)).OrderBy(t => t.Key).ToList();
            void applyTheme(Control c){ c.BackColor = Color.FromArgb(40,41,45); c.ForeColor = Color.Gainsboro; }
            applyTheme(_list); applyTheme(_filter); applyTheme(_ok); applyTheme(_cancel);
            _filter.PlaceholderText = "Filter by id or name";
            _filter.TextChanged += (s,e)=> Refresh();
            _list.DoubleClick += (s,e)=> { if (_list.SelectedItem is not null) { pick(); } };
            _ok.Click += (s,e)=> { pick(); };
            _cancel.Click += (s,e)=> { DialogResult = DialogResult.Cancel; Close(); };
            Refresh();
            void pick(){
                if (_list.SelectedItem is ItemWrapper w){ SelectedId = w.Id; DialogResult = DialogResult.OK; Close(); }
            }
        }
        private sealed class ItemWrapper { public uint Id; private readonly string _t; public ItemWrapper(uint id,string t){Id=id;_t=t;} public override string ToString()=>_t; }
        private void Refresh()
        {
            var f = _filter.Text?.Trim();
            _list.Items.Clear();
            foreach (var (id,name) in _data)
            {
                var display = string.IsNullOrWhiteSpace(name) ? id.ToString() : $"{id} - {name}";
                if (!string.IsNullOrWhiteSpace(f))
                {
                    var ok = display.IndexOf(f, StringComparison.OrdinalIgnoreCase) >= 0;
                    if (!ok) continue;
                }
                _list.Items.Add(new ItemWrapper(id, display));
            }
        }
    }

    public class MainForm : Form
    {
        private sealed class ListViewItemWrapper
        {
            public uint Id { get; }
            private readonly string _display;
            public ListViewItemWrapper(uint id, string display) { Id = id; _display = display; }
            public override string ToString() => _display;
        }

        private TextBox txtPath;
        private Button btnBrowse, btnLoad, btnSave;
    private ListBox lstCfgMobs, lstAvailMobs;
        private TextBox txtMobFilter;
    private Label lblMobFilter, lblCfgMobs, lblAvailMobs;
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
    private Button btnAddMob, btnUseAvailable;
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
            lblCfgMobs = new Label { Left = 10, Top = 96, AutoSize = true, Text = "Configured Mobs" };
            lstCfgMobs = new ListBox { Left = 10, Top = 116, Width = 300, Height = 270, Anchor = AnchorStyles.Top | AnchorStyles.Left, IntegralHeight = false };
            lblAvailMobs = new Label { Left = 10, Top = 392, AutoSize = true, Text = "Available Mobs" };
            lstAvailMobs = new ListBox { Left = 10, Top = 412, Width = 300, Height = 258, Anchor = AnchorStyles.Top | AnchorStyles.Left, IntegralHeight = false };
            lblGlobalSpawns = new Label { Left = 320, Top = 70, AutoSize = true, Text = "Global Spawns (all spawn: mob@ratexcount, ...)" };
            txtGlobalSpawns = new TextBox { Left = 320, Top = 90, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lblDrops = new Label { Left = 320, Top = 130, AutoSize = true, Text = "Drops (item@rate, ...)" };
            txtDrops = new TextBox { Left = 320, Top = 150, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lblSpawns = new Label { Left = 320, Top = 350, AutoSize = true, Text = "Spawns (mob@ratexcount, ...)" };
            txtSpawns = new TextBox { Left = 320, Top = 370, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lstDrops = new ListBox { Left = 320, Top = 182, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            lblDropItem = new Label { Left = 320, Top = 286, AutoSize = true, Text = "Item ID" };
            txtDropItemId = new TextBox { Left = 320, Top = 306, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblDropRate = new Label { Left = 470, Top = 286, AutoSize = true, Text = "Rate" };
            numDropRate = new NumericUpDown { Left = 470, Top = 306, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddDrop = new Button { Left = 580, Top = 306, Width = 90, Height = 28, Text = "Add" };
            btnRemoveDrop = new Button { Left = 680, Top = 306, Width = 90, Height = 28, Text = "Remove" };

            lstSpawns = new ListBox { Left = 320, Top = 418, Width = 760, Height = 120, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right, IntegralHeight = false };
            lblSpawnMob = new Label { Left = 320, Top = 548, AutoSize = true, Text = "Mob ID" };
            txtSpawnMobId = new TextBox { Left = 320, Top = 568, Width = 140, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnRate = new Label { Left = 470, Top = 548, AutoSize = true, Text = "Rate" };
            numSpawnRate = new NumericUpDown { Left = 470, Top = 568, Width = 100, Height = 28, DecimalPlaces = 2, Minimum = 0, Maximum = 100000, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            lblSpawnCount = new Label { Left = 580, Top = 548, AutoSize = true, Text = "Count" };
            numSpawnCount = new NumericUpDown { Left = 580, Top = 568, Width = 80, Height = 28, DecimalPlaces = 0, Minimum = 1, Maximum = 255, Increment = 1, Anchor = AnchorStyles.Top | AnchorStyles.Left };
            btnAddSpawn = new Button { Left = 670, Top = 568, Width = 90, Height = 28, Text = "Add" };
            btnRemoveSpawn = new Button { Left = 770, Top = 568, Width = 90, Height = 28, Text = "Remove" };

            lblMods = new Label { Left = 320, Top = 608, AutoSize = true, Text = "Modifiers (hp= engAtk= physAtk= ... sizeRate=)" };
            txtMods = new TextBox { Left = 320, Top = 628, Width = 760, Height = 28, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };

            lblAddMob = new Label { Left = 10, Top = 40, AutoSize = true, Text = "Add Mob ID:" };
            txtNewMobId = new TextBox { Left = 100, Top = 38, Width = 140, Height = 28 };
            btnAddMob = new Button { Left = 250, Top = 38, Width = 60, Height = 28, Text = "Add" };
            btnUseAvailable = new Button { Left = 240, Top = 410, Width = 70, Height = 24, Text = "Use ▶" };

            Controls.AddRange(new Control[] { txtPath, btnBrowse, btnLoad, btnSave, btnImportMobs, btnImportItems, lblMobFilter, txtMobFilter, btnClearFilter, lblCfgMobs, lstCfgMobs, lblAvailMobs, lstAvailMobs, btnUseAvailable, lblGlobalSpawns, txtGlobalSpawns, lblDrops, txtDrops, lstDrops, txtDropItemId, numDropRate, btnAddDrop, btnRemoveDrop, lblSpawns, txtSpawns, lstSpawns, txtSpawnMobId, numSpawnRate, numSpawnCount, btnAddSpawn, btnRemoveSpawn, lblMods, txtMods, lblAddMob, txtNewMobId, btnAddMob });

            btnBrowse.Click += (s, e) => BrowsePath();
            btnLoad.Click += (s, e) => LoadCfg();
            btnSave.Click += (s, e) => SaveCfg();
            btnImportMobs.Click += (s, e) => ImportMobs();
            btnImportItems.Click += (s, e) => ImportItems();
            lstCfgMobs.SelectedIndexChanged += (s, e) => { if (lstCfgMobs.SelectedIndex >= 0) { lstAvailMobs.ClearSelected(); LoadMobIntoEditors(); } };
            lstAvailMobs.SelectedIndexChanged += (s, e) => { if (lstAvailMobs.SelectedIndex >= 0) { lstCfgMobs.ClearSelected(); LoadMobIntoEditors(); } };
            txtDrops.Leave += (s, e) => SaveMobFromEditors();
            txtSpawns.Leave += (s, e) => SaveMobFromEditors();
            txtMods.Leave += (s, e) => SaveMobFromEditors();
            txtGlobalSpawns.Leave += (s, e) => SaveGlobalFromEditors();

            btnAddDrop.Click += (s, e) => AddDropFromInputs();
            btnRemoveDrop.Click += (s, e) => RemoveSelectedDrop();
            btnAddSpawn.Click += (s, e) => AddSpawnFromInputs();
            btnRemoveSpawn.Click += (s, e) => RemoveSelectedSpawn();
            btnAddMob.Click += (s, e) => AddMob();
            btnUseAvailable.Click += (s, e) => UseAvailableSelected();
            txtMobFilter.TextChanged += (s, e) => RefreshMobList(txtMobFilter.Text);
            btnClearFilter.Click += (s, e) => { txtMobFilter.Text = string.Empty; RefreshMobList(); };
            lstDrops.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedDrop(); e.Handled = true; } };
            lstSpawns.KeyDown += (s, e) => { if (e.KeyCode == Keys.Delete) { RemoveSelectedSpawn(); e.Handled = true; } };
            txtDropItemId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
            txtSpawnMobId.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
            numDropRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddDropFromInputs(); e.Handled = true; } };
            numSpawnRate.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };
            numSpawnCount.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) { AddSpawnFromInputs(); e.Handled = true; } };

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
            ApplyLabelTheme(lblSpawns);
            ApplyNumericTheme(numDropRate);
            ApplyNumericTheme(numSpawnRate);
            ApplyNumericTheme(numSpawnCount);
            ApplyListTheme(lstCfgMobs);
            ApplyListTheme(lstAvailMobs);
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
            StyleButton(btnUseAvailable);

            KeyPreview = true;
            KeyDown += (s, e) =>
            {
                if (e.Control && e.KeyCode == Keys.S) { btnSave.PerformClick(); e.Handled = true; }
                else if (e.Control && e.KeyCode == Keys.O) { btnLoad.PerformClick(); e.Handled = true; }
            };

            var __status = new Panel { Height = 26, Dock = DockStyle.Bottom, BackColor = Color.FromArgb(45, 47, 51), Padding = new Padding(8, 0, 8, 0) };
            _lblStatus = new Label { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft, ForeColor = Color.Gainsboro, Text = "Ready" };
            __status.Controls.Add(_lblStatus);
            Controls.Add(__status);

            var defaultPath = Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "DboServer", "ExecutionEnv", "config", "CustomDropEvent.cfg");
            txtPath.Text = Path.GetFullPath(defaultPath);
        }

        private void UseAvailableSelected()
        {
            if (lstAvailMobs.SelectedItem is not ListViewItemWrapper w) return;
            var id = w.Id;
            if (!_model.Mods.ContainsKey(id)) _model.Mods[id] = new Modifiers();
            RefreshMobList(txtMobFilter.Text);
            // select in configured list
            for (int i = 0; i < lstCfgMobs.Items.Count; i++)
            {
                if (lstCfgMobs.Items[i] is ListViewItemWrapper ww && ww.Id == id) { lstCfgMobs.SelectedIndex = i; break; }
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
                using var dlg = new ItemsDialog(_itemNames);
                if (dlg.ShowDialog(this) == DialogResult.OK && dlg.SelectedId.HasValue)
                {
                    txtDropItemId.Text = dlg.SelectedId.Value.ToString();
                }
                else
                {
                    MessageBox.Show(this, $"Imported {_itemNames.Count} items.");
                }
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
                if (lstCfgMobs.Items.Count > 0) lstCfgMobs.SelectedIndex = 0; else if (lstAvailMobs.Items.Count > 0) lstAvailMobs.SelectedIndex = 0;
                var mobCount = lstCfgMobs.Items.Count;
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
            var configured = new HashSet<uint>();
            foreach (var k in _model.Drops.Keys) configured.Add(k);
            foreach (var k in _model.Spawns.Keys) if (k != 0) configured.Add(k);
            foreach (var k in _model.Mods.Keys) if (k != 0) configured.Add(k);

            var allKnown = new HashSet<uint>(configured);
            foreach (var k in _mobNames.Keys) allKnown.Add(k);

            var filterText = filter?.Trim();
            lstCfgMobs.Items.Clear();
            lstAvailMobs.Items.Clear();

            foreach (var id in configured.OrderBy(x => x))
            {
                var display = _mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name) ? $"{id} - {name}" : id.ToString();
                if (!string.IsNullOrWhiteSpace(filterText) && display.IndexOf(filterText, StringComparison.OrdinalIgnoreCase) < 0) continue;
                lstCfgMobs.Items.Add(new ListViewItemWrapper(id, display));
            }
            foreach (var id in allKnown.Except(configured).OrderBy(x => x))
            {
                var display = _mobNames.TryGetValue(id, out var name) && !string.IsNullOrWhiteSpace(name) ? $"{id} - {name}" : id.ToString();
                if (!string.IsNullOrWhiteSpace(filterText) && display.IndexOf(filterText, StringComparison.OrdinalIgnoreCase) < 0) continue;
                lstAvailMobs.Items.Add(new ListViewItemWrapper(id, display));
            }
            if (lstCfgMobs.Items.Count > 0)
                lstCfgMobs.SelectedIndex = 0;
            else if (lstAvailMobs.Items.Count > 0)
                lstAvailMobs.SelectedIndex = 0;
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
            uint? selectedId = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper w1) selectedId = w1.Id;
            else if (lstAvailMobs.SelectedItem is ListViewItemWrapper w2) selectedId = w2.Id;
            if (selectedId is null) { txtDrops.Text = txtSpawns.Text = txtMods.Text = string.Empty; lstDrops.Items.Clear(); lstSpawns.Items.Clear(); return; }
            var id = selectedId.Value;

            // Prefill spawn input fields with defaults for quick add
            txtSpawnMobId.Text = id.ToString();
            try { numSpawnRate.Value = 100; } catch { /* ignore if out of range */ }
            try { numSpawnCount.Value = 1; } catch { /* ignore */ }
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
            else
                txtMods.Text = new Modifiers().ToString(); // show defaults for convenience
        }

        private void SaveMobFromEditors()
        {
            uint? selectedId2 = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper w3) selectedId2 = w3.Id;
            else if (lstAvailMobs.SelectedItem is ListViewItemWrapper w4) selectedId2 = w4.Id;
            if (selectedId2 is null) return;
            var id = selectedId2.Value;

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
            uint? selectedId = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper wc) selectedId = wc.Id; else if (lstAvailMobs.SelectedItem is ListViewItemWrapper wa) selectedId = wa.Id;
            if (selectedId is null) return;
            if (!uint.TryParse(txtDropItemId.Text.Trim(), out var itemId)) { MessageBox.Show(this, "Invalid Item ID"); return; }
            var rate = (float)numDropRate.Value;
            var id = selectedId.Value;
            if (!_model.Drops.TryGetValue(id, out var list)) { list = new List<DropEntry>(); _model.Drops[id] = list; }
            list.Add(new DropEntry { ItemTblidx = itemId, Rate = rate });
            RefreshDropsUI(list);
        }

        private void RemoveSelectedDrop()
        {
            uint? selectedId3 = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper w5) selectedId3 = w5.Id;
            else if (lstAvailMobs.SelectedItem is ListViewItemWrapper w6) selectedId3 = w6.Id;
            if (selectedId3 is null) return;
            var id = selectedId3.Value;
            if (!_model.Drops.TryGetValue(id, out var list)) return;
            var idx = lstDrops.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Drops.Remove(id);
            RefreshDropsUI(_model.Drops.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshDropsUI(List<DropEntry> list)
        {
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
            uint? selectedId = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper wc) selectedId = wc.Id; else if (lstAvailMobs.SelectedItem is ListViewItemWrapper wa) selectedId = wa.Id;
            if (selectedId is null) return;
            if (!uint.TryParse(txtSpawnMobId.Text.Trim(), out var mobId)) { MessageBox.Show(this, "Invalid Mob ID"); return; }
            var rate = (float)numSpawnRate.Value;
            var count = (byte)numSpawnCount.Value;
            var id = selectedId.Value;
            if (!_model.Spawns.TryGetValue(id, out var list)) { list = new List<SpawnEntry>(); _model.Spawns[id] = list; }
            list.Add(new SpawnEntry { MobTblidx = mobId, Rate = rate, Count = count });
            RefreshSpawnsUI(list);
        }

        private void RemoveSelectedSpawn()
        {
            uint? selectedId = null;
            if (lstCfgMobs.SelectedItem is ListViewItemWrapper wc) selectedId = wc.Id; else if (lstAvailMobs.SelectedItem is ListViewItemWrapper wa) selectedId = wa.Id;
            if (selectedId is null) return;
            var id = selectedId.Value;
            if (!_model.Spawns.TryGetValue(id, out var list)) return;
            var idx = lstSpawns.SelectedIndex;
            if (idx < 0 || idx >= list.Count) return;
            list.RemoveAt(idx);
            if (list.Count == 0) _model.Spawns.Remove(id);
            RefreshSpawnsUI(_model.Spawns.TryGetValue(id, out var l2) ? l2 : null);
        }

        private void RefreshSpawnsUI(List<SpawnEntry> list)
        {
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
            for (int i = 0; i < lstCfgMobs.Items.Count; i++)
            {
                if (lstCfgMobs.Items[i] is ListViewItemWrapper w && w.Id == id)
                {
                    lstCfgMobs.SelectedIndex = i;
                    break;
                }
            }
            _lblStatus.Text = $"Mob {id} ready. Add drops/spawns on the right.";
        }
    }
}