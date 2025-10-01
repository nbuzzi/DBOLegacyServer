using System;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using Markdig;

namespace MarkdownViewer
{
    public partial class MainForm : Form
    {
        private readonly MarkdownPipeline _pipeline;
        private string _currentContent = string.Empty;
        private string _currentFilePath = string.Empty;
        private int _lastSearchIndex = -1;
        private bool _webViewInitialized = false;
        private string _pendingFilePath = string.Empty;

        public MainForm()
        {
            InitializeComponent();
            _pipeline = new MarkdownPipelineBuilder()
                .UseAdvancedExtensions()
                .UseEmojiAndSmiley()
                .UseTaskLists()
                .Build();

            InitializeWebView();
            LoadDefaultReadmes();
        }

        private async void InitializeWebView()
        {
            try
            {
                await webView.EnsureCoreWebView2Async(null);
                _webViewInitialized = true;

                // If there was a pending file to load, load it now
                if (!string.IsNullOrEmpty(_pendingFilePath))
                {
                    LoadMarkdownFile(_pendingFilePath);
                    _pendingFilePath = string.Empty;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error initializing web view: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void LoadDefaultReadmes()
        {
            try
            {
                // Try to load from config file first
                string configFile = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "config.txt");
                string configPath = string.Empty;

                if (File.Exists(configFile))
                {
                    configPath = File.ReadAllText(configFile).Trim();
                }

                // If no config or path doesn't exist, try default locations
                if (string.IsNullOrEmpty(configPath) || !Directory.Exists(configPath))
                {
                    // First try Documentation folder next to executable (for published builds)
                    configPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Documentation");

                    // If not found, try relative path from Tools/MarkdownViewer (for development)
                    if (!Directory.Exists(configPath))
                    {
                        configPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "..", "..", "DboServer", "Documentation");
                        configPath = Path.GetFullPath(configPath);
                    }
                }

                if (Directory.Exists(configPath))
                {
                    LoadReadmesFromPath(configPath);
                }
                else
                {
                    lblStatus.Text = "README folder not found. Click 'Browse Folder' to select a path.";
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading READMEs: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void LoadReadmesFromPath(string path)
        {
            listBoxFiles.Items.Clear();

            // Search for all .md files recursively
            var mdFiles = Directory.GetFiles(path, "*.md", SearchOption.AllDirectories);

            foreach (var file in mdFiles.OrderBy(f => f))
            {
                // Create a relative path display name
                string relativePath = file.Substring(path.Length).TrimStart('\\', '/');
                listBoxFiles.Items.Add(new FileItem { FilePath = file, DisplayName = relativePath });
            }

            if (listBoxFiles.Items.Count > 0)
            {
                listBoxFiles.SelectedIndex = 0;
                lblStatus.Text = $"Loaded {listBoxFiles.Items.Count} markdown file(s) from: {path}";
            }
            else
            {
                lblStatus.Text = $"No markdown files found in: {path}";
            }
        }

        private void BrowseFolder()
        {
            using (var folderDialog = new FolderBrowserDialog())
            {
                folderDialog.Description = "Select folder containing README files";
                folderDialog.ShowNewFolderButton = false;

                if (folderDialog.ShowDialog() == DialogResult.OK)
                {
                    string selectedPath = folderDialog.SelectedPath;

                    // Save to config file
                    string configFile = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "config.txt");
                    File.WriteAllText(configFile, selectedPath);

                    LoadReadmesFromPath(selectedPath);
                }
            }
        }

        private void LoadMarkdownFile(string filePath)
        {
            try
            {
                _currentFilePath = filePath;
                _currentContent = File.ReadAllText(filePath);

                // Check if WebView is initialized
                if (!_webViewInitialized)
                {
                    _pendingFilePath = filePath;
                    lblStatus.Text = "Initializing viewer...";
                    return;
                }

                // Convert markdown to HTML
                string html = Markdown.ToHtml(_currentContent, _pipeline);

                // Wrap in nice HTML template
                string styledHtml = $@"
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }}
        h1 {{
            color: #2c3e50;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }}
        h2 {{
            color: #34495e;
            border-bottom: 2px solid #95a5a6;
            padding-bottom: 8px;
            margin-top: 30px;
        }}
        h3 {{
            color: #7f8c8d;
            margin-top: 25px;
        }}
        code {{
            background-color: #f4f4f4;
            border: 1px solid #ddd;
            border-radius: 3px;
            padding: 2px 5px;
            font-family: 'Consolas', 'Monaco', monospace;
            color: #c7254e;
        }}
        pre {{
            background-color: #2d2d2d;
            color: #f8f8f2;
            border-radius: 5px;
            padding: 15px;
            overflow-x: auto;
        }}
        pre code {{
            background: transparent;
            border: none;
            color: #f8f8f2;
            padding: 0;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 20px 0;
            background: white;
        }}
        th {{
            background-color: #3498db;
            color: white;
            padding: 12px;
            text-align: left;
        }}
        td {{
            border: 1px solid #ddd;
            padding: 10px;
        }}
        tr:nth-child(even) {{
            background-color: #f2f2f2;
        }}
        blockquote {{
            border-left: 4px solid #3498db;
            padding-left: 15px;
            color: #666;
            font-style: italic;
            margin: 20px 0;
        }}
        ul, ol {{
            margin: 15px 0;
            padding-left: 30px;
        }}
        li {{
            margin: 8px 0;
        }}
        a {{
            color: #3498db;
            text-decoration: none;
        }}
        a:hover {{
            text-decoration: underline;
        }}
        .highlight {{
            background-color: yellow;
            padding: 2px 4px;
        }}
        hr {{
            border: none;
            border-top: 2px solid #ecf0f1;
            margin: 30px 0;
        }}
    </style>
</head>
<body>
{html}
</body>
</html>";

                webView.NavigateToString(styledHtml);

                // Update status
                lblStatus.Text = $"Loaded: {Path.GetFileName(filePath)} ({_currentContent.Length} chars)";

                // Enable search
                txtSearch.Enabled = true;
                btnFind.Enabled = true;
                btnFindNext.Enabled = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading file: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void PerformSearch(bool findNext = false)
        {
            string searchText = txtSearch.Text;
            if (string.IsNullOrWhiteSpace(searchText))
            {
                MessageBox.Show("Please enter search text.", "Search", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (string.IsNullOrEmpty(_currentContent))
                return;

            int startIndex = findNext ? _lastSearchIndex + 1 : 0;

            int foundIndex = _currentContent.IndexOf(searchText, startIndex, StringComparison.OrdinalIgnoreCase);

            if (foundIndex == -1 && findNext && _lastSearchIndex != -1)
            {
                // Wrap around to beginning
                foundIndex = _currentContent.IndexOf(searchText, 0, StringComparison.OrdinalIgnoreCase);
                if (foundIndex != -1)
                {
                    MessageBox.Show("Search wrapped to beginning.", "Search", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }

            if (foundIndex != -1)
            {
                _lastSearchIndex = foundIndex;

                // Highlight the found text in HTML
                string beforeText = _currentContent.Substring(0, foundIndex);
                string foundText = _currentContent.Substring(foundIndex, searchText.Length);
                string afterText = _currentContent.Substring(foundIndex + searchText.Length);

                // Convert to HTML with highlight
                string htmlBefore = Markdown.ToHtml(beforeText, _pipeline);
                string htmlAfter = Markdown.ToHtml(afterText, _pipeline);
                string highlightedHtml = $"{htmlBefore}<span class='highlight' id='searchMatch'>{System.Security.SecurityElement.Escape(foundText)}</span>{htmlAfter}";

                string styledHtml = $@"
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }}
        h1, h2, h3 {{ color: #2c3e50; }}
        code {{
            background-color: #f4f4f4;
            border: 1px solid #ddd;
            border-radius: 3px;
            padding: 2px 5px;
            font-family: 'Consolas', 'Monaco', monospace;
        }}
        pre {{
            background-color: #2d2d2d;
            color: #f8f8f2;
            border-radius: 5px;
            padding: 15px;
            overflow-x: auto;
        }}
        .highlight {{
            background-color: #ffeb3b;
            padding: 3px 6px;
            border-radius: 3px;
            font-weight: bold;
        }}
    </style>
</head>
<body>
{highlightedHtml}
<script>
    document.getElementById('searchMatch')?.scrollIntoView({{ behavior: 'smooth', block: 'center' }});
</script>
</body>
</html>";

                webView.NavigateToString(styledHtml);
                lblStatus.Text = $"Found at position {foundIndex} - Press F3 or Find Next for next occurrence";
            }
            else
            {
                MessageBox.Show($"'{searchText}' not found.", "Search", MessageBoxButtons.OK, MessageBoxIcon.Information);
                _lastSearchIndex = -1;
            }
        }

        private void listBoxFiles_SelectedIndexChanged(object? sender, EventArgs e)
        {
            if (listBoxFiles.SelectedItem is FileItem fileItem)
            {
                LoadMarkdownFile(fileItem.FilePath);
            }
        }

        private void btnFind_Click(object? sender, EventArgs e)
        {
            PerformSearch(findNext: false);
        }

        private void btnFindNext_Click(object? sender, EventArgs e)
        {
            PerformSearch(findNext: true);
        }

        private void txtSearch_KeyDown(object? sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                PerformSearch(findNext: false);
            }
            else if (e.KeyCode == Keys.F3)
            {
                e.SuppressKeyPress = true;
                PerformSearch(findNext: true);
            }
        }

        private void MainForm_KeyDown(object? sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F3)
            {
                e.SuppressKeyPress = true;
                PerformSearch(findNext: true);
            }
        }

        private void btnBrowseFolder_Click(object? sender, EventArgs e)
        {
            BrowseFolder();
        }

        private class FileItem
        {
            public string FilePath { get; set; } = string.Empty;
            public string DisplayName { get; set; } = string.Empty;

            public override string ToString() => DisplayName;
        }
    }
}
