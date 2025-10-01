using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;

namespace DBOServerMonitor.Services;

public class BackupService : IDisposable
{
    private readonly AppSettings _settings;
    private readonly Func<string?> _getEnvPath;
    private System.Threading.Timer? _timer;
    public event Action<string>? OnLog;
    public event Action<bool, string>? OnCompleted;

    public BackupService(AppSettings settings, Func<string?> getEnvPath)
    {
        _settings = settings;
        _getEnvPath = getEnvPath;
        Reschedule();
    }

    public void Reschedule()
    {
        _timer?.Dispose();
        if (!_settings.BackupsEnabled)
            return;
        var now = DateTime.Now;
        var next = new DateTime(now.Year, now.Month, now.Day, _settings.BackupHour, _settings.BackupMinute, 0);
        if (next <= now) next = next.AddDays(1);
        var due = next - now;
    _timer = new System.Threading.Timer(async _ => await RunOnce(), null, due, Timeout.InfiniteTimeSpan);
        OnLog?.Invoke($"Backup scheduled at {next} (in {due.TotalMinutes:F0} min)");
    }

    public async Task RunOnce()
    {
        try
        {
            var env = _getEnvPath();
            string outDir;
            if (!string.IsNullOrWhiteSpace(env) && Directory.Exists(env))
            {
                outDir = Path.Combine(env!, "backups", DateTime.Now.ToString("yyyy-MM-dd"));
            }
            else
            {
                // If app is running inside an ExecutionEnv folder, use it
                var baseDir = AppContext.BaseDirectory;
                if (Directory.Exists(baseDir) && Directory.EnumerateFiles(baseDir, "*.exe").Any(f => Path.GetFileName(f).EndsWith("Server.exe", StringComparison.OrdinalIgnoreCase)))
                {
                    outDir = Path.Combine(baseDir, "backups", DateTime.Now.ToString("yyyy-MM-dd"));
                }
                else
                {
                    // Fallback to AppData
                    var appData = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "DBOServerMonitor", "backups", DateTime.Now.ToString("yyyy-MM-dd"));
                    outDir = appData;
                }
            }
            Directory.CreateDirectory(outDir);

            foreach (var db in _settings.Databases)
            {
                var file = Path.Combine(outDir, $"{db}.sql");
                var ok = await DumpDatabase(db, file);
                OnLog?.Invoke(ok ? $"Dumped {db} -> {file}" : $"Failed to dump {db}");
                if (!ok) throw new Exception($"mysqldump failed for {db}");
            }

            OnCompleted?.Invoke(true, outDir);
        }
        catch (Exception ex)
        {
            OnCompleted?.Invoke(false, ex.Message);
        }
        finally
        {
            // schedule next
            Reschedule();
        }
    }

    private async Task<bool> DumpDatabase(string db, string file)
    {
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = _settings.MySqlDumpPath,
                Arguments = $"-h {_settings.MySqlHost} -P {_settings.MySqlPort} -u {_settings.MySqlUser} -p{_settings.MySqlPassword} {db}",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
                StandardOutputEncoding = new UTF8Encoding(false)
            };
            using var proc = Process.Start(psi)!;
            await using var fs = new FileStream(file, FileMode.Create, FileAccess.Write, FileShare.None);
            var stdOutCopy = proc.StandardOutput.BaseStream.CopyToAsync(fs);
            var stdErrTask = proc.StandardError.ReadToEndAsync();
            await Task.WhenAll(stdOutCopy, stdErrTask);
            proc.WaitForExit();
            // Some mysqldump builds print warnings to stderr even on success. Treat ExitCode==0 as success regardless of stderr content.
            // If ExitCode!=0, include stderr text for diagnostics.
            if (proc.ExitCode == 0)
                return true;
            var err = stdErrTask.Result ?? string.Empty;
            OnLog?.Invoke($"mysqldump error for {db}: {err.Trim()}\nCommand: '{psi.FileName} {psi.Arguments}'");
            return false;
        }
        catch
        {
            return false;
        }
    }

    public void Dispose()
    {
        _timer?.Dispose();
    }
}
