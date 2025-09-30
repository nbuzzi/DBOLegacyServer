using System;
using System.IO;
using System.Text;
using System.Windows.Forms;
using RdfTableEditor.Model;
using RdfTableEditor.Model.Schema;
using Exporters = RdfTableEditor.Model.Exporters;

namespace RdfTableEditor.UI
{
    public partial class MainForm : Form
    {
        private readonly OpenFileDialog _openDlg = new OpenFileDialog { Filter = "RDF Tables (*.rdf)|*.rdf|All files (*.*)|*.*" };
        private readonly SaveFileDialog _saveDlg = new SaveFileDialog { Filter = "RDF Tables (*.rdf)|*.rdf|All files (*.*)|*.*" };
        private Model.RdfDocument? _doc;
        private string? _currentPath;
        private bool _lastWasDecrypted;
        private bool _originalWasEncrypted;
        private readonly System.Windows.Forms.Timer _searchTimer = new System.Windows.Forms.Timer();
        // Debounce for live search
        private TableSchema? _currentSchema; // schema actually used to read the document

        public MainForm()
        {
            InitializeComponent();
            Text = "RDF Table Editor";
            // Start in a windowed size (not fullscreen) and apply dark theme
            WindowState = FormWindowState.Normal;
            Width = 1200;
            Height = 800;
            Theme.ApplyDarkTheme(this);

            _searchTimer.Interval = 150; // ms
            _searchTimer.Tick += (_, __) => { _searchTimer.Stop(); ApplySearch(); };
        }

        private void OnOpen(object? sender, EventArgs e)
        {
            if (_openDlg.ShowDialog(this) != DialogResult.OK) return;

            var schema = TableRegistry.FromFilename(_openDlg.FileName);
            bool usedDecrypt = false;
            _originalWasEncrypted = false;
            _doc = null;
            _currentSchema = null;

            if (schema != null)
            {
                try
                {
                    var raw = File.ReadAllBytes(_openDlg.FileName);
                    RdfDocument? docRaw = null, docDec = null;
                    try { using var msRaw = new MemoryStream(raw); docRaw = BinaryTableIO.Read(msRaw, schema); } catch { }
                    if (RdfCrypto.TryDecrypt(raw, out var dec))
                    {
                        try { using var msDec = new MemoryStream(dec); docDec = BinaryTableIO.Read(msDec, schema); } catch { }
                    }
                    int rawCount = docRaw?.Rows.Count ?? 0;
                    int decCount = docDec?.Rows.Count ?? 0;

                    if (docDec != null && decCount > rawCount) { _doc = docDec; usedDecrypt = true; _originalWasEncrypted = true; }
                    else if (docRaw != null) { _doc = docRaw; usedDecrypt = false; _originalWasEncrypted = false; }
                    else if (docDec != null) { _doc = docDec; usedDecrypt = true; _originalWasEncrypted = true; }
                    else throw new InvalidOperationException("Unable to parse this table (raw or decrypted).");

                    if (string.Equals(schema.Name, "QuestText", StringComparison.OrdinalIgnoreCase) && docDec != null && docDec.Rows.Count > 0)
                    { _doc = docDec; usedDecrypt = true; _originalWasEncrypted = true; }

                    // If schema-based parse produced 0 rows, try the legacy reader as a safety net
                    if ((_doc?.Rows.Count ?? 0) == 0)
                    {
                        try
                        {
                            using var fs2 = File.OpenRead(_openDlg.FileName);
                            var legacy = RdfParser.Parse(fs2);
                            if (legacy.Rows.Count > 0)
                            {
                                _doc = legacy;
                                _currentSchema = null; // legacy reader isn't schema-driven
                                usedDecrypt = false; // legacy reader uses raw stream
                                toolStripStatusLabel1.Text = "Parsed via legacy reader fallback (schema parse yielded 0 rows).";
                            }
                        }
                        catch { }
                    }

                    _lastWasDecrypted = usedDecrypt;
                    if (_doc != null && _doc.Rows.Count > 0 && toolStripStatusLabel1.Text.IndexOf("legacy reader", StringComparison.OrdinalIgnoreCase) < 0)
                        _currentSchema = schema;
                    else if (_doc != null && _doc.Rows.Count == 0)
                        _currentSchema = schema;
                }
                catch (Exception ex)
                {
                    _doc = null;
                    MessageBox.Show(this, ex.Message, "Open failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

            // Fallback to legacy binary or CSV if schema parsing failed entirely
            if (_doc == null)
            {
                try
                {
                    using var fs = File.OpenRead(_openDlg.FileName);
                    _doc = RdfParser.Parse(fs);
                    _currentSchema = null;
                }
                catch (Exception ex)
                {
                    var text = File.ReadAllText(_openDlg.FileName, Encoding.UTF8);
                    _doc = RdfParser.Parse(text);
                    toolStripStatusLabel1.Text = $"Opened as CSV fallback; error: {ex.Message}";
                    _currentSchema = null;
                }
            }

            _currentPath = _openDlg.FileName;
            tableView1.SetDocument(_doc, _currentSchema);
            ApplySearch();
            UpdateRowInfo();
            var schemaName = _currentSchema?.Name ?? "(none)";
            toolStripStatusLabel1.Text = $"Loaded: {Path.GetFileName(_currentPath)} | Schema={schemaName} | Decrypted={(usedDecrypt ? "yes" : "no")}";
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
                var schema = _currentSchema ?? TableRegistry.FromFilename(_currentPath!);
                // Ensure in-grid edits are materialized back into _doc for schema-driven tables
                tableView1.SyncBackToDocument(schema);

                var ext = Path.GetExtension(_currentPath!)?.ToLowerInvariant();
                if (ext == ".edf")
                {
                    if (!Exporters.EdfExporter.TryWrite(fs, _doc, schema))
                        throw new InvalidOperationException("Failed to write EDF format for this table.");
                }
                else if (schema != null)
                {
                    // Plain RDF binary for schema-backed tables
                    using var ms = new MemoryStream();
                    BinaryTableIO.Write(ms, schema, _doc);
                    var bytes = ms.ToArray();
                    // Re-evaluate encryption based on original file's parse to avoid false positives from heuristics
                    bool shouldEncrypt = false;
                    try
                    {
                        if (File.Exists(_currentPath!))
                        {
                            var orig = File.ReadAllBytes(_currentPath!);
                            RdfDocument? oraw = null; RdfDocument? odec = null;
                            try { using var msr = new MemoryStream(orig); oraw = BinaryTableIO.Read(msr, schema); } catch { oraw = null; }
                            if (RdfCrypto.TryDecrypt(orig, out var decOrig))
                            {
                                try { using var msd = new MemoryStream(decOrig); odec = BinaryTableIO.Read(msd, schema); } catch { odec = null; }
                            }
                            int rc = oraw?.Rows.Count ?? 0; int dc = odec?.Rows.Count ?? 0;
                            shouldEncrypt = (odec != null) && (dc > rc);
                        }
                        else
                        {
                            shouldEncrypt = _originalWasEncrypted; // fallback to earlier signal
                        }
                    }
                    catch { shouldEncrypt = _originalWasEncrypted; }

                    if (shouldEncrypt && RdfCrypto.TryEncrypt(bytes, out var enc))
                        fs.Write(enc, 0, enc.Length);
                    else
                        fs.Write(bytes, 0, bytes.Length);
                }
                else
                {
                    // SAFETY: Do not save when we don't have an exact schema. The previous fallback
                    // wrote a CharTitle-shaped binary which corrupts non-CharTitle tables.
                    // Require adding a schema mapping in TableRegistry for this file type,
                    // or use an export format (XML/EDF) instead.
                    throw new NotSupportedException("Cannot save RDF: no schema matched this file. Saving without an exact schema will corrupt the table. Add a schema in TableRegistry or use Export XML/EDF.");
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
            _currentSchema = null;
            tableView1.SetDocument(_doc);
            toolStripStatusLabel1.Text = "New document";
            UpdateRowInfo();
        }

        private void OnExit(object? sender, EventArgs e)
        {
            Close();
        }

        // Export handlers
        private void OnExportXml(object? sender, EventArgs e)
        {
            if (_doc == null)
            {
                MessageBox.Show(this, "No table loaded.");
                return;
            }
            if (exportXmlDialog.ShowDialog(this) != DialogResult.OK) return;
            var schema = _currentSchema ?? (_currentPath != null ? TableRegistry.FromFilename(_currentPath) : null);
            tableView1.SyncBackToDocument(schema);
            using var fs = File.Create(exportXmlDialog.FileName);
            Exporters.XmlExporter.Write(fs, _doc, schema);
            toolStripStatusLabel1.Text = $"Exported XML: {Path.GetFileName(exportXmlDialog.FileName)}";
        }

        private void OnExportEdf(object? sender, EventArgs e)
        {
            if (_doc == null)
            {
                MessageBox.Show(this, "No table loaded.");
                return;
            }
            if (exportEdfDialog.ShowDialog(this) != DialogResult.OK) return;
            try
            {
                var schema = _currentSchema ?? (_currentPath != null ? TableRegistry.FromFilename(_currentPath) : null);
                tableView1.SyncBackToDocument(schema);
                using var fs = File.Create(exportEdfDialog.FileName);
                if (!Exporters.EdfExporter.TryWrite(fs, _doc, schema))
                    throw new InvalidOperationException("EDF exporter is not available for this table.");
                toolStripStatusLabel1.Text = $"Exported EDF: {Path.GetFileName(exportEdfDialog.FileName)}";
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "Export EDF failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
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

        // Edit actions
        private void OnAddRow(object? sender, EventArgs e)
        {
            tableView1.AddRow();
            UpdateRowInfo();
        }

        private void OnCloneRow(object? sender, EventArgs e)
        {
            tableView1.CloneSelectedRows();
            UpdateRowInfo();
        }

        private void OnDeleteRow(object? sender, EventArgs e)
        {
            tableView1.ConfirmAndDeleteSelectedRows();
            UpdateRowInfo();
        }
    }
}
