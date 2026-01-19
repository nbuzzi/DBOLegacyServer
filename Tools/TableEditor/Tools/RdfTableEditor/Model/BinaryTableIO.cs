using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.IO.Compression;

namespace RdfTableEditor.Model;

using RdfTableEditor.Model.Schema;

public static class BinaryTableIO
{
    private static readonly Encoding AnsiEncoding;

    static BinaryTableIO()
    {
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        AnsiEncoding = Encoding.GetEncoding(949); // Korean (CP949) used by DBO assets
    }
    // Entry point: read a table according to schema, handling special cases
    public static RdfDocument Read(Stream stream, TableSchema schema)
    {
        var doc = new RdfDocument { TableName = schema.Name };
        using var br = new BinaryReader(stream, Encoding.UTF8, leaveOpen: true);

        // Special-case containers and bespoke formats first
        if (string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase))
            return ReadTextAllWithFallback(br);
        if (string.Equals(schema.Name, "QuestText", StringComparison.OrdinalIgnoreCase))
            return ReadQuestText(br);

        // Common margin byte for many tables
        if (schema.HasMargin && br.BaseStream.Position < br.BaseStream.Length)
        {
            var m = br.ReadByte();
            doc.Margin = m;
        }

        // Adaptive variant handling for specific tables
        if (string.Equals(schema.Name, "Item", StringComparison.OrdinalIgnoreCase))
        {
            // Detect NameText flavor: fixed 41/65 WCHAR or variable WORD+UTF16
            DetectItemNameTextVariant(br, schema, doc);
            br.BaseStream.Position = schema.HasMargin ? 1 : 0;
        }

        while (br.BaseStream.Position < br.BaseStream.Length)
        {
            var row = new RdfRow();
            if (!TryReadRow(br, schema, row, doc)) break;
            doc.Rows.Add(row);
        }
        return doc;
    }

    // Writer: schema-driven serialization with pack(4) alignment
    public static void Write(Stream stream, TableSchema schema, RdfDocument doc)
    {
        using var bw = new BinaryWriter(stream, Encoding.UTF8, leaveOpen: true);
        if (string.Equals(schema.Name, "TextAll", StringComparison.OrdinalIgnoreCase))
            throw new NotSupportedException("Writing TextAll container is not supported yet. Use the game tools to rebuild TextAll from individual text tables.");

        if (schema.HasMargin)
        {
            // Preserve original margin when available, otherwise default to 1
            bw.Write(doc.Margin ?? (byte)1);
        }
        foreach (var row in doc.Rows)
            WriteRow(bw, schema, row, doc);
    }

    // Exact mirror of CTextTable reading used inside TextAll and quest tables
    public static RdfDocument ReadQuestText(BinaryReader br)
    {
        var doc = new RdfDocument { TableName = "QuestText" };
    if (br.BaseStream.Position >= br.BaseStream.Length) return doc;
        // margin (BYTE) — engine doesn't require a specific value
    var _ = br.ReadByte();
    doc.Margin = _;
        while (br.BaseStream.Position + 6 <= br.BaseStream.Length)
        {
            try
            {
                uint tblidx = br.ReadUInt32();
                ushort len = br.ReadUInt16();
                if (len == 0)
                {
                    var rEmpty = new RdfRow();
                    rEmpty.SetScalar("Tblidx", tblidx);
                    rEmpty.SetScalar("Quest_Text", string.Empty);
                    doc.Rows.Add(rEmpty);
                    continue;
                }
                if (br.BaseStream.Position + len * 2 > br.BaseStream.Length) break;
                var bytes = br.ReadBytes(len * 2);
                var text = Encoding.Unicode.GetString(bytes);
                var row = new RdfRow();
                row.SetScalar("Tblidx", tblidx);
                row.SetScalar("Quest_Text", text);
                doc.Rows.Add(row);
            }
            catch
            {
                break;
            }
        }
        return doc;
    }

    // Robust TextAll reader: parse current stream or try zlib-decompressed fallback if needed
    private static RdfDocument ReadTextAllWithFallback(BinaryReader br)
    {
        // Normalize to buffer
        long remain = br.BaseStream.Length - br.BaseStream.Position;
        if (remain <= 0) return new RdfDocument { TableName = "TextAll" };
        var src = br.ReadBytes((int)remain);

        var parsed = ParseTextAllFromBuffer(src);
        if (parsed.Rows.Count > 0) return parsed;

        // Some builds wrap the already-decrypted payload with zlib (RFC1950)
        if (TryZlibDecompress(src, out var dec))
        {
            var parsedDec = ParseTextAllFromBuffer(dec);
            if (parsedDec.Rows.Count > 0) return parsedDec;
        }
        // Try gzip or raw deflate as well
        if (TryAnyDecompress(src, out var decAny))
        {
            var parsedAny = ParseTextAllFromBuffer(decAny);
            if (parsedAny.Rows.Count > 0) return parsedAny;
        }
        // Try skipping a 4-byte prologue then zlib
        if (src.Length > 8 && TryZlibDecompress(src.AsSpan(4), out var dec2))
        {
            var parsedDec2 = ParseTextAllFromBuffer(dec2);
            if (parsedDec2.Rows.Count > 0) return parsedDec2;
        }
        if (src.Length > 8 && TryAnyDecompress(src.AsSpan(4), out var decAny2))
        {
            var parsedAny2 = ParseTextAllFromBuffer(decAny2);
            if (parsedAny2.Rows.Count > 0) return parsedAny2;
        }
        // Last resort: probe for an embedded compressed stream at small offsets
        if (TryProbeDecompress(src, out var decProbe))
        {
            var parsedProbe = ParseTextAllFromBuffer(decProbe);
            if (parsedProbe.Rows.Count > 0) return parsedProbe;
        }
        // Try simple obfuscation transforms (XOR/add/sub/bit-rot) often used after DES
        if (TrySimpleTransforms(src, out var deobf))
        {
            var parsedObf = ParseTextAllFromBuffer(deobf);
            if (parsedObf.Rows.Count > 0) return parsedObf;
        }
        // Deep-scan: look for a plausible container start anywhere in the stream
        if (TryDeepScanParse(src, out var parsedDeep))
        {
            if (parsedDeep.Rows.Count > 0) return parsedDeep;
        }
        return parsed; // empty document with correct name
    }

    // Parse TextAll from a raw buffer (no encryption/compression), scanning for a plausible window
    private static RdfDocument ParseTextAllFromBuffer(ReadOnlySpan<byte> buffer)
    {
        var doc = new RdfDocument { TableName = "TextAll" };
        if (buffer.Length < 8) return doc;
        using var ms = new MemoryStream(buffer.ToArray(), writable: false);
        using var br = new BinaryReader(ms, Encoding.UTF8, leaveOpen: true);

        const int TABLE_COUNT = 28; // CTextAllTable::TABLETYPE enum (0..27)
        long fileLen = buffer.Length;
        long basePos = 0;

        bool ValidatePayload(long start, int size)
        {
            if (size <= 0) return true; // allow empty
            if (start < 0 || start + size > fileLen) return false;
            br.BaseStream.Position = start;
            if (br.BaseStream.Position >= br.BaseStream.Length) return false;
            br.ReadByte(); // margin
            long pos = start + 1;
            int guard = 0;
            while (pos + 6 <= start + size)
            {
                br.BaseStream.Position = pos;
                uint _tbl = br.ReadUInt32();
                ushort len = br.ReadUInt16();
                pos += 6;
                long need = (long)len * 2;
                long remain = (start + size) - pos;
                if (need < 0 || need > remain) return false;
                pos += need;
                if (++guard > 20000) break;
            }
            long slack = (start + size) - pos;
            return slack >= 0 && slack <= 8;
        }

        (int rows, long endPos) ScanFrom(long absOff, long limitEnd)
        {
            long pos = absOff;
            int rows = 0;
            while (pos + 8 <= limitEnd)
            {
                br.BaseStream.Position = pos;
                int type = br.ReadInt32();
                int size = br.ReadInt32();
                if (type < 0 || type >= TABLE_COUNT) break;
                if (size < 0) break;
                long remain = limitEnd - (pos + 8);
                if (size > remain) break;
                if (!ValidatePayload(pos + 8, size)) break;
                pos = pos + 8 + size;
                rows++;
                if (rows > 4096) break;
                if (limitEnd - pos < 8) break;
            }
            return (rows, pos);
        }

        long winStart = basePos;
        long winEnd = fileLen;
        if (fileLen - basePos >= 12)
        {
            br.BaseStream.Position = basePos;
            int totalLenCandidate = br.ReadInt32();
            long after = basePos + 4 + Math.Max(0, totalLenCandidate);
            if (totalLenCandidate > 0 && after <= fileLen)
            {
                var probe = ScanFrom(basePos + 4, after);
                if (probe.rows > 0)
                {
                    winStart = basePos + 4;
                    winEnd = after;
                }
            }
        }

        long searchLimit = Math.Min(winStart + 1024 * 1024, winEnd);
        (long offset, int rows, long endPos)? best = null;
        foreach (var step in new long[] { 4, 2, 1 })
        {
            for (long off = winStart; off + 8 <= searchLimit; off += step)
            {
                br.BaseStream.Position = off;
                int t = br.ReadInt32();
                int s = br.ReadInt32();
                if (t < 0 || t >= TABLE_COUNT) continue;
                if (s < 0 || s > (winEnd - (off + 8))) continue;
                var (rows, endPos) = ScanFrom(off, winEnd);
                if (rows <= 0) continue;
                if (best == null) best = (off, rows, endPos);
                else
                {
                    var cur = best.Value;
                    if (rows > cur.rows || (rows == cur.rows && Math.Abs(winEnd - endPos) < Math.Abs(winEnd - cur.endPos)))
                        best = (off, rows, endPos);
                }
                if (best?.rows >= 8 && Math.Abs(winEnd - best.Value.endPos) < 64) break;
            }
            if (best != null) break;
        }

        if (best == null) return doc;

        // Materialize headers as rows
        long curPos = best.Value.offset;
        int index = 0;
        while (curPos + 8 <= winEnd)
        {
            br.BaseStream.Position = curPos;
            uint tableType = br.ReadUInt32();
            uint payloadSize = br.ReadUInt32();
            long remain2 = winEnd - (curPos + 8);
            if (tableType >= TABLE_COUNT) break;
            if (payloadSize > remain2) break;
            var r = new RdfRow { Id = index++ };
            r.SetScalar("TableType", tableType);
            r.SetScalar("PayloadSize", payloadSize);
            doc.Rows.Add(r);
            curPos = curPos + 8 + payloadSize;
        }
        return doc;
    }

    // Decompress RFC1950 (zlib) payload if present; probe at small offsets for header 0x78 0x??
    private static bool TryZlibDecompress(ReadOnlySpan<byte> src, out byte[] decompressed)
    {
        decompressed = Array.Empty<byte>();
        try
        {
            // Probe a small window near the start for a zlib header (RFC1950): 0x78 0x?? where (CMF*256+FLG) % 31 == 0 and CM=8
            int maxProbe = Math.Min(src.Length - 2, 64);
            for (int i = 0; i <= maxProbe; i++)
            {
                if (src[i] != 0x78 || i + 1 >= src.Length) continue;
                byte flg = src[i + 1];
                // CMF low 4 bits must be 8 for deflate; window size bits are high 4 bits (typically 7)
                if ( (src[i] & 0x0F) != 8 ) continue;
                int cmfflg = (src[i] << 8) | flg;
                if (cmfflg % 31 != 0) continue; // invalid zlib header
                using var ms = new MemoryStream(src.Slice(i).ToArray(), writable: false);
                using var z = new ZLibStream(ms, CompressionMode.Decompress, leaveOpen: true);
                using var outMs = new MemoryStream();
                z.CopyTo(outMs);
                var buf = outMs.ToArray();
                if (buf.Length > 0) { decompressed = buf; return true; }
            }
        }
        catch
        {
            // ignore
        }
        return false;
    }

    private static bool TryGzipDecompress(ReadOnlySpan<byte> src, out byte[] decompressed)
    {
        decompressed = Array.Empty<byte>();
        try
        {
            int maxProbe = Math.Min(src.Length - 2, 64);
            for (int i = 0; i <= maxProbe; i++)
            {
                if (i + 2 > src.Length) break;
                if (src[i] == 0x1F && src[i + 1] == 0x8B)
                {
                    using var ms = new MemoryStream(src.Slice(i).ToArray(), writable: false);
                    using var gz = new GZipStream(ms, CompressionMode.Decompress, leaveOpen: true);
                    using var outMs = new MemoryStream();
                    gz.CopyTo(outMs);
                    var buf = outMs.ToArray();
                    if (buf.Length > 0) { decompressed = buf; return true; }
                }
            }
        }
        catch { }
        return false;
    }

    private static bool TryDeflateDecompress(ReadOnlySpan<byte> src, out byte[] decompressed)
    {
        decompressed = Array.Empty<byte>();
        try
        {
            using var ms = new MemoryStream(src.ToArray(), writable: false);
            using var df = new DeflateStream(ms, CompressionMode.Decompress, leaveOpen: true);
            using var outMs = new MemoryStream();
            df.CopyTo(outMs);
            var buf = outMs.ToArray();
            if (buf.Length > 0) { decompressed = buf; return true; }
        }
        catch { }
        return false;
    }

    private static bool TryAnyDecompress(ReadOnlySpan<byte> src, out byte[] decompressed)
    {
        if (TryGzipDecompress(src, out decompressed)) return true;
        if (TryDeflateDecompress(src, out decompressed)) return true;
        decompressed = Array.Empty<byte>();
        return false;
    }

    // Probe for any embedded compressed stream by sliding start offset
    private static bool TryProbeDecompress(ReadOnlySpan<byte> src, out byte[] decompressed)
    {
        decompressed = Array.Empty<byte>();
        int limit = Math.Min(src.Length, 4096);
        // First, probe for zlib/gzip signatures at offsets
        for (int i = 0; i < limit - 2; i++)
        {
            if (TryZlibDecompress(src.Slice(i), out decompressed)) return true;
            if (TryGzipDecompress(src.Slice(i), out decompressed)) return true;
        }
        // Then, attempt raw deflate from a sliding window (best effort)
        for (int i = 0; i < limit - 32; i += 4)
        {
            if (TryDeflateDecompress(src.Slice(i), out decompressed)) return true;
        }
        return false;
    }

    private static bool TrySimpleTransforms(ReadOnlySpan<byte> src, out byte[] output)
    {
        static byte[] Apply(ReadOnlySpan<byte> s, Func<byte, byte> f)
        {
            var o = new byte[s.Length];
            for (int i = 0; i < s.Length; i++) o[i] = f(s[i]);
            return o;
        }
        static byte Rol(byte b, int n) => (byte)(((b << n) | (b >> (8 - n))) & 0xFF);
        static byte Ror(byte b, int n) => (byte)(((b >> n) | (b << (8 - n))) & 0xFF);

        // Candidates: XOR constants
        foreach (var c in new byte[] { 0xFF, 0xAA, 0x55, 0xCC, 0x33 })
        {
            var buf = Apply(src, b => (byte)(b ^ c));
            var doc = ParseTextAllFromBuffer(buf);
            if (doc.Rows.Count > 0) { output = buf; return true; }
        }
        // Add/Sub small constants
        foreach (var c in new byte[] { 1, 2, 4, 8 })
        {
            var bufAdd = Apply(src, b => (byte)(b + c));
            if (ParseTextAllFromBuffer(bufAdd).Rows.Count > 0) { output = bufAdd; return true; }
            var bufSub = Apply(src, b => (byte)(b - c));
            if (ParseTextAllFromBuffer(bufSub).Rows.Count > 0) { output = bufSub; return true; }
        }
        // Bit rotations
        foreach (var n in new int[] { 1, 2, 3 })
        {
            var bufRol = Apply(src, b => Rol(b, n));
            if (ParseTextAllFromBuffer(bufRol).Rows.Count > 0) { output = bufRol; return true; }
            var bufRor = Apply(src, b => Ror(b, n));
            if (ParseTextAllFromBuffer(bufRor).Rows.Count > 0) { output = bufRor; return true; }
        }
        output = Array.Empty<byte>();
        return false;
    }

    private static bool TryDeepScanParse(ReadOnlySpan<byte> src, out RdfDocument doc)
    {
        // Limit scan to first 8 MiB to avoid pathological runtimes
        int limit = Math.Min(src.Length, 8 * 1024 * 1024);
        foreach (int step in new int[] { 4, 1 })
        {
            for (int off = 0; off < limit; off += step)
            {
                var sub = src.Slice(off);
                var parsed = ParseTextAllFromBuffer(sub);
                if (parsed.Rows.Count > 0)
                {
                    doc = parsed;
                    return true;
                }
            }
        }
        doc = new RdfDocument { TableName = "TextAll" };
        return false;
    }

    // Core row reader for schema-driven tables
    private static bool TryReadRow(BinaryReader br, TableSchema schema, RdfRow row, RdfDocument doc)
    {
        long start = br.BaseStream.Position;
        int offset = 0;
        bool hasVariable = schema.Fields.Any(f => !f.IsArray && f.Type == ScalarType.WStringVar);
        try
        {
            foreach (var f in schema.Fields)
            {
                int elemAlign = Math.Min(GetScalarAlignment(f), schema.PackAlignment);
                int pad = Padding(offset, elemAlign);
                if (pad > 0)
                {
                    br.ReadBytes(pad);
                    offset += pad;
                }
                if (f.IsArray)
                {
                    switch (f.Type)
                    {
                        case ScalarType.U8:
                        case ScalarType.Bool:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadByte());
                            row.SetArray(f.Name, arr);
                            offset += f.Length;
                            break;
                        }
                        case ScalarType.S8:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadSByte());
                            row.SetArray(f.Name, arr);
                            offset += f.Length;
                            break;
                        }
                        case ScalarType.U16:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadUInt16());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 2;
                            break;
                        }
                        case ScalarType.S16:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadInt16());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 2;
                            break;
                        }
                        case ScalarType.U32:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadUInt32());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 4;
                            break;
                        }
                        case ScalarType.S32:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadInt32());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 4;
                            break;
                        }
                        case ScalarType.Float:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadSingle());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 4;
                            break;
                        }
                        case ScalarType.Double:
                        {
                            var arr = ReadArray(br, f.Length, () => br.ReadDouble());
                            row.SetArray(f.Name, arr);
                            offset += f.Length * 8;
                            break;
                        }
                        default:
                            throw new NotSupportedException($"Array type not supported: {f.Type}");
                    }
                }
                else
                {
                    switch (f.Type)
                    {
                        case ScalarType.U8:
                        case ScalarType.Bool:
                            row.SetScalar(f.Name, br.ReadByte()); offset += 1; break;
                        case ScalarType.S8:
                            row.SetScalar(f.Name, br.ReadSByte()); offset += 1; break;
                        case ScalarType.U16:
                            row.SetScalar(f.Name, br.ReadUInt16()); offset += 2; break;
                        case ScalarType.S16:
                            row.SetScalar(f.Name, br.ReadInt16()); offset += 2; break;
                        case ScalarType.U32:
                            row.SetScalar(f.Name, br.ReadUInt32()); offset += 4; break;
                        case ScalarType.S32:
                            row.SetScalar(f.Name, br.ReadInt32()); offset += 4; break;
                        case ScalarType.Float:
                            row.SetScalar(f.Name, br.ReadSingle()); offset += 4; break;
                        case ScalarType.Double:
                            row.SetScalar(f.Name, br.ReadDouble()); offset += 8; break;
                        case ScalarType.WStringFixed:
                        {
                            bool isItemName = string.Equals(schema.Name, "Item", StringComparison.OrdinalIgnoreCase) && string.Equals(f.Name, "NameText", StringComparison.Ordinal);
                            if (isItemName && doc.ItemNameTextIsVar)
                            {
                                ushort wlen = br.ReadUInt16(); offset += 2;
                                var bytes = br.ReadBytes(wlen * 2); offset += wlen * 2;
                                var s = Encoding.Unicode.GetString(bytes);
                                var cut = s.IndexOf('\0'); if (cut >= 0) s = s.Substring(0, cut);
                                row.SetScalar(f.Name, s);
                            }
                            else
                            {
                                int len = f.Length;
                                if (isItemName && doc.ItemNameTextChars is int det && (det == 41 || det == 65)) len = det;
                                bool tryAnsi = isItemName;
                                var before = br.BaseStream.Position;
                                var s = ReadFixedUnicode(br, len, tryAnsi);
                                if (isItemName)
                                {
                                    // Decide if ANSI fallback was used by comparing raw decode to heuristic
                                    br.BaseStream.Position = before;
                                    var bytes = br.ReadBytes(len * 2);
                                    var u16 = Encoding.Unicode.GetString(bytes);
                                    var idx = u16.IndexOf('\0'); u16 = idx >= 0 ? u16.Substring(0, idx) : u16;
                                    doc.ItemNameTextAnsiFallback = s != u16;
                                }
                                row.SetScalar(f.Name, s);
                                offset += len * 2;
                            }
                            break;
                        }
                        case ScalarType.WStringVar:
                        {
                            ushort len = br.ReadUInt16(); offset += 2;
                            string s = string.Empty;
                            if (len > 0)
                            {
                                var bytes = br.ReadBytes(len * 2);
                                s = Encoding.Unicode.GetString(bytes);
                                offset += len * 2;
                            }
                            row.SetScalar(f.Name, s);
                            break;
                        }
                        case ScalarType.AnsiStringFixed:
                        {
                            var s = ReadFixedAnsi(br, f.Length);
                            row.SetScalar(f.Name, s);
                            offset += f.Length;
                            break;
                        }
                        default:
                            throw new NotSupportedException($"Type not supported: {f.Type}");
                    }
                }
            }
            if (!hasVariable)
            {
                int tail = Padding(offset, schema.PackAlignment);
                if (tail > 0) { br.ReadBytes(tail); offset += tail; }
            }
            return true;
        }
        catch
        {
            // Rewind and signal end-of-file/parse failure to caller
            br.BaseStream.Position = start;
            return false;
        }
    }

    private static void WriteRow(BinaryWriter bw, TableSchema schema, RdfRow row, RdfDocument? doc)
    {
        int offset = 0;
        bool hasVariable = schema.Fields.Any(f => !f.IsArray && f.Type == ScalarType.WStringVar);
        foreach (var f in schema.Fields)
        {
            int elemAlign = Math.Min(GetScalarAlignment(f), schema.PackAlignment);
            int pad = Padding(offset, elemAlign);
            if (pad > 0) { bw.Write(new byte[pad]); offset += pad; }

            if (f.IsArray)
            {
                switch (f.Type)
                {
                    case ScalarType.U8:
                    case ScalarType.Bool:
                        foreach (var v in row.GetArray<byte>(f.Name, f.Length)) { bw.Write(v); offset += 1; }
                        break;
                    case ScalarType.S8:
                        foreach (var v in row.GetArray<sbyte>(f.Name, f.Length)) { bw.Write(v); offset += 1; }
                        break;
                    case ScalarType.U16:
                        foreach (var v in row.GetArray<ushort>(f.Name, f.Length)) { bw.Write(v); offset += 2; }
                        break;
                    case ScalarType.S16:
                        foreach (var v in row.GetArray<short>(f.Name, f.Length)) { bw.Write(v); offset += 2; }
                        break;
                    case ScalarType.U32:
                        foreach (var v in row.GetArray<uint>(f.Name, f.Length)) { bw.Write(v); offset += 4; }
                        break;
                    case ScalarType.S32:
                        foreach (var v in row.GetArray<int>(f.Name, f.Length)) { bw.Write(v); offset += 4; }
                        break;
                    case ScalarType.Float:
                        foreach (var v in row.GetArray<float>(f.Name, f.Length)) { bw.Write(v); offset += 4; }
                        break;
                    case ScalarType.Double:
                        foreach (var v in row.GetArray<double>(f.Name, f.Length)) { bw.Write(v); offset += 8; }
                        break;
                    default:
                        throw new NotSupportedException($"Array type not supported: {f.Type}");
                }
            }
            else
            {
                switch (f.Type)
                {
                    case ScalarType.U8:
                    case ScalarType.Bool:
                        bw.Write(Convert.ToByte(row.GetScalarOrDefault(f.Name, (byte)0))); offset += 1; break;
                    case ScalarType.S8:
                        bw.Write(Convert.ToSByte(row.GetScalarOrDefault(f.Name, (sbyte)0))); offset += 1; break;
                    case ScalarType.U16:
                        bw.Write(Convert.ToUInt16(row.GetScalarOrDefault(f.Name, (ushort)0))); offset += 2; break;
                    case ScalarType.S16:
                        bw.Write(Convert.ToInt16(row.GetScalarOrDefault(f.Name, (short)0))); offset += 2; break;
                    case ScalarType.U32:
                        bw.Write(Convert.ToUInt32(row.GetScalarOrDefault(f.Name, 0u))); offset += 4; break;
                    case ScalarType.S32:
                        bw.Write(Convert.ToInt32(row.GetScalarOrDefault(f.Name, 0))); offset += 4; break;
                    case ScalarType.Float:
                        bw.Write(Convert.ToSingle(row.GetScalarOrDefault(f.Name, 0f))); offset += 4; break;
                    case ScalarType.Double:
                        bw.Write(Convert.ToDouble(row.GetScalarOrDefault(f.Name, 0.0))); offset += 8; break;
                    case ScalarType.WStringFixed:
                    {
                        bool isItemName = string.Equals(schema.Name, "Item", StringComparison.OrdinalIgnoreCase) && string.Equals(f.Name, "NameText", StringComparison.Ordinal);
                        if (isItemName && (doc?.ItemNameTextIsVar ?? false))
                        {
                            var s = Convert.ToString(row.GetScalarOrDefault(f.Name, string.Empty)) ?? string.Empty;
                            var wlen = (ushort)s.Length;
                            bw.Write(wlen); offset += 2;
                            if (wlen > 0)
                            {
                                var bytes = Encoding.Unicode.GetBytes(s);
                                bw.Write(bytes); offset += bytes.Length;
                            }
                        }
                        else
                        {
                            int len = f.Length;
                            if (isItemName && doc?.ItemNameTextChars is int detected && (detected == 41 || detected == 65)) len = detected;
                            var s = Convert.ToString(row.GetScalarOrDefault(f.Name, string.Empty)) ?? string.Empty;
                            WriteFixedUnicode(bw, s, len); offset += len * 2;
                        }
                        break;
                    }
                    case ScalarType.WStringVar:
                    {
                        var s = Convert.ToString(row.GetScalarOrDefault(f.Name, string.Empty)) ?? string.Empty;
                        var len = (ushort)s.Length;
                        bw.Write(len); offset += 2;
                        if (len > 0)
                        {
                            var bytes = Encoding.Unicode.GetBytes(s);
                            bw.Write(bytes); offset += bytes.Length;
                        }
                        break;
                    }
                    case ScalarType.AnsiStringFixed:
                        WriteFixedAnsi(bw, Convert.ToString(row.GetScalarOrDefault(f.Name, string.Empty))!, f.Length); offset += f.Length; break;
                    default:
                        throw new NotSupportedException($"Type not supported: {f.Type}");
                }
            }
        }
        if (!hasVariable)
        {
            int tail = Padding(offset, schema.PackAlignment);
            if (tail > 0) { bw.Write(new byte[tail]); offset += tail; }
        }
    }

    private static void DetectItemNameTextVariant(BinaryReader br, TableSchema schema, RdfDocument doc)
    {
        long start = br.BaseStream.Position;

        int primaryHint = PeekItemNameLength(br, schema);
        bool allowVariable = primaryHint < 0 || primaryHint <= 512;

        var candidates = new List<ItemNameVariant>(3);
        if (allowVariable)
            candidates.Add(new ItemNameVariant("Variable", true, null));
        candidates.Add(new ItemNameVariant("Fixed41", false, 41));
        candidates.Add(new ItemNameVariant("Fixed65", false, 65));

        ItemVariantStats? bestStats = null;
        ItemNameVariant? bestVariant = null;

        foreach (var candidate in candidates)
        {
            var stats = ProbeItemVariant(br, schema, candidate);
            if (!stats.Success)
                continue;

            if (!candidate.IsVar)
            {
                int stride = EstimateRowStride(schema, candidate);
                if (stride > 0 && stats.RowsRead > 0)
                {
                    double diff = Math.Abs(stats.AverageRowSize - stride);
                    if (diff <= 4)
                        stats.Score += 15.0;
                    else if (diff <= 12)
                        stats.Score += 8.0;
                    else
                        stats.Score -= diff * 2.5;

                    if (stats.RowSizeMax - stats.RowSizeMin > 4)
                        stats.Score -= 20.0;
                }
            }
            else
            {
                if (!allowVariable)
                    stats.Score -= 400.0;
                else if (primaryHint > 512)
                    stats.Score -= 200.0;
            }

            if (bestStats == null || stats.Score > bestStats.Score)
            {
                bestStats = stats;
                bestVariant = candidate;
            }
        }

        if (bestVariant == null)
        {
            doc.ItemNameTextIsVar = false;
            doc.ItemNameTextChars = 41;
            doc.ItemNameTextAnsiFallback = false;
            br.BaseStream.Position = start;
            return;
        }

        doc.ItemNameTextIsVar = bestVariant.IsVar;
        doc.ItemNameTextChars = bestVariant.FixedChars;
        doc.ItemNameTextAnsiFallback = false;
        br.BaseStream.Position = start;
    }

    private sealed record ItemNameVariant(string Name, bool IsVar, int? FixedChars);

    private sealed class ItemVariantStats
    {
        public bool Success { get; set; }
        public int RowsRead { get; set; }
        public int ValidTblidx { get; set; }
        public int InvalidTblidx { get; set; }
        public int MonotonicTransitions { get; set; }
        public double IconPrintableRatio { get; set; }
        public double Score { get; set; }
        public long BytesConsumed { get; set; }
        public int RowSizeMin { get; set; } = int.MaxValue;
        public int RowSizeMax { get; set; }
        public double AverageRowSize { get; set; }
    }

    private static ItemVariantStats ProbeItemVariant(BinaryReader br, TableSchema schema, ItemNameVariant variant, int sampleRows = 128)
    {
        long savedPos = br.BaseStream.Position;
        long rowStartPos = schema.HasMargin ? 1 : 0;
        if (rowStartPos > br.BaseStream.Length)
            rowStartPos = br.BaseStream.Length;

        br.BaseStream.Position = rowStartPos;

        var stats = new ItemVariantStats();
        var probeDoc = new RdfDocument
        {
            TableName = schema.Name,
            ItemNameTextIsVar = variant.IsVar,
            ItemNameTextChars = variant.FixedChars
        };

        uint prevTbl = 0;
        bool havePrev = false;
        int printableTotal = 0;
        int printableValid = 0;
        bool parseFailed = false;

        for (int i = 0; i < sampleRows && br.BaseStream.Position < br.BaseStream.Length; i++)
        {
            var row = new RdfRow();
            long before = br.BaseStream.Position;
            if (!TryReadRow(br, schema, row, probeDoc))
            {
                parseFailed = true;
                break;
            }

            stats.RowsRead++;

            int rowBytes = (int)(br.BaseStream.Position - before);
            stats.BytesConsumed += rowBytes;
            if (rowBytes < stats.RowSizeMin) stats.RowSizeMin = rowBytes;
            if (rowBytes > stats.RowSizeMax) stats.RowSizeMax = rowBytes;

            if (row.TryGetScalar<uint>("Tblidx", out var tbl))
            {
                if (tbl > 0 && tbl < 1_000_000_000)
                {
                    stats.ValidTblidx++;
                    if (havePrev && tbl >= prevTbl)
                        stats.MonotonicTransitions++;
                    prevTbl = tbl;
                    havePrev = true;
                }
                else
                {
                    stats.InvalidTblidx++;
                }
            }
            else
            {
                stats.InvalidTblidx++;
            }

            if (row.TryGetScalar<string>("Icon_Name", out var icon) && !string.IsNullOrEmpty(icon))
            {
                printableTotal += icon.Length;
                printableValid += icon.Count(ch => ch >= 0x20 && ch < 0x7F);
            }

            if (br.BaseStream.Position == before)
            {
                parseFailed = true;
                break;
            }
        }

        if (printableTotal > 0)
            stats.IconPrintableRatio = (double)printableValid / printableTotal;

        if (stats.RowSizeMin == int.MaxValue)
            stats.RowSizeMin = 0;
        if (stats.RowsRead > 0)
            stats.AverageRowSize = stats.BytesConsumed / (double)stats.RowsRead;

        if (parseFailed)
        {
            stats.Success = false;
        }
        else if (stats.RowsRead == 0)
        {
            stats.Success = false;
        }
        else if (stats.ValidTblidx == 0)
        {
            stats.Success = false;
        }
        else if (stats.ValidTblidx < Math.Max(2, stats.RowsRead / 4))
        {
            stats.Success = false;
        }
        else if (stats.ValidTblidx > 1 && stats.MonotonicTransitions < (stats.ValidTblidx - 1) / 2)
        {
            stats.Success = false;
        }
        else
        {
            stats.Success = true;
        }

        double score;
        if (stats.Success)
        {
            double monotonicRatio = stats.ValidTblidx > 1
                ? (double)stats.MonotonicTransitions / (stats.ValidTblidx - 1)
                : 0.0;
            score = stats.ValidTblidx * 4;
            score += stats.RowsRead;
            score += monotonicRatio * 25.0;
            if (stats.IconPrintableRatio > 0.6)
                score += stats.IconPrintableRatio * 10.0;
            score -= stats.InvalidTblidx * 6;
            if (variant.IsVar)
                score += 1.0;
        }
        else
        {
            score = double.NegativeInfinity;
        }

        stats.Score = score;

        br.BaseStream.Position = savedPos;
        return stats;
    }

    private static int PeekItemNameLength(BinaryReader br, TableSchema schema)
    {
        long saved = br.BaseStream.Position;
        long basePos = schema.HasMargin ? 1 : 0;
        if (basePos >= br.BaseStream.Length)
            return -1;

        if (TryComputeItemNameOffset(schema, out int offset))
        {
            long target = basePos + offset;
            if (target + 2 <= br.BaseStream.Length)
            {
                br.BaseStream.Position = target;
                ushort len = br.ReadUInt16();
                br.BaseStream.Position = saved;
                return len;
            }
        }

        br.BaseStream.Position = saved;
        return -1;
    }

    private static bool TryComputeItemNameOffset(TableSchema schema, out int offset)
    {
        offset = 0;
        foreach (var field in schema.Fields)
        {
            int align = Math.Min(GetScalarAlignment(field), schema.PackAlignment);
            offset += Padding(offset, align);

            if (IsItemNameField(field))
                return true;

            offset += GetScalarSize(field);
        }
        return false;
    }

    private static bool IsItemNameField(Field field)
    {
        return string.Equals(field.Name, "NameText", StringComparison.Ordinal);
    }

    private static int EstimateRowStride(TableSchema schema, ItemNameVariant variant)
    {
        int offset = 0;
        bool hasVariable = schema.Fields.Any(f => !f.IsArray && f.Type == ScalarType.WStringVar);

        foreach (var field in schema.Fields)
        {
            int align = Math.Min(GetScalarAlignment(field), schema.PackAlignment);
            offset += Padding(offset, align);

            if (IsItemNameField(field))
            {
                if (variant.IsVar)
                {
                    offset += 2; // WORD length prefix
                }
                else
                {
                    int len = variant.FixedChars ?? field.Length;
                    offset += len * 2;
                }
            }
            else
            {
                offset += GetScalarSize(field);
            }
        }

        if (!hasVariable && !variant.IsVar)
        {
            int tail = Padding(offset, schema.PackAlignment);
            offset += tail;
        }

        return offset;
    }

    private static int GetScalarSize(Field field)
    {
        int elementCount = field.IsArray ? Math.Max(1, field.Length) : 1;

        return field.Type switch
        {
            ScalarType.U8 or ScalarType.S8 or ScalarType.Bool => elementCount,
            ScalarType.U16 or ScalarType.S16 => elementCount * 2,
            ScalarType.U32 or ScalarType.S32 or ScalarType.Float => elementCount * 4,
            ScalarType.Double => elementCount * 8,
            ScalarType.WStringFixed => field.Length * 2,
            ScalarType.WStringVar => elementCount * 2,
            ScalarType.AnsiStringFixed => field.Length,
            _ => elementCount
        };
    }

    private static int GetScalarAlignment(Field f) => f.Type switch
    {
        ScalarType.U8 or ScalarType.Bool => 1,
        ScalarType.S8 => 1,
        ScalarType.U16 => 2,
        ScalarType.S16 => 2,
        ScalarType.U32 => 4,
        ScalarType.S32 => 4,
        ScalarType.Float => 4,
        ScalarType.Double => 8,
        ScalarType.WStringFixed => 2,
        ScalarType.WStringVar => 2,
        ScalarType.AnsiStringFixed => 1,
        _ => 1
    };

    private static int Padding(int offset, int align)
    {
        if (align <= 1) return 0;
        int mod = offset % align;
        return mod == 0 ? 0 : (align - mod);
    }

    private static T[] ReadArray<T>(BinaryReader br, int len, Func<T> read)
    {
        var arr = new T[len];
        for (int i = 0; i < len; i++) arr[i] = read();
        return arr;
    }

    private static string ReadFixedUnicode(BinaryReader br, int wcharCount, bool tryAnsiFallback = false)
    {
        var bytes = br.ReadBytes(wcharCount * 2);
        var s = Encoding.Unicode.GetString(bytes);
        var idx = s.IndexOf('\0');
        var trimmed = idx >= 0 ? s.Substring(0, idx) : s;
        if (!tryAnsiFallback) return trimmed;
        // Heuristic: if most high bytes are zero and many low bytes are printable ASCII, prefer ANSI view
        int pairs = Math.Min(bytes.Length / 2, 64);
        int zeroHigh = 0;
        int printableLow = 0;
        for (int i = 0; i < pairs; i++)
        {
            byte low = bytes[i * 2 + 0];
            byte high = bytes[i * 2 + 1];
            if (high == 0) zeroHigh++;
            if (low >= 0x20 && low <= 0x7E) printableLow++;
        }
        if (zeroHigh >= pairs * 3 / 4 && printableLow >= pairs / 2)
        {
            // Reconstruct from low bytes only
            var ascii = new byte[wcharCount];
            for (int i = 0; i < wcharCount && i * 2 + 1 < bytes.Length; i++) ascii[i] = bytes[i * 2];
            var a = Encoding.ASCII.GetString(ascii);
            var cut = a.IndexOf('\0');
            return cut >= 0 ? a.Substring(0, cut) : a.TrimEnd('\0');
        }
        return trimmed;
    }

    private static void WriteFixedUnicode(BinaryWriter bw, string value, int wcharCount)
    {
        var chars = (value ?? string.Empty).ToCharArray();
        var take = Math.Min(chars.Length, Math.Max(1, wcharCount) - 1);
        var buf = new char[Math.Max(1, wcharCount)];
        if (take > 0) Array.Copy(chars, buf, take);
        buf[take] = '\0';
        var bytes = Encoding.Unicode.GetBytes(buf);
        bw.Write(bytes, 0, bytes.Length);
    }

    private static string ReadFixedAnsi(BinaryReader br, int charCount)
    {
        var bytes = br.ReadBytes(charCount);
        int end = Array.IndexOf<byte>(bytes, 0);
        var len = end >= 0 ? end : bytes.Length;
        if (len <= 0) return string.Empty;
        return AnsiEncoding.GetString(bytes, 0, len);
    }

    private static void WriteFixedAnsi(BinaryWriter bw, string value, int charCount)
    {
        int bufferLength = Math.Max(1, charCount);
        var buffer = new byte[bufferLength];
        int maxPayload = bufferLength - 1; // leave room for null terminator

        if (maxPayload > 0 && !string.IsNullOrEmpty(value))
        {
            var encoder = AnsiEncoding.GetEncoder();
            var chars = value.AsSpan();
            encoder.Convert(chars, buffer.AsSpan(0, maxPayload), true, out int charsUsed, out int bytesUsed, out bool _);
            // Null terminator already zero-initialized
        }

        bw.Write(buffer);
    }
}
