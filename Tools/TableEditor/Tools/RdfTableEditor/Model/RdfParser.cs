using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

namespace RdfTableEditor.Model
{
    public static class RdfParser
    {
        // Binary RDF reader for selected tables. Currently supports CharTitleTable format.
        // Fallback: if the data doesn't look like our known binary, parse simple CSV-like format.
        public static RdfDocument Parse(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return new RdfDocument();

            // Try binary first
            var bytes = System.Text.Encoding.UTF8.GetBytes(text);
            try
            {
                using var ms = new MemoryStream(bytes);
                return ParseBinary(ms);
            }
            catch
            {
                // CSV fallback
                return ParseCsv(text);
            }
        }

        public static RdfDocument Parse(Stream stream)
        {
            // Assume binary when reading from stream
            return ParseBinary(stream);
        }

        private static RdfDocument ParseCsv(string text)
        {
            var lines = text.Replace("\r", string.Empty).Split('\n');
            var doc = new RdfDocument();
            int i = 0;
            if (lines.Length > 0 && lines[0].StartsWith("["))
            {
                var end = lines[0].IndexOf(']');
                if (end > 1)
                    doc.TableName = lines[0].Substring(1, end - 1);
                i = 1;
            }
            for (; i < lines.Length; i++)
            {
                var line = lines[i].Trim();
                if (line.Length == 0 || line.StartsWith("#"))
                    continue;
                var parts = line.Split(',');
                if (parts.Length >= 1 && long.TryParse(parts[0], out long id))
                {
                    doc.Rows.Add(new RdfRow { Id = id, Name = parts.ElementAtOrDefault(1), Value = parts.ElementAtOrDefault(2) });
                }
            }
            return doc;
        }

        // Known layout for sCHARTITLE_TBLDAT as written by C++: little-endian, 4-byte pack, vptr skipped
        // We'll read records until EOF. First byte is a margin = 1.
        private static RdfDocument ParseBinary(Stream stream)
        {
            var doc = new RdfDocument { TableName = "CharTitle" };
            using var br = new BinaryReader(stream);
            // Read margin
            var margin = br.ReadByte();
            doc.Margin = margin;
            if (margin != 1)
                throw new InvalidDataException("Unexpected RDF margin");

            while (stream.Position < stream.Length)
            {
                if (!TryReadCharTitle(br, out var row))
                    break;
                doc.Rows.Add(row);
            }
            return doc;
        }

        private static bool TryReadCharTitle(BinaryReader br, out RdfRow row)
        {
            row = new RdfRow();
            // Define sizes based on C++ headers
            const int NTL_MAX_CHAR_TITLE_EFFECT = 3;
            const int DBO_MAX_LENGTH_TITLE_BONE_NAME = 0x200;
            const int DBO_MAX_LENGTH_TITLE_EFFECT_NAME = 0x200;
            const int DBO_MAX_LENGTH_TITLE_SOUND_NAME = 0x200;

            // Record layout (after vptr skipped by C++):
            // TBLIDX tblidx;
            // TBLIDX tblNameIndex;
            // BYTE byContentsType;
            // BYTE byRepresentationType;
            // WCHAR wszBoneName[DBO_MAX_LENGTH_TITLE_BONE_NAME + 1];
            // WCHAR wszEffectName[DBO_MAX_LENGTH_TITLE_EFFECT_NAME + 1];
            // WCHAR wszEffectSound[DBO_MAX_LENGTH_TITLE_SOUND_NAME + 1];
            // TBLIDX atblSystem_Effect_Index[3];
            // BYTE   abySystem_Effect_Type[3];
            // double abySystem_Effect_Value[3];

            // Compute total size to ensure we don't read past EOF
            int stringChars = (DBO_MAX_LENGTH_TITLE_BONE_NAME + 1)
                              + (DBO_MAX_LENGTH_TITLE_EFFECT_NAME + 1)
                              + (DBO_MAX_LENGTH_TITLE_SOUND_NAME + 1);
            int size = 4 // tblidx
                      + 4 // tblNameIndex
                      + 1 // byContentsType
                      + 1 // byRepresentationType
                      + (stringChars * 2) // WCHARs
                      + (NTL_MAX_CHAR_TITLE_EFFECT * 4) // TBLIDX array
                      + (NTL_MAX_CHAR_TITLE_EFFECT * 1) // type bytes
                      + (NTL_MAX_CHAR_TITLE_EFFECT * 8); // double array

            // If not enough bytes left, stop.
            var remaining = br.BaseStream.Length - br.BaseStream.Position;
            if (remaining < size)
                return false;

            row.Id = br.ReadUInt32();
            row.TitleNameIndex = br.ReadInt32();
            row.ContentsType = br.ReadByte();
            row.RepresentationType = br.ReadByte();

            row.BoneName = ReadFixedUnicode(br, DBO_MAX_LENGTH_TITLE_BONE_NAME + 1);
            row.EffectName = ReadFixedUnicode(br, DBO_MAX_LENGTH_TITLE_EFFECT_NAME + 1);
            row.EffectSound = ReadFixedUnicode(br, DBO_MAX_LENGTH_TITLE_SOUND_NAME + 1);

            row.SystemEffectTblidx = new int[NTL_MAX_CHAR_TITLE_EFFECT];
            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
                row.SystemEffectTblidx[i] = br.ReadInt32();

            row.SystemEffectType = new byte[NTL_MAX_CHAR_TITLE_EFFECT];
            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
                row.SystemEffectType[i] = br.ReadByte();

            row.SystemEffectValue = new double[NTL_MAX_CHAR_TITLE_EFFECT];
            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
                row.SystemEffectValue[i] = br.ReadDouble();

            return true;
        }

        private static string ReadFixedUnicode(BinaryReader br, int wcharCount)
        {
            // Read fixed-length UTF-16LE char buffer and trim at first NUL
            var bytes = br.ReadBytes(wcharCount * 2);
            var s = System.Text.Encoding.Unicode.GetString(bytes);
            var idx = s.IndexOf('\0');
            return idx >= 0 ? s.Substring(0, idx) : s;
        }
    }

    public static class RdfSerializer
    {
        public static void Serialize(RdfDocument doc, Stream stream)
        {
            // Write CharTitle binary; if TableName unknown, default to CharTitle
            using var bw = new BinaryWriter(stream, System.Text.Encoding.UTF8, leaveOpen: true);
            bw.Write(doc.Margin ?? (byte)1); // margin (preserve if known)
            foreach (var r in doc.Rows)
                WriteCharTitle(bw, r);
            bw.Flush();
        }

        private static void WriteCharTitle(BinaryWriter bw, RdfRow r)
        {
            const int NTL_MAX_CHAR_TITLE_EFFECT = 3;
            const int DBO_MAX_LENGTH_TITLE_BONE_NAME = 0x200;
            const int DBO_MAX_LENGTH_TITLE_EFFECT_NAME = 0x200;
            const int DBO_MAX_LENGTH_TITLE_SOUND_NAME = 0x200;

            bw.Write(unchecked((uint)r.Id));
            bw.Write(r.TitleNameIndex);
            bw.Write(r.ContentsType);
            bw.Write(r.RepresentationType);
            WriteFixedUnicode(bw, r.BoneName ?? string.Empty, DBO_MAX_LENGTH_TITLE_BONE_NAME + 1);
            WriteFixedUnicode(bw, r.EffectName ?? string.Empty, DBO_MAX_LENGTH_TITLE_EFFECT_NAME + 1);
            WriteFixedUnicode(bw, r.EffectSound ?? string.Empty, DBO_MAX_LENGTH_TITLE_SOUND_NAME + 1);

            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
            {
                int v = (r.SystemEffectTblidx != null && i < r.SystemEffectTblidx.Length) ? r.SystemEffectTblidx[i] : 0;
                bw.Write(v);
            }
            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
            {
                byte v = (r.SystemEffectType != null && i < r.SystemEffectType.Length) ? r.SystemEffectType[i] : (byte)0;
                bw.Write(v);
            }
            for (int i = 0; i < NTL_MAX_CHAR_TITLE_EFFECT; i++)
            {
                double v = (r.SystemEffectValue != null && i < r.SystemEffectValue.Length) ? r.SystemEffectValue[i] : 0.0;
                bw.Write(v);
            }
        }

        private static void WriteFixedUnicode(BinaryWriter bw, string value, int wcharCount)
        {
            // Write UTF-16LE fixed buffer (wcharCount), zero-padded, with NUL terminator ensured
            var chars = (value ?? string.Empty).ToCharArray();
            var take = Math.Min(chars.Length, wcharCount - 1); // leave room for NUL
            var buf = new char[wcharCount];
            Array.Copy(chars, buf, take);
            buf[take] = '\0';
            var bytes = System.Text.Encoding.Unicode.GetBytes(buf);
            bw.Write(bytes, 0, bytes.Length);
        }
    }
}
