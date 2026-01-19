namespace MarkdownViewer
{
    partial class MainForm
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
            splitContainer1 = new System.Windows.Forms.SplitContainer();
            listBoxFiles = new System.Windows.Forms.ListBox();
            panel1 = new System.Windows.Forms.Panel();
            btnBrowseFolder = new System.Windows.Forms.Button();
            btnFindNext = new System.Windows.Forms.Button();
            btnFind = new System.Windows.Forms.Button();
            txtSearch = new System.Windows.Forms.TextBox();
            label1 = new System.Windows.Forms.Label();
            webView = new Microsoft.Web.WebView2.WinForms.WebView2();
            statusStrip1 = new System.Windows.Forms.StatusStrip();
            lblStatus = new System.Windows.Forms.ToolStripStatusLabel();
            ((System.ComponentModel.ISupportInitialize)splitContainer1).BeginInit();
            splitContainer1.Panel1.SuspendLayout();
            splitContainer1.Panel2.SuspendLayout();
            splitContainer1.SuspendLayout();
            panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)webView).BeginInit();
            statusStrip1.SuspendLayout();
            SuspendLayout();
            //
            // splitContainer1
            //
            splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            splitContainer1.Location = new System.Drawing.Point(0, 0);
            splitContainer1.Name = "splitContainer1";
            //
            // splitContainer1.Panel1
            //
            splitContainer1.Panel1.Controls.Add(listBoxFiles);
            splitContainer1.Panel1.Controls.Add(panel1);
            splitContainer1.Panel1MinSize = 200;
            //
            // splitContainer1.Panel2
            //
            splitContainer1.Panel2.Controls.Add(webView);
            splitContainer1.Size = new System.Drawing.Size(1200, 700);
            splitContainer1.SplitterDistance = 250;
            splitContainer1.TabIndex = 0;
            //
            // listBoxFiles
            //
            listBoxFiles.Dock = System.Windows.Forms.DockStyle.Fill;
            listBoxFiles.Font = new System.Drawing.Font("Segoe UI", 10F);
            listBoxFiles.FormattingEnabled = true;
            listBoxFiles.ItemHeight = 23;
            listBoxFiles.Location = new System.Drawing.Point(0, 115);
            listBoxFiles.Name = "listBoxFiles";
            listBoxFiles.Size = new System.Drawing.Size(250, 585);
            listBoxFiles.TabIndex = 0;
            listBoxFiles.SelectedIndexChanged += listBoxFiles_SelectedIndexChanged;
            //
            // panel1
            //
            panel1.Controls.Add(btnBrowseFolder);
            panel1.Controls.Add(btnFindNext);
            panel1.Controls.Add(btnFind);
            panel1.Controls.Add(txtSearch);
            panel1.Controls.Add(label1);
            panel1.Dock = System.Windows.Forms.DockStyle.Top;
            panel1.Location = new System.Drawing.Point(0, 0);
            panel1.Name = "panel1";
            panel1.Padding = new System.Windows.Forms.Padding(5);
            panel1.Size = new System.Drawing.Size(250, 115);
            panel1.TabIndex = 1;
            //
            // btnBrowseFolder
            //
            btnBrowseFolder.Location = new System.Drawing.Point(8, 82);
            btnBrowseFolder.Name = "btnBrowseFolder";
            btnBrowseFolder.Size = new System.Drawing.Size(234, 28);
            btnBrowseFolder.TabIndex = 4;
            btnBrowseFolder.Text = "Browse Folder...";
            btnBrowseFolder.UseVisualStyleBackColor = true;
            btnBrowseFolder.Click += btnBrowseFolder_Click;
            //
            // btnFindNext
            //
            btnFindNext.Enabled = false;
            btnFindNext.Location = new System.Drawing.Point(125, 50);
            btnFindNext.Name = "btnFindNext";
            btnFindNext.Size = new System.Drawing.Size(115, 25);
            btnFindNext.TabIndex = 3;
            btnFindNext.Text = "Find Next (F3)";
            btnFindNext.UseVisualStyleBackColor = true;
            btnFindNext.Click += btnFindNext_Click;
            //
            // btnFind
            //
            btnFind.Enabled = false;
            btnFind.Location = new System.Drawing.Point(8, 50);
            btnFind.Name = "btnFind";
            btnFind.Size = new System.Drawing.Size(111, 25);
            btnFind.TabIndex = 2;
            btnFind.Text = "Find";
            btnFind.UseVisualStyleBackColor = true;
            btnFind.Click += btnFind_Click;
            //
            // txtSearch
            //
            txtSearch.Enabled = false;
            txtSearch.Location = new System.Drawing.Point(8, 23);
            txtSearch.Name = "txtSearch";
            txtSearch.Size = new System.Drawing.Size(234, 23);
            txtSearch.TabIndex = 1;
            txtSearch.KeyDown += txtSearch_KeyDown;
            //
            // label1
            //
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(8, 5);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(45, 15);
            label1.TabIndex = 0;
            label1.Text = "Search:";
            //
            // webView
            //
            webView.AllowExternalDrop = true;
            webView.CreationProperties = null;
            webView.DefaultBackgroundColor = System.Drawing.Color.White;
            webView.Dock = System.Windows.Forms.DockStyle.Fill;
            webView.Location = new System.Drawing.Point(0, 0);
            webView.Name = "webView";
            webView.Size = new System.Drawing.Size(946, 700);
            webView.TabIndex = 0;
            webView.ZoomFactor = 1D;
            //
            // statusStrip1
            //
            statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] { lblStatus });
            statusStrip1.Location = new System.Drawing.Point(0, 700);
            statusStrip1.Name = "statusStrip1";
            statusStrip1.Size = new System.Drawing.Size(1200, 22);
            statusStrip1.TabIndex = 1;
            statusStrip1.Text = "statusStrip1";
            //
            // lblStatus
            //
            lblStatus.Name = "lblStatus";
            lblStatus.Size = new System.Drawing.Size(39, 17);
            lblStatus.Text = "Ready";
            //
            // MainForm
            //
            AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            ClientSize = new System.Drawing.Size(1200, 722);
            Controls.Add(splitContainer1);
            Controls.Add(statusStrip1);
            KeyPreview = true;
            Name = "MainForm";
            StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            Text = "DBO Markdown Viewer";
            KeyDown += MainForm_KeyDown;
            splitContainer1.Panel1.ResumeLayout(false);
            splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainer1).EndInit();
            splitContainer1.ResumeLayout(false);
            panel1.ResumeLayout(false);
            panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)webView).EndInit();
            statusStrip1.ResumeLayout(false);
            statusStrip1.PerformLayout();
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.ListBox listBoxFiles;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Button btnBrowseFolder;
        private System.Windows.Forms.Button btnFindNext;
        private System.Windows.Forms.Button btnFind;
        private System.Windows.Forms.TextBox txtSearch;
        private System.Windows.Forms.Label label1;
        private Microsoft.Web.WebView2.WinForms.WebView2 webView;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel lblStatus;
    }
}
