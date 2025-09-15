using System.Windows.Forms;

namespace RdfTableEditor.UI
{
    partial class MainForm
    {
        private MenuStrip menuStrip1;
    private ToolStrip topToolStrip;
        private ToolStripMenuItem fileToolStripMenuItem;
        private ToolStripMenuItem newToolStripMenuItem;
        private ToolStripMenuItem openToolStripMenuItem;
        private ToolStripMenuItem saveToolStripMenuItem;
        private ToolStripMenuItem saveAsToolStripMenuItem;
        private ToolStripMenuItem exitToolStripMenuItem;
    private ToolStripMenuItem exportXmlToolStripMenuItem;
    private ToolStripMenuItem exportEdfToolStripMenuItem;
        private ToolStripMenuItem editToolStripMenuItem;
        private ToolStripMenuItem addRowToolStripMenuItem;
        private ToolStripMenuItem cloneRowToolStripMenuItem;
        private ToolStripMenuItem deleteRowToolStripMenuItem;
        private StatusStrip statusStrip1;
        private ToolStripStatusLabel toolStripStatusLabel1;
    private ToolStripStatusLabel toolStripRowInfo;
    private ToolStripTextBox toolStripSearchBox;
    private ToolStripButton toolStripOpenBtn;
    private ToolStripButton toolStripSaveBtn;
    private ToolStripButton toolStripSaveAsBtn;
    private ToolStripButton toolStripReloadBtn;
    private ToolStripButton toolStripAddBtn;
    private ToolStripButton toolStripCloneBtn;
    private ToolStripButton toolStripDeleteBtn;
        private Views.TableView tableView1;
    private SaveFileDialog exportXmlDialog;
    private SaveFileDialog exportEdfDialog;

        private void InitializeComponent()
        {
            menuStrip1 = new MenuStrip();
            fileToolStripMenuItem = new ToolStripMenuItem();
            newToolStripMenuItem = new ToolStripMenuItem();
            openToolStripMenuItem = new ToolStripMenuItem();
            saveToolStripMenuItem = new ToolStripMenuItem();
            saveAsToolStripMenuItem = new ToolStripMenuItem();
            exitToolStripMenuItem = new ToolStripMenuItem();
            statusStrip1 = new StatusStrip();
            toolStripStatusLabel1 = new ToolStripStatusLabel();
            toolStripRowInfo = new ToolStripStatusLabel();
            topToolStrip = new ToolStrip();
            toolStripOpenBtn = new ToolStripButton();
            toolStripSaveBtn = new ToolStripButton();
            toolStripSaveAsBtn = new ToolStripButton();
            toolStripReloadBtn = new ToolStripButton();
            toolStripSearchBox = new ToolStripTextBox();
            tableView1 = new Views.TableView();
            menuStrip1.SuspendLayout();
            statusStrip1.SuspendLayout();
            topToolStrip.SuspendLayout();
            SuspendLayout();
            // 
            // menuStrip1
            // 
            // Ensure Edit menu is instantiated before adding to the menu strip
            editToolStripMenuItem = new ToolStripMenuItem();
            addRowToolStripMenuItem = new ToolStripMenuItem();
            cloneRowToolStripMenuItem = new ToolStripMenuItem();
            deleteRowToolStripMenuItem = new ToolStripMenuItem();
            menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, editToolStripMenuItem });
            menuStrip1.Location = new System.Drawing.Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Size = new System.Drawing.Size(1000, 24);
            menuStrip1.TabIndex = 0;
            menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            exportXmlDialog = new SaveFileDialog { Filter = "XML Files (*.xml)|*.xml|All files (*.*)|*.*", DefaultExt = "xml" };
            exportEdfDialog = new SaveFileDialog { Filter = "EDF Files (*.edf)|*.edf|All files (*.*)|*.*", DefaultExt = "edf" };
            exportXmlToolStripMenuItem = new ToolStripMenuItem();
            exportEdfToolStripMenuItem = new ToolStripMenuItem();
            fileToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { newToolStripMenuItem, openToolStripMenuItem, saveToolStripMenuItem, saveAsToolStripMenuItem, new ToolStripSeparator(), exportXmlToolStripMenuItem, exportEdfToolStripMenuItem, new ToolStripSeparator(), exitToolStripMenuItem });
            fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            fileToolStripMenuItem.Text = "File";
            // 
            // exportXmlToolStripMenuItem
            // 
            exportXmlToolStripMenuItem.Name = "exportXmlToolStripMenuItem";
            exportXmlToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            exportXmlToolStripMenuItem.Text = "Export XML…";
            exportXmlToolStripMenuItem.Click += OnExportXml;
            // 
            // exportEdfToolStripMenuItem
            // 
            exportEdfToolStripMenuItem.Name = "exportEdfToolStripMenuItem";
            exportEdfToolStripMenuItem.Size = new System.Drawing.Size(180, 22);
            exportEdfToolStripMenuItem.Text = "Export EDF…";
            exportEdfToolStripMenuItem.Click += OnExportEdf;
            // 
            // editToolStripMenuItem
            // 
            editToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[]
            {
                addRowToolStripMenuItem,
                cloneRowToolStripMenuItem,
                deleteRowToolStripMenuItem
            });
            editToolStripMenuItem.Name = "editToolStripMenuItem";
            editToolStripMenuItem.Size = new System.Drawing.Size(39, 20);
            editToolStripMenuItem.Text = "Edit";
            // 
            // addRowToolStripMenuItem
            // 
            addRowToolStripMenuItem.Name = "addRowToolStripMenuItem";
            addRowToolStripMenuItem.Size = new System.Drawing.Size(138, 22);
            addRowToolStripMenuItem.Text = "Add Row";
            addRowToolStripMenuItem.ShortcutKeys = Keys.Insert;
            addRowToolStripMenuItem.ShowShortcutKeys = true;
            addRowToolStripMenuItem.Click += OnAddRow;
            // 
            // cloneRowToolStripMenuItem
            // 
            cloneRowToolStripMenuItem.Name = "cloneRowToolStripMenuItem";
            cloneRowToolStripMenuItem.Size = new System.Drawing.Size(138, 22);
            cloneRowToolStripMenuItem.Text = "Clone Row";
            cloneRowToolStripMenuItem.ShortcutKeys = Keys.Control | Keys.D;
            cloneRowToolStripMenuItem.ShowShortcutKeys = true;
            cloneRowToolStripMenuItem.Click += OnCloneRow;
            // 
            // deleteRowToolStripMenuItem
            // 
            deleteRowToolStripMenuItem.Name = "deleteRowToolStripMenuItem";
            deleteRowToolStripMenuItem.Size = new System.Drawing.Size(138, 22);
            deleteRowToolStripMenuItem.Text = "Delete Row";
            deleteRowToolStripMenuItem.ShortcutKeys = Keys.Delete;
            deleteRowToolStripMenuItem.ShowShortcutKeys = true;
            deleteRowToolStripMenuItem.Click += OnDeleteRow;
            // 
            // newToolStripMenuItem
            // 
            newToolStripMenuItem.Name = "newToolStripMenuItem";
            newToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
            newToolStripMenuItem.Text = "New";
            newToolStripMenuItem.Click += OnNew;
            // 
            // openToolStripMenuItem
            // 
            openToolStripMenuItem.Name = "openToolStripMenuItem";
            openToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
            openToolStripMenuItem.Text = "Open…";
            openToolStripMenuItem.Click += OnOpen;
            // 
            // saveToolStripMenuItem
            // 
            saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            saveToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
            saveToolStripMenuItem.Text = "Save";
            saveToolStripMenuItem.Click += OnSave;
            // 
            // saveAsToolStripMenuItem
            // 
            saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            saveAsToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
            saveAsToolStripMenuItem.Text = "Save As…";
            saveAsToolStripMenuItem.Click += OnSaveAs;
            // 
            // exitToolStripMenuItem
            // 
            exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            exitToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
            exitToolStripMenuItem.Text = "Exit";
            exitToolStripMenuItem.Click += OnExit;
            // 
            // statusStrip1
            // 
            statusStrip1.Items.AddRange(new ToolStripItem[] { toolStripStatusLabel1, new ToolStripSpringLabel(), toolStripRowInfo });
            statusStrip1.Location = new System.Drawing.Point(0, 578);
            statusStrip1.Name = "statusStrip1";
            statusStrip1.Size = new System.Drawing.Size(1000, 22);
            statusStrip1.TabIndex = 1;
            statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            toolStripStatusLabel1.Size = new System.Drawing.Size(39, 17);
            toolStripStatusLabel1.Text = "Ready";

            // Row info label
            toolStripRowInfo.Name = "toolStripRowInfo";
            toolStripRowInfo.Size = new System.Drawing.Size(0, 17);

            // topToolStrip
            topToolStrip.GripStyle = ToolStripGripStyle.Hidden;
            topToolStrip.ImageScalingSize = new System.Drawing.Size(16, 16);
            // Pre-create edit buttons so AddRange receives non-null instances
            toolStripAddBtn = new ToolStripButton();
            toolStripCloneBtn = new ToolStripButton();
            toolStripDeleteBtn = new ToolStripButton();
            topToolStrip.Items.AddRange(new ToolStripItem[]
            {
                toolStripOpenBtn,
                toolStripSaveBtn,
                toolStripSaveAsBtn,
                new ToolStripSeparator(),
                toolStripReloadBtn,
                new ToolStripSeparator(),
                toolStripAddBtn,
                toolStripCloneBtn,
                toolStripDeleteBtn,
                new ToolStripSpringLabel(),
                new ToolStripLabel("Search:"),
                toolStripSearchBox
            });
            topToolStrip.Location = new System.Drawing.Point(0, 24);
            topToolStrip.Name = "topToolStrip";
            topToolStrip.Padding = new System.Windows.Forms.Padding(6,0,6,0);
            topToolStrip.Size = new System.Drawing.Size(1000, 27);
            topToolStrip.TabIndex = 3;

            // Open button
            toolStripOpenBtn.Text = "Open";
            toolStripOpenBtn.Click += OnOpen;
            // Save button
            toolStripSaveBtn.Text = "Save";
            toolStripSaveBtn.Click += OnSave;
            // Save As button
            toolStripSaveAsBtn.Text = "Save As";
            toolStripSaveAsBtn.Click += OnSaveAs;
            // Reload button
            toolStripReloadBtn.Text = "Reload";
            toolStripReloadBtn.Click += (s,e)=> OnOpen(s,e);
            // Add Row button (instance created above)
            toolStripAddBtn.Text = "Add";
            toolStripAddBtn.ToolTipText = "Add new row (Insert)";
            toolStripAddBtn.Click += OnAddRow;
            // Clone Row button (instance created above)
            toolStripCloneBtn.Text = "Clone";
            toolStripCloneBtn.ToolTipText = "Clone selected row(s) (Ctrl+D)";
            toolStripCloneBtn.Click += OnCloneRow;
            // Delete Row button (instance created above)
            toolStripDeleteBtn.Text = "Delete";
            toolStripDeleteBtn.ToolTipText = "Delete selected row(s) (Del)";
            toolStripDeleteBtn.Click += OnDeleteRow;
            // Search box
            toolStripSearchBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            toolStripSearchBox.AutoSize = false;
            toolStripSearchBox.Width = 260;
            toolStripSearchBox.ToolTipText = "Type to filter rows";
            toolStripSearchBox.TextChanged += OnSearchTextChanged;
            // 
            // tableView1
            // 
            tableView1.Dock = DockStyle.Fill;
            tableView1.Location = new System.Drawing.Point(0, 51);
            tableView1.Name = "tableView1";
            tableView1.Size = new System.Drawing.Size(1000, 527);
            tableView1.TabIndex = 2;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new System.Drawing.Size(1000, 600);
            Controls.Add(tableView1);
            Controls.Add(topToolStrip);
            Controls.Add(statusStrip1);
            Controls.Add(menuStrip1);
            MainMenuStrip = menuStrip1;
            Name = "MainForm";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "RDF Table Editor";
            // WindowState set in code-behind to Normal
            menuStrip1.ResumeLayout(false);
            menuStrip1.PerformLayout();
            statusStrip1.ResumeLayout(false);
            statusStrip1.PerformLayout();
            topToolStrip.ResumeLayout(false);
            topToolStrip.PerformLayout();
            ResumeLayout(false);
            PerformLayout();
        }
    }
}
