using System;
using System.IO;
using RdfTableEditor.Model.Schema;

namespace RdfTableEditor.Model.Exporters
{
    public static class EdfExporter
    {
        public delegate bool EdfExportHandler(Stream destination, RdfDocument doc, TableSchema? schema);
        private static EdfExportHandler? _handler;

        public static void Register(EdfExportHandler handler)
        {
            _handler = handler;
        }

        public static bool TryWrite(Stream destination, RdfDocument doc, TableSchema? schema)
        {
            // Prefer external handler when provided (e.g., by the table container/loader)
            if (_handler != null)
            {
                try { return _handler(destination, doc, schema); }
                catch { return false; }
            }

            // Fallback: if we have a schema, write RDF binary with BinaryTableIO, then let the container re-wrap as EDF externally
            try
            {
                if (schema != null)
                {
                    using var ms = new MemoryStream();
                    BinaryTableIO.Write(ms, schema, doc);
                    ms.Position = 0;
                    ms.CopyTo(destination);
                    return true;
                }
            }
            catch { /* ignore */ }

            return false;
        }
    }
}
