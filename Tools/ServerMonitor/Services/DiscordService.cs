using System.Net.Http;
using System.Net.Http.Json;

namespace DBOServerMonitor.Services;

public class DiscordService
{
    private readonly AppSettings _settings;
    private readonly HttpClient _http = new();
    public DiscordService(AppSettings settings) { _settings = settings; }

    public async Task SendAsync(string message)
    {
        try
        {
            if (!_settings.DiscordAlertsEnabled || string.IsNullOrWhiteSpace(_settings.DiscordWebhookUrl)) return;
            var url = _settings.DiscordWebhookUrl!;
            var payload = new { content = message };
            var resp = await _http.PostAsJsonAsync(url, payload);
            resp.EnsureSuccessStatusCode();
        }
        catch
        {
            // swallow errors to avoid loops
        }
    }

    public async Task<(bool ok, string message)> TestAsync(string? message = null)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_settings.DiscordWebhookUrl))
                return (false, "Webhook URL is empty");
            var url = _settings.DiscordWebhookUrl!;
            var payload = new { content = message ?? ":wave: Hello from DBO Server Monitor" };
            var resp = await _http.PostAsJsonAsync(url, payload);
            if (resp.IsSuccessStatusCode)
                return (true, "Message sent");
            var body = await resp.Content.ReadAsStringAsync();
            return (false, $"HTTP {(int)resp.StatusCode}: {body}");
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }
}
