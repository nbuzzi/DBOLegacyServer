using System.IO;
using System.Text.Json;

namespace DBOServerMonitor.Services;

public class AppSettings
{
    public string? ExecutionEnvPath { get; set; }
    // MySQL backup
    public string MySqlHost { get; set; } = "127.0.0.1";
    public int MySqlPort { get; set; } = 3306;
    public string MySqlUser { get; set; } = "root";
    public string MySqlPassword { get; set; } = "";
    public string MySqlDumpPath { get; set; } = "mysqldump"; // in PATH by default
    public string[] Databases { get; set; } = new[] { "dbo_char", "dbo_acc", "dbo_log" };
    public bool BackupsEnabled { get; set; } = false;
    public int BackupHour { get; set; } = 3; // 03:00 local time by default
    public int BackupMinute { get; set; } = 0;
    public bool UploadToDrive { get; set; } = false;
    public bool ZipBackups { get; set; } = true;

    // Google Drive
    public string? GoogleServiceAccountJsonPath { get; set; }
    public string? GoogleDriveParentFolderId { get; set; }
    public bool UseOAuthForDrive { get; set; } = false; // If true, use user OAuth instead of service account
    public string? OAuthClientSecretsJsonPath { get; set; } // OAuth client_secret.json for installed app

    // Discord
    public string? DiscordWebhookUrl { get; set; }
    public bool DiscordAlertsEnabled { get; set; } = false;

    // UI theme
    public string ThemeBase { get; set; } = "Dark"; // Dark or Light
    public string ThemeAccent { get; set; } = "Blue"; // e.g., Blue, Red, Purple

    // Startup behavior
    public bool RunAtStartup { get; set; } = false; // Register to run at Windows startup
    public bool StartAllOnLaunch { get; set; } = false; // Start all servers automatically when app launches

    public static string SettingsDir => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "DBOServerMonitor");
    public static string SettingsFile => Path.Combine(SettingsDir, "settings.json");

    public static AppSettings Load()
    {
        try
        {
            if (File.Exists(SettingsFile))
            {
                var json = File.ReadAllText(SettingsFile);
                return JsonSerializer.Deserialize<AppSettings>(json) ?? new AppSettings();
            }
        }
        catch { }
        return new AppSettings();
    }

    public void Save()
    {
        Directory.CreateDirectory(SettingsDir);
        var json = JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(SettingsFile, json);
    }
}
