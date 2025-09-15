using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace RdfTableEditor.Model.Translation
{
    // Lightweight Google Cloud Translation (v2) client using API key.
    // Docs: https://cloud.google.com/translate/docs/reference/rest/v2/translate
    public sealed class GoogleTranslateV2 : ITranslationService
    {
        private static readonly HttpClient _http = new HttpClient();
        private readonly string _apiKey;
        private readonly string _endpoint;

        public GoogleTranslateV2(string apiKey, string? endpoint = null)
        {
            _apiKey = apiKey ?? throw new ArgumentNullException(nameof(apiKey));
            _endpoint = string.IsNullOrWhiteSpace(endpoint) ? "https://translation.googleapis.com/language/translate/v2" : endpoint!;
        }

        public async Task<string> TranslateAsync(string text, string? fromLanguage = null, string toLanguage = "en", CancellationToken ct = default)
        {
            if (string.IsNullOrWhiteSpace(text)) return text;
            text = CleanText(text);
            // Attempt to repair mojibake caused by UTF-16 endianness mismatch
            var repaired = TryRepairEncoding(text);
            var protectedText = ProtectPlaceholders(repaired, out var phMap);
            var uri = _endpoint + "?key=" + Uri.EscapeDataString(_apiKey);

            // Google v2 accepts multiple 'q' values; split long text into safe chunks (~4500 chars)
            var chunks = SplitIntoChunks(protectedText, 4500);
            var payload = new Dictionary<string, object?>
            {
                ["q"] = chunks,
                ["target"] = toLanguage ?? "en",
                ["format"] = "text"
            };
            if (!string.IsNullOrWhiteSpace(fromLanguage))
                payload["source"] = fromLanguage;

            var json = JsonSerializer.Serialize(payload, new JsonSerializerOptions { DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull });
            using var req = new HttpRequestMessage(HttpMethod.Post, uri)
            {
                Content = new StringContent(json, Encoding.UTF8, "application/json")
            };
            using var res = await _http.SendAsync(req, ct).ConfigureAwait(false);
            var body = await res.Content.ReadAsStringAsync(ct).ConfigureAwait(false);
            if (!res.IsSuccessStatusCode)
            {
                try
                {
                    using var errDoc = JsonDocument.Parse(body);
                    if (errDoc.RootElement.TryGetProperty("error", out var err))
                    {
                        var code = err.TryGetProperty("code", out var c) ? c.GetInt32().ToString() : res.StatusCode.ToString();
                        var status = err.TryGetProperty("status", out var s) ? s.GetString() : null;
                        var msg = err.TryGetProperty("message", out var m) ? m.GetString() : body;
                        throw new InvalidOperationException($"Google Translate error ({code}{(string.IsNullOrEmpty(status)?"":$" {status}") }): {msg}");
                    }
                }
                catch (JsonException)
                {
                    // ignore parse error, throw generic
                }
                throw new InvalidOperationException($"Google Translate request failed: {(int)res.StatusCode} {res.ReasonPhrase}\n{body}");
            }

            using var doc = JsonDocument.Parse(body);
            var root = doc.RootElement;
            if (root.TryGetProperty("data", out var data) && data.TryGetProperty("translations", out var translations) && translations.ValueKind == JsonValueKind.Array && translations.GetArrayLength() > 0)
            {
                var sb = new StringBuilder();
                for (int i = 0; i < translations.GetArrayLength(); i++)
                {
                    var t = translations[i];
                    if (t.TryGetProperty("translatedText", out var tt))
                    {
                        var s = tt.GetString() ?? string.Empty;
                        sb.Append(System.Net.WebUtility.HtmlDecode(s));
                    }
                }
                var result = sb.ToString();
                if (!string.IsNullOrEmpty(result))
                {
                    result = RestorePlaceholders(result, phMap);
                    return result;
                }
            }
            throw new InvalidOperationException("No translation returned by Google.");
        }

        private static string CleanText(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            var sb = new StringBuilder(s.Length);
            foreach (var ch in s)
            {
                // Keep tabs/newlines; drop other control chars including NUL and zero-width spaces
                if (ch == '\t' || ch == '\n' || ch == '\r') { sb.Append(ch); continue; }
                if (char.IsControl(ch)) continue;
                // Common zero-width chars
                if (ch == '\u200B' || ch == '\u200C' || ch == '\u200D' || ch == '\uFEFF') continue;
                sb.Append(ch);
            }
            return sb.ToString();
        }

        private static List<string> SplitIntoChunks(string input, int max)
        {
            var chunks = new List<string>();
            if (input.Length <= max)
            {
                chunks.Add(input);
                return chunks;
            }
            int i = 0;
            while (i < input.Length)
            {
                int len = Math.Min(max, input.Length - i);
                int end = i + len;
                // Try to break at a natural boundary within the last 200 chars
                int probeStart = Math.Max(i, end - 200);
                int breakAt = -1;
                for (int j = end - 1; j >= probeStart; j--)
                {
                    char ch = input[j];
                    if (ch == '\n' || ch == '.' || ch == '!' || ch == '?' || ch == '。' || ch == '！' || ch == '？')
                    {
                        breakAt = j + 1;
                        break;
                    }
                }
                if (breakAt <= i) breakAt = end;
                chunks.Add(input.Substring(i, breakAt - i));
                i = breakAt;
            }
            return chunks;
        }

        private static string TryRepairEncoding(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;
            bool looksCjk = LooksLikeCjk(s);
            if (looksCjk) return s;
            var swapped = ByteSwapUtf16(s);
            if (LooksLikeCjk(swapped)) return swapped;
            return s;
        }

        private static string ByteSwapUtf16(string s)
        {
            var arr = s.ToCharArray();
            for (int i = 0; i < arr.Length; i++)
            {
                ushort w = arr[i];
                w = (ushort)(((w & 0x00FF) << 8) | ((w & 0xFF00) >> 8));
                arr[i] = (char)w;
            }
            return new string(arr);
        }

        private static bool LooksLikeCjk(string s)
        {
            if (string.IsNullOrEmpty(s)) return false;
            int cjk = 0;
            int letters = 0;
            foreach (var ch in s)
            {
                if (char.IsLetter(ch)) letters++;
                // CJK Unified Ideographs
                if ((ch >= '\u4E00' && ch <= '\u9FFF') ||
                    (ch >= '\u3400' && ch <= '\u4DBF') ||
                    (ch >= '\uF900' && ch <= '\uFAFF'))
                {
                    cjk++;
                }
            }
            if (letters == 0) return cjk > 0;
            return cjk >= Math.Max(2, letters / 3);
        }

        // Replace placeholders with stable markers so the translator doesn't alter them
        private static string ProtectPlaceholders(string input, out List<string> map)
        {
            if (string.IsNullOrEmpty(input)) { map = new List<string>(); return input; }
            var local = new List<string>();
            // Common patterns: %d, %u, %s, %f, %1$s, {0}, {1:format}
            var patterns = new Regex[]
            {
                new Regex("%(?:\\d+\\$)?[dsufx]", RegexOptions.Compiled),
                new Regex("\\{\\d+(?::[^}]*)?\\}", RegexOptions.Compiled)
            };
            string output = input;
            foreach (var rx in patterns)
            {
                output = rx.Replace(output, m =>
                {
                    var idx = local.Count;
                    local.Add(m.Value);
                    return $"__PH{idx}__";
                });
            }
            map = local;
            return output;
        }

        private static string RestorePlaceholders(string s, List<string> map)
        {
            if (map.Count == 0 || string.IsNullOrEmpty(s)) return s;
            for (int i = 0; i < map.Count; i++)
            {
                s = s.Replace($"__PH{i}__", map[i]);
            }
            return s;
        }
    }
}
