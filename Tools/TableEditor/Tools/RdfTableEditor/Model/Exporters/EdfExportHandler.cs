using System;
using System.IO;
using RdfTableEditor.Model.Schema;

namespace RdfTableEditor.Model.Exporters
{
    // Registers an EDF writer that matches the server's CNtlFileSerializer-secured format:
    // payload = [int32 little-endian length][RDF bytes], then DES-encrypted with KEY_FOR_GAME_DATA_TABLE.
    internal static class EdfExportBootstrap
    {
        static EdfExportBootstrap()
        {
            EdfExporter.Register(WriteEdf);
        }

        // Called from Program.Main to ensure static ctor runs and handler is registered.
        public static void EnsureRegistered() { /* no-op */ }

        private static bool WriteEdf(Stream destination, RdfDocument doc, TableSchema? schema)
        {
            if (schema == null)
                return false; // EDF requires schema-driven binary

            // 1) Serialize RDF using the known schema
            using var msRdf = new MemoryStream();
            BinaryTableIO.Write(msRdf, schema, doc);
            var rdfBytes = msRdf.ToArray();

            // 2) Wrap with size prefix the same way TableContainer does
            using var msPayload = new MemoryStream();
            using (var bw = new BinaryWriter(msPayload, System.Text.Encoding.UTF8, leaveOpen: true))
            {
                bw.Write(rdfBytes.Length); // little-endian int32
                bw.Write(rdfBytes);
                bw.Flush();
            }
            var payload = msPayload.ToArray();

            // 3) Pad to DES block size (8) with zeros, then encrypt using the same key
            int pad = (8 - (payload.Length % 8)) & 7;
            if (pad > 0)
            {
                Array.Resize(ref payload, payload.Length + pad); // Array.Resize zero-fills the new tail
            }

            if (!RdfCrypto.TryEncrypt(payload, out var encrypted))
                return false;

            destination.Write(encrypted, 0, encrypted.Length);
            return true;
        }
    }
}
