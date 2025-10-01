using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using MahApps.Metro.Controls;
using ControlzEx.Theming;
using DBOServerMonitor.Services;

namespace DBOServerMonitor;

public partial class SettingsWindow : MetroWindow
{
    private readonly AppSettings _settings;
    public ObservableCollection<string> AvailableBaseThemes { get; } = new(["Dark","Light"]);
    public ObservableCollection<string> AvailableAccents { get; } = new(["Blue","Red","Purple","Green","Orange","Lime","Emerald","Teal","Cyan","Indigo","Violet","Pink","Magenta","Crimson","Amber","Yellow","Brown","Cobalt","Sienna"]);
    public string SelectedBaseTheme { get => _settings.ThemeBase; set { if (!string.IsNullOrWhiteSpace(value)) { _settings.ThemeBase = value; ApplyTheme(); } } }
    public string SelectedAccent { get => _settings.ThemeAccent; set { if (!string.IsNullOrWhiteSpace(value)) { _settings.ThemeAccent = value; ApplyTheme(); } } }
    public SettingsWindow(AppSettings settings)
    {
        InitializeComponent();
        _settings = settings;
        DataContext = _settings;
        ApplyTheme();
        Loaded += (_, __) => { Pwd.Password = _settings.MySqlPassword ?? string.Empty; };
    }

    private void ApplyTheme()
    {
    var baseTheme = string.IsNullOrWhiteSpace(_settings.ThemeBase) ? "Dark" : _settings.ThemeBase;
    var accent = string.IsNullOrWhiteSpace(_settings.ThemeAccent) ? "Blue" : _settings.ThemeAccent;
    var name = $"{baseTheme}.{accent}";
    ThemeManager.Current.ChangeTheme(System.Windows.Application.Current, name);
    }

    private void Save_Click(object sender, RoutedEventArgs e)
    {
    _settings.MySqlPassword = Pwd.Password;
        // guard defaults
        _settings.ThemeBase = string.IsNullOrWhiteSpace(_settings.ThemeBase) ? "Dark" : _settings.ThemeBase;
        _settings.ThemeAccent = string.IsNullOrWhiteSpace(_settings.ThemeAccent) ? "Blue" : _settings.ThemeAccent;
        _settings.Save();
        TryApplyRunAtStartup(_settings.RunAtStartup);
        DialogResult = true;
        Close();
    }

    private void Cancel_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = false;
        Close();
    }

    private void TryApplyRunAtStartup(bool enable)
    {
        try
        {
            // Register/unregister in HKCU Run with quoted exe path
            using var key = Microsoft.Win32.Registry.CurrentUser.OpenSubKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run", writable: true);
            if (key == null) return;
            var exe = System.Diagnostics.Process.GetCurrentProcess().MainModule?.FileName;
            if (string.IsNullOrWhiteSpace(exe)) return;
            var name = "DBO Server Monitor";
            if (enable)
                key.SetValue(name, $"\"{exe}\"");
            else
                key.DeleteValue(name, false);
        }
        catch
        {
            // non-fatal
        }
    }

    private void BrowseJson_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Select Google service account JSON",
            Filter = "JSON files (*.json)|*.json|All files (*.*)|*.*",
            CheckFileExists = true,
            Multiselect = false
        };
        if (!string.IsNullOrWhiteSpace(_settings.GoogleServiceAccountJsonPath))
        {
            try { dlg.InitialDirectory = System.IO.Path.GetDirectoryName(_settings.GoogleServiceAccountJsonPath); } catch { }
        }
        var res = dlg.ShowDialog(this);
        if (res == true)
        {
            _settings.GoogleServiceAccountJsonPath = dlg.FileName;
            // Update textbox immediately
            if (JsonPathText != null) JsonPathText.Text = dlg.FileName;
            _settings.Save();
        }
    }

    private void BrowseDump_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Select mysqldump.exe",
            Filter = "mysqldump (mysqldump.exe)|mysqldump.exe|Executable files (*.exe)|*.exe|All files (*.*)|*.*",
            CheckFileExists = true,
            Multiselect = false
        };
        if (!string.IsNullOrWhiteSpace(_settings.MySqlDumpPath))
        {
            try { dlg.InitialDirectory = System.IO.Path.GetDirectoryName(_settings.MySqlDumpPath); } catch { }
        }
        var res = dlg.ShowDialog(this);
        if (res == true)
        {
            _settings.MySqlDumpPath = dlg.FileName;
            // DataContext is bound; UI will reflect the change
            _settings.Save();
        }
    }

    private void BrowseOAuthJson_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Select OAuth client_secret.json",
            Filter = "JSON files (*.json)|*.json|All files (*.*)|*.*",
            CheckFileExists = true,
            Multiselect = false
        };
        if (!string.IsNullOrWhiteSpace(_settings.OAuthClientSecretsJsonPath))
        {
            try { dlg.InitialDirectory = System.IO.Path.GetDirectoryName(_settings.OAuthClientSecretsJsonPath); } catch { }
        }
        var res = dlg.ShowDialog(this);
        if (res == true)
        {
            _settings.OAuthClientSecretsJsonPath = dlg.FileName;
            if (OAuthJsonText != null) OAuthJsonText.Text = dlg.FileName;
            _settings.Save();
        }
    }

    private async void SignInOAuth_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_settings.OAuthClientSecretsJsonPath) || !System.IO.File.Exists(_settings.OAuthClientSecretsJsonPath))
            {
                System.Windows.MessageBox.Show("Please select a valid OAuth client_secret.json first.");
                return;
            }
            var svc = await Services.DriveOAuthService.SignInAndCreateServiceAsync(_settings.OAuthClientSecretsJsonPath);
            // Probe the Drive API to ensure token works
            var aboutReq = svc.About.Get();
            aboutReq.Fields = "user,storageQuota";
            var about = await aboutReq.ExecuteAsync();
            _settings.UseOAuthForDrive = true;
            _settings.Save();
            System.Windows.MessageBox.Show($"Signed in as: {about?.User?.EmailAddress ?? "(unknown)"}");
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Google sign-in failed: {ex.Message}");
        }
    }

    private async void RunBackupNow_Click(object sender, RoutedEventArgs e)
    {
        // Create a transient backup service using current settings and ExecutionEnv
        try
        {
            bool ok = false; string data = string.Empty;
            using var svc = new Services.BackupService(_settings, () => _settings.ExecutionEnvPath);
            svc.OnCompleted += (success, msg) => { ok = success; data = msg; };
            await svc.RunOnce();

            if (!ok)
            {
                System.Windows.MessageBox.Show($"Backup failed: {data}");
                return;
            }

            // Optionally zip and/or upload to Google Drive (mirror MainWindow behavior)
            string uploadPath = data;
            string dateName = System.IO.Path.GetFileName(data);
            if (_settings.ZipBackups && System.IO.Directory.Exists(data))
            {
                var zipPath = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(data)!, dateName + ".zip");
                try
                {
                    if (System.IO.File.Exists(zipPath)) System.IO.File.Delete(zipPath);
                    System.IO.Compression.ZipFile.CreateFromDirectory(data, zipPath, System.IO.Compression.CompressionLevel.Optimal, includeBaseDirectory: false);
                    uploadPath = zipPath;
                }
                catch (Exception zex)
                {
                    // non-fatal; continue without zip
                    System.Windows.MessageBox.Show($"Backup zipped failed, uploading raw files instead: {zex.Message}");
                }
            }

            if (_settings.UploadToDrive)
            {
                try
                {
                    var drive = new Services.GoogleDriveService(_settings);
                    var (okUpload, msg) = await drive.UploadFolderAsync(uploadPath, dateName);
                    System.Windows.MessageBox.Show(okUpload ? $"Drive upload success: {msg}" : $"Drive upload failed: {msg}");
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show($"Drive upload error: {ex.Message}");
                }
            }

            // Final confirmation that files exist
            try
            {
                if (System.IO.Directory.Exists(data))
                {
                    var sqls = System.IO.Directory.EnumerateFiles(data, "*.sql", System.IO.SearchOption.TopDirectoryOnly);
                    if (sqls.Any())
                    {
                        System.Windows.MessageBox.Show($"Backup completed: {data}");
                        return;
                    }
                }
                else if (System.IO.File.Exists(uploadPath))
                {
                    System.Windows.MessageBox.Show($"Backup completed: {uploadPath}");
                    return;
                }
            }
            catch { }
            System.Windows.MessageBox.Show("Backup reported success, but no .sql files were found. Please verify mysqldump path and credentials in Settings.");
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Backup failed: {ex.Message}");
        }
    }

    private async void TestDiscord_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var svc = new Services.DiscordService(_settings);
            var (ok, msg) = await svc.TestAsync();
            System.Windows.MessageBox.Show(ok ? $"Discord test OK: {msg}" : $"Discord test failed: {msg}");
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Discord test failed: {ex.Message}");
        }
    }

    private async void TestDrive_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var drive = new Services.GoogleDriveService(_settings);
            var tmpDir = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "dbo-drive-test");
            System.IO.Directory.CreateDirectory(tmpDir);
            var testFile = System.IO.Path.Combine(tmpDir, "test.txt");
            await System.IO.File.WriteAllTextAsync(testFile, "hello drive");
            var dateName = DateTime.Now.ToString("yyyy-MM-dd");
            var (ok, msg) = await drive.UploadFolderAsync(testFile, dateName);
            System.Windows.MessageBox.Show(ok ? $"Drive test OK: {msg}" : $"Drive test failed: {msg}");
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Drive test failed: {ex.Message}");
        }
    }
}
