using System;
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Windows.Forms;
using System.Drawing;
using RdfTableEditor.Model;
using RdfTableEditor.Model.Schema;
using RdfTableEditor.Model.Translation;

namespace RdfTableEditor.Views
{
    public class TableView : UserControl
    {
    private readonly DataGridView _grid;
    private BindingList<RdfRow> _rows = new BindingList<RdfRow>();
    private DataTable? _table;
    private TableSchema? _schema;
    private RdfDocument? _doc;
    private BindingSource _binding = new BindingSource();
    private readonly ContextMenuStrip _ctxMenu = new ContextMenuStrip();
    private readonly ToolStripMenuItem _ctxAdd;
    private readonly ToolStripMenuItem _ctxClone;
    private readonly ToolStripMenuItem _ctxDelete;
    private readonly ToolStripMenuItem _ctxTranslate;
    private readonly DataGridViewCheckBoxColumn _selectColumn = new DataGridViewCheckBoxColumn();
    private readonly CheckBox _headerCheckBox = new CheckBox();
    private bool _suppressHeaderCheckChanged;
    private readonly ToolTip _tip = new ToolTip();
    private const string SearchIndexColumnName = "__SearchIndex";
    private bool _suppressIndexRebuild = false;

        public TableView()
        {
            _grid = new DataGridView
            {
                Dock = DockStyle.Fill,
                AutoGenerateColumns = false,
                AllowUserToAddRows = false, // reduce confusion and improve perf
                AllowUserToDeleteRows = true
            };
            Controls.Add(_grid);
            BuildCharTitleColumns();
            _binding.DataSource = _rows;
            _grid.DataSource = _binding;
            RdfTableEditor.UI.Theme.StyleGrid(_grid);

            // Key shortcuts
            _grid.KeyDown += Grid_KeyDown;

            // Context menu
            _ctxAdd = new ToolStripMenuItem("Add Row", null, (s, e) => AddRow());
            _ctxClone = new ToolStripMenuItem("Clone Row", null, (s, e) => CloneSelectedRows());
            _ctxDelete = new ToolStripMenuItem("Delete Row", null, (s, e) => ConfirmAndDeleteSelectedRows());
            _ctxTranslate = new ToolStripMenuItem("Translate to English", null, async (s, e) => await TranslateCurrentCellAsync());
            _ctxMenu.Items.AddRange(new ToolStripItem[] { _ctxAdd, _ctxClone, _ctxDelete, new ToolStripSeparator(), _ctxTranslate });
            _ctxMenu.Opening += CtxMenu_Opening;
            _grid.ContextMenuStrip = _ctxMenu;
            _grid.MouseDown += Grid_MouseDown_SelectOnRightClick;

            // Selection checkbox column + header select-all
            _selectColumn.Name = "Select";
            _selectColumn.HeaderText = string.Empty;
            _selectColumn.Width = 32;
            _selectColumn.Resizable = DataGridViewTriState.False;
            _selectColumn.Frozen = false;
            _selectColumn.ReadOnly = false;
            _selectColumn.ThreeState = false;
            _grid.CurrentCellDirtyStateChanged += (s, e) =>
            {
                if (_grid.IsCurrentCellDirty)
                    _grid.CommitEdit(DataGridViewDataErrorContexts.Commit);
            };
            _grid.CellValueChanged += (s, e) =>
            {
                if (e.ColumnIndex == _selectColumn.Index)
                {
                    // Update header checkbox state (checked only if all visible rows are checked)
                    UpdateHeaderCheckBoxState();
                }
            };
            _grid.ColumnWidthChanged += (s, e) => PositionHeaderCheckBox();
            _grid.SizeChanged += (s, e) => PositionHeaderCheckBox();
            _grid.Scroll += (s, e) => PositionHeaderCheckBox();
            _grid.DataBindingComplete += (s, e) => PositionHeaderCheckBox();
            _grid.ColumnAdded += (s, e) => PositionHeaderCheckBox();
            _headerCheckBox.Size = new Size(15, 15);
            _headerCheckBox.BackColor = Color.Transparent;
            _headerCheckBox.Visible = false; // hidden until positioned
            _headerCheckBox.CheckedChanged += (s, e) =>
            {
                if (_suppressHeaderCheckChanged) return;
                ToggleAllRows(_headerCheckBox.Checked);
            };
            _grid.Controls.Add(_headerCheckBox);
        }

        public void SetDocument(RdfDocument? doc, TableSchema? schema = null)
        {
            _doc = doc;
            _schema = schema;
            if (schema == null)
            {
                // Fallback to CharTitle typed columns
                _table = null;
                _grid.AutoGenerateColumns = false;
                _grid.Columns.Clear();
                BuildCharTitleColumns();
                EnsureSelectColumnPresent();
                _rows = new BindingList<RdfRow>(doc?.Rows ?? new System.Collections.Generic.List<RdfRow>());
                _binding.DataSource = _rows;
                _grid.DataSource = _binding;
                PositionHeaderCheckBox();
            }
            else
            {
                // Schema-driven DataTable view
                BuildSchemaTable();
                EnsureSelectColumnPresent();
                PositionHeaderCheckBox();
            }
        }

        private void BuildCharTitleColumns()
        {
            _grid.Columns.Clear();
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.Id), HeaderText = "Tblidx", Width = 80 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.TitleNameIndex), HeaderText = "NameIndex", Width = 90 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.ContentsType), HeaderText = "ContentsType", Width = 90 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.RepresentationType), HeaderText = "DirectType", Width = 90 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.BoneName), HeaderText = "BoneName", Width = 200 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.EffectName), HeaderText = "EffectName", Width = 200 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.EffectSound), HeaderText = "EffectSound", Width = 200 });

            // Flatten arrays into simple CSV text cells for now
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.SystemEffectTblidxCsv), HeaderText = "SysEff Tblidx[3]", Width = 140 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.SystemEffectTypeCsv), HeaderText = "SysEff Type[3]", Width = 140 });
            _grid.Columns.Add(new DataGridViewTextBoxColumn { DataPropertyName = nameof(RdfRow.SystemEffectValueCsv), HeaderText = "SysEff Value[3]", Width = 160 });
        }

        private void BuildSchemaTable()
        {
            if (_doc == null || _schema == null)
            {
                _grid.DataSource = null;
                return;
            }

            _table = new DataTable(_schema.Name);

            // Create columns
            foreach (var f in _schema.Fields)
            {
                if (!f.IsArray)
                {
                    _table.Columns.Add(f.Name, MapType(f.Type));
                }
                else
                {
                    for (int i = 0; i < f.Length; i++)
                    {
                        _table.Columns.Add($"{f.Name}[{i}]", MapType(f.Type));
                    }
                }
            }
            // Add hidden search index column (materialized for reliable RowFilter)
            if (!_table.Columns.Contains(SearchIndexColumnName))
                _table.Columns.Add(SearchIndexColumnName, typeof(string));

            // Fill rows
            foreach (var r in _doc.Rows)
            {
                var row = _table.NewRow();
                foreach (var f in _schema.Fields)
                {
                    if (!f.IsArray)
                    {
                        object val = f.Type switch
                        {
                            ScalarType.U8 => r.GetScalarOrDefault<byte>(f.Name, 0),
                            ScalarType.Bool => r.GetScalarOrDefault<byte>(f.Name, 0),
                            ScalarType.U16 => r.GetScalarOrDefault<ushort>(f.Name, 0),
                            ScalarType.U32 => r.GetScalarOrDefault<uint>(f.Name, 0),
                            ScalarType.Float => r.GetScalarOrDefault<float>(f.Name, 0f),
                            ScalarType.Double => r.GetScalarOrDefault<double>(f.Name, 0.0),
                            ScalarType.WStringFixed => r.GetScalarOrDefault<string>(f.Name, string.Empty),
                            ScalarType.WStringVar => r.GetScalarOrDefault<string>(f.Name, string.Empty),
                            _ => null!
                        };
                        row[f.Name] = val ?? DBNull.Value;
                    }
                    else
                    {
                        switch (f.Type)
                        {
                            case ScalarType.U8:
                            case ScalarType.Bool:
                            {
                                var arr = r.GetArray<byte>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i];
                                break;
                            }
                            case ScalarType.U16:
                            {
                                var arr = r.GetArray<ushort>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i];
                                break;
                            }
                            case ScalarType.U32:
                            {
                                var arr = r.GetArray<uint>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i];
                                break;
                            }
                            case ScalarType.Float:
                            {
                                var arr = r.GetArray<float>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i];
                                break;
                            }
                            case ScalarType.Double:
                            {
                                var arr = r.GetArray<double>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i];
                                break;
                            }
                            case ScalarType.WStringFixed:
                            {
                                var arr = r.GetArray<string>(f.Name, f.Length);
                                for (int i = 0; i < f.Length; i++) row[$"{f.Name}[{i}]"] = arr[i] ?? string.Empty;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
                _table.Rows.Add(row);
            }

            // Build search index content before binding
            RebuildSearchIndex();

            _grid.AutoGenerateColumns = true;
            _binding.DataSource = _table;
            _grid.DataSource = _binding;
            EnsureSelectColumnPresent();
            // Tune generated columns for performance
            foreach (DataGridViewColumn col in _grid.Columns)
            {
                col.AutoSizeMode = DataGridViewAutoSizeColumnMode.None;
                col.Width = 120;
                if (string.Equals(col.DataPropertyName, SearchIndexColumnName, StringComparison.Ordinal))
                {
                    col.Visible = false; // hide index column
                    continue;
                }
                if (_schema.Fields.FirstOrDefault(f => f.Name == col.DataPropertyName) is { } f)
                {
                    if (f.Type == ScalarType.WStringFixed || f.Type == ScalarType.WStringVar)
                    {
                        col.Width = 320;
                        if (string.Equals(f.Name, "Quest_Text", StringComparison.OrdinalIgnoreCase))
                            col.Width = 600; // large text, but capped
                    }
                }
            }
        }

        private static Type MapType(ScalarType t) => t switch
        {
            ScalarType.U8 => typeof(byte),
            ScalarType.Bool => typeof(byte), // stored as 1 byte in RDF
            ScalarType.U16 => typeof(ushort),
            ScalarType.U32 => typeof(uint),
            ScalarType.Float => typeof(float),
            ScalarType.Double => typeof(double),
            ScalarType.WStringFixed => typeof(string),
            ScalarType.WStringVar => typeof(string),
            ScalarType.AnsiStringFixed => typeof(string),
            _ => typeof(string)
        };

        public void SyncBackToDocument(TableSchema? schema)
        {
            if (_doc == null || schema == null || _table == null)
                return;

            _doc.Rows.Clear();
            foreach (DataRow dr in _table.Rows)
            {
                var rr = new RdfRow();
                foreach (var f in schema.Fields)
                {
                    if (!f.IsArray)
                    {
                        var cell = dr[f.Name];
                        object val;
                        switch (f.Type)
                        {
                            case ScalarType.WStringFixed:
                            case ScalarType.WStringVar:
                            case ScalarType.AnsiStringFixed:
                                val = cell == DBNull.Value ? string.Empty : Convert.ToString(cell) ?? string.Empty;
                                break;
                            case ScalarType.U8:
                            case ScalarType.Bool:
                                val = cell == DBNull.Value ? (byte)0 : Convert.ToByte(cell);
                                break;
                            case ScalarType.U16:
                                val = cell == DBNull.Value ? (ushort)0 : Convert.ToUInt16(cell);
                                break;
                            case ScalarType.U32:
                                val = cell == DBNull.Value ? 0u : Convert.ToUInt32(cell);
                                break;
                            case ScalarType.Float:
                                val = cell == DBNull.Value ? 0f : Convert.ToSingle(cell);
                                break;
                            case ScalarType.Double:
                                val = cell == DBNull.Value ? 0.0 : Convert.ToDouble(cell);
                                break;
                            default:
                                val = cell;
                                break;
                        }
                        rr.SetScalar(f.Name, val);
                    }
                    else
                    {
                        switch (f.Type)
                        {
                            case ScalarType.U8:
                            case ScalarType.Bool:
                                {
                                    var arr = new byte[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToByte(dr[$"{f.Name}[{i}]"]);
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            case ScalarType.U16:
                                {
                                    var arr = new ushort[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToUInt16(dr[$"{f.Name}[{i}]"]);
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            case ScalarType.U32:
                                {
                                    var arr = new uint[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToUInt32(dr[$"{f.Name}[{i}]"]);
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            case ScalarType.Float:
                                {
                                    var arr = new float[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToSingle(dr[$"{f.Name}[{i}]"]);
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            case ScalarType.Double:
                                {
                                    var arr = new double[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToDouble(dr[$"{f.Name}[{i}]"]);
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            case ScalarType.WStringFixed:
                                {
                                    var arr = new string[f.Length];
                                    for (int i = 0; i < f.Length; i++) arr[i] = Convert.ToString(dr[$"{f.Name}[{i}]"]) ?? string.Empty;
                                    rr.SetArray(f.Name, arr);
                                    break;
                                }
                            default:
                                break;
                        }
                    }
                }
                _doc.Rows.Add(rr);
            }
        }

        public void ApplyQuickFilter(string text)
        {
        if (_table != null)
            {
                if (string.IsNullOrWhiteSpace(text))
                {
            _binding.RemoveFilter();
                }
                else
                {
            // Rebuild search index and filter on a single safe column to avoid quoting issues
            RebuildSearchIndex();
            string pattern = $"%{EscapeLike(text)}%";
            var filter = $"Convert({QuoteIdentifier(SearchIndexColumnName)}, 'System.String') LIKE '{pattern}'";
            _binding.Filter = filter;
                }
            }
            else
            {
                // List binding: filter by string props if available
                if (string.IsNullOrWhiteSpace(text))
                {
                    _binding.DataSource = _doc?.Rows ?? new System.Collections.Generic.List<RdfRow>();
                }
                else
                {
                    var t = text;
                    bool Match(RdfRow r)
                    {
                        // Check typed fields
                        if ((r.BoneName?.IndexOf(t, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0) return true;
                        if ((r.EffectName?.IndexOf(t, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0) return true;
                        if ((r.EffectSound?.IndexOf(t, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0) return true;
                        if (r.Id.ToString().IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if (r.TitleNameIndex.ToString().IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if (r.ContentsType.ToString().IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if (r.RepresentationType.ToString().IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        // Arrays via CSV proxies
                        if ((r.SystemEffectTblidxCsv ?? string.Empty).IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if ((r.SystemEffectTypeCsv ?? string.Empty).IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if ((r.SystemEffectValueCsv ?? string.Empty).IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        // Legacy placeholders
                        if ((r.Name ?? string.Empty).IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        if ((r.Value ?? string.Empty).IndexOf(t, StringComparison.OrdinalIgnoreCase) >= 0) return true;
                        return false;
                    }
                    var q = (_doc?.Rows ?? new System.Collections.Generic.List<RdfRow>())
                        .Where(Match)
                        .ToList();
                    _binding.DataSource = new BindingList<RdfRow>(q);
                }
                _grid.DataSource = _binding;
            }
        }

        public int VisibleRowCount
        {
            get
            {
                if (_table != null) return _binding.Count;
                return (_binding.List as System.Collections.IList)?.Count ?? 0;
            }
        }

        private static string EscapeLike(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            // RowFilter escaping rules: escape % and * by wrapping with [], escape [ as [[] and ] as []]
            const string OB = "[";           // open bracket
            const string OBE = "[[]";       // escaped open bracket (literal '[')
            const string CB = "]";           // close bracket
            const string CBE = "[]]";       // escaped close bracket
            const string PCTE = "[%]";      // escaped percent
            const string STARE = "[*]";     // escaped star
            return s
                .Replace("'", "''")
                .Replace(OB, OBE)
                .Replace(CB, CBE)
                .Replace("%", PCTE)
                .Replace("*", STARE);
        }

        private static string QuoteIdentifier(string columnName)
        {
            if (string.IsNullOrEmpty(columnName)) return columnName;
            // In RowFilter, identifiers are bracket-delimited. Escape any closing brackets in the name by doubling them.
            var inner = columnName.Replace("]", "]]" );
            return "[" + inner + "]";
        }

        private void RebuildSearchIndex()
        {
            if (_table == null) return;
            if (!_table.Columns.Contains(SearchIndexColumnName)) return;
            var cols = _table.Columns.Cast<DataColumn>().Where(c => !string.Equals(c.ColumnName, SearchIndexColumnName, StringComparison.Ordinal)).ToArray();
            _suppressIndexRebuild = true;
            try
            {
                foreach (DataRow r in _table.Rows)
                {
                    if (r.RowState == DataRowState.Deleted || r.RowState == DataRowState.Detached) continue;
                    var built = BuildRowIndex(r, cols);
                    var current = r[SearchIndexColumnName] as string;
                    if (!string.Equals(current, built, StringComparison.Ordinal))
                        r[SearchIndexColumnName] = built;
                }
            }
            finally { _suppressIndexRebuild = false; }
        }

        private static string BuildRowIndex(DataRow r, DataColumn[] cols)
        {
            var parts = new System.Text.StringBuilder();
            for (int i = 0; i < cols.Length; i++)
            {
                if (r.RowState == DataRowState.Deleted) break;
                var v = r[cols[i]];
                if (v != DBNull.Value && v != null)
                {
                    // Avoid any custom type conversions; treat strings as-is, others via InvariantCulture
                    if (v is string sv)
                        parts.Append(sv);
                    else
                        parts.Append(Convert.ToString(v, System.Globalization.CultureInfo.InvariantCulture));
                }
                parts.Append('\u001F'); // unit separator
            }
            return parts.ToString();
        }

        private void UpdateRowSearchIndex(DataRow row)
        {
            if (_table == null) return;
            if (!_table.Columns.Contains(SearchIndexColumnName)) return;
            if (row == null || row.RowState == DataRowState.Detached || row.RowState == DataRowState.Deleted) return;
            var cols = _table.Columns.Cast<DataColumn>().Where(c => !string.Equals(c.ColumnName, SearchIndexColumnName, StringComparison.Ordinal)).ToArray();
            _suppressIndexRebuild = true;
            try
            {
                var built = BuildRowIndex(row, cols);
                var current = row[SearchIndexColumnName] as string;
                if (!string.Equals(current, built, StringComparison.Ordinal))
                    row[SearchIndexColumnName] = built;
            }
            catch
            {
                // ignore transient edit states
            }
            finally { _suppressIndexRebuild = false; }
        }

        private void Grid_KeyDown(object? sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Insert)
            {
                AddRow();
                e.Handled = true;
            }
            else if (e.KeyCode == Keys.Delete && !_grid.ReadOnly)
            {
                ConfirmAndDeleteSelectedRows();
                e.Handled = true;
            }
            else if (e.Control && e.KeyCode == Keys.D)
            {
                CloneSelectedRows();
                e.Handled = true;
            }
        }

        // Public edit API
        public void AddRow()
        {
            if (_doc == null)
            {
                _doc = new RdfDocument();
            }

            if (_table != null && _schema != null)
            {
                var nr = _table.NewRow();
                // Fill defaults (zeros/empty strings)
                foreach (DataColumn col in _table.Columns)
                {
                    if (col.DataType == typeof(string)) nr[col] = string.Empty; else nr[col] = Activator.CreateInstance(col.DataType) ?? 0;
                }
                // Smart Tblidx default: max+1
                if (_table.Columns.Contains("Tblidx"))
                {
                    try
                    {
                        uint next = 1;
                        if (_table.Rows.Count > 0)
                        {
                            var max = _table.AsEnumerable()
                                .Select(r => r.Field<object>("Tblidx"))
                                .Select(o => Convert.ToUInt32(o))
                                .DefaultIfEmpty(0u).Max();
                            next = max + 1;
                        }
                        nr["Tblidx"] = next;
                    }
                    catch { /* ignore */ }
                }
                _table.Rows.Add(nr);
                // Move selection
                _grid.ClearSelection();
                var idx = _table.Rows.Count - 1;
                if (idx >= 0)
                {
                    _grid.Rows[idx].Selected = true;
                    _grid.FirstDisplayedScrollingRowIndex = Math.Max(0, idx);
                }
                // Uncheck header since not all items are selected by default
                _headerCheckBox.Checked = false;
            }
            else
            {
                // fallback list mode
                var rr = new RdfRow
                {
                    Id = (_doc.Rows.Count == 0) ? 1 : _doc.Rows.Max(r => r.Id) + 1,
                    TitleNameIndex = 0,
                    ContentsType = 0,
                    RepresentationType = 0,
                    BoneName = string.Empty,
                    EffectName = string.Empty,
                    EffectSound = string.Empty,
                    SystemEffectTblidx = new int[3],
                    SystemEffectType = new byte[3],
                    SystemEffectValue = new double[3]
                };
                _doc.Rows.Add(rr);
                _rows.Add(rr);
                _binding.ResetBindings(false);
                if (_grid.Rows.Count > 0)
                {
                    var idx = _grid.Rows.Count - 1;
                    _grid.ClearSelection();
                    _grid.Rows[idx].Selected = true;
                    _grid.FirstDisplayedScrollingRowIndex = Math.Max(0, idx);
                }
                _headerCheckBox.Checked = false;
            }
        }

        public void CloneSelectedRows()
        {
            if (_doc == null) return;

            var checkedRows = GetCheckedGridRows();
            if (_table != null && _schema != null)
            {
                var rowsToClone = checkedRows.Any() ? checkedRows : _grid.SelectedRows.Cast<DataGridViewRow>();
                foreach (DataGridViewRow sel in rowsToClone)
                {
                    if (sel.DataBoundItem is DataRowView drv)
                    {
                        var src = drv.Row;
                        var nr = _table.NewRow();
                        nr.ItemArray = (object?[])src.ItemArray.Clone();
                        // bump Tblidx if present
                        if (_table.Columns.Contains("Tblidx"))
                        {
                            try
                            {
                                var max = _table.AsEnumerable()
                                    .Select(r => r.Field<object>("Tblidx"))
                                    .Select(o => Convert.ToUInt32(o))
                                    .DefaultIfEmpty(0u).Max();
                                nr["Tblidx"] = max + 1;
                            }
                            catch { /* ignore */ }
                        }
                        _table.Rows.Add(nr);
                    }
                }
            }
            else
            {
                // list mode
                var selected = (checkedRows.Any() ? checkedRows : _grid.SelectedRows.Cast<DataGridViewRow>())
                    .Select(r => r.DataBoundItem as RdfRow)
                    .Where(r => r != null).Cast<RdfRow>()
                    .ToList();
                foreach (var r in selected)
                {
                    var clone = new RdfRow
                    {
                        Id = (_doc.Rows.Count == 0) ? 1 : _doc.Rows.Max(x => x.Id) + 1,
                        TitleNameIndex = r.TitleNameIndex,
                        ContentsType = r.ContentsType,
                        RepresentationType = r.RepresentationType,
                        BoneName = r.BoneName,
                        EffectName = r.EffectName,
                        EffectSound = r.EffectSound,
                        SystemEffectTblidx = (int[]?)r.SystemEffectTblidx?.Clone(),
                        SystemEffectType = (byte[]?)r.SystemEffectType?.Clone(),
                        SystemEffectValue = (double[]?)r.SystemEffectValue?.Clone()
                    };
                    _doc.Rows.Add(clone);
                    _rows.Add(clone);
                }
                _binding.ResetBindings(false);
            }
        }

        public void DeleteSelectedRows()
        {
            if (_doc == null) return;

            var checkedRows = GetCheckedGridRows();
            if (_table != null && _schema != null)
            {
                // collect to avoid modifying while iterating
                var baseRows = checkedRows.Any() ? checkedRows : _grid.SelectedRows.Cast<DataGridViewRow>();
                var toDelete = baseRows
                    .Select(r => (r.DataBoundItem as DataRowView)?.Row)
                    .Where(r => r != null).Cast<DataRow>()
                    .ToList();
                foreach (var row in toDelete)
                {
                    _table.Rows.Remove(row);
                }
            }
            else
            {
                var baseRows = checkedRows.Any() ? checkedRows : _grid.SelectedRows.Cast<DataGridViewRow>();
                var toDelete = baseRows
                    .Select(r => r.DataBoundItem as RdfRow)
                    .Where(r => r != null).Cast<RdfRow>()
                    .ToList();
                foreach (var r in toDelete)
                {
                    _rows.Remove(r);
                    _doc.Rows.Remove(r);
                }
                _binding.ResetBindings(false);
            }
            // After delete, clear header check state
            _headerCheckBox.Checked = false;
        }

        public void ConfirmAndDeleteSelectedRows()
        {
            var checkedRows = GetCheckedGridRows();
            var count = checkedRows.Any() ? checkedRows.Count : _grid.SelectedRows.Count;
            if (count == 0) return;
            var owner = this.FindForm();
            var result = MessageBox.Show(owner,
                count == 1 ? "Delete the selected row?" : $"Delete the {count} selected rows?",
                "Confirm Delete",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning,
                MessageBoxDefaultButton.Button2);
            if (result == DialogResult.Yes)
            {
                DeleteSelectedRows();
            }
        }

        private void Grid_MouseDown_SelectOnRightClick(object? sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                var hit = _grid.HitTest(e.X, e.Y);
                if (hit.RowIndex >= 0 && hit.RowIndex < _grid.Rows.Count)
                {
                    if (!_grid.Rows[hit.RowIndex].Selected)
                    {
                        _grid.ClearSelection();
                        _grid.Rows[hit.RowIndex].Selected = true;
                        _grid.CurrentCell = _grid.Rows[hit.RowIndex].Cells[Math.Max(0, hit.ColumnIndex)];
                    }
                }
            }
        }

        private void CtxMenu_Opening(object? sender, CancelEventArgs e)
        {
            var anySelected = _grid.SelectedRows.Count > 0 || GetCheckedGridRows().Any();
            _ctxClone.Enabled = anySelected;
            _ctxDelete.Enabled = anySelected;
            _ctxAdd.Enabled = true;
            // Enable translate if current cell is a string cell, service is available, and value is non-empty
            bool canTranslate = false;
            if (_grid.CurrentCell != null)
            {
                var col = _grid.Columns[_grid.CurrentCell.ColumnIndex];
                var svcAvailable = Translator.CreateFromEnvironment() != null;
                if (col is DataGridViewTextBoxColumn && svcAvailable)
                {
                    var curVal = Convert.ToString(_grid.CurrentCell.Value);
                    canTranslate = !string.IsNullOrWhiteSpace(curVal);
                }
            }
            _ctxTranslate.Enabled = canTranslate;
        }

        // Checkbox selection helpers
        private void EnsureSelectColumnPresent()
        {
            if (_grid.Columns.Contains(_selectColumn.Name))
            {
                _selectColumn.DisplayIndex = 0;
                PositionHeaderCheckBox();
                return;
            }
            _selectColumn.DisplayIndex = 0;
            _grid.Columns.Insert(0, _selectColumn);
            PositionHeaderCheckBox();
        }

        private void PositionHeaderCheckBox()
        {
            if (!_grid.Columns.Contains(_selectColumn.Name))
            {
                _headerCheckBox.Visible = false;
                return;
            }
            var rect = _grid.GetCellDisplayRectangle(_selectColumn.DisplayIndex, -1, true);
            if (rect.Width <= 0 || rect.Height <= 0)
            {
                _headerCheckBox.Visible = false;
                return;
            }
            _headerCheckBox.Location = new Point(rect.Left + (rect.Width - _headerCheckBox.Width) / 2, rect.Top + (rect.Height - _headerCheckBox.Height) / 2);
            _headerCheckBox.Visible = true;
        }

        private void UpdateHeaderCheckBoxState()
        {
            bool allChecked = true;
            bool anyChecked = false;
            foreach (DataGridViewRow row in _grid.Rows)
            {
                if (!row.Visible) continue;
                var val = Convert.ToBoolean(row.Cells[_selectColumn.DisplayIndex].Value ?? false);
                allChecked &= val;
                anyChecked |= val;
            }
            // Avoid triggering ToggleAll when programmatically updating
            _suppressHeaderCheckChanged = true;
            _headerCheckBox.Checked = allChecked && anyChecked;
            _suppressHeaderCheckChanged = false;
        }

        private void ToggleAllRows(bool check)
        {
            foreach (DataGridViewRow row in _grid.Rows)
            {
                if (!row.Visible) continue;
                row.Cells[_selectColumn.DisplayIndex].Value = check;
            }
        }

        private System.Collections.Generic.List<DataGridViewRow> GetCheckedGridRows()
        {
            var list = new System.Collections.Generic.List<DataGridViewRow>();
            foreach (DataGridViewRow row in _grid.Rows)
            {
                if (!row.Visible) continue;
                var val = Convert.ToBoolean(row.Cells[_selectColumn.DisplayIndex].Value ?? false);
                if (val) list.Add(row);
            }
            return list;
        }

        private async System.Threading.Tasks.Task TranslateCurrentCellAsync()
        {
            try
            {
                var svc = Translator.CreateFromEnvironment();
                if (svc == null)
                {
                    MessageBox.Show(this.FindForm(), "Translation service not configured. Add your API key to RdfTableEditor.config.json.", "Translate", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                if (_grid.CurrentCell == null) return;
                var colObj = _grid.Columns[_grid.CurrentCell.ColumnIndex];
                if (!(colObj is DataGridViewTextBoxColumn)) return;
                var val = Convert.ToString(_grid.CurrentCell.Value);
                if (string.IsNullOrWhiteSpace(val))
                {
                    MessageBox.Show(this.FindForm(), "Cell is empty.", "Translate", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                _grid.Cursor = Cursors.WaitCursor;
                // Let the service auto-detect source language to avoid mislabeling CJK (zh/ja/ko)
                var translated = await svc.TranslateAsync(val, null, "en");

                // Write back to underlying data source rather than just the cell,
                // so the change persists and bindings update properly.
                var rowIndex = _grid.CurrentCell.RowIndex;
                var colIndex = _grid.CurrentCell.ColumnIndex;
                var dataProp = string.IsNullOrEmpty(colObj.DataPropertyName) ? colObj.Name : colObj.DataPropertyName;

        if (_table != null)
                {
                    if (_grid.Rows[rowIndex].DataBoundItem is DataRowView drv)
                    {
                        if (!string.IsNullOrEmpty(dataProp) && drv.Row.Table.Columns.Contains(dataProp))
                            drv.Row[dataProp] = translated ?? string.Empty;
                        else
                            _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
            // Ensure the visible cell shows the new value
            _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
                    }
                    else
                    {
                        _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
                    }
                }
                else
                {
                    // List model (RdfRow) - set property via reflection when possible
                    if (_grid.Rows[rowIndex].DataBoundItem is RdfRow rr)
                    {
                        var prop = typeof(RdfRow).GetProperty(dataProp);
                        if (prop != null && prop.CanWrite && prop.PropertyType == typeof(string))
                        {
                            prop.SetValue(rr, translated ?? string.Empty);
                            _binding.ResetCurrentItem();
                        }
                        else
                        {
                            _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
                        }
                        // ensure visible cell
                        _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
                    }
                    else
                    {
                        _grid.Rows[rowIndex].Cells[colIndex].Value = translated;
                    }
                }
                // Commit binding/grid edits
                try { _binding.EndEdit(); } catch { }
                try
                {
                    if (_grid.DataSource != null && this.BindingContext != null)
                    {
                        var cm = this.BindingContext[_grid.DataSource] as CurrencyManager;
                        cm?.EndCurrentEdit();
                    }
                }
                catch { }
                _grid.NotifyCurrentCellDirty(true);
                _grid.CommitEdit(DataGridViewDataErrorContexts.Commit);
                _grid.EndEdit();
                _grid.Refresh();
                if ((translated ?? string.Empty).Trim() == (val ?? string.Empty).Trim())
                {
                    // Inform when API returns same text (already English or not translatable)
                    MessageBox.Show(this.FindForm(), "No change: already English or not translatable.", "Translate", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(this.FindForm(), $"Translation failed: {ex.Message}", "Translate", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                _grid.Cursor = Cursors.Default;
            }
        }

        private static bool SeemsChinese(string s)
        {
            if (string.IsNullOrEmpty(s)) return false;
            int cjk = 0;
            int letters = 0;
            foreach (var ch in s)
            {
                if (char.IsLetter(ch)) letters++;
                // Basic CJK ranges
                if ((ch >= '\u4E00' && ch <= '\u9FFF') || (ch >= '\u3400' && ch <= '\u4DBF')) cjk++;
            }
            return cjk > 0 && cjk >= Math.Max(1, letters / 4);
        }
    }
}
