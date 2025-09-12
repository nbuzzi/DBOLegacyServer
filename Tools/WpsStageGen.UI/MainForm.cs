using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Windows.Forms;
using System.Drawing;

namespace WpsStageGen.UI
{
    public class MainForm : Form
    {
    TextBox txtWpsPath = new TextBox { Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
    TextBox txtGenPath = new TextBox { Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
        NumericUpDown numAdd = new NumericUpDown { Minimum = 1, Maximum = 1000, Value = 5 };
        NumericUpDown numBossEvery = new NumericUpDown { Minimum = 1, Maximum = 1000, Value = 5 };
        NumericUpDown numDropItem = new NumericUpDown { Minimum = 1, Maximum = int.MaxValue, Value = 7000014 };
        NumericUpDown numDropBase = new NumericUpDown { Minimum = 0, Maximum = 1000, Value = 2 };
        NumericUpDown numDropStep = new NumericUpDown { Minimum = 0, Maximum = 1000, Value = 1 };
        NumericUpDown numMobBase = new NumericUpDown { Minimum = 1, Maximum = 999999, Value = 9101 };
        NumericUpDown numBossGroup = new NumericUpDown { Minimum = 1, Maximum = 999999, Value = 9999 };
    TextBox txtArgs = new TextBox { Multiline = true, ScrollBars = ScrollBars.Vertical, Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right, ReadOnly = true };
    TextBox txtOutput = new TextBox { Multiline = true, ScrollBars = ScrollBars.Both, Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right, ReadOnly = true, WordWrap = false };
    Button btnBrowseWps = new Button { Text = "Browse" };
    Button btnBrowseGen = new Button { Text = "Browse" };
    Button btnRun = new Button { Text = "Run" };
    Button btnCopyArgs = new Button { Text = "Copy" };
    Button btnReset = new Button { Text = "Reset" };
    ComboBox cboBossEvery = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
    ComboBox cboMobBase = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
    ToolTip tt = new ToolTip();
    Panel header = new Panel { Height = 64, Dock = DockStyle.Top };
    Label lblTitle = new Label();
    Label lblSub = new Label();
    Panel status = new Panel { Height = 28, Dock = DockStyle.Bottom };
    Label lblStatus = new Label { AutoSize = false, Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
    ProgressBar prg = new ProgressBar { Dock = DockStyle.Right, Style = ProgressBarStyle.Marquee, MarqueeAnimationSpeed = 0, Width = 120, Visible = false };

        public MainForm()
        {
            Text = "WpsStageGen UI";
            Width = 1280; Height = 860; StartPosition = FormStartPosition.CenterScreen;
            WindowState = FormWindowState.Normal;
            MinimumSize = new Size(1024, 720);
            Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point);

            // Theme
            BackColor = Color.FromArgb(32, 33, 36);
            ForeColor = Color.Gainsboro;
            ApplyFieldTheme(txtWpsPath);
            ApplyFieldTheme(txtGenPath);
            ApplyFieldTheme(txtArgs, monospace: true);
            ApplyFieldTheme(txtOutput, monospace: true);
            ApplyNumericTheme(numAdd, numBossEvery, numDropItem, numDropBase, numDropStep, numMobBase, numBossGroup);
            txtWpsPath.MinimumSize = new Size(700, 0);
            txtGenPath.MinimumSize = new Size(700, 0);
            numAdd.Width = 240;
            numBossEvery.Width = 240;
            numDropItem.Width = 300;
            numDropBase.Width = 240;
            numDropStep.Width = 240;
            numMobBase.Width = 260;
            numBossGroup.Width = 260;
            StyleButton(btnRun, primary: true);
            StyleButton(btnBrowseWps);
            StyleButton(btnBrowseGen);
            StyleButton(btnCopyArgs);
            StyleButton(btnReset);
            btnBrowseWps.AutoSize = true; btnBrowseGen.AutoSize = true; btnCopyArgs.AutoSize = true; btnRun.AutoSize = true;

            // Header
            header.BackColor = Color.FromArgb(45, 47, 51);
            header.Padding = new Padding(24, 12, 24, 12);
            var ver = typeof(MainForm).Assembly.GetName().Version?.ToString() ?? "1.0.0";
            lblTitle.Text = $"WpsStageGen v{ver}";
            lblTitle.Font = new Font("Segoe UI Semibold", 16F, FontStyle.Bold, GraphicsUnit.Point);
            lblTitle.ForeColor = Color.WhiteSmoke;
            lblTitle.AutoSize = true;
            lblSub.Text = "CCBD floors generator • Boss + patterns UI";
            lblSub.Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);
            lblSub.ForeColor = Color.Silver;
            lblSub.AutoSize = true;
            lblSub.Top = 34; lblSub.Left = 18;
            header.Controls.Add(lblTitle);
            header.Controls.Add(lblSub);
            Controls.Add(header);

            // Status bar
            status.BackColor = Color.FromArgb(45, 47, 51);
            status.Padding = new Padding(8, 0, 8, 0);
            lblStatus.Text = $"Ready • v{ver}";
            status.Controls.Add(prg);
            status.Controls.Add(lblStatus);
            Controls.Add(status);

            // Root grid inside a scrollable container
            var root = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 1, RowCount = 6, Padding = new Padding(24), AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Generator
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Parameters
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Drops
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Mobs
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Args
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // Output

            // Generator card
            var genTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 3, RowCount = 2, Padding = new Padding(8), AutoSize = false };
            genTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            genTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            genTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            genTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            genTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            int r0 = 0;
            genTable.Controls.Add(StyledLabel("WPS file:"), 0, r0);
            genTable.Controls.Add(txtWpsPath, 1, r0);
            btnBrowseWps.Anchor = AnchorStyles.Right;
            genTable.Controls.Add(btnBrowseWps, 2, r0);
            r0++;
            genTable.Controls.Add(StyledLabel("Generator dir:"), 0, r0);
            genTable.Controls.Add(txtGenPath, 1, r0);
            btnBrowseGen.Anchor = AnchorStyles.Right;
            genTable.Controls.Add(btnBrowseGen, 2, r0);
            var genCard = CreateCard("Generator", genTable, "\uE8B7");
            root.Controls.Add(genCard, 0, 0);

            // Parameters card
            var paramTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 6, RowCount = 1, Padding = new Padding(8), AutoSize = false };
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 35));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 25));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 40));
            paramTable.Controls.Add(StyledLabel("Add floors:"), 0, 0);
            paramTable.Controls.Add(numAdd, 1, 0);
            paramTable.Controls.Add(StyledLabel("Boss every:"), 2, 0);
            paramTable.Controls.Add(numBossEvery, 3, 0);
            paramTable.Controls.Add(StyledLabel("Presets:"), 4, 0);
            paramTable.Controls.Add(cboBossEvery, 5, 0);
            var paramCard = CreateCard("Parameters", paramTable, "\uE713");
            root.Controls.Add(paramCard, 0, 1);

            // Drops and Mobs cards side by side
            var dropsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 3, Padding = new Padding(8), AutoSize = false };
            dropsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            dropsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            dropsTable.Controls.Add(StyledLabel("Drop item:"), 0, 0);
            dropsTable.Controls.Add(numDropItem, 1, 0);
            dropsTable.Controls.Add(StyledLabel("Drop base:"), 0, 1);
            dropsTable.Controls.Add(numDropBase, 1, 1);
            dropsTable.Controls.Add(StyledLabel("Drop step:"), 0, 2);
            dropsTable.Controls.Add(numDropStep, 1, 2);
            var dropsCard = CreateCard("Drops", dropsTable, "\uE896");
            root.Controls.Add(dropsCard, 0, 2);

            var mobsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 3, RowCount = 2, Padding = new Padding(8), AutoSize = false };
            mobsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            mobsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            mobsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            mobsTable.Controls.Add(StyledLabel("Mob base:"), 0, 0);
            mobsTable.Controls.Add(numMobBase, 1, 0);
            mobsTable.Controls.Add(cboMobBase, 2, 0);
            mobsTable.Controls.Add(StyledLabel("Boss group:"), 0, 1);
            mobsTable.Controls.Add(numBossGroup, 1, 1);
            var mobsCard = CreateCard("Mobs", mobsTable, "\uE7F8");
            root.Controls.Add(mobsCard, 0, 3);

            // Args card
            var argsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 5, RowCount = 1, Padding = new Padding(8), AutoSize = false };
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.Controls.Add(StyledLabel("Args preview:"), 0, 0);
            argsTable.Controls.Add(txtArgs, 1, 0);
            btnCopyArgs.AutoSize = true;
            btnCopyArgs.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            btnRun.AutoSize = true;
            btnRun.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            argsTable.Controls.Add(btnCopyArgs, 2, 0);
            argsTable.Controls.Add(btnReset, 3, 0);
            argsTable.Controls.Add(btnRun, 4, 0);
            var argsCard = CreateCard("Action", argsTable, "\uE768");
            root.Controls.Add(argsCard, 0, 4);

            // Output card (expands)
            var outputHolder = new Panel { Dock = DockStyle.Fill, MinimumSize = new Size(0, 220) };
            txtOutput.Dock = DockStyle.Fill;
            outputHolder.Controls.Add(txtOutput);
            var outputCard = CreateCard("Output", outputHolder, "\uE7C3");
            root.Controls.Add(outputCard, 0, 5);

            var scroll = new Panel { Dock = DockStyle.Fill, AutoScroll = true, AutoScrollMargin = new Size(0, 16) };
            scroll.Controls.Add(root);
            Controls.Add(scroll);

            btnBrowseWps.Click += (s, e) => BrowseFile(txtWpsPath, "WPS (*.wps)|*.wps|All files (*.*)|*.*");
            btnBrowseGen.Click += (s, e) => BrowseFolder(txtGenPath);
            btnRun.Click += async (s, e) => await RunGeneratorAsync();
            btnCopyArgs.Click += (s, e) => { try { Clipboard.SetText(txtArgs.Text); lblStatus.Text = "Args copied"; } catch { } };
            btnReset.Click += (s, e) => ResetDefaults();

            // Preset options
            cboBossEvery.Items.AddRange(new object[] { 3, 5, 10, 15 });
            cboBossEvery.SelectedIndexChanged += (s, e) => { if (cboBossEvery.SelectedItem != null) numBossEvery.Value = Convert.ToDecimal(cboBossEvery.SelectedItem); };
            cboMobBase.Items.AddRange(new object[] { 9001, 9101, 9201, 9301 });
            cboMobBase.SelectedIndexChanged += (s, e) => { if (cboMobBase.SelectedItem != null) numMobBase.Value = Convert.ToDecimal(cboMobBase.SelectedItem); };

            // Tooltips
            tt.SetToolTip(btnBrowseWps, "Browse for a .wps file");
            tt.SetToolTip(btnBrowseGen, "Browse to the WpsStageGen folder");
            tt.SetToolTip(btnRun, "Run generator with the selected parameters");
            tt.SetToolTip(btnCopyArgs, "Copy args to clipboard");

            foreach (var c in new Control[] { txtWpsPath, txtGenPath, numAdd, numBossEvery, numDropItem, numDropBase, numDropStep, numMobBase, numBossGroup })
            {
                c.TextChanged += (s, e) => UpdateArgs();
            }
            numAdd.ValueChanged += (s, e) => UpdateArgs();
            numBossEvery.ValueChanged += (s, e) => UpdateArgs();
            numDropItem.ValueChanged += (s, e) => UpdateArgs();
            numDropBase.ValueChanged += (s, e) => UpdateArgs();
            numDropStep.ValueChanged += (s, e) => UpdateArgs();
            numMobBase.ValueChanged += (s, e) => UpdateArgs();
            numBossGroup.ValueChanged += (s, e) => UpdateArgs();

            LoadSettings();
            UpdateArgs();
            AcceptButton = btnRun;
        }

        void BrowseFile(TextBox target, string filter)
        {
            using var ofd = new OpenFileDialog { Filter = filter, CheckFileExists = true };
            if (ofd.ShowDialog(this) == DialogResult.OK)
                target.Text = ofd.FileName;
        }

        void BrowseFolder(TextBox target)
        {
            using var fbd = new FolderBrowserDialog();
            if (fbd.ShowDialog(this) == DialogResult.OK)
                target.Text = fbd.SelectedPath;
        }

        async System.Threading.Tasks.Task RunGeneratorAsync()
        {
            txtOutput.Clear();
            if (!File.Exists(txtWpsPath.Text))
            {
                MessageBox.Show(this, "Select a valid WPS file.");
                return;
            }
            if (!Directory.Exists(txtGenPath.Text))
            {
                MessageBox.Show(this, "Select the generator directory (contains .csproj).\nExample: D\\projects\\dbo-legacy\\OpenDBO-Core\\Tools\\WpsStageGen");
                return;
            }

            var psi = new ProcessStartInfo
            {
                FileName = "dotnet",
                WorkingDirectory = txtGenPath.Text,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                Arguments = $"run -- in=\"{txtWpsPath.Text}\" add={numAdd.Value} bossEvery={numBossEvery.Value} dropItem={numDropItem.Value} dropBase={numDropBase.Value} dropStep={numDropStep.Value} mobBase={numMobBase.Value} bossGroup={numBossGroup.Value}"
            };

            var sb = new StringBuilder();
            using var proc = new Process { StartInfo = psi, EnableRaisingEvents = true };
            btnRun.Enabled = false;
            prg.Visible = true;
            prg.MarqueeAnimationSpeed = 30;
            lblStatus.Text = "Running generator...";
            proc.OutputDataReceived += (s, e) => { if (e.Data != null) AppendLine(e.Data); };
            proc.ErrorDataReceived += (s, e) => { if (e.Data != null) AppendLine(e.Data); };

            proc.Start();
            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();
            await System.Threading.Tasks.Task.Run(() => proc.WaitForExit());

            btnRun.Enabled = true;
            prg.MarqueeAnimationSpeed = 0;
            prg.Visible = false;
            lblStatus.Text = proc.ExitCode == 0 ? "Completed" : $"Failed (exit {proc.ExitCode})";
            SaveSettings();

            void AppendLine(string line)
            {
                if (txtOutput.InvokeRequired)
                {
                    txtOutput.Invoke(new Action<string>(AppendLine), line);
                }
                else
                {
                    txtOutput.AppendText(line + Environment.NewLine);
                }
            }
        }

        string SettingsPath => Path.Combine(AppContext.BaseDirectory, "WpsStageGen.UI.settings.json");

        void SaveSettings()
        {
            try
            {
                var dto = new
                {
                    wps = txtWpsPath.Text,
                    gen = txtGenPath.Text,
                    add = (int)numAdd.Value,
                    bossEvery = (int)numBossEvery.Value,
                    dropItem = (int)numDropItem.Value,
                    dropBase = (int)numDropBase.Value,
                    dropStep = (int)numDropStep.Value,
                    mobBase = (int)numMobBase.Value,
                    bossGroup = (int)numBossGroup.Value
                };
                File.WriteAllText(SettingsPath, JsonSerializer.Serialize(dto, new JsonSerializerOptions { WriteIndented = true }));
            }
            catch { }
        }

        void LoadSettings()
        {
            try
            {
                if (!File.Exists(SettingsPath)) return;
                var json = File.ReadAllText(SettingsPath);
                using var doc = JsonDocument.Parse(json);
                var r = doc.RootElement;
                if (r.TryGetProperty("wps", out var w)) txtWpsPath.Text = w.GetString();
                if (r.TryGetProperty("gen", out var g)) txtGenPath.Text = g.GetString();
                if (r.TryGetProperty("add", out var a)) numAdd.Value = a.GetInt32();
                if (r.TryGetProperty("bossEvery", out var be)) numBossEvery.Value = be.GetInt32();
                if (r.TryGetProperty("dropItem", out var di)) numDropItem.Value = di.GetInt32();
                if (r.TryGetProperty("dropBase", out var db)) numDropBase.Value = db.GetInt32();
                if (r.TryGetProperty("dropStep", out var ds)) numDropStep.Value = ds.GetInt32();
                if (r.TryGetProperty("mobBase", out var mb)) numMobBase.Value = mb.GetInt32();
                if (r.TryGetProperty("bossGroup", out var bg)) numBossGroup.Value = bg.GetInt32();
            }
            catch { }
        }

        void ResetDefaults()
        {
            txtWpsPath.Clear();
            txtGenPath.Clear();
            numAdd.Value = 5;
            numBossEvery.Value = 5;
            numDropItem.Value = 7000014;
            numDropBase.Value = 2;
            numDropStep.Value = 1;
            numMobBase.Value = 9101;
            numBossGroup.Value = 9999;
            cboBossEvery.SelectedIndex = -1;
            cboMobBase.SelectedIndex = -1;
            lblStatus.Text = "Defaults restored";
            UpdateArgs();
        }

        Label StyledLabel(string text)
        {
            return new Label
            {
                Text = text,
                AutoSize = true,
                Margin = new Padding(0, 12, 12, 6),
                ForeColor = Color.Gainsboro,
                Font = new Font("Segoe UI", 10f, FontStyle.Regular, GraphicsUnit.Point)
            };
        }

        void ApplyFieldTheme(TextBox tb, bool monospace = false)
        {
            tb.BorderStyle = BorderStyle.FixedSingle;
            tb.BackColor = Color.FromArgb(40, 41, 45);
            tb.ForeColor = Color.Gainsboro;
            tb.Margin = new Padding(4);
            tb.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            if (monospace)
            {
                tb.Font = new Font("Consolas", 9F, FontStyle.Regular, GraphicsUnit.Point);
            }
        }

        void ApplyNumericTheme(params NumericUpDown[] nums)
        {
            foreach (var n in nums)
            {
                n.BackColor = Color.FromArgb(40, 41, 45);
                n.ForeColor = Color.Gainsboro;
                n.BorderStyle = BorderStyle.FixedSingle;
                n.Margin = new Padding(4);
                n.Width = 220;
                n.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            }
        }

        void StyleButton(Button btn, bool primary = false)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.FlatAppearance.BorderColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(70, 72, 78);
            btn.FlatAppearance.BorderSize = primary ? 1 : 1;
            btn.ForeColor = Color.WhiteSmoke;
            btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
            btn.Margin = new Padding(6);
            btn.Padding = new Padding(10, 6, 10, 6);
            btn.Cursor = Cursors.Hand;
            btn.MouseEnter += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 105, 190) : Color.FromArgb(75, 77, 83);
            btn.MouseLeave += (s, e) => btn.BackColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(60, 62, 66);
            btn.MinimumSize = new Size(primary ? 100 : 32, 28);
        }

        Control CreateCard(string title, Control inner, string? iconGlyph = null)
        {
            var panel = new Panel { Padding = new Padding(12), BackColor = Color.FromArgb(38, 39, 43), Margin = new Padding(0, 8, 8, 8), Dock = DockStyle.Top };
            var titleBar = new Panel { Height = 28, Dock = DockStyle.Top };
            var icon = new Label
            {
                Text = iconGlyph ?? string.Empty,
                Font = new Font("Segoe MDL2 Assets", 12f, FontStyle.Regular, GraphicsUnit.Point),
                ForeColor = Color.Silver,
                AutoSize = true,
                Dock = DockStyle.Left,
                Padding = new Padding(2, 4, 8, 0),
                Visible = !string.IsNullOrEmpty(iconGlyph)
            };
            var titleLbl = new Label
            {
                Text = title,
                Font = new Font("Segoe UI Semibold", 11f, FontStyle.Bold, GraphicsUnit.Point),
                ForeColor = Color.WhiteSmoke,
                AutoSize = true,
                Dock = DockStyle.Left,
                Padding = new Padding(2, 4, 0, 0)
            };
            titleBar.Controls.Add(titleLbl);
            titleBar.Controls.Add(icon);
            var divider = new Panel { Height = 1, Dock = DockStyle.Top, BackColor = Color.FromArgb(64, 66, 71), Margin = new Padding(0, 0, 0, 8) };
            inner.Dock = DockStyle.Fill;
            panel.Controls.Add(inner);
            panel.Controls.Add(divider);
            panel.Controls.Add(titleBar);
            return panel;
        }

        void UpdateArgs()
        {
            var inArg = txtWpsPath.Text.Length > 0 ? $"in=\"{txtWpsPath.Text}\"" : string.Empty;
            var args = new[]
            {
                inArg,
                $"add={(int)numAdd.Value}",
                $"bossEvery={(int)numBossEvery.Value}",
                $"dropItem={(int)numDropItem.Value}",
                $"dropBase={(int)numDropBase.Value}",
                $"dropStep={(int)numDropStep.Value}",
                $"mobBase={(int)numMobBase.Value}",
                $"bossGroup={(int)numBossGroup.Value}"
            };
            txtArgs.Text = string.Join(" ", args);
        }
    }
}
