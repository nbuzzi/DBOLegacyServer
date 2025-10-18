using System.Linq;
using WpsStageGen;

namespace DungeonGenerator;

public class BossConfigDialog : Form
{
    private ComboBox cmbFloor = null!;
    private NumericUpDown numBossGroup = null!;
    private ComboBox cmbMechanics = null!;
    private ComboBox cmbArena = null!;
    private NumericUpDown numReward = null!;
    private ComboBox cmbRewardPreset = null!;
    private TextBox txtDescription = null!;
    private Button btnOk = null!;
    private Button btnCancel = null!;

    public IndividualBossConfig? BossConfig { get; private set; }
    private readonly DungeonProfile _profile;

    public BossConfigDialog(List<int> availableFloors, IndividualBossConfig? existingConfig, DungeonProfile profile)
    {
        _profile = profile;
        InitializeComponents(availableFloors);

        if (existingConfig != null)
        {
            LoadConfig(existingConfig);
        }
        else
        {
            // Set defaults for new boss
            if (availableFloors.Count > 0)
            {
                cmbFloor.SelectedItem = availableFloors[0];
            }
            numBossGroup.Value = profile.BossConfig.BossGroupBase;
        }
    }

    private void InitializeComponents(List<int> availableFloors)
    {
        Text = "Configure Individual Boss";
        Size = new Size(500, 400);
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;

        var y = 20;

        // Floor selection
        var lblFloor = new Label { Text = "Boss Floor:", Location = new Point(20, y), AutoSize = true };
        cmbFloor = new ComboBox
        {
            Location = new Point(150, y - 3),
            Width = 300,
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        foreach (var floor in availableFloors)
        {
            cmbFloor.Items.Add(floor);
        }
        y += 35;

        // Boss Group
        var lblBossGroup = new Label { Text = "Boss Group ID:", Location = new Point(20, y), AutoSize = true };
        numBossGroup = new NumericUpDown
        {
            Location = new Point(150, y - 3),
            Width = 150,
            Maximum = 99999,
            Minimum = 1,
            Value = 9999
        };
        y += 35;

        // Mechanics
        var lblMechanics = new Label { Text = "Mechanics Template:", Location = new Point(20, y), AutoSize = true };
        cmbMechanics = new ComboBox
        {
            Location = new Point(150, y - 3),
            Width = 300,
            DropDownStyle = ComboBoxStyle.DropDownList
        };
        cmbMechanics.Items.AddRange(new object[] {
            "None (Simple Boss)",
            "templates/boss_simple_enrage.wps",
            "templates/boss_phases_91_71_61_41_25_20.wps"
        });
        cmbMechanics.SelectedIndex = 0;
        y += 35;

        // Arena
        var lblArena = new Label { Text = "Arena World ID:", Location = new Point(20, y), AutoSize = true };
        cmbArena = new ComboBox
        {
            Location = new Point(150, y - 3),
            Width = 300,
            DropDownStyle = ComboBoxStyle.DropDown,
            AutoCompleteMode = AutoCompleteMode.SuggestAppend,
            AutoCompleteSource = AutoCompleteSource.ListItems
        };
        cmbArena.Items.AddRange(ReferenceData.ArenaOptions.ToArray());
        y += 35;

        // Reward
        var lblReward = new Label { Text = "Reward Item:", Location = new Point(20, y), AutoSize = true };
        numReward = new NumericUpDown
        {
            Location = new Point(150, y - 3),
            Width = 150,
            Maximum = 99999999,
            Minimum = 0,
            Value = 0
        };
        numReward.ValueChanged += NumReward_ValueChanged;

        cmbRewardPreset = new ComboBox
        {
            Location = new Point(310, y - 3),
            Width = 140,
            DropDownStyle = ComboBoxStyle.DropDownList,
            FormattingEnabled = true
        };
        foreach (var reward in ReferenceData.RewardItems)
        {
            cmbRewardPreset.Items.Add(reward);
        }
        cmbRewardPreset.SelectedIndexChanged += CmbRewardPreset_SelectedIndexChanged;
        y += 35;

        // Description
        var lblDesc = new Label { Text = "Description:", Location = new Point(20, y), AutoSize = true };
        txtDescription = new TextBox
        {
            Location = new Point(150, y - 3),
            Width = 300,
            PlaceholderText = "E.g., Fire boss with enrage at 30%"
        };
        y += 35;

        // Buttons
        btnOk = new Button
        {
            Text = "OK",
            Location = new Point(290, y + 20),
            Width = 75,
            DialogResult = DialogResult.OK
        };
        btnOk.Click += BtnOk_Click;

        btnCancel = new Button
        {
            Text = "Cancel",
            Location = new Point(375, y + 20),
            Width = 75,
            DialogResult = DialogResult.Cancel
        };

        Controls.AddRange(new Control[] {
            lblFloor, cmbFloor,
            lblBossGroup, numBossGroup,
            lblMechanics, cmbMechanics,
            lblArena, cmbArena,
            lblReward, numReward, cmbRewardPreset,
            lblDesc, txtDescription,
            btnOk, btnCancel
        });

        AcceptButton = btnOk;
        CancelButton = btnCancel;
    }

    private void LoadConfig(IndividualBossConfig config)
    {
        BossConfig = config;

        cmbFloor.SelectedItem = config.Floor;
        numBossGroup.Value = config.BossGroup;

        if (string.IsNullOrWhiteSpace(config.MechanicsTemplate))
        {
            cmbMechanics.SelectedIndex = 0;
        }
        else if (config.MechanicsTemplate.Contains("enrage"))
        {
            cmbMechanics.SelectedIndex = 1;
        }
        else if (config.MechanicsTemplate.Contains("phases"))
        {
            cmbMechanics.SelectedIndex = 2;
        }

        cmbArena.Text = config.Arena ?? string.Empty;

        if (config.RewardItem.HasValue && config.RewardItem.Value > 0)
        {
            numReward.Value = config.RewardItem.Value;
        }
        else
        {
            numReward.Value = 0;
            cmbRewardPreset.SelectedIndex = -1;
        }

        txtDescription.Text = config.Description ?? "";
    }

    private void CmbRewardPreset_SelectedIndexChanged(object? sender, EventArgs e)
    {
        if (cmbRewardPreset.SelectedItem is ReferenceData.RewardItemOption option)
        {
            if (numReward.Value != option.ItemId)
            {
                numReward.Value = option.ItemId;
            }
        }
    }

    private void NumReward_ValueChanged(object? sender, EventArgs e)
    {
        var index = ReferenceData.FindRewardItemIndex((int)numReward.Value);
        if (index >= 0)
        {
            if (cmbRewardPreset.SelectedIndex != index)
            {
                cmbRewardPreset.SelectedIndex = index;
            }
        }
        else if (cmbRewardPreset.SelectedIndex != -1)
        {
            cmbRewardPreset.SelectedIndex = -1;
        }
    }

    private void BtnOk_Click(object? sender, EventArgs e)
    {
        if (cmbFloor.SelectedItem == null)
        {
            MessageBox.Show("Please select a boss floor.", "Validation Error",
                          MessageBoxButtons.OK, MessageBoxIcon.Warning);
            DialogResult = DialogResult.None;
            return;
        }

        BossConfig = new IndividualBossConfig
        {
            Floor = (int)cmbFloor.SelectedItem,
            BossGroup = (int)numBossGroup.Value,
            MechanicsTemplate = cmbMechanics.SelectedIndex > 0 ? cmbMechanics.Text : null,
            Arena = string.IsNullOrWhiteSpace(cmbArena.Text) ? null : cmbArena.Text,
            RewardItem = numReward.Value > 0 ? (int)numReward.Value : null,
            Description = txtDescription.Text,
            Variables = new Dictionary<string, string>()
        };
    }
}
