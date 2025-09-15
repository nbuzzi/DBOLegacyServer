using System;
using System.IO;
using System.Text.Json;

namespace RdfTableEditor.Model.Configuration
{
    public sealed class AppConfig
    {
        public TranslationSection Translation { get; set; } = new TranslationSection();

        public sealed class TranslationSection
        {
            public string? Provider { get; set; } // "google" or "azure" (optional)
            public string? GoogleApiKey { get; set; }
            public string? GoogleEndpoint { get; set; }
            public string? AzureKey { get; set; }
            public string? AzureRegion { get; set; }
            public string? AzureEndpoint { get; set; }
        }
    }

    public static class AppConfigLoader
    {
        private static AppConfig? _cached;
        public static AppConfig? Current
        {
            get
            {
                if (_cached != null) return _cached;
                _cached = TryLoad();
                return _cached;
            }
        }

        public static AppConfig? TryLoad()
        {
            try
            {
                var explicitPath = Environment.GetEnvironmentVariable("RDFTE_CONFIG_PATH");
                if (!string.IsNullOrWhiteSpace(explicitPath) && File.Exists(explicitPath))
                    return LoadFromFile(explicitPath);

                var baseDir = AppContext.BaseDirectory?.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar) ?? string.Empty;
                var localPath = Path.Combine(baseDir, "RdfTableEditor.config.json");
                if (File.Exists(localPath)) return LoadFromFile(localPath);

                var appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
                var roamingPath = Path.Combine(appData, "RdfTableEditor", "config.json");
                if (File.Exists(roamingPath)) return LoadFromFile(roamingPath);
            }
            catch { /* ignore and return null */ }
            return null;
        }

        private static AppConfig LoadFromFile(string path)
        {
            var json = File.ReadAllText(path);
            var opts = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
            return JsonSerializer.Deserialize<AppConfig>(json, opts) ?? new AppConfig();
        }
    }
}
