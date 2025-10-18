using System;
using System.Collections.Generic;

namespace RdfTableEditor.Model
{
    public class RdfDocument
    {
        public string TableName { get; set; } = string.Empty;
        public List<RdfRow> Rows { get; set; } = new List<RdfRow>();
    // Some tables start with a margin/padding byte. Preserve the value we read so we can write it back unchanged.
    public byte? Margin { get; set; }
    // For Item table variants: some builds use 41 WCHARs for NameText, others 65.
    // We detect and remember the value so we can write back identically.
    public int? ItemNameTextChars { get; set; }
    // Some retail dumps store NameText as WORD length + UTF-16 (variable). If detected, set this and ignore fixed count.
    public bool ItemNameTextIsVar { get; set; }
    // If ANSI fallback decoding was chosen for NameText during read, remember that to guide write normalization if needed.
    public bool ItemNameTextAnsiFallback { get; set; }
    }

    public class RdfRow
    {
    // Common key
    public long Id { get; set; }

    // CharTitle fields
    public int TitleNameIndex { get; set; }
    public byte ContentsType { get; set; }
    public byte RepresentationType { get; set; }
    public string? BoneName { get; set; }
    public string? EffectName { get; set; }
    public string? EffectSound { get; set; }
    public int[]? SystemEffectTblidx { get; set; }
    public byte[]? SystemEffectType { get; set; }
    public double[]? SystemEffectValue { get; set; }

        // UI CSV proxies for simple editing of 3-length arrays
        public string SystemEffectTblidxCsv
        {
            get => SystemEffectTblidx == null ? string.Empty : string.Join(",", SystemEffectTblidx);
            set => SystemEffectTblidx = ParseIntArray(value, 3);
        }
        public string SystemEffectTypeCsv
        {
            get => SystemEffectType == null ? string.Empty : string.Join(",", SystemEffectType);
            set => SystemEffectType = ParseByteArray(value, 3);
        }
        public string SystemEffectValueCsv
        {
            get => SystemEffectValue == null ? string.Empty : string.Join(",", SystemEffectValue);
            set => SystemEffectValue = ParseDoubleArray(value, 3);
        }

        private static int[] ParseIntArray(string? csv, int len)
        {
            var res = new int[len];
            if (string.IsNullOrWhiteSpace(csv)) return res;
            var parts = csv.Split(',');
            for (int i = 0; i < Math.Min(len, parts.Length); i++)
                res[i] = int.TryParse(parts[i].Trim(), out var v) ? v : 0;
            return res;
        }
        private static byte[] ParseByteArray(string? csv, int len)
        {
            var res = new byte[len];
            if (string.IsNullOrWhiteSpace(csv)) return res;
            var parts = csv.Split(',');
            for (int i = 0; i < Math.Min(len, parts.Length); i++)
                res[i] = byte.TryParse(parts[i].Trim(), out var v) ? v : (byte)0;
            return res;
        }
        private static double[] ParseDoubleArray(string? csv, int len)
        {
            var res = new double[len];
            if (string.IsNullOrWhiteSpace(csv)) return res;
            var parts = csv.Split(',');
            for (int i = 0; i < Math.Min(len, parts.Length); i++)
                res[i] = double.TryParse(parts[i].Trim(), out var v) ? v : 0.0;
            return res;
        }

    // Legacy placeholders for CSV fallback
    public string? Name { get; set; }
    public string? Value { get; set; }

    // Generic dynamic storage for schema-driven IO
    private readonly Dictionary<string, object?> _scalars = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, Array> _arrays = new(StringComparer.OrdinalIgnoreCase);

    public void SetScalar(string name, object? value)
    {
        _scalars[name] = value;
        // Map common aliases to typed CharTitle properties for UI
        switch (name)
        {
            case "Tblidx":
                // Tblidx is stored as an unsigned 32-bit value in RDF assets; preserve the full range.
                if (value is uint u)
                {
                    Id = u;
                }
                else if (value is ulong ul)
                {
                    Id = unchecked((long)ul);
                }
                else if (value is long l)
                {
                    Id = l;
                }
                else
                {
                    try { Id = Convert.ToInt64(value ?? 0); }
                    catch { Id = 0; }
                }
                break;
            case "NameIndex":
                TitleNameIndex = Convert.ToInt32(value ?? 0);
                break;
            case "ContentsType":
                ContentsType = Convert.ToByte(value ?? (byte)0);
                break;
            case "RepresentationType":
                RepresentationType = Convert.ToByte(value ?? (byte)0);
                break;
            case "BoneName":
                BoneName = Convert.ToString(value);
                break;
            case "EffectName":
                EffectName = Convert.ToString(value);
                break;
            case "EffectSound":
                EffectSound = Convert.ToString(value);
                break;
        }
    }

    public void SetArray<T>(string name, T[] values)
    {
        _arrays[name] = values;
        // CharTitle mappings
        if (name.Equals("SystemEffectTblidx", StringComparison.OrdinalIgnoreCase))
        {
            SystemEffectTblidx = values as int[] ?? Array.ConvertAll(values, x => Convert.ToInt32(x));
        }
        else if (name.Equals("SystemEffectType", StringComparison.OrdinalIgnoreCase))
        {
            SystemEffectType = values as byte[] ?? Array.ConvertAll(values, x => Convert.ToByte(x));
        }
        else if (name.Equals("SystemEffectValue", StringComparison.OrdinalIgnoreCase))
        {
            SystemEffectValue = values as double[] ?? Array.ConvertAll(values, x => Convert.ToDouble(x));
        }
    }

    public bool TryGetScalar<T>(string name, out T value)
    {
        if (_scalars.TryGetValue(name, out var boxed) && boxed is not null)
        {
            value = (T)Convert.ChangeType(boxed, typeof(T));
            return true;
        }
        // Fallback to typed CharTitle mapping
        object? fallback = null;
        switch (name)
        {
            case "Tblidx": fallback = Id; break;
            case "NameIndex": fallback = TitleNameIndex; break;
            case "ContentsType": fallback = ContentsType; break;
            case "RepresentationType": fallback = RepresentationType; break;
            case "BoneName": fallback = BoneName; break;
            case "EffectName": fallback = EffectName; break;
            case "EffectSound": fallback = EffectSound; break;
        }
        if (fallback is not null)
        {
            value = (T)Convert.ChangeType(fallback, typeof(T));
            return true;
        }
        value = default!;
        return false;
    }

    public T GetScalarOrDefault<T>(string name, T defaultValue)
    {
        return TryGetScalar<T>(name, out var v) ? v : defaultValue;
    }

    public T[] GetArray<T>(string name, int expectedLength)
    {
        if (_arrays.TryGetValue(name, out var arr) && arr is Array a)
        {
            var result = new T[expectedLength];
            for (int i = 0; i < expectedLength; i++)
            {
                result[i] = i < a.Length ? (T)Convert.ChangeType(a.GetValue(i)!, typeof(T)) : default!;
            }
            return result;
        }
        // Fallback to typed CharTitle arrays
        if (name.Equals("SystemEffectTblidx", StringComparison.OrdinalIgnoreCase))
        {
            var src = SystemEffectTblidx ?? Array.Empty<int>();
            var res = new T[expectedLength];
            for (int i = 0; i < expectedLength; i++) res[i] = (T)Convert.ChangeType(i < src.Length ? src[i] : 0, typeof(T));
            return res;
        }
        if (name.Equals("SystemEffectType", StringComparison.OrdinalIgnoreCase))
        {
            var src = SystemEffectType ?? Array.Empty<byte>();
            var res = new T[expectedLength];
            for (int i = 0; i < expectedLength; i++) res[i] = (T)Convert.ChangeType(i < src.Length ? src[i] : (byte)0, typeof(T));
            return res;
        }
        if (name.Equals("SystemEffectValue", StringComparison.OrdinalIgnoreCase))
        {
            var src = SystemEffectValue ?? Array.Empty<double>();
            var res = new T[expectedLength];
            for (int i = 0; i < expectedLength; i++) res[i] = (T)Convert.ChangeType(i < src.Length ? src[i] : 0.0, typeof(T));
            return res;
        }
        return new T[expectedLength];
    }
    }
}
