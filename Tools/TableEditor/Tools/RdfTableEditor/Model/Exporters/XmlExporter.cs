using System;
using System.Data;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using RdfTableEditor.Model.Schema;

namespace RdfTableEditor.Model.Exporters
{
    public static class XmlExporter
    {
        public static void Write(Stream stream, RdfDocument doc, TableSchema? schema)
        {
            var settings = new XmlWriterSettings
            {
                Indent = true,
                Encoding = new UTF8Encoding(false)
            };
            using var xw = XmlWriter.Create(stream, settings);
            xw.WriteStartDocument();
            xw.WriteStartElement("Table");
            xw.WriteAttributeString("Name", doc.TableName ?? (schema?.Name ?? ""));

            foreach (var row in doc.Rows)
            {
                xw.WriteStartElement("Row");
                if (schema == null)
                {
                    // Fallback known fields
                    xw.WriteAttributeString("Tblidx", row.Id.ToString());
                    xw.WriteElementString("NameIndex", row.TitleNameIndex.ToString());
                    xw.WriteElementString("ContentsType", row.ContentsType.ToString());
                    xw.WriteElementString("RepresentationType", row.RepresentationType.ToString());
                    if (!string.IsNullOrEmpty(row.BoneName)) xw.WriteElementString("BoneName", row.BoneName);
                    if (!string.IsNullOrEmpty(row.EffectName)) xw.WriteElementString("EffectName", row.EffectName);
                    if (!string.IsNullOrEmpty(row.EffectSound)) xw.WriteElementString("EffectSound", row.EffectSound);
                    if (row.SystemEffectTblidx != null) xw.WriteElementString("SystemEffectTblidx", string.Join(",", row.SystemEffectTblidx));
                    if (row.SystemEffectType != null) xw.WriteElementString("SystemEffectType", string.Join(",", row.SystemEffectType));
                    if (row.SystemEffectValue != null) xw.WriteElementString("SystemEffectValue", string.Join(",", row.SystemEffectValue.Select(v=>v.ToString(System.Globalization.CultureInfo.InvariantCulture))));
                }
                else
                {
                    foreach (var f in schema.Fields)
                    {
                        if (!f.IsArray)
                        {
                            var val = row.GetScalarOrDefault<object?>(f.Name, null!);
                            if (val != null)
                                xw.WriteElementString(f.Name, ConvertToString(val));
                        }
                        else
                        {
                            xw.WriteStartElement(f.Name);
                            for (int i = 0; i < f.Length; i++)
                            {
                                object? v = f.Type switch
                                {
                                    ScalarType.U8 or ScalarType.Bool => row.GetArray<byte>(f.Name, f.Length)[i],
                                    ScalarType.U16 => row.GetArray<ushort>(f.Name, f.Length)[i],
                                    ScalarType.U32 => row.GetArray<uint>(f.Name, f.Length)[i],
                                    ScalarType.Float => row.GetArray<float>(f.Name, f.Length)[i],
                                    ScalarType.Double => row.GetArray<double>(f.Name, f.Length)[i],
                                    ScalarType.WStringFixed or ScalarType.WStringVar or ScalarType.AnsiStringFixed => row.GetArray<string>(f.Name, f.Length)[i],
                                    _ => null
                                };
                                xw.WriteElementString("Item", v == null ? string.Empty : ConvertToString(v));
                            }
                            xw.WriteEndElement();
                        }
                    }
                }
                xw.WriteEndElement(); // Row
            }

            xw.WriteEndElement(); // Table
            xw.WriteEndDocument();
        }

        private static string ConvertToString(object v)
        {
            return v switch
            {
                float f => f.ToString(System.Globalization.CultureInfo.InvariantCulture),
                double d => d.ToString(System.Globalization.CultureInfo.InvariantCulture),
                _ => Convert.ToString(v, System.Globalization.CultureInfo.InvariantCulture) ?? string.Empty
            };
        }
    }
}
