using System;
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Windows.Forms;
using RdfTableEditor.Model;
using RdfTableEditor.Model.Schema;

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
                _rows = new BindingList<RdfRow>(doc?.Rows ?? new System.Collections.Generic.List<RdfRow>());
                _binding.DataSource = _rows;
                _grid.DataSource = _binding;
            }
            else
            {
                // Schema-driven DataTable view
                BuildSchemaTable();
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

            _grid.AutoGenerateColumns = true;
            _binding.DataSource = _table;
            _grid.DataSource = _binding;
            // Tune generated columns for performance
            foreach (DataGridViewColumn col in _grid.Columns)
            {
                col.AutoSizeMode = DataGridViewAutoSizeColumnMode.None;
                col.Width = 120;
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
                    // Build a simple contains filter across string-like columns
                    var cols = _table.Columns.Cast<DataColumn>()
                        .Where(c => c.DataType == typeof(string))
                        .Select(c => $"Convert([{c.ColumnName}], 'System.String') LIKE '%{EscapeLike(text)}%'");
                    var filter = string.Join(" OR ", cols);
                    _binding.Filter = string.IsNullOrEmpty(filter) ? null : filter;
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
                    var q = (_doc?.Rows ?? new System.Collections.Generic.List<RdfRow>())
                        .Where(r => (r.BoneName?.IndexOf(text, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0
                                 || (r.EffectName?.IndexOf(text, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0
                                 || (r.EffectSound?.IndexOf(text, StringComparison.OrdinalIgnoreCase) ?? -1) >= 0
                                 || r.Id.ToString().Contains(text))
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

        private static string EscapeLike(string s) => s.Replace("[", "[[").Replace("]", "]] ").Replace("%", "[%]").Replace("*", "[*]").Replace("'", "''");
    }
}
