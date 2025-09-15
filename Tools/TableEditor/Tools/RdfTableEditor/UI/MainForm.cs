using System;
using System.IO;
using System.Text;
using System.Windows.Forms;
using RdfTableEditor.Model;
using RdfTableEditor.Model.Schema;

namespace RdfTableEditor.UI
{
    public partial class MainForm : Form
    {
        private readonly OpenFileDialog _openDlg = new OpenFileDialog { Filter = "RDF Tables (*.rdf)|*.rdf|All files (*.*)|*.*" };
        private readonly SaveFileDialog _saveDlg = new SaveFileDialog { Filter = "RDF Tables (*.rdf)|*.rdf|All files (*.*)|*.*" };
    private Model.RdfDocument? _doc;
    private string? _currentPath;
    private bool _lastWasDecrypted;
    private readonly System.Windows.Forms.Timer _searchTimer = new System.Windows.Forms.Timer();

        public MainForm()
        {
            InitializeComponent();
            Text = "RDF Table Editor";
            // Start in a windowed size (not fullscreen) and apply dark theme
            WindowState = FormWindowState.Normal;
            Width = 1200;
            Height = 800;
            Theme.ApplyDarkTheme(this);
            // Debounce for live search
            _searchTimer.Interval = 150; // ms
            _searchTimer.Tick += (_, __) => { _searchTimer.Stop(); ApplySearch(); };
        }

        private void OnOpen(object? sender, EventArgs e)
        {
            if (_openDlg.ShowDialog(this) == DialogResult.OK)
            {
                // Try schema-based binary first
                var schema = TableRegistry.FromFilename(_openDlg.FileName);
                bool usedDecrypt = false;
                if (schema != null)
                {
                    try
                    {
                        var raw = File.ReadAllBytes(_openDlg.FileName);
                        RdfDocument? docRaw = null;
                        RdfDocument? docDec = null;
                        // Try raw first
                        try
                        {
                            using var msRaw = new MemoryStream(raw);
                            docRaw = BinaryTableIO.Read(msRaw, schema);
                        }
                        catch { docRaw = null; }

                        // Try decrypt
                        if (RdfCrypto.TryDecrypt(raw, out var dec))
                        {
                            try
                            {
                                using var msDec = new MemoryStream(dec);
                                docDec = BinaryTableIO.Read(msDec, schema);
                            }
                            catch { docDec = null; }
                        }

                        // Choose the one with more rows; for o_table_* and certain schemas, prefer decrypted when tie
                        int rawCount = docRaw?.Rows.Count ?? 0;
                        int decCount = docDec?.Rows.Count ?? 0;
                        var fileNameOnly = Path.GetFileName(_openDlg.FileName);
                        bool preferDec = fileNameOnly.StartsWith("o_table_", StringComparison.OrdinalIgnoreCase)
                                         || string.Equals(schema.Name, "QuestText", StringComparison.OrdinalIgnoreCase)
                                         || string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase);

                        if (decCount > rawCount || (preferDec && docDec != null && decCount >= rawCount))
                        { _doc = docDec; usedDecrypt = true; }
                        else
                        { _doc = docRaw ?? docDec; usedDecrypt = _doc == docDec && _doc != null; }
                        if (_doc == null)
                            throw new InvalidOperationException("Unable to parse this table (raw or decrypted).");

                        // If both produce 0 rows for a file we expect encrypted, keep decrypted and show a hint
                        if ((_doc.Rows.Count == 0) && preferDec && docDec != null)
                        {
                            _doc = docDec; usedDecrypt = true;
                            toolStripStatusLabel1.Text = "Parsed 0 rows; using decrypted bytes due to schema/file hint.";
                            // If schema suggests QuestText and still 0 rows, use the dedicated reader
                            if (string.Equals(schema.Name, "QuestText", StringComparison.OrdinalIgnoreCase))
                            {
                                try
                                {
                                    using var msDec2 = new MemoryStream(RdfCrypto.TryDecrypt(raw, out var dec2) ? dec2 : raw);
                                    using var br = new BinaryReader(msDec2, Encoding.UTF8, leaveOpen: true);
                                    var qDoc = BinaryTableIO.ReadQuestText(br);
                                    if (qDoc.Rows.Count > 0)
                                        _doc = qDoc;
                                }
                                catch { /* ignore and leave as empty */ }
                            }
                        }
                        // remember if decrypted was used
                        _lastWasDecrypted = usedDecrypt;
                    }
                    catch (Exception ex)
                    {
                        _doc = null;
                        MessageBox.Show(this, ex.Message, "Open failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
                // Fallback to legacy binary (CharTitle-specific) then CSV
                if (_doc == null)
                {
                    try
                    {
                        using var fs = File.OpenRead(_openDlg.FileName);
                        _doc = RdfParser.Parse(fs);
                    }
                    catch (Exception ex)
                    {
                        var text = File.ReadAllText(_openDlg.FileName, Encoding.UTF8);
                        _doc = RdfParser.Parse(text);
                        toolStripStatusLabel1.Text = $"Opened as CSV fallback; error: {ex.Message}";
                    }
                }
                _currentPath = _openDlg.FileName;
                tableView1.SetDocument(_doc, schema);
                ApplySearch();
                UpdateRowInfo();
                var schemaName = schema?.Name ?? "(none)";
                toolStripStatusLabel1.Text = $"Loaded: {Path.GetFileName(_currentPath)} | Schema={schemaName} | Decrypted={(usedDecrypt ? "yes" : "no")}";
            }
        }

        private void OnSave(object? sender, EventArgs e)
        {
            if (_doc == null)
            {
                MessageBox.Show(this, "Nothing to save.");
                return;
            }

            if (string.IsNullOrEmpty(_currentPath))
            {
                OnSaveAs(sender, e);
                return;
            }

            try
            {
                using var fs = File.Create(_currentPath!);
                var schema = TableRegistry.FromFilename(_currentPath!);
                // Ensure in-grid edits are materialized back into _doc for schema-driven tables
                tableView1.SyncBackToDocument(schema);
                if (schema != null)
                {
                    // Write to memory then (re-)encrypt if needed
                    using var ms = new MemoryStream();
                    BinaryTableIO.Write(ms, schema, _doc);
                    var bytes = ms.ToArray();
                    var fileNameOnly = Path.GetFileName(_currentPath);
                    bool shouldEncrypt = fileNameOnly.StartsWith("o_table_", StringComparison.OrdinalIgnoreCase) || _lastWasDecrypted;
                    if (shouldEncrypt && RdfCrypto.TryEncrypt(bytes, out var enc))
                        fs.Write(enc, 0, enc.Length);
                    else
                        fs.Write(bytes, 0, bytes.Length);
                }
                else
                {
                    RdfSerializer.Serialize(_doc, fs);
                }
                UpdateRowInfo();
                toolStripStatusLabel1.Text = $"Saved: {Path.GetFileName(_currentPath)}";
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Save failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void OnSaveAs(object? sender, EventArgs e)
        {
            if (_doc == null)
            {
                MessageBox.Show(this, "Nothing to save.");
                return;
            }

            if (_saveDlg.ShowDialog(this) == DialogResult.OK)
            {
                _currentPath = _saveDlg.FileName;
                OnSave(sender, e);
            }
        }

        private void OnNew(object? sender, EventArgs e)
        {
            _doc = new Model.RdfDocument();
            _currentPath = null;
            tableView1.SetDocument(_doc);
            toolStripStatusLabel1.Text = "New document";
            UpdateRowInfo();
        }

        private void OnExit(object? sender, EventArgs e)
        {
            Close();
        }

        private void ApplySearch()
        {
            var text = toolStripSearchBox.Text ?? string.Empty;
            tableView1.ApplyQuickFilter(text);
            UpdateRowInfo();
        }

        private void OnSearchTextChanged(object? sender, EventArgs e)
        {
            _searchTimer.Stop();
            _searchTimer.Start();
        }

        private void UpdateRowInfo()
        {
            if (_doc == null) { toolStripRowInfo.Text = string.Empty; return; }
            var visible = tableView1.VisibleRowCount;
            toolStripRowInfo.Text = $"Rows: {visible} / {_doc.Rows.Count}";
        }
    }
}
