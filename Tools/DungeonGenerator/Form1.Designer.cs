namespace DungeonGenerator;

partial class Form1
{
    /// <summary>
    ///  Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    ///  Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null))
        {
            components.Dispose();
        }
        base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    ///  Required method for Designer support - do not modify
    ///  the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
        TabControl tabControl;
        TabPage tabBasic;
        TabPage tabBosses;
        TabPage tabOutput;
        GroupBox grpBasicInfo;
        GroupBox grpFloorSettings;
        GroupBox grpBossDefaults;
        GroupBox grpArenaReward;
        GroupBox grpBossList;
        GroupBox grpOutput;
        Label lblName;
        Label lblWpsId;
        Label lblDescription;
        Label lblFloorCount;
        Label lblBossInterval;
        Label lblStartFloor;
        Label lblBossGroupBase;
    Label lblMechanicsTemplate;
    Label lblArenaRotation;
    Label lblRewardItems;
    Label lblBaseWps;
    Label lblArenaQuickPick;
    Label lblRewardPresets;

        tabControl = new TabControl();
        tabBasic = new TabPage();
        tabBosses = new TabPage();
        tabOutput = new TabPage();
        grpBasicInfo = new GroupBox();
        grpFloorSettings = new GroupBox();
        grpBossDefaults = new GroupBox();
        grpArenaReward = new GroupBox();
        grpBossList = new GroupBox();
        grpOutput = new GroupBox();
        lblName = new Label();
        lblWpsId = new Label();
        lblDescription = new Label();
        lblFloorCount = new Label();
        lblBossInterval = new Label();
        lblStartFloor = new Label();
        lblBossGroupBase = new Label();
        lblMechanicsTemplate = new Label();
        lblArenaRotation = new Label();
        lblRewardItems = new Label();
    lblBaseWps = new Label();
    lblArenaQuickPick = new Label();
    lblRewardPresets = new Label();

        txtDungeonName = new TextBox();
        numWpsId = new NumericUpDown();
        btnFindNextId = new Button();
    txtDescription = new TextBox();
    cmbBaseWpsFile = new ComboBox();
    btnBrowseBaseWps = new Button();
        numFloorCount = new NumericUpDown();
        numBossInterval = new NumericUpDown();
        numStartFloor = new NumericUpDown();
        numBossGroupBase = new NumericUpDown();
        cmbMechanicsTemplate = new ComboBox();
        chkIncrementBossGroup = new CheckBox();
    txtArenaRotation = new TextBox();
    txtRewardItems = new TextBox();
    clbArenaOptions = new CheckedListBox();
    btnArenaApply = new Button();
    btnArenaClear = new Button();
    lstRewardPresets = new ListBox();
    btnAddRewardPreset = new Button();
    btnClearRewardPreset = new Button();
        lstIndividualBosses = new ListBox();
        btnAddBoss = new Button();
        btnEditBoss = new Button();
        btnRemoveBoss = new Button();
        txtOutput = new TextBox();
        btnGenerateProfile = new Button();
        btnSaveJson = new Button();
        btnLoadTemplate = new Button();
        btnCopyCommand = new Button();
        lblStatus = new Label();

        tabControl.SuspendLayout();
        tabBasic.SuspendLayout();
        tabBosses.SuspendLayout();
        tabOutput.SuspendLayout();
        grpBasicInfo.SuspendLayout();
        grpFloorSettings.SuspendLayout();
        grpBossDefaults.SuspendLayout();
        grpArenaReward.SuspendLayout();
        grpBossList.SuspendLayout();
        grpOutput.SuspendLayout();
        ((System.ComponentModel.ISupportInitialize)numWpsId).BeginInit();
        ((System.ComponentModel.ISupportInitialize)numFloorCount).BeginInit();
        ((System.ComponentModel.ISupportInitialize)numBossInterval).BeginInit();
        ((System.ComponentModel.ISupportInitialize)numStartFloor).BeginInit();
        ((System.ComponentModel.ISupportInitialize)numBossGroupBase).BeginInit();
        SuspendLayout();

        //
        // tabControl
        //
        tabControl.Controls.Add(tabBasic);
        tabControl.Controls.Add(tabBosses);
        tabControl.Controls.Add(tabOutput);
        tabControl.Dock = DockStyle.Fill;
        tabControl.Location = new Point(0, 0);
        tabControl.Name = "tabControl";
        tabControl.SelectedIndex = 0;
        tabControl.Size = new Size(1000, 650);
        tabControl.TabIndex = 0;

        //
        // tabBasic
        //
        tabBasic.Controls.Add(grpBasicInfo);
        tabBasic.Controls.Add(grpFloorSettings);
        tabBasic.Controls.Add(grpBossDefaults);
        tabBasic.Controls.Add(grpArenaReward);
        tabBasic.Location = new Point(4, 24);
        tabBasic.Name = "tabBasic";
        tabBasic.Padding = new Padding(10);
        tabBasic.Size = new Size(992, 622);
        tabBasic.TabIndex = 0;
        tabBasic.Text = "Basic Configuration";
        tabBasic.UseVisualStyleBackColor = true;

        //
        // grpBasicInfo
        //
    grpBasicInfo.Controls.Add(lblBaseWps);
    grpBasicInfo.Controls.Add(cmbBaseWpsFile);
    grpBasicInfo.Controls.Add(btnBrowseBaseWps);
    grpBasicInfo.Controls.Add(lblName);
        grpBasicInfo.Controls.Add(txtDungeonName);
        grpBasicInfo.Controls.Add(lblWpsId);
        grpBasicInfo.Controls.Add(numWpsId);
        grpBasicInfo.Controls.Add(btnFindNextId);
        grpBasicInfo.Controls.Add(lblDescription);
        grpBasicInfo.Controls.Add(txtDescription);
        grpBasicInfo.Location = new Point(13, 13);
        grpBasicInfo.Name = "grpBasicInfo";
    grpBasicInfo.Size = new Size(966, 180);
        grpBasicInfo.TabIndex = 0;
        grpBasicInfo.TabStop = false;
        grpBasicInfo.Text = "Basic Information";

        //
        // lblName
        //
        lblName.AutoSize = true;
        lblName.Location = new Point(15, 25);
        lblName.Name = "lblName";
        lblName.Size = new Size(91, 15);
        lblName.TabIndex = 0;
        lblName.Text = "Dungeon Name:";

        //
        // txtDungeonName
        //
        txtDungeonName.Location = new Point(130, 22);
        txtDungeonName.Name = "txtDungeonName";
        txtDungeonName.Size = new Size(400, 23);
        txtDungeonName.TabIndex = 1;
        txtDungeonName.Text = "My Custom Dungeon";

        //
        // lblWpsId
        //
        lblWpsId.AutoSize = true;
        lblWpsId.Location = new Point(15, 54);
        lblWpsId.Name = "lblWpsId";
        lblWpsId.Size = new Size(49, 15);
        lblWpsId.TabIndex = 2;
        lblWpsId.Text = "WPS ID:";

        //
        // numWpsId
        //
        numWpsId.Location = new Point(130, 51);
        numWpsId.Maximum = new decimal(new int[] { 99999, 0, 0, 0 });
        numWpsId.Minimum = new decimal(new int[] { 83001, 0, 0, 0 });
        numWpsId.Name = "numWpsId";
        numWpsId.Size = new Size(120, 23);
        numWpsId.TabIndex = 3;
        numWpsId.Value = new decimal(new int[] { 83001, 0, 0, 0 });
        numWpsId.ValueChanged += NumWpsId_ValueChanged;

        //
        // btnFindNextId
        //
        btnFindNextId.Location = new Point(256, 50);
        btnFindNextId.Name = "btnFindNextId";
        btnFindNextId.Size = new Size(140, 25);
        btnFindNextId.TabIndex = 4;
        btnFindNextId.Text = "Find Next Available";
        btnFindNextId.UseVisualStyleBackColor = true;
        btnFindNextId.Click += BtnFindNextId_Click;

        //
        // lblDescription
        //
        lblDescription.AutoSize = true;
        lblDescription.Location = new Point(15, 83);
        lblDescription.Name = "lblDescription";
        lblDescription.Size = new Size(70, 15);
        lblDescription.TabIndex = 5;
        lblDescription.Text = "Description:";

        //
        // txtDescription
        //
        txtDescription.Location = new Point(130, 80);
        txtDescription.Multiline = true;
        txtDescription.Name = "txtDescription";
        txtDescription.Size = new Size(820, 45);
        txtDescription.TabIndex = 6;
        txtDescription.Text = "A custom CCBD-like dungeon";

    //
    // lblBaseWps
    //
    lblBaseWps.AutoSize = true;
    lblBaseWps.Location = new Point(15, 131);
    lblBaseWps.Name = "lblBaseWps";
    lblBaseWps.Size = new Size(88, 15);
    lblBaseWps.TabIndex = 7;
    lblBaseWps.Text = "Base WPS File:";

    //
    // cmbBaseWpsFile
    //
    cmbBaseWpsFile.DropDownStyle = ComboBoxStyle.DropDownList;
    cmbBaseWpsFile.FormattingEnabled = true;
    cmbBaseWpsFile.Location = new Point(130, 128);
    cmbBaseWpsFile.Name = "cmbBaseWpsFile";
    cmbBaseWpsFile.Size = new Size(300, 23);
    cmbBaseWpsFile.TabIndex = 8;
    cmbBaseWpsFile.SelectedIndexChanged += CmbBaseWpsFile_SelectedIndexChanged;

    //
    // btnBrowseBaseWps
    //
    btnBrowseBaseWps.Location = new Point(436, 127);
    btnBrowseBaseWps.Name = "btnBrowseBaseWps";
    btnBrowseBaseWps.Size = new Size(94, 25);
    btnBrowseBaseWps.TabIndex = 9;
    btnBrowseBaseWps.Text = "Browse...";
    btnBrowseBaseWps.UseVisualStyleBackColor = true;
    btnBrowseBaseWps.Click += BtnBrowseBaseWps_Click;

        //
        // grpFloorSettings
        //
        grpFloorSettings.Controls.Add(lblFloorCount);
        grpFloorSettings.Controls.Add(numFloorCount);
        grpFloorSettings.Controls.Add(lblBossInterval);
        grpFloorSettings.Controls.Add(numBossInterval);
        grpFloorSettings.Controls.Add(lblStartFloor);
        grpFloorSettings.Controls.Add(numStartFloor);
    grpFloorSettings.Location = new Point(13, 199);
        grpFloorSettings.Name = "grpFloorSettings";
        grpFloorSettings.Size = new Size(966, 80);
        grpFloorSettings.TabIndex = 1;
        grpFloorSettings.TabStop = false;
        grpFloorSettings.Text = "Floor Settings";

        //
        // lblFloorCount
        //
        lblFloorCount.AutoSize = true;
        lblFloorCount.Location = new Point(15, 30);
        lblFloorCount.Name = "lblFloorCount";
        lblFloorCount.Size = new Size(72, 15);
        lblFloorCount.TabIndex = 0;
        lblFloorCount.Text = "Total Floors:";

        //
        // numFloorCount
        //
        numFloorCount.Location = new Point(130, 27);
        numFloorCount.Maximum = new decimal(new int[] { 255, 0, 0, 0 });
        numFloorCount.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
        numFloorCount.Name = "numFloorCount";
        numFloorCount.Size = new Size(120, 23);
        numFloorCount.TabIndex = 1;
        numFloorCount.Value = new decimal(new int[] { 50, 0, 0, 0 });
        numFloorCount.ValueChanged += FloorSettings_ValueChanged;

        //
        // lblBossInterval
        //
        lblBossInterval.AutoSize = true;
        lblBossInterval.Location = new Point(280, 30);
        lblBossInterval.Name = "lblBossInterval";
        lblBossInterval.Size = new Size(114, 15);
        lblBossInterval.TabIndex = 2;
        lblBossInterval.Text = "Boss Every N Floors:";

        //
        // numBossInterval
        //
        numBossInterval.Location = new Point(400, 27);
        numBossInterval.Maximum = new decimal(new int[] { 50, 0, 0, 0 });
        numBossInterval.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
        numBossInterval.Name = "numBossInterval";
        numBossInterval.Size = new Size(120, 23);
        numBossInterval.TabIndex = 3;
        numBossInterval.Value = new decimal(new int[] { 5, 0, 0, 0 });
        numBossInterval.ValueChanged += FloorSettings_ValueChanged;

        //
        // lblStartFloor
        //
        lblStartFloor.AutoSize = true;
        lblStartFloor.Location = new Point(550, 30);
        lblStartFloor.Name = "lblStartFloor";
        lblStartFloor.Size = new Size(106, 15);
        lblStartFloor.TabIndex = 4;
        lblStartFloor.Text = "Start Floor (0=auto):";

        //
        // numStartFloor
        //
        numStartFloor.Location = new Point(662, 27);
        numStartFloor.Maximum = new decimal(new int[] { 255, 0, 0, 0 });
        numStartFloor.Name = "numStartFloor";
        numStartFloor.Size = new Size(120, 23);
        numStartFloor.TabIndex = 5;
        numStartFloor.Value = new decimal(new int[] { 0, 0, 0, 0 });

        //
        // grpBossDefaults
        //
        grpBossDefaults.Controls.Add(lblBossGroupBase);
        grpBossDefaults.Controls.Add(numBossGroupBase);
        grpBossDefaults.Controls.Add(lblMechanicsTemplate);
        grpBossDefaults.Controls.Add(cmbMechanicsTemplate);
        grpBossDefaults.Controls.Add(chkIncrementBossGroup);
    grpBossDefaults.Location = new Point(13, 285);
        grpBossDefaults.Name = "grpBossDefaults";
        grpBossDefaults.Size = new Size(966, 100);
        grpBossDefaults.TabIndex = 2;
        grpBossDefaults.TabStop = false;
        grpBossDefaults.Text = "Default Boss Configuration";

        //
        // lblBossGroupBase
        //
        lblBossGroupBase.AutoSize = true;
        lblBossGroupBase.Location = new Point(15, 30);
        lblBossGroupBase.Name = "lblBossGroupBase";
        lblBossGroupBase.Size = new Size(102, 15);
        lblBossGroupBase.TabIndex = 0;
        lblBossGroupBase.Text = "Boss Group Base:";

        //
        // numBossGroupBase
        //
        numBossGroupBase.Location = new Point(130, 27);
        numBossGroupBase.Maximum = new decimal(new int[] { 99999, 0, 0, 0 });
        numBossGroupBase.Name = "numBossGroupBase";
        numBossGroupBase.Size = new Size(120, 23);
        numBossGroupBase.TabIndex = 1;
        numBossGroupBase.Value = new decimal(new int[] { 9999, 0, 0, 0 });

        //
        // lblMechanicsTemplate
        //
        lblMechanicsTemplate.AutoSize = true;
        lblMechanicsTemplate.Location = new Point(15, 64);
        lblMechanicsTemplate.Name = "lblMechanicsTemplate";
        lblMechanicsTemplate.Size = new Size(109, 15);
        lblMechanicsTemplate.TabIndex = 2;
        lblMechanicsTemplate.Text = "Mechanics Template:";

        //
        // cmbMechanicsTemplate
        //
        cmbMechanicsTemplate.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbMechanicsTemplate.FormattingEnabled = true;
        cmbMechanicsTemplate.Items.AddRange(new object[] {
            "None (Simple Boss)",
            "templates/boss_simple_enrage.wps",
            "templates/boss_phases_91_71_61_41_25_20.wps"
        });
        cmbMechanicsTemplate.Location = new Point(130, 61);
        cmbMechanicsTemplate.Name = "cmbMechanicsTemplate";
        cmbMechanicsTemplate.Size = new Size(350, 23);
        cmbMechanicsTemplate.TabIndex = 3;
        cmbMechanicsTemplate.SelectedIndex = 0;

        //
        // chkIncrementBossGroup
        //
        chkIncrementBossGroup.AutoSize = true;
        chkIncrementBossGroup.Location = new Point(280, 29);
        chkIncrementBossGroup.Name = "chkIncrementBossGroup";
        chkIncrementBossGroup.Size = new Size(152, 19);
        chkIncrementBossGroup.TabIndex = 4;
        chkIncrementBossGroup.Text = "Increment Boss Group ID";
        chkIncrementBossGroup.UseVisualStyleBackColor = true;

        //
        // grpArenaReward
        //
    grpArenaReward.Controls.Add(lblArenaQuickPick);
    grpArenaReward.Controls.Add(clbArenaOptions);
    grpArenaReward.Controls.Add(btnArenaApply);
    grpArenaReward.Controls.Add(btnArenaClear);
    grpArenaReward.Controls.Add(lblRewardPresets);
    grpArenaReward.Controls.Add(lstRewardPresets);
    grpArenaReward.Controls.Add(btnAddRewardPreset);
    grpArenaReward.Controls.Add(btnClearRewardPreset);
    grpArenaReward.Controls.Add(lblArenaRotation);
    grpArenaReward.Controls.Add(txtArenaRotation);
    grpArenaReward.Controls.Add(lblRewardItems);
    grpArenaReward.Controls.Add(txtRewardItems);
    grpArenaReward.Location = new Point(13, 391);
        grpArenaReward.Name = "grpArenaReward";
    grpArenaReward.Size = new Size(966, 230);
        grpArenaReward.TabIndex = 3;
        grpArenaReward.TabStop = false;
        grpArenaReward.Text = "Arena && Rewards";

        //
        // lblArenaRotation
        //
        lblArenaRotation.AutoSize = true;
        lblArenaRotation.Location = new Point(15, 30);
        lblArenaRotation.Name = "lblArenaRotation";
        lblArenaRotation.Size = new Size(88, 15);
        lblArenaRotation.TabIndex = 0;
        lblArenaRotation.Text = "Arena Rotation:";

        //
        // txtArenaRotation
        //
        txtArenaRotation.Location = new Point(130, 27);
        txtArenaRotation.Name = "txtArenaRotation";
        txtArenaRotation.PlaceholderText = "ARENA_FIRE,ARENA_ICE,ARENA_LIGHTNING (leave empty for defaults)";
        txtArenaRotation.Size = new Size(820, 23);
        txtArenaRotation.TabIndex = 1;
    txtArenaRotation.TextChanged += TxtArenaRotation_TextChanged;

        //
        // lblRewardItems
        //
        lblRewardItems.AutoSize = true;
        lblRewardItems.Location = new Point(15, 64);
        lblRewardItems.Name = "lblRewardItems";
        lblRewardItems.Size = new Size(79, 15);
        lblRewardItems.TabIndex = 2;
        lblRewardItems.Text = "Reward Items:";

        //
        // txtRewardItems
        //
        txtRewardItems.Location = new Point(130, 61);
        txtRewardItems.Name = "txtRewardItems";
        txtRewardItems.PlaceholderText = "7000002,7000003,7000004 (comma-separated item IDs)";
        txtRewardItems.Size = new Size(820, 23);
        txtRewardItems.TabIndex = 3;
        txtRewardItems.Text = "7000002";
    txtRewardItems.TextChanged += TxtRewardItems_TextChanged;

    //
    // lblArenaQuickPick
    //
    lblArenaQuickPick.AutoSize = true;
    lblArenaQuickPick.Location = new Point(15, 105);
    lblArenaQuickPick.Name = "lblArenaQuickPick";
    lblArenaQuickPick.Size = new Size(95, 15);
    lblArenaQuickPick.TabIndex = 4;
    lblArenaQuickPick.Text = "Arena quick pick:";

    //
    // clbArenaOptions
    //
    clbArenaOptions.CheckOnClick = true;
    clbArenaOptions.FormattingEnabled = true;
    clbArenaOptions.Location = new Point(130, 100);
    clbArenaOptions.Name = "clbArenaOptions";
    clbArenaOptions.Size = new Size(300, 112);
    clbArenaOptions.TabIndex = 5;
    clbArenaOptions.ItemCheck += ClbArenaOptions_ItemCheck;

    //
    // btnArenaApply
    //
    btnArenaApply.Location = new Point(440, 100);
    btnArenaApply.Name = "btnArenaApply";
    btnArenaApply.Size = new Size(120, 25);
    btnArenaApply.TabIndex = 6;
    btnArenaApply.Text = "Use Selection";
    btnArenaApply.UseVisualStyleBackColor = true;
    btnArenaApply.Click += BtnArenaApply_Click;

    //
    // btnArenaClear
    //
    btnArenaClear.Location = new Point(440, 131);
    btnArenaClear.Name = "btnArenaClear";
    btnArenaClear.Size = new Size(120, 25);
    btnArenaClear.TabIndex = 7;
    btnArenaClear.Text = "Clear Selection";
    btnArenaClear.UseVisualStyleBackColor = true;
    btnArenaClear.Click += BtnArenaClear_Click;

    //
    // lblRewardPresets
    //
    lblRewardPresets.AutoSize = true;
    lblRewardPresets.Location = new Point(580, 105);
    lblRewardPresets.Name = "lblRewardPresets";
    lblRewardPresets.Size = new Size(90, 15);
    lblRewardPresets.TabIndex = 8;
    lblRewardPresets.Text = "Reward presets:";

    //
    // lstRewardPresets
    //
    lstRewardPresets.FormattingEnabled = true;
    lstRewardPresets.ItemHeight = 15;
    lstRewardPresets.Location = new Point(580, 123);
    lstRewardPresets.Name = "lstRewardPresets";
    lstRewardPresets.SelectionMode = SelectionMode.MultiExtended;
    lstRewardPresets.Size = new Size(270, 64);
    lstRewardPresets.TabIndex = 9;
    lstRewardPresets.DoubleClick += LstRewardPresets_DoubleClick;

    //
    // btnAddRewardPreset
    //
    btnAddRewardPreset.Location = new Point(580, 200);
    btnAddRewardPreset.Name = "btnAddRewardPreset";
    btnAddRewardPreset.Size = new Size(120, 25);
    btnAddRewardPreset.TabIndex = 10;
    btnAddRewardPreset.Text = "Add Selected";
    btnAddRewardPreset.UseVisualStyleBackColor = true;
    btnAddRewardPreset.Click += BtnAddRewardPreset_Click;

    //
    // btnClearRewardPreset
    //
    btnClearRewardPreset.Location = new Point(710, 200);
    btnClearRewardPreset.Name = "btnClearRewardPreset";
    btnClearRewardPreset.Size = new Size(120, 25);
    btnClearRewardPreset.TabIndex = 11;
    btnClearRewardPreset.Text = "Clear";
    btnClearRewardPreset.UseVisualStyleBackColor = true;
    btnClearRewardPreset.Click += BtnClearRewardPreset_Click;

        //
        // tabBosses
        //
        tabBosses.Controls.Add(grpBossList);
        tabBosses.Location = new Point(4, 24);
        tabBosses.Name = "tabBosses";
        tabBosses.Padding = new Padding(10);
        tabBosses.Size = new Size(992, 622);
        tabBosses.TabIndex = 1;
        tabBosses.Text = "Individual Bosses";
        tabBosses.UseVisualStyleBackColor = true;

        //
        // grpBossList
        //
        grpBossList.Controls.Add(lstIndividualBosses);
        grpBossList.Controls.Add(btnAddBoss);
        grpBossList.Controls.Add(btnEditBoss);
        grpBossList.Controls.Add(btnRemoveBoss);
        grpBossList.Dock = DockStyle.Fill;
        grpBossList.Location = new Point(10, 10);
        grpBossList.Name = "grpBossList";
        grpBossList.Size = new Size(972, 602);
        grpBossList.TabIndex = 0;
        grpBossList.TabStop = false;
        grpBossList.Text = "Custom Boss Configuration (Mix Different Mechanics)";

        //
        // lstIndividualBosses
        //
        lstIndividualBosses.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
        lstIndividualBosses.FormattingEnabled = true;
        lstIndividualBosses.ItemHeight = 15;
        lstIndividualBosses.Location = new Point(15, 25);
        lstIndividualBosses.Name = "lstIndividualBosses";
        lstIndividualBosses.Size = new Size(942, 529);
        lstIndividualBosses.TabIndex = 0;
        lstIndividualBosses.DoubleClick += BtnEditBoss_Click;

        //
        // btnAddBoss
        //
        btnAddBoss.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnAddBoss.Location = new Point(15, 565);
        btnAddBoss.Name = "btnAddBoss";
        btnAddBoss.Size = new Size(120, 30);
        btnAddBoss.TabIndex = 1;
        btnAddBoss.Text = "Add Boss";
        btnAddBoss.UseVisualStyleBackColor = true;
        btnAddBoss.Click += BtnAddBoss_Click;

        //
        // btnEditBoss
        //
        btnEditBoss.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnEditBoss.Location = new Point(141, 565);
        btnEditBoss.Name = "btnEditBoss";
        btnEditBoss.Size = new Size(120, 30);
        btnEditBoss.TabIndex = 2;
        btnEditBoss.Text = "Edit Boss";
        btnEditBoss.UseVisualStyleBackColor = true;
        btnEditBoss.Click += BtnEditBoss_Click;

        //
        // btnRemoveBoss
        //
        btnRemoveBoss.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnRemoveBoss.Location = new Point(267, 565);
        btnRemoveBoss.Name = "btnRemoveBoss";
        btnRemoveBoss.Size = new Size(120, 30);
        btnRemoveBoss.TabIndex = 3;
        btnRemoveBoss.Text = "Remove Boss";
        btnRemoveBoss.UseVisualStyleBackColor = true;
        btnRemoveBoss.Click += BtnRemoveBoss_Click;

        //
        // tabOutput
        //
        tabOutput.Controls.Add(grpOutput);
        tabOutput.Location = new Point(4, 24);
        tabOutput.Name = "tabOutput";
        tabOutput.Padding = new Padding(10);
        tabOutput.Size = new Size(992, 622);
        tabOutput.TabIndex = 2;
        tabOutput.Text = "Generate && Export";
        tabOutput.UseVisualStyleBackColor = true;

        //
        // grpOutput
        //
        grpOutput.Controls.Add(txtOutput);
        grpOutput.Controls.Add(btnGenerateProfile);
        grpOutput.Controls.Add(btnSaveJson);
        grpOutput.Controls.Add(btnLoadTemplate);
        grpOutput.Controls.Add(btnCopyCommand);
        grpOutput.Dock = DockStyle.Fill;
        grpOutput.Location = new Point(10, 10);
        grpOutput.Name = "grpOutput";
        grpOutput.Size = new Size(972, 602);
        grpOutput.TabIndex = 0;
        grpOutput.TabStop = false;
        grpOutput.Text = "Profile && Command Output";

        //
        // txtOutput
        //
        txtOutput.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
        txtOutput.Font = new Font("Consolas", 9F, FontStyle.Regular, GraphicsUnit.Point);
        txtOutput.Location = new Point(15, 25);
        txtOutput.Multiline = true;
        txtOutput.Name = "txtOutput";
        txtOutput.ReadOnly = true;
        txtOutput.ScrollBars = ScrollBars.Both;
        txtOutput.Size = new Size(942, 520);
        txtOutput.TabIndex = 0;
        txtOutput.WordWrap = false;

        //
        // btnGenerateProfile
        //
        btnGenerateProfile.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnGenerateProfile.BackColor = Color.FromArgb(0, 123, 255);
        btnGenerateProfile.FlatStyle = FlatStyle.Flat;
        btnGenerateProfile.ForeColor = Color.White;
        btnGenerateProfile.Location = new Point(15, 555);
        btnGenerateProfile.Name = "btnGenerateProfile";
        btnGenerateProfile.Size = new Size(150, 35);
        btnGenerateProfile.TabIndex = 1;
        btnGenerateProfile.Text = "Generate Profile";
        btnGenerateProfile.UseVisualStyleBackColor = false;
        btnGenerateProfile.Click += BtnGenerateProfile_Click;

        //
        // btnSaveJson
        //
        btnSaveJson.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnSaveJson.BackColor = Color.FromArgb(40, 167, 69);
        btnSaveJson.FlatStyle = FlatStyle.Flat;
        btnSaveJson.ForeColor = Color.White;
        btnSaveJson.Location = new Point(171, 555);
        btnSaveJson.Name = "btnSaveJson";
        btnSaveJson.Size = new Size(150, 35);
        btnSaveJson.TabIndex = 2;
        btnSaveJson.Text = "Save JSON";
        btnSaveJson.UseVisualStyleBackColor = false;
        btnSaveJson.Click += BtnSaveJson_Click;

        //
        // btnLoadTemplate
        //
        btnLoadTemplate.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnLoadTemplate.Location = new Point(327, 555);
        btnLoadTemplate.Name = "btnLoadTemplate";
        btnLoadTemplate.Size = new Size(150, 35);
        btnLoadTemplate.TabIndex = 3;
        btnLoadTemplate.Text = "Load Template";
        btnLoadTemplate.UseVisualStyleBackColor = true;
        btnLoadTemplate.Click += BtnLoadTemplate_Click;

        //
        // btnCopyCommand
        //
        btnCopyCommand.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
        btnCopyCommand.Location = new Point(483, 555);
        btnCopyCommand.Name = "btnCopyCommand";
        btnCopyCommand.Size = new Size(150, 35);
        btnCopyCommand.TabIndex = 4;
        btnCopyCommand.Text = "Copy Command";
        btnCopyCommand.UseVisualStyleBackColor = true;
        btnCopyCommand.Click += BtnCopyCommand_Click;

        //
        // lblStatus
        //
        lblStatus.Dock = DockStyle.Bottom;
        lblStatus.Location = new Point(0, 650);
        lblStatus.Name = "lblStatus";
        lblStatus.Padding = new Padding(5);
        lblStatus.Size = new Size(1000, 30);
        lblStatus.TabIndex = 1;
        lblStatus.Text = "Ready";

        //
        // Form1
        //
        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(1000, 680);
        Controls.Add(tabControl);
        Controls.Add(lblStatus);
        MinimumSize = new Size(900, 600);
        Name = "Form1";
        StartPosition = FormStartPosition.CenterScreen;
        Text = "OpenDBO Dungeon Generator";
        Load += Form1_Load;
        tabControl.ResumeLayout(false);
        tabBasic.ResumeLayout(false);
        tabBosses.ResumeLayout(false);
        tabOutput.ResumeLayout(false);
        grpBasicInfo.ResumeLayout(false);
        grpBasicInfo.PerformLayout();
        grpFloorSettings.ResumeLayout(false);
        grpFloorSettings.PerformLayout();
        grpBossDefaults.ResumeLayout(false);
        grpBossDefaults.PerformLayout();
        grpArenaReward.ResumeLayout(false);
        grpArenaReward.PerformLayout();
        grpBossList.ResumeLayout(false);
        grpOutput.ResumeLayout(false);
        grpOutput.PerformLayout();
        ((System.ComponentModel.ISupportInitialize)numWpsId).EndInit();
        ((System.ComponentModel.ISupportInitialize)numFloorCount).EndInit();
        ((System.ComponentModel.ISupportInitialize)numBossInterval).EndInit();
        ((System.ComponentModel.ISupportInitialize)numStartFloor).EndInit();
        ((System.ComponentModel.ISupportInitialize)numBossGroupBase).EndInit();
        ResumeLayout(false);
    }

    #endregion

    private TextBox txtDungeonName;
    private NumericUpDown numWpsId;
    private Button btnFindNextId;
    private TextBox txtDescription;
    private ComboBox cmbBaseWpsFile;
    private Button btnBrowseBaseWps;
    private NumericUpDown numFloorCount;
    private NumericUpDown numBossInterval;
    private NumericUpDown numStartFloor;
    private NumericUpDown numBossGroupBase;
    private ComboBox cmbMechanicsTemplate;
    private CheckBox chkIncrementBossGroup;
    private TextBox txtArenaRotation;
    private TextBox txtRewardItems;
    private CheckedListBox clbArenaOptions;
    private Button btnArenaApply;
    private Button btnArenaClear;
    private ListBox lstRewardPresets;
    private Button btnAddRewardPreset;
    private Button btnClearRewardPreset;
    private ListBox lstIndividualBosses;
    private Button btnAddBoss;
    private Button btnEditBoss;
    private Button btnRemoveBoss;
    private TextBox txtOutput;
    private Button btnGenerateProfile;
    private Button btnSaveJson;
    private Button btnLoadTemplate;
    private Button btnCopyCommand;
    private Label lblStatus;
}
