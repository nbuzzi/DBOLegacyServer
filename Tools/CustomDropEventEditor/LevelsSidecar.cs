using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace CustomDropEventEditor
{
    internal static class LevelsSidecar
    {
        private static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.General)
        {
            WriteIndented = true
        };

        public static string GetSidecarPath(string cfgPath)
        {
            var dir = Path.GetDirectoryName(cfgPath) ?? string.Empty;
            var file = Path.GetFileNameWithoutExtension(cfgPath);
            return Path.Combine(dir, file + ".levels.json");
        }

        public static Dictionary<uint, int> Load(string cfgPath)
        {
            var path = GetSidecarPath(cfgPath);
            if (!File.Exists(path))
                return new Dictionary<uint, int>();

            var json = File.ReadAllText(path);
            var data = JsonSerializer.Deserialize<Dictionary<string, int>>(json) ?? new();
            var dict = new Dictionary<uint, int>(data.Count);
            foreach (var kv in data)
            {
                if (uint.TryParse(kv.Key, out var id))
                    dict[id] = Math.Clamp(kv.Value, 1, 255);
            }
            return dict;
        }

        public static void Save(string cfgPath, Dictionary<uint, int> levels)
        {
            var path = GetSidecarPath(cfgPath);
            var data = levels.ToDictionary(k => k.Key.ToString(), v => Math.Clamp(v.Value, 1, 255));
            var json = JsonSerializer.Serialize(data, Options);
            File.WriteAllText(path, json);
        }
    }
}
