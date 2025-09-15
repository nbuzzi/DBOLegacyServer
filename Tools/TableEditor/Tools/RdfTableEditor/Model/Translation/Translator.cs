using System;
using RdfTableEditor.Model.Configuration;

namespace RdfTableEditor.Model.Translation
{
    public static class Translator
    {
        // Try to create a translator from ENV configuration
        // Azure: AZURE_TRANSLATOR_KEY (required), AZURE_TRANSLATOR_REGION (required), AZURE_TRANSLATOR_ENDPOINT (optional)
        public static ITranslationService? CreateFromEnvironment()
        {
            // 1) Config file first
            var cfg = AppConfigLoader.Current;
            if (cfg?.Translation != null)
            {
                var t = cfg.Translation;
                var provider = (t.Provider ?? string.Empty).Trim().ToLowerInvariant();
                if (!string.IsNullOrWhiteSpace(t.GoogleApiKey) && (provider == "google" || string.IsNullOrEmpty(provider)))
                    return new GoogleTranslateV2(t.GoogleApiKey!, t.GoogleEndpoint);
                if (!string.IsNullOrWhiteSpace(t.AzureKey) && !string.IsNullOrWhiteSpace(t.AzureRegion) && (provider == "azure" || string.IsNullOrEmpty(provider)))
                    return new AzureTranslator(t.AzureKey!, t.AzureRegion!, t.AzureEndpoint);
            }
            // Prefer Google if configured
            var gKey = Environment.GetEnvironmentVariable("GOOGLE_TRANSLATE_API_KEY")
                      ?? Environment.GetEnvironmentVariable("GOOGLE_API_KEY");
            var gEndpoint = Environment.GetEnvironmentVariable("GOOGLE_TRANSLATE_ENDPOINT");
            if (!string.IsNullOrWhiteSpace(gKey))
            {
                return new GoogleTranslateV2(gKey!, gEndpoint);
            }
            var key = Environment.GetEnvironmentVariable("AZURE_TRANSLATOR_KEY");
            var region = Environment.GetEnvironmentVariable("AZURE_TRANSLATOR_REGION");
            var endpoint = Environment.GetEnvironmentVariable("AZURE_TRANSLATOR_ENDPOINT");
            if (!string.IsNullOrWhiteSpace(key) && !string.IsNullOrWhiteSpace(region))
            {
                return new AzureTranslator(key!, region!, endpoint);
            }
            // Future: Add Google provider detection here if GOOGLE_API_KEY present
            return null;
        }
    }
}
