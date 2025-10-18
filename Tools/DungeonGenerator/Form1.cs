using System.Linq;
using System.Text;
using System.Text.Json;
using WpsStageGen;

namespace DungeonGenerator;

public partial class Form1 : Form
{
    private sealed class BaseWpsOption
    {
        public BaseWpsOption(string displayName, string fullPath)
        {
            DisplayName = displayName;
            FullPath = fullPath;
        }

        public string DisplayName { get; }
        public string FullPath { get; }

        public override string ToString()
        {
            return DisplayName;
        }
    }

    private DungeonProfile? _currentProfile;
    private string _wpsDirectory = "";
    private bool _syncingArenaSelection;
    private bool _syncingRewardSelection;

    public Form1()
    {
        InitializeComponent();
    }

    private void Form1_Load(object sender, EventArgs e)
    {
        // Try to find WPS directory relative to application
        var appDir = AppDomain.CurrentDomain.BaseDirectory;
        var possibleWpsDir = Path.GetFullPath(Path.Combine(appDir, @"..\..\..\..\..\..\DboServer\ExecutionEnv\resource\server_data\wps\wps"));

        if (Directory.Exists(possibleWpsDir))
        {
            _wpsDirectory = possibleWpsDir;
            UpdateStatus($"WPS Directory found: {_wpsDirectory}");
        }
        else
        {
            // Try alternative paths
            possibleWpsDir = Path.GetFullPath(Path.Combine(appDir, @"..\..\..\DboServer\ExecutionEnv\resource\server_data\wps\wps"));
            if (Directory.Exists(possibleWpsDir))
            {
                _wpsDirectory = possibleWpsDir;
                UpdateStatus($"WPS Directory found: {_wpsDirectory}");
            }
            else
            {
                UpdateStatus("Warning: WPS directory not found. Some features may not work.");
            }
        }

        PopulateReferenceControls();

        // Initialize with default profile
        CreateNewProfile();
    }

    private void PopulateReferenceControls()
    {
        PopulateBaseWpsOptions();
        PopulateArenaOptions();
        PopulateRewardOptions();
        SyncArenaSelectionFromText();
        SyncRewardSelectionFromText();
    }

    private void CreateNewProfile()
    {
        var baseWpsPath = GetSelectedBaseWpsPath();

        _currentProfile = new DungeonProfile
        {
            Name = txtDungeonName.Text,
            Description = txtDescription.Text,
            WpsId = (int)numWpsId.Value,
            BaseWpsFile = baseWpsPath,
            FloorCount = (int)numFloorCount.Value,
            BossInterval = (int)numBossInterval.Value,
            StartFloor = (int)numStartFloor.Value,
            WaveConfig = new WaveConfig(),
            BossConfig = new BossConfig
            {
                BossGroupBase = (int)numBossGroupBase.Value,
                IncrementBossGroup = chkIncrementBossGroup.Checked
            },
            Variables = new VariableConfig()
        };

        UpdateFormFromProfile();
    }

    private void PopulateBaseWpsOptions()
    {
        cmbBaseWpsFile.Items.Clear();

        if (!string.IsNullOrWhiteSpace(_wpsDirectory) && Directory.Exists(_wpsDirectory))
        {
            var files = Directory.GetFiles(_wpsDirectory, "*.wps")
                .Select(Path.GetFileName)
                .Where(name => !string.IsNullOrWhiteSpace(name))
                .OrderBy(name => name)
                .ToList();

            foreach (var file in files)
            {
                var fullPath = Path.Combine(_wpsDirectory, file!);
                cmbBaseWpsFile.Items.Add(new BaseWpsOption(file!, fullPath));
            }
        }

        if (cmbBaseWpsFile.Items.Count == 0)
        {
            var fallback = ResolveBaseWpsPath("83000.wps");
            cmbBaseWpsFile.Items.Add(new BaseWpsOption(Path.GetFileName(fallback) ?? fallback, fallback));
        }

        if (cmbBaseWpsFile.Items.Count > 0 && cmbBaseWpsFile.SelectedIndex < 0)
        {
            int defaultIndex = FindBaseWpsOptionIndex("83000.wps");
            cmbBaseWpsFile.SelectedIndex = defaultIndex >= 0 ? defaultIndex : 0;
        }
    }

    private void PopulateArenaOptions()
    {
        clbArenaOptions.Items.Clear();
        foreach (var arena in ReferenceData.ArenaOptions)
        {
            clbArenaOptions.Items.Add(arena);
        }
    }

    private void PopulateRewardOptions()
    {
        lstRewardPresets.Items.Clear();
        foreach (var reward in ReferenceData.RewardItems)
        {
            lstRewardPresets.Items.Add(reward);
        }
    }

    private void UpdateProfileFromForm()
    {
        if (_currentProfile == null) return;

        _currentProfile.BaseWpsFile = GetSelectedBaseWpsPath();
        _currentProfile.Name = txtDungeonName.Text;
        _currentProfile.Description = txtDescription.Text;
        _currentProfile.WpsId = (int)numWpsId.Value;
        _currentProfile.FloorCount = (int)numFloorCount.Value;
        _currentProfile.BossInterval = (int)numBossInterval.Value;
        _currentProfile.StartFloor = (int)numStartFloor.Value;
        _currentProfile.BossConfig.BossGroupBase = (int)numBossGroupBase.Value;
        _currentProfile.BossConfig.IncrementBossGroup = chkIncrementBossGroup.Checked;

        // Mechanics template
        if (cmbMechanicsTemplate.SelectedIndex > 0)
        {
            _currentProfile.BossConfig.MechanicsTemplate = cmbMechanicsTemplate.Text;
        }
        else
        {
            _currentProfile.BossConfig.MechanicsTemplate = null;
        }

        // Arena rotation
        if (!string.IsNullOrWhiteSpace(txtArenaRotation.Text))
        {
            _currentProfile.BossConfig.ArenaRotation = txtArenaRotation.Text
                .Split(',', StringSplitOptions.RemoveEmptyEntries)
                .Select(s => s.Trim())
                .ToList();
        }
        else
        {
            _currentProfile.BossConfig.ArenaRotation = new List<string>();
        }

        // Reward items
        if (!string.IsNullOrWhiteSpace(txtRewardItems.Text))
        {
            _currentProfile.BossConfig.RewardItems = txtRewardItems.Text
                .Split(',', StringSplitOptions.RemoveEmptyEntries)
                .Select(s => int.TryParse(s.Trim(), out var id) ? id : 0)
                .Where(id => id > 0)
                .ToList();
        }
        else
        {
            _currentProfile.BossConfig.RewardItems = new List<int>();
        }
    }

    private void UpdateFormFromProfile()
    {
        if (_currentProfile == null) return;

        if (!string.IsNullOrWhiteSpace(_currentProfile.BaseWpsFile))
        {
            var option = EnsureBaseWpsOptionPresent(_currentProfile.BaseWpsFile);
            cmbBaseWpsFile.SelectedItem = option;
        }

        txtDungeonName.Text = _currentProfile.Name;
        txtDescription.Text = _currentProfile.Description;
        numWpsId.Value = _currentProfile.WpsId;
        numFloorCount.Value = _currentProfile.FloorCount;
        numBossInterval.Value = _currentProfile.BossInterval;
        numStartFloor.Value = _currentProfile.StartFloor;
        numBossGroupBase.Value = _currentProfile.BossConfig.BossGroupBase;
        chkIncrementBossGroup.Checked = _currentProfile.BossConfig.IncrementBossGroup;

        // Mechanics template
        if (string.IsNullOrWhiteSpace(_currentProfile.BossConfig.MechanicsTemplate))
        {
            cmbMechanicsTemplate.SelectedIndex = 0;
        }
        else if (_currentProfile.BossConfig.MechanicsTemplate.Contains("enrage"))
        {
            cmbMechanicsTemplate.SelectedIndex = 1;
        }
        else if (_currentProfile.BossConfig.MechanicsTemplate.Contains("phases"))
        {
            cmbMechanicsTemplate.SelectedIndex = 2;
        }

        // Arena rotation
        if (_currentProfile.BossConfig.ArenaRotation != null && _currentProfile.BossConfig.ArenaRotation.Count > 0)
        {
            txtArenaRotation.Text = string.Join(",", _currentProfile.BossConfig.ArenaRotation);
        }
        else
        {
            txtArenaRotation.Text = "";
        }

        // Reward items
        if (_currentProfile.BossConfig.RewardItems != null && _currentProfile.BossConfig.RewardItems.Count > 0)
        {
            txtRewardItems.Text = string.Join(",", _currentProfile.BossConfig.RewardItems);
        }
        else
        {
            txtRewardItems.Text = "";
        }

        SyncArenaSelectionFromText();
        SyncRewardSelectionFromText();
        RefreshBossList();
    }

    private string GetSelectedBaseWpsPath()
    {
        if (cmbBaseWpsFile.SelectedItem is BaseWpsOption option)
        {
            return option.FullPath;
        }

        return ResolveBaseWpsPath(cmbBaseWpsFile.Text);
    }

    private string ResolveBaseWpsPath(string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return string.IsNullOrWhiteSpace(_wpsDirectory)
                ? "83000.wps"
                : Path.Combine(_wpsDirectory, "83000.wps");
        }

        if (Path.IsPathRooted(value))
        {
            return value;
        }

        return string.IsNullOrWhiteSpace(_wpsDirectory)
            ? value
            : Path.Combine(_wpsDirectory, value);
    }

    private int FindBaseWpsOptionIndex(string fileNameOrPath)
    {
        if (string.IsNullOrWhiteSpace(fileNameOrPath))
            return -1;

        string targetName = Path.GetFileName(fileNameOrPath) ?? fileNameOrPath;

        for (int i = 0; i < cmbBaseWpsFile.Items.Count; i++)
        {
            if (cmbBaseWpsFile.Items[i] is not BaseWpsOption option)
                continue;

            if (string.Equals(option.DisplayName, fileNameOrPath, StringComparison.OrdinalIgnoreCase))
                return i;

            if (string.Equals(option.FullPath, fileNameOrPath, StringComparison.OrdinalIgnoreCase))
                return i;

            if (string.Equals(Path.GetFileName(option.FullPath), targetName, StringComparison.OrdinalIgnoreCase))
                return i;
        }

        return -1;
    }

    private BaseWpsOption EnsureBaseWpsOptionPresent(string fullPath)
    {
        string normalized = NormalizePath(fullPath);

        for (int i = 0; i < cmbBaseWpsFile.Items.Count; i++)
        {
            if (cmbBaseWpsFile.Items[i] is BaseWpsOption option)
            {
                if (string.Equals(NormalizePath(option.FullPath), normalized, StringComparison.OrdinalIgnoreCase))
                {
                    return option;
                }
            }
        }

        var display = GetBaseWpsDisplayName(fullPath);
        var newOption = new BaseWpsOption(display, fullPath);
        cmbBaseWpsFile.Items.Add(newOption);
        return newOption;
    }

    private static string NormalizePath(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
            return path;

        try
        {
            return Path.GetFullPath(path);
        }
        catch
        {
            return path;
        }
    }

    private string GetBaseWpsDisplayName(string fullPath)
    {
        if (string.IsNullOrWhiteSpace(fullPath))
            return fullPath;

        if (!string.IsNullOrWhiteSpace(_wpsDirectory))
        {
            var directory = NormalizePath(_wpsDirectory);
            var normalized = NormalizePath(fullPath);
            if (!string.IsNullOrWhiteSpace(directory) && normalized.StartsWith(directory, StringComparison.OrdinalIgnoreCase))
            {
                return Path.GetFileName(normalized) ?? fullPath;
            }
        }

        return Path.GetFileName(fullPath) ?? fullPath;
    }

    private void SyncArenaSelectionFromText()
    {
        if (_syncingArenaSelection)
            return;

        try
        {
            _syncingArenaSelection = true;
            var selections = txtArenaRotation.Text
                .Split(',', StringSplitOptions.RemoveEmptyEntries)
                .Select(s => s.Trim())
                .Where(s => s.Length > 0)
                .ToHashSet(StringComparer.OrdinalIgnoreCase);

            for (int i = 0; i < clbArenaOptions.Items.Count; i++)
            {
                var item = clbArenaOptions.Items[i]?.ToString() ?? string.Empty;
                clbArenaOptions.SetItemChecked(i, selections.Contains(item));
            }
        }
        finally
        {
            _syncingArenaSelection = false;
        }
    }

    private void UpdateArenaTextFromCheckedItems()
    {
        if (_syncingArenaSelection)
            return;

        try
        {
            _syncingArenaSelection = true;
            var selected = clbArenaOptions.CheckedItems
                .Cast<object>()
                .Select(item => item?.ToString() ?? string.Empty)
                .Where(text => !string.IsNullOrWhiteSpace(text))
                .ToList();

            txtArenaRotation.Text = string.Join(",", selected);
        }
        finally
        {
            _syncingArenaSelection = false;
        }
    }

    private void TxtArenaRotation_TextChanged(object? sender, EventArgs e)
    {
        if (_syncingArenaSelection)
            return;

        SyncArenaSelectionFromText();
    }

    private void ClbArenaOptions_ItemCheck(object? sender, ItemCheckEventArgs e)
    {
        if (_syncingArenaSelection)
            return;

        BeginInvoke(new Action(UpdateArenaTextFromCheckedItems));
    }

    private void BtnArenaApply_Click(object? sender, EventArgs e)
    {
        UpdateArenaTextFromCheckedItems();
    }

    private void BtnArenaClear_Click(object? sender, EventArgs e)
    {
        if (_syncingArenaSelection)
            return;

        try
        {
            _syncingArenaSelection = true;
            for (int i = 0; i < clbArenaOptions.Items.Count; i++)
            {
                clbArenaOptions.SetItemChecked(i, false);
            }
            txtArenaRotation.Text = string.Empty;
        }
        finally
        {
            _syncingArenaSelection = false;
        }
    }

    private List<int> ParseRewardItemsFromText()
    {
        return txtRewardItems.Text
            .Split(',', StringSplitOptions.RemoveEmptyEntries)
            .Select(s => int.TryParse(s.Trim(), out var id) ? id : 0)
            .Where(id => id > 0)
            .ToList();
    }

    private void SyncRewardSelectionFromText()
    {
        if (_syncingRewardSelection)
            return;

        try
        {
            _syncingRewardSelection = true;
            var selectedIds = ParseRewardItemsFromText().ToHashSet();

            for (int i = 0; i < lstRewardPresets.Items.Count; i++)
            {
                if (lstRewardPresets.Items[i] is ReferenceData.RewardItemOption option)
                {
                    lstRewardPresets.SetSelected(i, selectedIds.Contains(option.ItemId));
                }
            }
        }
        finally
        {
            _syncingRewardSelection = false;
        }
    }

    private void TxtRewardItems_TextChanged(object? sender, EventArgs e)
    {
        if (_syncingRewardSelection)
            return;

        SyncRewardSelectionFromText();
    }

    private void AddRewardItems(IEnumerable<int> itemIds)
    {
        var current = ParseRewardItemsFromText();
        foreach (var id in itemIds)
        {
            if (!current.Contains(id))
            {
                current.Add(id);
            }
        }

        _syncingRewardSelection = true;
        try
        {
            txtRewardItems.Text = string.Join(",", current);
        }
        finally
        {
            _syncingRewardSelection = false;
        }

        SyncRewardSelectionFromText();
    }

    private void BtnAddRewardPreset_Click(object? sender, EventArgs e)
    {
        var selected = lstRewardPresets.SelectedItems
            .Cast<ReferenceData.RewardItemOption>()
            .Select(option => option.ItemId)
            .ToList();

        if (selected.Count == 0)
            return;

        AddRewardItems(selected);
    }

    private void BtnClearRewardPreset_Click(object? sender, EventArgs e)
    {
        txtRewardItems.Text = string.Empty;
    }

    private void LstRewardPresets_DoubleClick(object? sender, EventArgs e)
    {
        BtnAddRewardPreset_Click(sender, e);
    }

    private void CmbBaseWpsFile_SelectedIndexChanged(object? sender, EventArgs e)
    {
        if (_currentProfile == null)
            return;

        _currentProfile.BaseWpsFile = GetSelectedBaseWpsPath();
    }

    private void BtnBrowseBaseWps_Click(object? sender, EventArgs e)
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "WPS Files (*.wps)|*.wps|All Files (*.*)|*.*",
            InitialDirectory = Directory.Exists(_wpsDirectory) ? _wpsDirectory : AppDomain.CurrentDomain.BaseDirectory
        };

        if (dialog.ShowDialog() == DialogResult.OK)
        {
            var option = EnsureBaseWpsOptionPresent(dialog.FileName);
            cmbBaseWpsFile.SelectedItem = option;
            if (_currentProfile != null)
            {
                _currentProfile.BaseWpsFile = option.FullPath;
            }
        }
    }

    private void RefreshBossList()
    {
        lstIndividualBosses.Items.Clear();

        if (_currentProfile?.BossConfig.IndividualBosses == null) return;

        foreach (var boss in _currentProfile.BossConfig.IndividualBosses.OrderBy(b => b.Floor))
        {
            var mechanic = string.IsNullOrWhiteSpace(boss.MechanicsTemplate) ? "Simple" :
                          boss.MechanicsTemplate.Contains("enrage") ? "Enrage" :
                          boss.MechanicsTemplate.Contains("phases") ? "Multi-Phase" : "Custom";

            var arena = string.IsNullOrWhiteSpace(boss.Arena) ? "" : $" | {boss.Arena}";
            var desc = string.IsNullOrWhiteSpace(boss.Description) ? "" : $" - {boss.Description}";

            lstIndividualBosses.Items.Add($"Floor {boss.Floor}: Boss {boss.BossGroup} ({mechanic}){arena}{desc}");
        }
    }

    private void NumWpsId_ValueChanged(object sender, EventArgs e)
    {
        if (string.IsNullOrEmpty(_wpsDirectory)) return;

        var wpsId = (int)numWpsId.Value;
        var wpsPath = Path.Combine(_wpsDirectory, $"{wpsId}.wps");

        if (File.Exists(wpsPath))
        {
            UpdateStatus($"⚠ WPS ID {wpsId} already exists!");
        }
        else
        {
            UpdateStatus($"✓ WPS ID {wpsId} is available");
        }
    }

    private void BtnFindNextId_Click(object sender, EventArgs e)
    {
        if (string.IsNullOrEmpty(_wpsDirectory))
        {
            MessageBox.Show("WPS directory not found. Cannot check for available IDs.",
                          "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        try
        {
            var nextId = DungeonProfile.FindNextAvailableWpsId(_wpsDirectory);
            numWpsId.Value = nextId;
            UpdateStatus($"Found next available WPS ID: {nextId}");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error finding next ID: {ex.Message}",
                          "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void FloorSettings_ValueChanged(object sender, EventArgs e)
    {
        // Refresh boss list when floor settings change
        if (_currentProfile != null)
        {
            UpdateProfileFromForm();
            RefreshBossList();
        }
    }

    private void BtnAddBoss_Click(object sender, EventArgs e)
    {
        if (_currentProfile == null) return;

        UpdateProfileFromForm();

        var bossFloors = _currentProfile.GetBossFloors();
        if (bossFloors.Count == 0)
        {
            MessageBox.Show("No boss floors available. Check your floor count and boss interval settings.",
                          "No Boss Floors", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        using var dialog = new BossConfigDialog(bossFloors, null, _currentProfile);
        if (dialog.ShowDialog() == DialogResult.OK && dialog.BossConfig != null)
        {
            _currentProfile.SetBossConfigForFloor(dialog.BossConfig.Floor, dialog.BossConfig);
            RefreshBossList();
            UpdateStatus($"Added boss configuration for floor {dialog.BossConfig.Floor}");
        }
    }

    private void BtnEditBoss_Click(object sender, EventArgs e)
    {
        if (_currentProfile == null || lstIndividualBosses.SelectedIndex < 0) return;

        var selectedBoss = _currentProfile.BossConfig.IndividualBosses[lstIndividualBosses.SelectedIndex];
        var bossFloors = _currentProfile.GetBossFloors();

        using var dialog = new BossConfigDialog(bossFloors, selectedBoss, _currentProfile);
        if (dialog.ShowDialog() == DialogResult.OK && dialog.BossConfig != null)
        {
            // Remove old config if floor changed
            if (selectedBoss.Floor != dialog.BossConfig.Floor)
            {
                _currentProfile.RemoveBossConfigForFloor(selectedBoss.Floor);
            }

            _currentProfile.SetBossConfigForFloor(dialog.BossConfig.Floor, dialog.BossConfig);
            RefreshBossList();
            UpdateStatus($"Updated boss configuration for floor {dialog.BossConfig.Floor}");
        }
    }

    private void BtnRemoveBoss_Click(object sender, EventArgs e)
    {
        if (_currentProfile == null || lstIndividualBosses.SelectedIndex < 0) return;

        var selectedBoss = _currentProfile.BossConfig.IndividualBosses[lstIndividualBosses.SelectedIndex];

        if (MessageBox.Show($"Remove boss configuration for floor {selectedBoss.Floor}?",
                          "Confirm", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
        {
            _currentProfile.RemoveBossConfigForFloor(selectedBoss.Floor);
            RefreshBossList();
            UpdateStatus($"Removed boss configuration for floor {selectedBoss.Floor}");
        }
    }

    private void BtnGenerateProfile_Click(object sender, EventArgs e)
    {
        try
        {
            UpdateProfileFromForm();

            if (_currentProfile == null)
            {
                MessageBox.Show("No profile to generate.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Validate
            var errors = _currentProfile.Validate();
            if (errors.Count > 0)
            {
                var errorMsg = "Validation errors:\n\n" + string.Join("\n", errors);
                MessageBox.Show(errorMsg, "Validation Failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Generate output
            var output = new StringBuilder();
            output.AppendLine("=== DUNGEON PROFILE ===");
            output.AppendLine($"Name: {_currentProfile.Name}");
            output.AppendLine($"WPS ID: {_currentProfile.WpsId}");
            output.AppendLine($"Floors: {_currentProfile.FloorCount}");
            output.AppendLine($"Boss Every: {_currentProfile.BossInterval} floors");
            output.AppendLine($"Total Bosses: {_currentProfile.GetBossFloors().Count}");
            output.AppendLine($"Individual Boss Configs: {_currentProfile.BossConfig.IndividualBosses.Count}");
            output.AppendLine();

            output.AppendLine("=== JSON PROFILE ===");
            output.AppendLine(JsonSerializer.Serialize(_currentProfile, new JsonSerializerOptions
            {
                WriteIndented = true
            }));
            output.AppendLine();

            output.AppendLine("=== COMMAND LINE ===");
            var command = $"dotnet run --project WpsStageGen -- {_currentProfile.ToCommandLineArgs()}";
            output.AppendLine(command);
            output.AppendLine();

            output.AppendLine("=== FULL COMMAND WITH OUTPUT ===");
            var outputPath = _currentProfile.GetOutputWpsPath(_wpsDirectory);
            output.AppendLine($"cd Tools");
            output.AppendLine($"dotnet run --project WpsStageGen -- {_currentProfile.ToCommandLineArgs()} out=\"{outputPath}\"");

            txtOutput.Text = output.ToString();
            UpdateStatus("Profile generated successfully!");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error generating profile: {ex.Message}",
                          "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void BtnSaveJson_Click(object sender, EventArgs e)
    {
        try
        {
            UpdateProfileFromForm();

            if (_currentProfile == null)
            {
                MessageBox.Show("No profile to save.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // Validate first
            var errors = _currentProfile.Validate();
            if (errors.Count > 0)
            {
                var errorMsg = "Validation errors:\n\n" + string.Join("\n", errors);
                MessageBox.Show(errorMsg, "Validation Failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            using var dialog = new SaveFileDialog
            {
                Filter = "JSON Files (*.json)|*.json|All Files (*.*)|*.*",
                DefaultExt = "json",
                FileName = $"{_currentProfile.WpsId}_{_currentProfile.Name.Replace(" ", "_").ToLower()}.json",
                InitialDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, @"..\..\..\..\WpsStageGen\profiles")
            };

            if (dialog.ShowDialog() == DialogResult.OK)
            {
                _currentProfile.SaveToFile(dialog.FileName);
                UpdateStatus($"Profile saved to: {dialog.FileName}");
                MessageBox.Show($"Profile saved successfully to:\n{dialog.FileName}",
                              "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error saving profile: {ex.Message}",
                          "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void BtnLoadTemplate_Click(object sender, EventArgs e)
    {
        var templateDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, @"..\..\..\..\WpsStageGen\profiles");

        using var dialog = new OpenFileDialog
        {
            Filter = "JSON Files (*.json)|*.json|All Files (*.*)|*.*",
            DefaultExt = "json",
            InitialDirectory = Directory.Exists(templateDir) ? templateDir : AppDomain.CurrentDomain.BaseDirectory
        };

        if (dialog.ShowDialog() == DialogResult.OK)
        {
            try
            {
                _currentProfile = DungeonProfile.LoadFromFile(dialog.FileName);
                UpdateFormFromProfile();
                UpdateStatus($"Loaded profile from: {dialog.FileName}");
                MessageBox.Show($"Profile loaded successfully!",
                              "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading profile: {ex.Message}",
                              "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }

    private void BtnCopyCommand_Click(object sender, EventArgs e)
    {
        if (string.IsNullOrWhiteSpace(txtOutput.Text))
        {
            MessageBox.Show("Generate a profile first.", "No Output",
                          MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        try
        {
            // Extract the command line from output
            var lines = txtOutput.Text.Split('\n');
            var commandStartIndex = Array.FindIndex(lines, l => l.Contains("=== FULL COMMAND"));
            if (commandStartIndex >= 0 && commandStartIndex + 2 < lines.Length)
            {
                var command = lines[commandStartIndex + 2].Trim(); // Get the dotnet run line
                Clipboard.SetText(command);
                UpdateStatus("Command copied to clipboard!");
            }
            else
            {
                Clipboard.SetText(txtOutput.Text);
                UpdateStatus("Output copied to clipboard!");
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error copying to clipboard: {ex.Message}",
                          "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void UpdateStatus(string message)
    {
        lblStatus.Text = message;
        Application.DoEvents();
    }
}
