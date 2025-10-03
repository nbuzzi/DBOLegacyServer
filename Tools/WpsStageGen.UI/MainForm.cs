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
        TextBox txtWpsPath = new TextBox();
        TextBox txtGenPath = new TextBox();
        NumericUpDown numAdd = new NumericUpDown { Minimum = 1, Maximum = 1000, Value = 50 };
        NumericUpDown numStartStage = new NumericUpDown { Minimum = 0, Maximum = 255, Value = 0 };
        NumericUpDown numBossEvery = new NumericUpDown { Minimum = 1, Maximum = 1000, Value = 5 };
        NumericUpDown numRewardItem = new NumericUpDown { Minimum = 1, Maximum = int.MaxValue, Value = 7000002 };
        NumericUpDown numBossGroup = new NumericUpDown { Minimum = 1, Maximum = 999999, Value = 9999 };
        TextBox txtPattern = new TextBox { Text = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)" };
        TextBox txtBossWorlds = new TextBox();
        TextBox txtArgs = new TextBox { Multiline = true, ScrollBars = ScrollBars.Vertical, ReadOnly = true };
        TextBox txtOutput = new TextBox { Multiline = true, ScrollBars = ScrollBars.Both, ReadOnly = true, WordWrap = false };
        Button btnBrowseWps = new Button { Text = "Browse" };
        Button btnBrowseGen = new Button { Text = "Browse" };
        TextBox txtOutPath = new TextBox();
        Button btnBrowseOut = new Button { Text = "Browse" };
        TextBox txtBossTemplate = new TextBox();
        Button btnBrowseBossTpl = new Button { Text = "Browse" };
        TextBox txtRegularTemplate = new TextBox();
        Button btnBrowseRegTpl = new Button { Text = "Browse" };
        TextBox txtVarsFile = new TextBox();
        Button btnBrowseVars = new Button { Text = "Browse" };
        TextBox txtVarsInline = new TextBox { Multiline = true, ScrollBars = ScrollBars.Vertical };
        Button btnRun = new Button { Text = "Run" };
        Button btnCopyArgs = new Button { Text = "Copy" };
        Button btnReset = new Button { Text = "Reset" };
        Button btnOpenDocs = new Button { Text = "📖 Help" };
        Button btnOpenTemplates = new Button { Text = "📁 Templates" };
        ComboBox cboBossEvery = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
        ComboBox cboTemplatePreset = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList };
        Label lblStageInfo = new Label { AutoSize = true, ForeColor = Color.LightSkyBlue };
        Label lblMaxStage = new Label { AutoSize = true, ForeColor = Color.Orange, Text = "⚠ Max: 255 stages" };
        ToolTip tt = new ToolTip { AutoPopDelay = 8000, InitialDelay = 500 };
        Panel header = new Panel { Height = 64, Dock = DockStyle.Top };
        Label lblTitle = new Label();
        Label lblSub = new Label();
        Panel status = new Panel { Height = 28, Dock = DockStyle.Bottom };
        Label lblStatus = new Label { AutoSize = false, Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleLeft };
        ProgressBar prg = new ProgressBar { Dock = DockStyle.Right, Style = ProgressBarStyle.Marquee, MarqueeAnimationSpeed = 0, Width = 120, Visible = false };

        public MainForm()
        {
            Text = "CCBD Floor Generator - WpsStageGen v4";
            Width = 1600; Height = 1080; StartPosition = FormStartPosition.CenterScreen;
            WindowState = FormWindowState.Normal;
            MinimumSize = new Size(1400, 900);
            Font = new Font("Segoe UI", 11F, FontStyle.Regular, GraphicsUnit.Point);

            // Theme
            BackColor = Color.FromArgb(32, 33, 36);
            ForeColor = Color.Gainsboro;
            ApplyFieldTheme(txtWpsPath);
            ApplyFieldTheme(txtGenPath);
            ApplyFieldTheme(txtOutPath);
            ApplyFieldTheme(txtBossTemplate);
            ApplyFieldTheme(txtRegularTemplate);
            ApplyFieldTheme(txtArgs, monospace: true);
            ApplyFieldTheme(txtOutput, monospace: true);
            ApplyNumericTheme(numAdd, numBossEvery, numRewardItem, numBossGroup);
            ApplyFieldTheme(txtVarsFile);
            ApplyFieldTheme(txtVarsInline, monospace: true);
            txtWpsPath.MinimumSize = new Size(700, 0);
            txtGenPath.MinimumSize = new Size(700, 0);
            txtOutPath.MinimumSize = new Size(700, 0);
            numAdd.Width = 240;
            numBossEvery.Width = 240;
            numRewardItem.Width = 300;
            numBossGroup.Width = 260;
            StyleButton(btnRun, primary: true);
            StyleButton(btnBrowseWps);
            StyleButton(btnBrowseGen);
            StyleButton(btnBrowseOut);
            StyleButton(btnBrowseBossTpl);
            StyleButton(btnBrowseRegTpl);
            StyleButton(btnBrowseVars);
            StyleButton(btnCopyArgs);
            StyleButton(btnReset);
            btnBrowseWps.Text = "Browse..."; btnBrowseGen.Text = "Browse..."; btnBrowseOut.Text = "Browse..."; btnBrowseBossTpl.Text = "Browse..."; btnBrowseRegTpl.Text = "Browse..."; btnBrowseVars.Text = "Browse...";
            btnCopyArgs.Text = "Copy"; btnReset.Text = "Reset"; btnRun.Text = "Run";
            btnBrowseWps.AutoSize = false; btnBrowseGen.AutoSize = false; btnBrowseOut.AutoSize = false; btnBrowseBossTpl.AutoSize = false; btnBrowseRegTpl.AutoSize = false; btnBrowseVars.AutoSize = false; btnCopyArgs.AutoSize = false; btnRun.AutoSize = false; btnReset.AutoSize = false;
            foreach (var b in new[] { btnBrowseWps, btnBrowseGen, btnBrowseOut, btnBrowseBossTpl, btnBrowseRegTpl, btnBrowseVars }) { b.Width = 140; b.Height = 120; }
            btnCopyArgs.Width = 110; btnCopyArgs.Height = 40; btnReset.Width = 110; btnReset.Height = 40; btnRun.Width = 120; btnRun.Height = 75;

            // Header
            header.BackColor = Color.FromArgb(45, 47, 51);
            header.Padding = new Padding(24, 12, 24, 12);
            var ver = typeof(MainForm).Assembly.GetName().Version?.ToString() ?? "1.0.0";
            lblTitle.Text = $"🎰 CCBD Floor Generator v{ver}";
            lblTitle.Font = new Font("Segoe UI Semibold", 18F, FontStyle.Bold, GraphicsUnit.Point);
            lblTitle.ForeColor = Color.WhiteSmoke;
            lblTitle.AutoSize = true;
            lblSub.Text = "Generate Crazy Casino floors • Up to 255 stages • Advanced templates & mechanics";
            lblSub.Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point);
            lblSub.ForeColor = Color.LightSkyBlue;
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

            // Root container: left column (stack of cards) + right expansion with Output beneath
            var root = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(24) };
            root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));

            // Generator card
            var genTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(12) };
            genTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            genTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            genTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            genTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            genTable.Controls.Add(FilePickerStack("WPS source file:", txtWpsPath, btnBrowseWps, "Path to the source .wps (e.g., 83000.wps)"));
            genTable.Controls.Add(FilePickerStack("Generator folder (.csproj):", txtGenPath, btnBrowseGen, "Folder that contains WpsStageGen.csproj"));
            genTable.Controls.Add(FilePickerStack("Output .wps (optional):", txtOutPath, btnBrowseOut, "Writes new floors to this file (recommended)"));
            var genCard = CreateCard("Generator", genTable, "\uE8B7");
            root.Controls.Add(genCard, 0, 0);

            // Parameters card
            var paramTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, Padding = new Padding(12) };
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            paramTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            paramTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            paramTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            paramTable.RowStyles.Add(new RowStyle(SizeType.AutoSize));

            // Add stage info label
            var stagePanel = new Panel { Dock = DockStyle.Fill, AutoSize = true };
            stagePanel.Controls.Add(FieldStack("Floors to append:", numAdd, "How many new floors to generate (e.g., 50 floors will create 10 boss stages)"));
            stagePanel.Controls.Add(lblStageInfo);
            lblStageInfo.Top = 75;
            paramTable.Controls.Add(stagePanel, 0, 0);

            var startPanel = new Panel { Dock = DockStyle.Fill, AutoSize = true };
            startPanel.Controls.Add(FieldStack("Starting stage:", numStartStage, "First stage number (0 = auto-detect from WPS file)"));
            startPanel.Controls.Add(lblMaxStage);
            lblMaxStage.Top = 75;
            paramTable.Controls.Add(startPanel, 1, 0);

            paramTable.Controls.Add(FieldStack("Boss every N floors:", numBossEvery, "Boss appears every Nth floor (typically 5)"), 0, 1);
            paramTable.Controls.Add(FieldStack("Presets:", cboBossEvery, "Quick select for 'Boss every'"), 1, 1);

            // Template preset selection
            paramTable.Controls.Add(FieldStack("Template preset:", cboTemplatePreset, "Pre-configured template combinations"), 0, 2);

            var paramCard = CreateCard("⚙ Stage Parameters", paramTable, "\uE713");
            // Create a two-column row for params + boss worlds
            var row1 = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            row1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            row1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            row1.Controls.Add(paramCard, 0, 0);

            // Drops and Mobs cards side by side
            var dropsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(12) };
            dropsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            dropsTable.Controls.Add(FieldStack("CCBD reward item (tblidx):", numRewardItem));
            ApplyFieldTheme(txtPattern, monospace: true);
            txtPattern.Dock = DockStyle.Fill;
            dropsTable.Controls.Add(FieldStack("Regular floor pattern list:", txtPattern, "Example: (1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)"));
            var dropsCard = CreateCard("Rewards & Pattern", dropsTable, "\uE896");
            row1.Controls.Add(CreateCard("Boss & Worlds", new Panel(), "\uE7F8")); // placeholder to be replaced below
            root.Controls.Add(row1, 0, 1);

            // Row 2: rewards & pattern + templates & variables
            var row2 = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            row2.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            row2.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
            row2.Controls.Add(dropsCard, 0, 0);

            var mobsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(12) };
            mobsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            mobsTable.Controls.Add(FieldStack("Boss mob group:", numBossGroup, "Group index used for boss spawns"));
            ApplyFieldTheme(txtBossWorlds, monospace: true);
            txtBossWorlds.Dock = DockStyle.Fill;
            mobsTable.Controls.Add(FieldStack("Boss arenas rotation (comma):", txtBossWorlds, "e.g., ARENA_A,ARENA_B,ARENA_C (tags and future teleports)"));
            var mobsCard = CreateCard("Boss & Worlds", mobsTable, "\uE7F8");
            // Replace placeholder with the actual Boss & Worlds card in row1
            row1.Controls.RemoveAt(1);
            row1.Controls.Add(mobsCard, 1, 0);

            // Templates & Variables card
            var tplTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(12) };
            tplTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            tplTable.Controls.Add(FilePickerStack("Regular template (optional):", txtRegularTemplate, btnBrowseRegTpl, "WPS snippet injected into regular floors"));
            tplTable.Controls.Add(FilePickerStack("Boss template (optional):", txtBossTemplate, btnBrowseBossTpl, "WPS snippet injected into boss floors (phases, buffs, etc.)"));
            tplTable.Controls.Add(FilePickerStack("Vars file (optional):", txtVarsFile, btnBrowseVars, "INI/TXT with NAME=VALUE lines (see templates/sample_vars.ini)"));
            txtVarsInline.MinimumSize = new Size(0, 120);
            tplTable.Controls.Add(FieldStack("Inline vars NAME=VALUE (one per line):", txtVarsInline, hint: "Example: PHASE91_GROUP=301, INVINCIBLE_BUFF=1900101"));
            var tplCard = CreateCard("Templates & Variables", tplTable, "\uE8D2");
            row2.Controls.Add(tplCard, 1, 0);
            root.Controls.Add(row2, 0, 2);

            // Args card
            var argsTable = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 7, RowCount = 1, Padding = new Padding(8) };
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            argsTable.Controls.Add(StyledLabel("Args preview:"), 0, 0);
            txtArgs.Dock = DockStyle.Fill;
            argsTable.Controls.Add(txtArgs, 1, 0);
            btnCopyArgs.AutoSize = true;
            btnCopyArgs.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            btnReset.AutoSize = true;
            btnReset.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            btnOpenDocs.AutoSize = true;
            btnOpenDocs.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            btnOpenTemplates.AutoSize = true;
            btnOpenTemplates.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            btnRun.AutoSize = true;
            btnRun.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            argsTable.Controls.Add(btnCopyArgs, 2, 0);
            argsTable.Controls.Add(btnReset, 3, 0);
            argsTable.Controls.Add(btnOpenTemplates, 4, 0);
            argsTable.Controls.Add(btnOpenDocs, 5, 0);
            argsTable.Controls.Add(btnRun, 6, 0);
            var argsCard = CreateCard("🚀 Action", argsTable, "\uE768");
            root.Controls.Add(argsCard, 0, 3);

            // Output card (expands)
            var outputHolder = new Panel { Dock = DockStyle.Fill, MinimumSize = new Size(0, 260) };
            txtOutput.Dock = DockStyle.Fill;
            outputHolder.Controls.Add(txtOutput);
            var outputCard = CreateCard("Output", outputHolder, "\uE7C3");
            Controls.Add(root);
            outputCard.Dock = DockStyle.Fill;
            root.Controls.Add(outputCard, 0, 4);

            btnBrowseWps.Click += (s, e) => BrowseFile(txtWpsPath, "WPS (*.wps)|*.wps|All files (*.*)|*.*");
            btnBrowseGen.Click += (s, e) => BrowseFolder(txtGenPath);
            btnBrowseOut.Click += (s, e) => BrowseSaveFile(txtOutPath, "WPS (*.wps)|*.wps|All files (*.*)|*.*");
            btnBrowseBossTpl.Click += (s, e) => BrowseFile(txtBossTemplate, "WPS snippet (*.wps;*.txt)|*.wps;*.txt|All files (*.*)|*.*");
            btnBrowseRegTpl.Click += (s, e) => BrowseFile(txtRegularTemplate, "WPS snippet (*.wps;*.txt)|*.wps;*.txt|All files (*.*)|*.*");
            btnBrowseVars.Click += (s, e) => BrowseFile(txtVarsFile, "Vars (*.ini;*.txt)|*.ini;*.txt|All files (*.*)|*.*");
            btnRun.Click += async (s, e) => await RunGeneratorAsync();
            btnCopyArgs.Click += (s, e) => { try { Clipboard.SetText(txtArgs.Text); lblStatus.Text = "Args copied"; } catch { } };
            btnReset.Click += (s, e) => ResetDefaults();

            // Preset options
            cboBossEvery.Items.AddRange(new object[] { 3, 5, 10, 15 });
            cboBossEvery.SelectedIndexChanged += (s, e) => { if (cboBossEvery.SelectedItem != null) numBossEvery.Value = Convert.ToDecimal(cboBossEvery.SelectedItem); };

            // Template presets
            cboTemplatePreset.Items.AddRange(new object[] {
                "None (Basic floors only)",
                "Simple Boss (Enrage at 30%)",
                "Advanced Boss (4 phases)",
                "Enhanced Regular + Simple Boss",
                "Enhanced Regular + Advanced Boss (Recommended)",
                "Custom (Select files manually)"
            });
            cboTemplatePreset.SelectedIndex = 4; // Default to recommended
            cboTemplatePreset.SelectedIndexChanged += (s, e) => ApplyTemplatePreset();

            // Help buttons
            StyleButton(btnOpenDocs, primary: false);
            StyleButton(btnOpenTemplates, primary: false);
            btnOpenDocs.Click += (s, e) => OpenDocumentation();
            btnOpenTemplates.Click += (s, e) => OpenTemplatesFolder();

            // Dynamic stage info update
            numAdd.ValueChanged += (s, e) => UpdateStageInfo();
            numStartStage.ValueChanged += (s, e) => UpdateStageInfo();
            numBossEvery.ValueChanged += (s, e) => UpdateStageInfo();

            // Enhanced Tooltips
            tt.SetToolTip(btnBrowseWps, "Browse for the source CCBD WPS file (e.g., 83000.wps)");
            tt.SetToolTip(btnBrowseGen, "Browse to the WpsStageGen folder (optional, auto-detected if tool is bundled)");
            tt.SetToolTip(btnRun, "Generate new CCBD floors with selected templates and parameters");
            tt.SetToolTip(btnCopyArgs, "Copy command-line arguments to clipboard");
            tt.SetToolTip(btnReset, "Reset all fields to default values");
            tt.SetToolTip(btnOpenDocs, "Open the comprehensive documentation and usage guide");
            tt.SetToolTip(btnOpenTemplates, "Open the templates folder to view/edit template files");
            tt.SetToolTip(numAdd, "Number of new floors to generate (e.g., 50 = 45 regular + 10 boss stages)");
            tt.SetToolTip(numStartStage, "Starting stage number (0 = auto-detect from WPS file, 151 = start after floor 150)");
            tt.SetToolTip(numBossEvery, "Boss appears every N floors (5 = boss at 5,10,15,20... | 10 = boss at 10,20,30...)");
            tt.SetToolTip(numBossGroup, "Mob group ID for boss spawns (must exist in mob tables)");
            tt.SetToolTip(numRewardItem, "Reward item table ID given after boss clear");
            tt.SetToolTip(txtPattern, "Mob spawn pattern with probabilities: (pattern_id, percentage)");
            tt.SetToolTip(txtBossWorlds, "Boss arena rotation (comma-separated): ARENA_A,ARENA_B,ARENA_C");
            tt.SetToolTip(cboTemplatePreset, "Quick select pre-configured template combinations");
            tt.SetToolTip(cboBossEvery, "Quick select boss interval (overrides 'Boss every N floors')");
            tt.SetToolTip(txtBossTemplate, "WPS template file for boss mechanics (phases, buffs, etc.)");
            tt.SetToolTip(txtRegularTemplate, "WPS template file for regular floor enhancements");
            tt.SetToolTip(txtVarsFile, "Variables file (.ini) with custom placeholder values");
            tt.SetToolTip(txtVarsInline, "Inline variables (one per line): VAR_NAME=value");

            foreach (var c in new Control[] { txtWpsPath, txtGenPath, txtOutPath, txtBossTemplate, txtRegularTemplate, txtVarsFile, txtVarsInline, numAdd, numStartStage, numBossEvery, numRewardItem, numBossGroup, txtPattern, txtBossWorlds })
            {
                c.TextChanged += (s, e) => UpdateArgs();
            }
            numAdd.ValueChanged += (s, e) => UpdateArgs();
            numBossEvery.ValueChanged += (s, e) => UpdateArgs();
            numRewardItem.ValueChanged += (s, e) => UpdateArgs();
            numBossGroup.ValueChanged += (s, e) => UpdateArgs();

            LoadSettings();
            AutoLoadBundledTemplatesIfAvailable();
            UpdateArgs();
            UpdateStageInfo();
            AcceptButton = btnRun;
        }

        void AutoLoadBundledTemplatesIfAvailable()
        {
            try
            {
                string baseDir = AppContext.BaseDirectory;
                string tplDir = Path.Combine(baseDir, "templates");
                if (!Directory.Exists(tplDir)) return;

                // Regular template: prefer a file starting with "regular_" or fallback to first .wps
                if (string.IsNullOrWhiteSpace(txtRegularTemplate.Text) || !File.Exists(txtRegularTemplate.Text))
                {
                    var reg = GetFirstFile(tplDir, new[] { "regular_*.wps", "*regular*.wps", "*.wps" });
                    if (reg != null) txtRegularTemplate.Text = reg;
                }

                // Boss template: prefer a file starting with "boss_" or containing "phases"
                if (string.IsNullOrWhiteSpace(txtBossTemplate.Text) || !File.Exists(txtBossTemplate.Text))
                {
                    var boss = GetFirstFile(tplDir, new[] { "boss_*.wps", "*phases*.wps", "*.wps" });
                    if (boss != null) txtBossTemplate.Text = boss;
                }

                // Vars file
                if (string.IsNullOrWhiteSpace(txtVarsFile.Text) || !File.Exists(txtVarsFile.Text))
                {
                    var vf = GetFirstFile(tplDir, new[] { "*.ini", "*.txt" });
                    if (vf != null) txtVarsFile.Text = vf;
                }

                // If pattern is empty and we have a default suggestion, keep existing; otherwise set a sane default
                if (string.IsNullOrWhiteSpace(txtPattern.Text))
                {
                    txtPattern.Text = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)";
                }

                string? GetFirstFile(string dir, string[] patterns)
                {
                    foreach (var p in patterns)
                    {
                        var files = Directory.GetFiles(dir, p, SearchOption.TopDirectoryOnly);
                        if (files != null && files.Length > 0) return files[0];
                    }
                    return null;
                }
            }
            catch { }
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

        void BrowseSaveFile(TextBox target, string filter)
        {
            using var sfd = new SaveFileDialog { Filter = filter, OverwritePrompt = false, AddExtension = true, DefaultExt = ".wps" };
            if (sfd.ShowDialog(this) == DialogResult.OK)
                target.Text = sfd.FileName;
        }

        async System.Threading.Tasks.Task RunGeneratorAsync()
        {
            txtOutput.Clear();
            if (!File.Exists(txtWpsPath.Text))
            {
                MessageBox.Show(this, "Select a valid WPS file.");
                return;
            }
            // Resolve generator location: packaged exe/dll next to UI, or in selected folder, else fall back to dotnet run
            var invoke = ResolveGeneratorInvocation();
            if (invoke == null)
            {
                MessageBox.Show(this, "Could not locate WpsStageGen (exe/dll/csproj).\nPick the generator folder or run the publisher so it ships the generator alongside the UI.");
                return;
            }

            var psi = new ProcessStartInfo
            {
                FileName = invoke.Value.fileName,
                WorkingDirectory = invoke.Value.workingDir,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                Arguments = invoke.Value.arguments
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

        (string fileName, string workingDir, string arguments)? ResolveGeneratorInvocation()
        {
            // Prefer an explicit generator directory if provided; otherwise default to the app folder
            string baseDir = AppContext.BaseDirectory;
            string genDir = Directory.Exists(txtGenPath.Text) ? txtGenPath.Text : baseDir;

            string exePath = Path.Combine(genDir, "WpsStageGen.exe");
            string dllPath = Path.Combine(genDir, "WpsStageGen.dll");
            string csprojPath = Path.Combine(genDir, "WpsStageGen.csproj");
            string argsOnly = BuildArgsOnly();

            if (File.Exists(exePath))
            {
                return (exePath, genDir, argsOnly);
            }
            if (File.Exists(dllPath))
            {
                // run dll via dotnet
                return ("dotnet", genDir, $"\"{dllPath}\" {argsOnly}");
            }
            if (File.Exists(csprojPath))
            {
                // fallback to dotnet run on source
                return ("dotnet", genDir, $"run -- {argsOnly}");
            }

            return null;
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
                    outPath = txtOutPath.Text,
                    bossTemplate = txtBossTemplate.Text,
                    regularTemplate = txtRegularTemplate.Text,
                    varsFile = txtVarsFile.Text,
                    varsInline = txtVarsInline.Text,
                    add = (int)numAdd.Value,
                    bossEvery = (int)numBossEvery.Value,
                    bossGroup = (int)numBossGroup.Value,
                    rewardItem = (int)numRewardItem.Value,
                    pattern = txtPattern.Text,
                    bossWorlds = txtBossWorlds.Text
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
                if (r.TryGetProperty("outPath", out var op)) txtOutPath.Text = op.GetString();
                if (r.TryGetProperty("bossTemplate", out var bt)) txtBossTemplate.Text = bt.GetString();
                if (r.TryGetProperty("regularTemplate", out var rt)) txtRegularTemplate.Text = rt.GetString();
                if (r.TryGetProperty("add", out var a)) numAdd.Value = a.GetInt32();
                if (r.TryGetProperty("bossEvery", out var be)) numBossEvery.Value = be.GetInt32();
                if (r.TryGetProperty("bossGroup", out var bg)) numBossGroup.Value = bg.GetInt32();
                if (r.TryGetProperty("rewardItem", out var ri)) numRewardItem.Value = ri.GetInt32();
                if (r.TryGetProperty("pattern", out var p)) txtPattern.Text = p.GetString() ?? txtPattern.Text;
                if (r.TryGetProperty("bossWorlds", out var bw)) txtBossWorlds.Text = bw.GetString() ?? string.Empty;
                if (r.TryGetProperty("varsFile", out var vf)) txtVarsFile.Text = vf.GetString() ?? string.Empty;
                if (r.TryGetProperty("varsInline", out var vi)) txtVarsInline.Text = vi.GetString() ?? string.Empty;
            }
            catch { }
        }

        void ResetDefaults()
        {
            txtWpsPath.Clear();
            txtGenPath.Clear();
            txtOutPath.Clear();
            txtBossTemplate.Clear();
            txtRegularTemplate.Clear();
            numAdd.Value = 50; // Changed from 5 to 50 for better default
            numStartStage.Value = 0;
            numBossEvery.Value = 5;
            numRewardItem.Value = 7000002;
            numBossGroup.Value = 9999;
            cboBossEvery.SelectedIndex = -1;
            cboTemplatePreset.SelectedIndex = 4; // Enhanced Regular + Advanced Boss
            txtPattern.Text = "(1, 35%), (2, 35%), (3, 10%), (4, 10%), (6, 10%)";
            txtBossWorlds.Text = string.Empty;
            txtVarsFile.Clear();
            txtVarsInline.Clear();
            lblStatus.Text = "Defaults restored";
            UpdateArgs();
            UpdateStageInfo();
        }

        void UpdateStageInfo()
        {
            int add = (int)numAdd.Value;
            int start = (int)numStartStage.Value;
            int bossEvery = (int)numBossEvery.Value;

            int end = start > 0 ? start + add - 1 : add; // If start is 0, just show count
            int bossCount = add / bossEvery;
            int regularCount = add - bossCount;

            if (start == 0)
            {
                lblStageInfo.Text = $"💡 {regularCount} regular + {bossCount} boss stages";
            }
            else
            {
                lblStageInfo.Text = $"💡 Stages {start}-{end} ({regularCount} regular + {bossCount} boss)";
            }

            // Warning if exceeding 255
            if (end > 255)
            {
                lblMaxStage.Text = "⚠ Warning: Exceeds max 255 stages!";
                lblMaxStage.ForeColor = Color.Red;
            }
            else if (end > 200)
            {
                lblMaxStage.Text = $"⚠ High stage count ({end}/255)";
                lblMaxStage.ForeColor = Color.Orange;
            }
            else
            {
                lblMaxStage.Text = $"✓ Within limits ({end}/255)";
                lblMaxStage.ForeColor = Color.LightGreen;
            }
        }

        void ApplyTemplatePreset()
        {
            string baseDir = AppContext.BaseDirectory;
            string tplDir = Path.Combine(baseDir, "templates");

            switch (cboTemplatePreset.SelectedIndex)
            {
                case 0: // None
                    txtBossTemplate.Clear();
                    txtRegularTemplate.Clear();
                    txtVarsFile.Clear();
                    break;

                case 1: // Simple Boss
                    txtBossTemplate.Text = Path.Combine(tplDir, "boss_simple.wps");
                    txtRegularTemplate.Clear();
                    txtVarsFile.Text = Path.Combine(tplDir, "sample_vars.ini");
                    break;

                case 2: // Advanced Boss
                    txtBossTemplate.Text = Path.Combine(tplDir, "boss_phases.wps");
                    txtRegularTemplate.Clear();
                    txtVarsFile.Text = Path.Combine(tplDir, "sample_vars.ini");
                    break;

                case 3: // Enhanced Regular + Simple Boss
                    txtBossTemplate.Text = Path.Combine(tplDir, "boss_simple.wps");
                    txtRegularTemplate.Text = Path.Combine(tplDir, "regular_enhanced.wps");
                    txtVarsFile.Text = Path.Combine(tplDir, "sample_vars.ini");
                    break;

                case 4: // Enhanced Regular + Advanced Boss (Recommended)
                    txtBossTemplate.Text = Path.Combine(tplDir, "boss_phases.wps");
                    txtRegularTemplate.Text = Path.Combine(tplDir, "regular_enhanced.wps");
                    txtVarsFile.Text = Path.Combine(tplDir, "sample_vars.ini");
                    break;

                case 5: // Custom - don't change anything
                    break;
            }

            UpdateArgs();
        }

        void OpenDocumentation()
        {
            try
            {
                string docPath = Path.Combine(AppContext.BaseDirectory, "templates", "README.md");
                if (File.Exists(docPath))
                {
                    Process.Start(new ProcessStartInfo(docPath) { UseShellExecute = true });
                    lblStatus.Text = "Documentation opened";
                }
                else
                {
                    MessageBox.Show(this, "Documentation file not found: " + docPath, "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "Failed to open documentation: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        void OpenTemplatesFolder()
        {
            try
            {
                string tplDir = Path.Combine(AppContext.BaseDirectory, "templates");
                if (Directory.Exists(tplDir))
                {
                    Process.Start(new ProcessStartInfo(tplDir) { UseShellExecute = true });
                    lblStatus.Text = "Templates folder opened";
                }
                else
                {
                    MessageBox.Show(this, "Templates folder not found: " + tplDir, "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "Failed to open templates folder: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
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
            // Dock is set by layout containers; keep default here
            if (monospace)
            {
                tb.Font = new Font("Consolas", 10.5F, FontStyle.Regular, GraphicsUnit.Point);
            }
            else
            {
                tb.Font = new Font("Segoe UI", 11F, FontStyle.Regular, GraphicsUnit.Point);
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
                n.Font = new Font("Segoe UI", 11F, FontStyle.Regular, GraphicsUnit.Point);
            }
        }

        void StyleButton(Button btn, bool primary = false)
        {
            btn.FlatStyle = FlatStyle.Flat;
            btn.UseVisualStyleBackColor = false;
            btn.UseCompatibleTextRendering = true;
            btn.FlatAppearance.BorderColor = primary ? Color.FromArgb(0, 120, 215) : Color.FromArgb(86, 88, 94);
            btn.FlatAppearance.BorderSize = 1;
            btn.ForeColor = Color.White;
            // Higher-contrast backgrounds
            var secondary = Color.FromArgb(74, 77, 84);      // #4A4D54
            var secondaryHover = Color.FromArgb(90, 94, 102); // #5A5E66
            var primaryBg = Color.FromArgb(0, 120, 215);
            var primaryHover = Color.FromArgb(0, 105, 190);
            btn.BackColor = primary ? primaryBg : secondary;
            btn.Margin = new Padding(6);
            btn.Padding = new Padding(16, 8, 16, 8);
            btn.Cursor = Cursors.Hand;
            btn.MouseEnter += (s, e) => btn.BackColor = primary ? primaryHover : secondaryHover;
            btn.MouseLeave += (s, e) => btn.BackColor = primary ? primaryBg : secondary;
            btn.MinimumSize = new Size(primary ? 120 : 64, 36);
            btn.Font = new Font("Segoe UI", primary ? 12F : 11.5F, primary ? FontStyle.Bold : FontStyle.Regular, GraphicsUnit.Point);
            btn.UseMnemonic = false;
            btn.AutoEllipsis = true;
            btn.TextAlign = ContentAlignment.MiddleCenter;
        }

        Control CreateCard(string title, Control inner, string? iconGlyph = null)
        {
            var panel = new Panel { Padding = new Padding(12), BackColor = Color.FromArgb(38, 39, 43), Margin = new Padding(0, 8, 8, 8), Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
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
            inner.Dock = DockStyle.Top;
            panel.Controls.Add(inner);
            panel.Controls.Add(divider);
            panel.Controls.Add(titleBar);
            return panel;
        }

        Control FieldStack(string label, Control field, string? hint = null)
        {
            var pnl = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 1, Padding = new Padding(0, 4, 0, 4), AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            pnl.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            var lbl = StyledLabel(label);
            lbl.Margin = new Padding(0, 6, 0, 4);
            field.Dock = DockStyle.Fill;
            if (field is TextBox tb)
            {
                if (!tb.Multiline)
                {
                    tb.AutoSize = false;
                    tb.MinimumSize = new Size(0, 32);
                    tb.Height = 32;
                }
            }
            else if (field is ComboBox cb)
            {
                cb.AutoSize = false;
                cb.MinimumSize = new Size(0, 32);
                cb.Height = 32;
                cb.DropDownStyle = ComboBoxStyle.DropDownList;
                cb.IntegralHeight = false;
            }
            else if (field is NumericUpDown nud)
            {
                nud.AutoSize = false;
                nud.MinimumSize = new Size(0, 32);
                nud.Height = 32;
            }
            pnl.Controls.Add(lbl, 0, 0);
            pnl.Controls.Add(field, 0, 1);
            if (!string.IsNullOrWhiteSpace(hint))
            {
                var hl = new Label
                {
                    Text = hint,
                    AutoSize = true,
                    ForeColor = Color.Silver,
                    Font = new Font("Segoe UI", 8f, FontStyle.Italic, GraphicsUnit.Point),
                    Margin = new Padding(0, 3, 0, 0)
                };
                pnl.Controls.Add(hl, 0, 2);
            }
            return pnl;
        }

        Control FilePickerStack(string label, TextBox textbox, Button browseButton, string? hint = null)
        {
            var pnl = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, Padding = new Padding(0, 4, 0, 4) };
            pnl.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            pnl.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            var lbl = StyledLabel(label);
            lbl.Margin = new Padding(0, 6, 0, 4);
            var stack = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 2, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            stack.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            stack.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
            textbox.Dock = DockStyle.Fill;
            textbox.AutoSize = false;
            textbox.MinimumSize = new Size(0, 32);
            textbox.Height = 32;
            stack.Controls.Add(textbox, 0, 0);
            browseButton.Anchor = AnchorStyles.Right;
            browseButton.AutoSize = false;
            browseButton.MinimumSize = new Size(120, 32);
            browseButton.Width = 120;
            browseButton.Height = 32;
            stack.Controls.Add(browseButton, 1, 0);
            var outer = new Panel { Dock = DockStyle.Top };
            var v = new TableLayoutPanel { Dock = DockStyle.Top, ColumnCount = 1, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink };
            v.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            v.Controls.Add(lbl, 0, 0);
            v.Controls.Add(stack, 0, 1);
            if (!string.IsNullOrWhiteSpace(hint))
            {
                var hl = new Label
                {
                    Text = hint,
                    AutoSize = true,
                    ForeColor = Color.Silver,
                    Font = new Font("Segoe UI", 8f, FontStyle.Italic, GraphicsUnit.Point),
                    Margin = new Padding(0, 3, 0, 0)
                };
                v.Controls.Add(hl, 0, 2);
            }
            outer.Controls.Add(v);
            pnl.Controls.Add(outer, 0, 0);
            return pnl;
        }

        string BuildArgsForProcess()
        {
            var parts = new System.Collections.Generic.List<string>();
            parts.Add($"run -- {BuildArgsOnly()}");
            return string.Join(" ", parts);
        }

        string BuildArgsOnly()
        {
            var parts = new System.Collections.Generic.List<string>();
            parts.Add($"in=\\\"{txtWpsPath.Text}\\\"");
            parts.Add($"add={numAdd.Value}");
            if (numStartStage.Value > 0) parts.Add($"start={numStartStage.Value}");
            parts.Add($"bossEvery={numBossEvery.Value}");
            parts.Add($"bossGroup={numBossGroup.Value}");
            parts.Add($"rewardItem={numRewardItem.Value}");
            if (!string.IsNullOrWhiteSpace(txtOutPath.Text)) parts.Add($"out=\\\"{txtOutPath.Text}\\\"");
            if (!string.IsNullOrWhiteSpace(txtPattern.Text))
            {
                var patternEsc = txtPattern.Text.Replace("\"", "\\\"");
                parts.Add($"pattern=\"{patternEsc}\"");
            }
            if (!string.IsNullOrWhiteSpace(txtBossWorlds.Text))
            {
                var bw = txtBossWorlds.Text.Replace(" ", string.Empty);
                parts.Add($"bossWorlds={bw}");
            }
            if (!string.IsNullOrWhiteSpace(txtBossTemplate.Text)) parts.Add($"bossTemplate=\\\"{txtBossTemplate.Text}\\\"");
            if (!string.IsNullOrWhiteSpace(txtRegularTemplate.Text)) parts.Add($"regularTemplate=\\\"{txtRegularTemplate.Text}\\\"");
            if (!string.IsNullOrWhiteSpace(txtVarsFile.Text)) parts.Add($"varsFile=\\\"{txtVarsFile.Text}\\\"");
            // Inline vars: each line NAME=VALUE → var.NAME=VALUE
            if (!string.IsNullOrWhiteSpace(txtVarsInline.Text))
            {
                var lines = txtVarsInline.Text.Replace("\r", string.Empty).Split('\n');
                foreach (var line in lines)
                {
                    var t = line.Trim();
                    if (t.Length == 0 || t.StartsWith("#") || !t.Contains("=")) continue;
                    var idx = t.IndexOf('=');
                    var name = t.Substring(0, idx).Trim();
                    var val = t.Substring(idx + 1).Trim().Replace("\"", "\\\"");
                    if (name.Length > 0)
                        parts.Add($"var.{name}=\"{val}\"");
                }
            }
            return string.Join(" ", parts);
        }

        void UpdateArgs()
        {
            var inArg = txtWpsPath.Text.Length > 0 ? $"in=\"{txtWpsPath.Text}\"" : string.Empty;
            var list = new System.Collections.Generic.List<string>();
            if (!string.IsNullOrEmpty(inArg)) list.Add(inArg);
            list.Add($"add={(int)numAdd.Value}");
            if (numStartStage.Value > 0) list.Add($"start={(int)numStartStage.Value}");
            list.Add($"bossEvery={(int)numBossEvery.Value}");
            list.Add($"bossGroup={(int)numBossGroup.Value}");
            list.Add($"rewardItem={(int)numRewardItem.Value}");
            if (!string.IsNullOrWhiteSpace(txtOutPath.Text)) list.Add($"out=\"{txtOutPath.Text}\"");
            if (!string.IsNullOrWhiteSpace(txtPattern.Text))
            {
                var patternEsc = txtPattern.Text.Replace("\"", "\\\"");
                list.Add($"pattern=\"{patternEsc}\"");
            }
            if (!string.IsNullOrWhiteSpace(txtBossWorlds.Text))
            {
                var bw = txtBossWorlds.Text.Replace(" ", string.Empty);
                list.Add($"bossWorlds={bw}");
            }
            if (!string.IsNullOrWhiteSpace(txtBossTemplate.Text)) list.Add($"bossTemplate=\"{txtBossTemplate.Text}\"");
            if (!string.IsNullOrWhiteSpace(txtRegularTemplate.Text)) list.Add($"regularTemplate=\"{txtRegularTemplate.Text}\"");
            if (!string.IsNullOrWhiteSpace(txtVarsFile.Text)) list.Add($"varsFile=\"{txtVarsFile.Text}\"");
            if (!string.IsNullOrWhiteSpace(txtVarsInline.Text))
            {
                var lines = txtVarsInline.Text.Replace("\r", string.Empty).Split('\n');
                foreach (var line in lines)
                {
                    var t = line.Trim();
                    if (t.Length == 0 || t.StartsWith("#") || !t.Contains("=")) continue;
                    var idx = t.IndexOf('=');
                    var name = t.Substring(0, idx).Trim();
                    var val = t.Substring(idx + 1).Trim().Replace("\"", "\\\"");
                    if (name.Length > 0)
                        list.Add($"var.{name}=\"{val}\"");
                }
            }
            txtArgs.Text = string.Join(" ", list);
        }
    }
}
