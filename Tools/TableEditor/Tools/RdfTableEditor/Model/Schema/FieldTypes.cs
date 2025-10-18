namespace RdfTableEditor.Model.Schema;

public enum ScalarType
{
    U8,
    S8,
    Bool, // 1 byte
    U16,
    S16,
    U32,
    S32,
    Float,
    Double,
    WStringFixed, // UTF-16LE fixed-length (wcharCount)
    WStringVar,    // UTF-16LE variable-length, prefixed by WORD length (wchar count)
    AnsiStringFixed // 8-bit fixed-length (charCount)
}

public sealed class Field
{
    public string Name { get; }
    public ScalarType Type { get; }
    public int Length { get; } // for WStringFixed: wchar count; for arrays: element count
    public bool IsArray { get; }

    public Field(string name, ScalarType type, int length = 0, bool isArray = false)
    {
        Name = name;
        Type = type;
        Length = length;
        IsArray = isArray;
    }
}

public sealed class TableSchema
{
    public string Name { get; }
    public bool HasMargin { get; }
    public IReadOnlyList<Field> Fields { get; }
    public int PackAlignment { get; } // usually 4 as per C++ pack(4)

    public TableSchema(string name, bool hasMargin, IEnumerable<Field> fields, int packAlignment = 4)
    {
        Name = name;
        HasMargin = hasMargin;
        Fields = fields.ToList();
        PackAlignment = packAlignment;
    }
}
