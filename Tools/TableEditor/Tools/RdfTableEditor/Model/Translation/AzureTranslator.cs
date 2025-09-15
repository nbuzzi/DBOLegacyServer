using System;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace RdfTableEditor.Model.Translation
{
    public sealed class AzureTranslator : ITranslationService
    {
        private static readonly HttpClient _http = new HttpClient();
        private readonly string _endpoint;
        private readonly string _subscriptionKey;
        private readonly string _region;

        public AzureTranslator(string subscriptionKey, string region, string? endpoint = null)
        {
            _subscriptionKey = subscriptionKey ?? throw new ArgumentNullException(nameof(subscriptionKey));
            _region = region ?? throw new ArgumentNullException(nameof(region));
            _endpoint = string.IsNullOrWhiteSpace(endpoint) ? "https://api.cognitive.microsofttranslator.com" : endpoint!;
        }

        public async Task<string> TranslateAsync(string text, string? fromLanguage = null, string toLanguage = "en", CancellationToken ct = default)
        {
            if (string.IsNullOrWhiteSpace(text)) return text;
            // Microsoft Translator Text API v3.0
            var uri = $"{_endpoint.TrimEnd('/')}/translate?api-version=3.0&to={Uri.EscapeDataString(toLanguage ?? "en")}";
            if (!string.IsNullOrWhiteSpace(fromLanguage))
                uri += "&from=" + Uri.EscapeDataString(fromLanguage!);

            using var req = new HttpRequestMessage(HttpMethod.Post, uri);
            req.Headers.Add("Ocp-Apim-Subscription-Key", _subscriptionKey);
            req.Headers.Add("Ocp-Apim-Subscription-Region", _region);
            req.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            var payload = JsonSerializer.Serialize(new[] { new { Text = text } });
            req.Content = new StringContent(payload, Encoding.UTF8, "application/json");

            using var res = await _http.SendAsync(req, ct).ConfigureAwait(false);
            res.EnsureSuccessStatusCode();
            var json = await res.Content.ReadAsStringAsync(ct).ConfigureAwait(false);

            // Response schema: [ { "translations": [ { "text": "...", "to": "en" } ], ... } ]
            using var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;
            if (root.ValueKind == JsonValueKind.Array && root.GetArrayLength() > 0)
            {
                var first = root[0];
                if (first.TryGetProperty("translations", out var translations) && translations.ValueKind == JsonValueKind.Array && translations.GetArrayLength() > 0)
                {
                    var t = translations[0];
                    if (t.TryGetProperty("text", out var textProp))
                        return textProp.GetString() ?? text;
                }
            }
            return text;
        }
    }
}
