using System;
using System.IO;
using System.Linq;
using System.Text;
using RdfTableEditor.Model;
using RdfTableEditor.Model.Schema;

class Program
{
    static int Main(string[] args)
    {
        if (args.Length == 0)
        {
            Console.Error.WriteLine("Usage: RdfTableInspector <path-to-rdf>");
            return 2;
        }
        var path = args[0];
        bool trace = args.Length > 1 && string.Equals(args[1], "--trace", StringComparison.OrdinalIgnoreCase);
        int traceRow = -1; // optional: --trace-row=N
        bool traceUntilFail = false; // optional: --trace-until-fail
    bool dump = false; // optional: --dump
    int headCount = 0; // optional: --head=20
        string? containsFilter = null; // optional: --contains=BattleDungeon
        for (int i = 1; i < args.Length; i++)
        {
            if (args[i].StartsWith("--trace-row="))
            {
                var val = args[i].Substring("--trace-row=".Length);
                if (int.TryParse(val, out var n)) traceRow = n;
            }
            else if (string.Equals(args[i], "--trace-until-fail", StringComparison.OrdinalIgnoreCase))
            {
                traceUntilFail = true;
            }
            else if (string.Equals(args[i], "--dump", StringComparison.OrdinalIgnoreCase))
            {
                dump = true;
            }
            else if (args[i].StartsWith("--head=", StringComparison.OrdinalIgnoreCase))
            {
                var val = args[i].Substring("--head=".Length);
                if (int.TryParse(val, out var n) && n > 0)
                    headCount = n;
            }
            else if (args[i].StartsWith("--contains=", StringComparison.OrdinalIgnoreCase))
            {
                containsFilter = args[i].Substring("--contains=".Length);
            }
        }
        if (!File.Exists(path))
        {
            Console.Error.WriteLine($"File not found: {path}");
            return 2;
        }
        var schema = TableRegistry.FromFilename(path);
        if (schema == null)
        {
            Console.Error.WriteLine("No schema match for this file; cannot inspect generically.");
            return 3;
        }
        try
        {
            var raw = File.ReadAllBytes(path);
            byte[] dec;
            bool hasDec = RdfTableEditor.Model.RdfCrypto.TryDecrypt(raw, out dec);

            // Trace mode uses decrypted bytes if available
            if (trace || traceRow >= 0 || traceUntilFail)
            {
                using var fs = new MemoryStream(hasDec ? dec : raw, writable: false);
                Console.WriteLine($"Schema: {schema.Name} (trace mode)\n");
                using var br = new BinaryReader(fs, Encoding.UTF8, leaveOpen: true);
                if (schema.HasMargin)
                {
                    var margin = br.ReadByte();
                    Console.WriteLine($"Margin: {margin}");
                }
                int offset = 0;
                int rowIndex = 0;
                try
                {
                    bool hasVariable = schema.Fields.Any(f => !f.IsArray && f.Type == ScalarType.WStringVar);
                    void ReadField(Field f)
                    {
                        int elemAlign = Math.Min(GetScalarAlignment(f), schema.PackAlignment);
                        int pad = Padding(offset, elemAlign);
                        if (pad > 0)
                        {
                            br.ReadBytes(pad);
                            offset += pad;
                            if (trace) Console.WriteLine($"[pad] +{pad} -> off={offset}");
                        }
                        if (trace) Console.Write($"Field '{f.Name}' ({f.Type}{(f.IsArray ? $"[{f.Length}]" : string.Empty)}) at off={offset} ... ");
                        if (f.IsArray)
                        {
                            for (int i = 0; i < f.Length; i++)
                            {
                                switch (f.Type)
                                {
                                    case ScalarType.U8:
                                    case ScalarType.Bool:
                                        br.ReadByte(); offset += 1; break;
                                    case ScalarType.U16:
                                        br.ReadUInt16(); offset += 2; break;
                                    case ScalarType.U32:
                                        br.ReadUInt32(); offset += 4; break;
                                    case ScalarType.Float:
                                        br.ReadSingle(); offset += 4; break;
                                    case ScalarType.Double:
                                        br.ReadDouble(); offset += 8; break;
                                    default:
                                        throw new NotSupportedException($"Array type not supported: {f.Type}");
                                }
                            }
                        }
                        else
                        {
                            switch (f.Type)
                            {
                                case ScalarType.U8:
                                case ScalarType.Bool:
                                    br.ReadByte(); offset += 1; break;
                                case ScalarType.U16:
                                    br.ReadUInt16(); offset += 2; break;
                                case ScalarType.U32:
                                    br.ReadUInt32(); offset += 4; break;
                                case ScalarType.Float:
                                    br.ReadSingle(); offset += 4; break;
                                case ScalarType.Double:
                                    br.ReadDouble(); offset += 8; break;
                                case ScalarType.WStringFixed:
                                    br.ReadBytes(f.Length * 2); offset += f.Length * 2; break;
                                case ScalarType.WStringVar:
                                {
                                    var len = br.ReadUInt16(); offset += 2;
                                    if (len > 0)
                                    {
                                        br.ReadBytes(len * 2);
                                        offset += len * 2;
                                    }
                                    break;
                                }
                                case ScalarType.AnsiStringFixed:
                                    br.ReadBytes(f.Length); offset += f.Length; break;
                                default:
                                    throw new NotSupportedException($"Type not supported: {f.Type}");
                            }
                        }
                        if (trace) Console.WriteLine($"ok -> off={offset}");
                    }

                    void ReadRowOnce()
                    {
                        foreach (var f in schema.Fields)
                            ReadField(f);
                        if (!hasVariable)
                        {
                            int tail = Padding(offset, schema.PackAlignment);
                            if (tail > 0) { br.ReadBytes(tail); offset += tail; if (trace) Console.WriteLine($"[tail pad] +{tail} -> off={offset}"); }
                        }
                    }

                    if (traceRow >= 0)
                    {
                        for (int i = 0; i < traceRow; i++) { offset = 0; ReadRowOnce(); rowIndex++; }
                        offset = 0; ReadRowOnce();
                        Console.WriteLine($"Row #{traceRow} parsed successfully. Row size={offset}");
                        return 0;
                    }
                    if (traceUntilFail)
                    {
                        while (br.BaseStream.Position < br.BaseStream.Length)
                        {
                            offset = 0; ReadRowOnce(); rowIndex++;
                            if (rowIndex % 500 == 0) Console.WriteLine($".. parsed {rowIndex} rows");
                        }
                        Console.WriteLine($"Parsed all rows successfully: {rowIndex}");
                        return 0;
                    }
                    ReadRowOnce();
                    Console.WriteLine($"First row parsed successfully. Total size={offset}");
                    return 0;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"\nTRACE FAIL at row={rowIndex} off={offset}: {ex.Message}\n{ex}");
                    return 1;
                }
            }

            // Normal mode: try raw vs decrypted and pick best. For TextAll, don't force decrypted.
            RdfDocument? docRaw = null; int rawCount = -1;
            RdfDocument? docDec = null; int decCount = -1;
            using (var msRaw = new MemoryStream(raw, writable: false))
            {
                try { docRaw = BinaryTableIO.Read(msRaw, schema); rawCount = docRaw.Rows.Count; } catch { rawCount = -1; }
            }
            if (hasDec)
            {
                using var msDec = new MemoryStream(dec, writable: false);
                try { docDec = BinaryTableIO.Read(msDec, schema); decCount = docDec.Rows.Count; } catch { decCount = -1; }
            }
            var fileNameOnly = Path.GetFileName(path);
            bool preferDec = fileNameOnly.StartsWith("o_table_", StringComparison.OrdinalIgnoreCase)
                              || string.Equals(schema.Name, "QuestText", StringComparison.OrdinalIgnoreCase)
                              || string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase);
            var doc = (decCount > rawCount || (preferDec && decCount >= rawCount)) ? (docDec ?? docRaw) : (docRaw ?? docDec);
            bool usedDecrypt = ReferenceEquals(doc, docDec);

            Console.WriteLine($"Schema: {schema.Name}");
            Console.WriteLine($"Rows loaded: {doc?.Rows.Count ?? 0} (decrypted={(usedDecrypt ? "yes" : "no")})");
            if (doc != null && string.Equals(schema.Name, "Item", StringComparison.OrdinalIgnoreCase))
            {
                Console.WriteLine($"ItemName variant: var={doc.ItemNameTextIsVar} chars={doc.ItemNameTextChars}");
            }

            if (headCount > 0 && doc != null && doc.Rows.Count > 0)
            {
                int take = Math.Min(headCount, doc.Rows.Count);
                Console.WriteLine($"Head {take} rows:");
                for (int i = 0; i < take; i++)
                {
                    var row = doc.Rows[i];
                    row.TryGetScalar<uint>("Tblidx", out var tblidx);
                    row.TryGetScalar<uint>("Name", out var nameIdx);
                    row.TryGetScalar<string>("NameText", out var nameText);
                    row.TryGetScalar<string>("Model", out var model);
                    row.TryGetScalar<byte>("Item_Type", out var itemType);
                    row.TryGetScalar<byte>("Equip_Type", out var equipType);
                    row.TryGetScalar<byte>("Rank", out var rank);
                    string nameDisplay = string.IsNullOrEmpty(nameText) ? "" : nameText.Replace('\n', ' ');
                    if (nameDisplay.Length > 60)
                        nameDisplay = nameDisplay.Substring(0, 57) + "...";
                    Console.WriteLine($"  [{i,4}] Tblidx={tblidx} NameIdx={nameIdx} ItemType={itemType} EquipType={equipType} Rank={rank} Model='{model}' NameText='{nameDisplay}'");
                }
            }

            if (string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase))
            {
                static string Hex(byte[] arr, int n)
                {
                    n = Math.Min(n, arr.Length);
                    var sb = new StringBuilder(n * 3);
                    for (int i = 0; i < n; i++) sb.Append(arr[i].ToString("X2")).Append(' ');
                    return sb.ToString();
                }
                Console.WriteLine($"[Diag] Raw prefix: {Hex(raw, 32)}\n[Diag] Dec prefix: {Hex(hasDec ? dec : raw, 32)}");
                var selected = usedDecrypt ? (hasDec ? dec : raw) : raw;
                using var msSel = new MemoryStream(selected, writable: false);
                using var br2 = new BinaryReader(msSel, Encoding.UTF8, leaveOpen: true);
                long pos0 = br2.BaseStream.Position;
                byte margin = 0;
                if (br2.BaseStream.Position < br2.BaseStream.Length)
                {
                    margin = br2.ReadByte();
                }
                Console.WriteLine($"[Diag] Margin byte: {margin}");
                br2.BaseStream.Position = pos0;
                for (int s = 0; s <= 8; s++)
                {
                    br2.BaseStream.Position = pos0 + s;
                    if (br2.BaseStream.Position + 8 > br2.BaseStream.Length) break;
                    uint a = br2.ReadUInt32();
                    uint b = br2.ReadUInt32();
                    Console.WriteLine($"  shift={s} @{pos0 + s}: A={a} (0x{a:X8}) B={b} (0x{b:X8})");
                }
                // Heuristic: if there is a 4-byte total-size prologue, print candidate headers at +4
                if (br2.BaseStream.Length >= 12)
                {
                    br2.BaseStream.Position = pos0;
                    uint total = br2.ReadUInt32();
                    Console.WriteLine($"[Diag] 32-bit prologue candidate: totalSize={total} nextHeader=@{pos0 + 4}");
                    if (pos0 + 12 <= br2.BaseStream.Length)
                    {
                        br2.BaseStream.Position = pos0 + 4;
                        uint t = br2.ReadUInt32();
                        uint s = br2.ReadUInt32();
                        Console.WriteLine($"        header@+4: type={t} size={s}");
                    }
                }
                // Extra: scan for zlib header (including FDICT) and report DICTID if present
                var probe = usedDecrypt ? (hasDec ? dec : raw) : raw;
                int maxProbe = Math.Min(probe.Length - 2, 64);
                for (int i = 0; i <= maxProbe; i++)
                {
                    if (probe[i] == 0x78 && i + 1 < probe.Length)
                    {
                        byte cmf = probe[i]; byte flg = probe[i + 1];
                        int cmfflg = (cmf << 8) | flg;
                        if ( (cmf & 0x0F) == 8 && cmfflg % 31 == 0)
                        {
                            bool fdict = (flg & 0x20) != 0;
                            Console.WriteLine($"[Diag] zlib header at +{i}: CMF=0x{cmf:X2} FLG=0x{flg:X2} (FDICT={(fdict ? "yes" : "no")})");
                            if (fdict && i + 6 < probe.Length)
                            {
                                uint dictId = (uint)(probe[i + 2] << 24 | probe[i + 3] << 16 | probe[i + 4] << 8 | probe[i + 5]);
                                Console.WriteLine($"       DICTID=0x{dictId:X8} (preset dictionary required)");
                            }
                            break;
                        }
                    }
                }
            }

            if (doc == null) return 1;
            if (!string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase))
            {
                using var ms = new MemoryStream();
                using var bw = new BinaryWriter(ms, Encoding.UTF8, leaveOpen: true);
                var empty = new RdfDocument { TableName = schema.Name };
                empty.Rows.Add(new RdfRow());
                BinaryTableIO.Write(ms, schema, empty);
                var header = schema.HasMargin ? 1 : 0;
                int stride = (int)ms.Length - header;
                Console.WriteLine($"Estimated record size: {stride} bytes");
                var fileSize = new FileInfo(path).Length;
                var payload = fileSize - header;
                if (payload > 0 && stride > 0)
                {
                    double expected = payload / (double)stride;
                    Console.WriteLine($"File bytes: {fileSize} (payload: {payload}); payload/stride ≈ {expected:F2}");
                }
            }
            else
            {
                Console.WriteLine("Record-size estimate skipped for TextAll container.");
            }
            if (dump && doc.Rows.Count > 0)
            {
                Console.WriteLine();
                foreach (var row in doc.Rows)
                {
                    row.TryGetScalar<uint>("Tblidx", out var tblidx);
                    row.TryGetScalar<string>("Name", out var name);
                    var values = new string[10];
                    for (int v = 0; v < values.Length; v++)
                    {
                        row.TryGetScalar<string>($"Value{v}", out var val);
                        values[v] = val ?? string.Empty;
                    }

                    if (!string.IsNullOrEmpty(containsFilter))
                    {
                        bool matches = false;
                        if (!string.IsNullOrEmpty(name) && name.IndexOf(containsFilter, StringComparison.OrdinalIgnoreCase) >= 0)
                            matches = true;
                        else
                        {
                            foreach (var val in values)
                            {
                                if (!string.IsNullOrEmpty(val) && val.IndexOf(containsFilter, StringComparison.OrdinalIgnoreCase) >= 0)
                                {
                                    matches = true;
                                    break;
                                }
                            }
                        }
                        if (!matches)
                            continue;
                    }

                    Console.WriteLine($"[{tblidx,3}] {name}");
                    for (int v = 0; v < values.Length; v++)
                    {
                        if (!string.IsNullOrEmpty(values[v]))
                        {
                            Console.WriteLine($"    ({v}) {values[v]}");
                        }
                    }
                }
            }
            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.ToString());
            return 1;
        }
    }

    // Copied small helpers from BinaryTableIO for tracing
    private static int GetScalarAlignment(Field f) => f.Type switch
    {
        ScalarType.U8 or ScalarType.Bool => 1,
        ScalarType.U16 => 2,
        ScalarType.U32 => 4,
        ScalarType.Float => 4,
        ScalarType.Double => 8,
        ScalarType.WStringFixed => 2,
        ScalarType.WStringVar => 2,
        ScalarType.AnsiStringFixed => 1,
        _ => 1
    };
    private static int Padding(int offset, int align)
    {
        int mod = offset % align;
        return mod == 0 ? 0 : (align - mod);
    }
}
 
