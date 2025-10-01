using Microsoft.Win32;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using MahApps.Metro.Controls;
using System.Windows.Input;
using System.Management;
using ScottPlot;
using ScottPlot.Plottable;
using System.Drawing;
using DBOServerMonitor.Services;

namespace DBOServerMonitor;

public partial class MainWindow : MetroWindow, INotifyPropertyChanged
{
    public ObservableCollection<ProcItem> Processes { get; } = new();
    public ObservableCollection<string> IniFiles { get; } = new();
    public ObservableCollection<string> ChannelSummaries { get; } = new();

    private ProcItem? _selectedProcess;
    public ProcItem? SelectedProcess { get => _selectedProcess; set { _selectedProcess = value; OnPropertyChanged(); UpdatePlot(); } }

    private string _executionEnvPath = string.Empty;
    public string ExecutionEnvPath { get => _executionEnvPath; set { _executionEnvPath = value; OnPropertyChanged(); RefreshIniFiles(); RefreshChannelsSummary(); } }

    private string? _selectedIniFile;
    public string? SelectedIniFile { get => _selectedIniFile; set { _selectedIniFile = value; OnPropertyChanged(); LoadIniContent(); } }

    private string _iniContent = string.Empty;
    public string IniContent { get => _iniContent; set { _iniContent = value; OnPropertyChanged(); } }

    private bool _autoRestart = true;
    public bool AutoRestart { get => _autoRestart; set { _autoRestart = value; OnPropertyChanged(); } }

    public ICommand StartCommand { get; }
    public ICommand StopCommand { get; }
    public ICommand RestartCommand { get; }
    public ICommand OpenLogsCommand { get; }

    private readonly string[] Order = new[] { "MasterServer.exe", "QueryServer.exe", "AuthServer.exe", "CharServer.exe", "ChatServer.exe", "GameServer.exe" };
    private readonly AppSettings _settings = AppSettings.Load();
    private readonly BackupService _backupService;
    private readonly GoogleDriveService _drive;
    private readonly DiscordService _discord;
    private static readonly TimeSpan StartGrace = TimeSpan.FromSeconds(25);

    public MainWindow()
    {
        InitializeComponent();
        DataContext = this;

        StartCommand = new RelayCommand(p => StartProcess((ProcItem)p!), p => p is ProcItem item && item.Status != ProcStatus.Running);
        StopCommand = new RelayCommand(p => StopProcess((ProcItem)p!), p => p is ProcItem item && item.Status == ProcStatus.Running);
        RestartCommand = new RelayCommand(p => RestartProcess((ProcItem)p!), p => p is ProcItem);
        OpenLogsCommand = new RelayCommand(p => OpenLogs((ProcItem)p!), p => p is ProcItem);

    // Default ExecutionEnv path: user setting or known repo path
    ExecutionEnvPath = _settings.ExecutionEnvPath ?? FindDefaultExecutionEnv() ?? string.Empty;

        BuildProcessList();
        InitPlot();
        RefreshChannelNamesFromIni();

        // Initialize integrations first so any early events can use them safely
        _discord = new DiscordService(_settings);
        _drive = new GoogleDriveService(_settings);

        _backupService = new BackupService(_settings, () => ExecutionEnvPath);
        _backupService.OnLog += msg => { /* Optionally surface somewhere */ };
        _backupService.OnCompleted += async (ok, data) =>
        {
            // Upload to Drive if configured
            if (ok && _settings.UploadToDrive)
            {
                try
                {
                    var localPath = data; // local backup folder path
                    string uploadPath = localPath;
                    var dateName = System.IO.Path.GetFileName(localPath);
                    if (_settings.ZipBackups && System.IO.Directory.Exists(localPath))
                    {
                        var zipPath = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(localPath)!, dateName + ".zip");
                        try
                        {
                            if (System.IO.File.Exists(zipPath)) System.IO.File.Delete(zipPath);
                            System.IO.Compression.ZipFile.CreateFromDirectory(localPath, zipPath, System.IO.Compression.CompressionLevel.Optimal, includeBaseDirectory: false);
                            uploadPath = zipPath;
                        }
                        catch (Exception zex)
                        {
                            await _discord.SendAsync($":warning: Failed to zip backups, uploading raw files instead: {zex.Message}");
                        }
                    }
                    var (okUpload, msg) = await _drive.UploadFolderAsync(uploadPath, dateName);
                    await _discord.SendAsync(okUpload ? $":white_check_mark: Drive upload success: {msg}" : $":x: Drive upload failed: {msg}");
                }
                catch (Exception ex)
                {
                    await _discord.SendAsync($":x: Drive upload error: {ex.Message}");
                }
            }
            else
            {
                await _discord.SendAsync(ok ? ":white_check_mark: Backup finished successfully" : $":x: Backup failed: {data}");
            }
            await Task.CompletedTask;
        };

    // Start background watcher after services are ready
    _ = StartWatcherLoop();
    // Auto-start all servers on launch if configured
    if (_settings.StartAllOnLaunch)
    {
        _ = Dispatcher.InvokeAsync(async () => await StartAllSequenceAsync());
    }
    }

    private string? FindDefaultExecutionEnv()
    {
        try
        {
            // 1) If the app is running inside ExecutionEnv, use current base directory
            var baseDir = AppContext.BaseDirectory;
            if (IsExecutionEnvFolder(baseDir))
                return baseDir;

            // 2) Otherwise, try to locate repo-root-based ExecutionEnv
            var repoRoot = FindRepoRoot();
            if (repoRoot != null)
            {
                var path = Path.Combine(repoRoot, "DboServer", "ExecutionEnv");
                if (Directory.Exists(path)) return path;
            }
            return null;
        }
        catch { return null; }
    }

    private static bool IsExecutionEnvFolder(string path)
    {
        try
        {
            if (!Directory.Exists(path)) return false;
            var cfg = Path.Combine(path, "config");
            if (Directory.Exists(cfg)) return true;
            var known = new[] { "MasterServer.exe", "QueryServer.exe", "AuthServer.exe", "CharServer.exe", "ChatServer.exe", "GameServer.exe" };
            return known.Any(n => File.Exists(Path.Combine(path, n)));
        }
        catch { return false; }
    }

    private string ResolveExecutionEnvPath()
    {
        // Prefer the bound ExecutionEnvPath if it exists
        if (!string.IsNullOrWhiteSpace(ExecutionEnvPath) && Directory.Exists(ExecutionEnvPath))
            return ExecutionEnvPath;
        // Otherwise prefer settings
        if (!string.IsNullOrWhiteSpace(_settings.ExecutionEnvPath) && Directory.Exists(_settings.ExecutionEnvPath))
            return _settings.ExecutionEnvPath!;
        // If running inside an ExecutionEnv folder, use current base directory
        var baseDir = AppContext.BaseDirectory;
        if (IsExecutionEnvFolder(baseDir))
            return baseDir;
        // Fallback: return current property (may be empty) to avoid blocking flows
        return ExecutionEnvPath;
    }

    private static string? FindRepoRoot()
    {
        // Walk up from current directory to find folder that contains DboServer
        var dir = AppContext.BaseDirectory;
        var di = new DirectoryInfo(dir);
        for (var i = 0; i < 6 && di != null; i++, di = di.Parent)
        {
            if (di.GetDirectories("DboServer").Any()) return di.FullName;
        }
        return null;
    }

    private void BuildProcessList()
    {
        Processes.Clear();
        // Core singletons
        Processes.Add(new ProcItem("MasterServer.exe", ProcType.Core));
        Processes.Add(new ProcItem("QueryServer.exe", ProcType.Core));
        Processes.Add(new ProcItem("AuthServer.exe", ProcType.Core));
        Processes.Add(new ProcItem("CharServer.exe", ProcType.Core, args: @".\\config\\CharServer.ini"));
        Processes.Add(new ProcItem("ChatServer.exe", ProcType.Core));

        // Channels (0..9)
        for (int ch = 0; ch <= 9; ch++)
        {
            var ini = ch == 0 ? ".\\config\\GameServer.ini" : $".\\config\\GameServer{ch}.ini";
            Processes.Add(new ProcItem("GameServer.exe", ProcType.Channel, ch, ini));
        }
    }

    private async Task StartWatcherLoop()
    {
        while (true)
        {
            try
            {
                await Dispatcher.InvokeAsync(() =>
                {
                    UpdateStatuses();
                }, System.Windows.Threading.DispatcherPriority.Background);

                await Dispatcher.InvokeAsync(() =>
                {
                    SampleMemory();
                    UpdatePlot();
                }, System.Windows.Threading.DispatcherPriority.Background);
            }
            catch { /* ignore */ }
            await Task.Delay(1000);
        }
    }

    private void UpdateStatuses()
    {
        var envStatus = ResolveExecutionEnvPath();
        if (string.IsNullOrWhiteSpace(envStatus) || !Directory.Exists(envStatus)) return;
        // Preload command lines via WMI to identify GameServer channel args
        var cmdLines = QueryCommandLines();
        foreach (var item in Processes)
        {
            var baseName = Path.GetFileNameWithoutExtension(item.Name);
            var exePath = Path.Combine(envStatus, item.Name);
            // First: if we have a known PID, trust it
            if (item.Pid is int knownPid && IsProcessAlive(knownPid, exePath))
            {
                item.Status = ProcStatus.Running;
                continue;
            }

            // Try to locate the process instance (useful after app restart)
            var procs = Process.GetProcessesByName(baseName);
            Process? matched = null;
            foreach (var p in procs)
            {
                var full = SafeMainModuleFileName(p);
                if (!string.Equals(full, exePath, StringComparison.OrdinalIgnoreCase)) continue;
                if (item.Type == ProcType.Channel)
                {
                    if (cmdLines.TryGetValue(p.Id, out var cmd))
                    {
                        // Some systems return empty command lines for processes depending on permissions
                        if (string.IsNullOrWhiteSpace(cmd))
                        {
                            matched = p; break;
                        }
                        var expectedRaw = item.Arguments ?? string.Empty; // e.g. .\\config\\GameServer1.ini
                        if (CommandLineContains(cmd, expectedRaw, exePath)) { matched = p; break; }
                    }
                    else
                    {
                        // If command line isn't available yet (startup), accept the path match to avoid false negatives
                        matched = p; break;
                    }
                }
                else { matched = p; break; }
            }

            if (matched != null)
            {
                item.Pid = matched.Id;
                item.Status = ProcStatus.Running;
                continue;
            }

            // Not found. If we recently started, allow a grace window to avoid false restarts.
            if (item.Status == ProcStatus.Running)
            {
                if (item.LastStartUtc.HasValue && DateTime.UtcNow - item.LastStartUtc.Value < StartGrace)
                {
                    // keep it in running state during grace period
                    continue;
                }
                // If we had a known PID and it's gone, it's a true exit -> handle
                if (item.Pid.HasValue)
                {
                    OnProcessExited(item);
                }
            }
            item.Pid = null;
            item.Status = ProcStatus.Stopped;
            item.ClearMemory();
        }
        // Update channel summary text after statuses change
        RefreshChannelsSummary();
    }

    private static bool IsProcessAlive(int pid, string expectedExePath)
    {
        try
        {
            var p = Process.GetProcessById(pid);
            var full = SafeMainModuleFileName(p);
            return string.Equals(full, expectedExePath, StringComparison.OrdinalIgnoreCase);
        }
        catch { return false; }
    }

    private static bool CommandLineContains(string commandLine, string expectedArg, string exePath)
    {
        if (string.IsNullOrWhiteSpace(commandLine)) return false;
        // Normalize quotes and slashes
        string norm(string s) => s.Replace("\"", string.Empty).Replace("\\\\", "\\").Trim();
        var cmd = norm(commandLine);
        var expect1 = norm(expectedArg);
        // try fully-qualified path for relative .\\config paths
        string expectFull = expect1;
        try
        {
            var workDir = Path.GetDirectoryName(exePath) ?? Environment.CurrentDirectory;
            expectFull = norm(Path.GetFullPath(Path.Combine(workDir, expect1)));
        }
        catch { }
        return cmd.Contains(expect1, StringComparison.OrdinalIgnoreCase) || cmd.Contains(expectFull, StringComparison.OrdinalIgnoreCase);
    }

    private static Dictionary<int, string> QueryCommandLines()
    {
        var dict = new Dictionary<int, string>();
        try
        {
            using var searcher = new ManagementObjectSearcher("SELECT ProcessId, CommandLine FROM Win32_Process");
            using var results = searcher.Get();
            foreach (ManagementObject mo in results)
            {
                var pid = Convert.ToInt32(mo["ProcessId"] ?? 0);
                var cl = mo["CommandLine"]?.ToString() ?? string.Empty;
                dict[pid] = cl;
            }
        }
        catch { }
        return dict;
    }

    private static string SafeMainModuleFileName(Process p)
    {
        try { return p.MainModule?.FileName ?? string.Empty; } catch { return string.Empty; }
    }

    private void OnProcessExited(ProcItem item)
    {
        try
        {
            CollectLogs(item);
            // Alert about unexpected exit
            _ = _discord.SendAsync($":warning: {item.Name}{(item.Channel is int ch ? " (ch " + ch + ")" : string.Empty)} exited. AutoRestart={(AutoRestart ? "on" : "off")}");
            if (AutoRestart)
            {
                StartProcess(item);
                _ = _discord.SendAsync($":arrows_counterclockwise: Restarted {item.Name}{(item.Channel is int ch2 ? " (ch " + ch2 + ")" : string.Empty)}");
            }
            else
            {
                item.LastStartUtc = null;
            }
        }
        catch { }
    }

    private void CollectLogs(ProcItem item)
    {
        try
        {
            // Copy recent log files for the component
            var logsRoot = Path.Combine(ResolveExecutionEnvPath(), "logs");
            if (!Directory.Exists(logsRoot)) return;

            var stamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            var dest = Path.Combine(logsRoot, "_monitor", item.SafeId, stamp);
            Directory.CreateDirectory(dest);

            foreach (var file in Directory.EnumerateFiles(logsRoot, "*", SearchOption.AllDirectories))
            {
                var name = Path.GetFileName(file);
                // heuristic: copy only files matching server kind
                if (name.Contains(item.BaseName, StringComparison.OrdinalIgnoreCase))
                {
                    var rel = Path.GetRelativePath(logsRoot, file);
                    var target = Path.Combine(dest, rel);
                    Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                    File.Copy(file, target, overwrite: true);
                }
            }
        }
        catch { }
    }

    private void StartProcess(ProcItem item)
    {
        try
        {
            var env = ResolveExecutionEnvPath();
            // If we just started this item, avoid duplicate starts while it's warming up
            if (item.Pid is int pid && IsProcessAlive(pid, Path.Combine(env, item.Name)))
                return;
            if (item.LastStartUtc.HasValue && DateTime.UtcNow - item.LastStartUtc.Value < StartGrace)
                return;

            var psi = new ProcessStartInfo
            {
                FileName = Path.Combine(env, item.Name),
                WorkingDirectory = env,
                UseShellExecute = true,
                Arguments = item.Arguments ?? string.Empty,
            };
            var p = Process.Start(psi);
            if (p != null)
            {
                item.Pid = p.Id;
                item.Status = ProcStatus.Running;
                item.LastStartUtc = DateTime.UtcNow;
                _ = _discord.SendAsync($":white_check_mark: Started {item.Name}{(item.Channel is int ch ? " (ch " + ch + ")" : string.Empty)} PID={p.Id}");
            }
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Failed to start {item.Name}: {ex.Message}");
            _ = _discord.SendAsync($":x: Failed to start {item.Name}: {ex.Message}");
        }
    }

    private void StopProcess(ProcItem item)
    {
        try
        {
            var procs = Process.GetProcessesByName(item.BaseName);
            foreach (var p in procs)
            {
                try
                {
                    // Try graceful close first
                    p.CloseMainWindow();
                    if (!p.WaitForExit(1500))
                    {
                        p.Kill(true);
                    }
                }
                catch { }
            }
            item.Status = ProcStatus.Stopped;
            item.Pid = null;
            item.LastStartUtc = null;
            _ = _discord.SendAsync($":stop_sign: Stopped {item.Name}{(item.Channel is int ch ? " (ch " + ch + ")" : string.Empty)}");
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Failed to stop {item.Name}: {ex.Message}");
            _ = _discord.SendAsync($":x: Failed to stop {item.Name}: {ex.Message}");
        }
    }

    private void RestartProcess(ProcItem item)
    {
        StopProcess(item);
        // slight delay to release port/file locks
        Task.Delay(500).ContinueWith(_ => Dispatcher.Invoke(() => { item.LastStartUtc = null; StartProcess(item); }));
    }

    private void OpenLogs(ProcItem item)
    {
        try
        {
            var logsRoot = Path.Combine(ResolveExecutionEnvPath(), "logs");
            if (Directory.Exists(logsRoot)) Process.Start(new ProcessStartInfo("explorer.exe", logsRoot) { UseShellExecute = true });
        }
        catch { }
    }

    private void OpenLogsViewer_Click(object sender, RoutedEventArgs e)
    {
        // Allow opening even if path validation fails; the window will handle missing folders gracefully
        var basePath = ExecutionEnvPath;
        if (string.IsNullOrWhiteSpace(basePath)) basePath = _settings.ExecutionEnvPath ?? string.Empty;
        var win = new LogsWindow(basePath);
        win.Owner = this;
        win.Show();
    }

    private void OpenDumps_Click(object sender, RoutedEventArgs e)
    {
        var basePath = ExecutionEnvPath;
        if (string.IsNullOrWhiteSpace(basePath)) basePath = _settings.ExecutionEnvPath ?? string.Empty;
        var win = new DmpWindow(basePath, _settings);
        win.Owner = this;
        win.Show();
    }

    private void OpenSettings_Click(object sender, RoutedEventArgs e)
    {
        var win = new SettingsWindow(_settings);
        win.Owner = this;
        if (win.ShowDialog() == true)
        {
            // apply settings that affect runtime
            if (!string.Equals(_settings.ExecutionEnvPath, ExecutionEnvPath, StringComparison.OrdinalIgnoreCase) && !string.IsNullOrWhiteSpace(_settings.ExecutionEnvPath))
            {
                ExecutionEnvPath = _settings.ExecutionEnvPath!;
            }
            _backupService.Reschedule();
        }
    }

    private async void RunBackupNow_Click(object sender, RoutedEventArgs e)
    {
        await _discord.SendAsync(":hourglass_flowing_sand: Starting backup now...");
        await _backupService.RunOnce();
    }

    private async void StartAll_Click(object sender, RoutedEventArgs e)
    {
        await StartAllSequenceAsync();
    }

    private async Task StartAllSequenceAsync()
    {
        // Match the user batch: Master -> Query -> Auth -> Char -> GameServer 0,1,2,3,9 -> Chat

        var seq = new List<ProcItem?>();
        ProcItem? find(string name, int? ch = null) => Processes.FirstOrDefault(p => p.Name == name && (ch == null || p.Channel == ch));

        seq.Add(find("MasterServer.exe"));
        seq.Add(find("QueryServer.exe"));
        seq.Add(find("AuthServer.exe"));
        seq.Add(find("CharServer.exe"));

        int[] desiredChannels = new[] { 0, 1, 2, 3, 9 };
        foreach (var ch in desiredChannels)
        {
            if (IniExistsForChannel(ch))
                seq.Add(find("GameServer.exe", ch));
        }

        // Chat after the game servers per current batch
        seq.Add(find("ChatServer.exe"));

        foreach (var item in seq.Where(i => i != null)!)
        {
            try
            {
                if (item!.Status != ProcStatus.Running)
                    StartProcess(item);
            }
            catch { }
            await Task.Delay(1000); // 1 second between starts
        }
    }

    private void StopAll_Click(object sender, RoutedEventArgs e)
    {
        foreach (var item in Processes.ToList())
        {
            StopProcess(item);
        }
    }

    private void BrowseEnv_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new System.Windows.Forms.FolderBrowserDialog
        {
            Description = "Select ExecutionEnv folder (contains server EXEs and config)",
            UseDescriptionForTitle = true,
        };
        var res = dlg.ShowDialog();
        if (res == System.Windows.Forms.DialogResult.OK)
        {
            ExecutionEnvPath = dlg.SelectedPath;
            _settings.ExecutionEnvPath = ExecutionEnvPath;
            _settings.Save();
        }
    }

    private void RefreshIniFiles()
    {
        IniFiles.Clear();
        var env = ResolveExecutionEnvPath();
        if (string.IsNullOrWhiteSpace(env)) return;
        var cfg = Path.Combine(env, "config");
        if (!Directory.Exists(cfg)) return;
        foreach (var file in Directory.EnumerateFiles(cfg, "*.ini"))
        {
            IniFiles.Add(file);
        }
        foreach (var file in Directory.EnumerateFiles(cfg, "*.cfg"))
        {
            IniFiles.Add(file);
        }
        SelectedIniFile ??= IniFiles.FirstOrDefault();
        RefreshChannelNamesFromIni();
    }

    private void LoadIniContent()
    {
        try
        {
            if (SelectedIniFile == null) { IniContent = string.Empty; return; }
            IniContent = File.ReadAllText(SelectedIniFile, Encoding.UTF8);
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Failed to read INI: {ex.Message}");
        }
    }

    private void ReloadIni_Click(object sender, RoutedEventArgs e) => LoadIniContent();

    private void SaveIni_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (SelectedIniFile == null) return;
            // Backup
            var backup = SelectedIniFile + ".bak_" + DateTime.Now.ToString("yyyyMMdd_HHmmss");
            File.Copy(SelectedIniFile, backup, overwrite: true);
            File.WriteAllText(SelectedIniFile, IniContent, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
            System.Windows.MessageBox.Show("Saved.");
            // Update channel names if a GameServer INI was modified
            RefreshChannelNamesFromIni();
        }
        catch (Exception ex)
        {
            System.Windows.MessageBox.Show($"Failed to save INI: {ex.Message}");
        }
    }

    private void StartNextChannel_Click(object sender, RoutedEventArgs e)
    {
        var next = NextAvailableChannel();
        if (next == null)
        {
            System.Windows.MessageBox.Show("All channels 1..9 are running or INIs missing.");
            return;
        }
        var item = Processes.First(p => p.Type == ProcType.Channel && p.Channel == next);
        StartProcess(item);
    }

    private int? NextAvailableChannel()
    {
        var running = Processes.Where(p => p.Type == ProcType.Channel && p.Status == ProcStatus.Running).Select(p => p.Channel ?? -1).ToHashSet();
        for (int ch = 1; ch <= 9; ch++)
        {
            if (!running.Contains(ch) && IniExistsForChannel(ch)) return ch;
        }
        return null;
    }

    private bool IniExistsForChannel(int ch)
    {
        var env = ResolveExecutionEnvPath();
        if (string.IsNullOrWhiteSpace(env)) return false;
        var cfg = Path.Combine(env, "config");
        var name = ch == 0 ? "GameServer.ini" : $"GameServer{ch}.ini";
        return File.Exists(Path.Combine(cfg, name));
    }

    private void EnsureDojo_Click(object sender, RoutedEventArgs e)
    {
        var dojo = Processes.First(p => p.Type == ProcType.Channel && p.Channel == 9);
        if (dojo.Status != ProcStatus.Running)
        {
            StartProcess(dojo);
        }
    }

    private void RefreshChannelsSummary()
    {
        ChannelSummaries.Clear();
        foreach (var ch in Processes.Where(p => p.Type == ProcType.Channel).OrderBy(p => p.Channel))
        {
            var name = string.IsNullOrWhiteSpace(ch.ChannelName) ? string.Empty : $" [{ch.ChannelName}]";
            ChannelSummaries.Add($"Ch {ch.Channel}{name}: {ch.Status} {(ch.Pid.HasValue ? "PID=" + ch.Pid : string.Empty)}");
        }
    }

    private void RefreshChannelNamesFromIni()
    {
        try
        {
            var env = ResolveExecutionEnvPath();
            if (string.IsNullOrWhiteSpace(env)) return;
            var cfg = Path.Combine(env, "config");
            if (!Directory.Exists(cfg)) return;
            foreach (var item in Processes.Where(p => p.Type == ProcType.Channel && p.Channel is int))
            {
                var ini = item.Channel == 0 ? Path.Combine(cfg, "GameServer.ini") : Path.Combine(cfg, $"GameServer{item.Channel}.ini");
                item.ChannelName = TryReadIniValue(ini, "Channelname");
            }
            RefreshChannelsSummary();
        }
        catch { }
    }

    private static string? TryReadIniValue(string iniPath, string key)
    {
        try
        {
            if (!File.Exists(iniPath)) return null;
            foreach (var line in File.ReadLines(iniPath))
            {
                var trimmed = line.Trim();
                if (trimmed.StartsWith("#") || trimmed.StartsWith(";") || string.IsNullOrWhiteSpace(trimmed)) continue;
                // format: Key = Value
                var idx = trimmed.IndexOf('=');
                if (idx > 0)
                {
                    var k = trimmed.Substring(0, idx).Trim();
                    if (k.Equals(key, StringComparison.OrdinalIgnoreCase))
                    {
                        return trimmed.Substring(idx + 1).Trim();
                    }
                }
            }
        }
        catch { }
        return null;
    }

    // ScottPlot members for memory chart
    private ScottPlot.WpfPlot? Plot => MemoryPlot;
    private ScatterPlot? _memSeries; // selected process
    private readonly Dictionary<string, ScatterPlot> _allSeries = new(); // per-process series
    private ScatterPlot? _aggSeries; // aggregate series
    private Text? _peakText;
    private const int MemoryWindowSeconds = 600; // 10 minutes

    private void InitPlot()
    {
        if (Plot is null) return;
        var plt = Plot.Plot;
        plt.Title("Working Set (MB)");
        plt.XLabel("Seconds (last 10 min)");
        plt.YLabel("MB");
        // Apply dark theme to match app
        plt.Style(ScottPlot.Style.Black);
        // Set initial X axis range so it doesn't show negative spans before data arrives
        plt.SetAxisLimitsX(0, MemoryWindowSeconds);
        // Lazily create scatter series when we have points to plot to avoid empty-series exceptions
        _memSeries = null;
        _allSeries.Clear();
        _aggSeries = null;
        _peakText = plt.AddText("", 0, 0);
        _peakText.Color = Color.OrangeRed;
        _peakText.FontSize = 12;
        _peakText.IsVisible = false;
        plt.Legend(true, ScottPlot.Alignment.UpperRight);
        Plot.Refresh();
    }

    private void SampleMemory()
    {
        var now = DateTime.UtcNow;
        foreach (var item in Processes)
        {
            if (item.Pid is int pid)
            {
                try
                {
                    var p = Process.GetProcessById(pid);
                    var mb = p.WorkingSet64 / (1024.0 * 1024.0);
                    item.AddMemorySample(now, mb, MemoryWindowSeconds);
                }
                catch
                {
                    item.AddMemorySample(now, null, MemoryWindowSeconds);
                }
            }
            else
            {
                item.AddMemorySample(now, null, MemoryWindowSeconds);
            }
        }
    }

    private void UpdatePlot()
    {
        if (Plot is null || _peakText is null) return;
        var plt = Plot.Plot;

        // Selected process series
        var sel = SelectedProcess;
        bool showSelected = ChkShowSelected?.IsChecked ?? true;
        if (sel != null && showSelected)
        {
            var (xs, ys) = sel.GetSeriesSeconds(MemoryWindowSeconds);
            if (xs.Length > 0 && ys.Length > 0)
            {
                if (_memSeries == null)
                {
                    _memSeries = plt.AddScatter(xs, ys, color: Color.DeepSkyBlue, lineWidth: 2, label: sel.SafeId);
                    _memSeries.MarkerSize = 5;
                    _memSeries.MarkerShape = ScottPlot.MarkerShape.filledCircle;
                }
                else
                    _memSeries.Update(xs, ys);
                _memSeries.Label = sel.SafeId;
                _memSeries.IsVisible = true;

                var max = ys.Max();
                var idx = Array.IndexOf(ys, max);
                _peakText.Label = $"Peak: {max:F1} MB";
                _peakText.X = xs[idx];
                _peakText.Y = max;
                _peakText.IsVisible = true;
            }
            else if (_memSeries != null)
            {
                _memSeries.IsVisible = false;
                _peakText.IsVisible = false;
            }
        }
        else
        {
            if (_memSeries != null) _memSeries.IsVisible = false;
            _peakText.IsVisible = false;
        }

        // Per-process series (All Processes)
        bool showAll = ChkShowAll?.IsChecked ?? false;
        var colorIdx = 0;
        var palette = new[] { Color.Lime, Color.Orange, Color.Cyan, Color.Magenta, Color.Yellow, Color.LightSkyBlue, Color.Violet, Color.LightGreen, Color.Tomato };
        foreach (var p in Processes)
        {
            var key = p.SafeId;
            var (xs, ys) = p.GetSeriesSeconds(MemoryWindowSeconds);
            if (showAll && xs.Length > 0 && ys.Length > 0)
            {
                if (!_allSeries.TryGetValue(key, out var sp))
                {
                    var col = palette[colorIdx++ % palette.Length];
                    sp = plt.AddScatter(xs, ys, color: col, lineWidth: 1, label: key);
                    sp.MarkerSize = 3;
                    sp.MarkerShape = ScottPlot.MarkerShape.filledCircle;
                    _allSeries[key] = sp;
                }
                else
                {
                    sp.Update(xs, ys);
                }
                sp.IsVisible = true;
            }
            else if (_allSeries.TryGetValue(key, out var sp))
            {
                sp.IsVisible = false;
            }
        }

        // Aggregate (sum of all current samples at each X)
        bool showAgg = ChkShowAggregate?.IsChecked ?? false;
        if (showAgg)
        {
            var xs = Enumerable.Range(0, MemoryWindowSeconds + 1).Select(i => (double)i).ToArray();
            var ys = new double[xs.Length];
            foreach (var p in Processes)
            {
                var (px, py) = p.GetSeriesSeconds(MemoryWindowSeconds);
                if (px.Length == 0) continue;
                int j = 0;
                for (int i = 0; i < xs.Length; i++)
                {
                    var x = xs[i];
                    while (j + 1 < px.Length && px[j + 1] <= x) j++;
                    if (j < py.Length)
                        ys[i] += py[j];
                }
            }
            if (_aggSeries == null)
                _aggSeries = plt.AddScatter(xs, ys, color: Color.WhiteSmoke, lineWidth: 2, label: "Aggregate");
            else
                _aggSeries.Update(xs, ys);
            _aggSeries.IsVisible = true;
        }
        else if (_aggSeries != null)
        {
            _aggSeries.IsVisible = false;
        }

        // Keep X axis 0..window
        plt.SetAxisLimitsX(0, MemoryWindowSeconds);
        Plot.Refresh();
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

public enum ProcStatus { Stopped, Running }
public enum ProcType { Core, Channel }

public class ProcItem : INotifyPropertyChanged
{
    public string Name { get; }
    public string BaseName => Path.GetFileNameWithoutExtension(Name);
    public string SafeId => Channel is int ch ? $"{BaseName}_ch{ch}" : BaseName;
    public ProcType Type { get; }
    public int? Channel { get; }
    public string? Arguments { get; }
    private string? _channelName;
    public string? ChannelName { get => _channelName; set { _channelName = value; OnPropertyChanged(); } }

    private ProcStatus _status;
    public ProcStatus Status { get => _status; set { _status = value; OnPropertyChanged(); } }

    private int? _pid;
    public int? Pid { get => _pid; set { _pid = value; OnPropertyChanged(); } }

    public string TypeDisplay => Type.ToString();
    public string? IniPath => Arguments;
    public DateTime? LastStartUtc { get; set; }

    public ProcItem(string name, ProcType type, int? channel = null, string? args = null)
    {
        Name = name;
        Type = type;
        Channel = channel;
        Arguments = args;
        Status = ProcStatus.Stopped;
    }

    // Memory tracking (last N seconds window)
    private readonly LinkedList<(DateTime t, double? mb)> _mem = new();
    private double? _peakMb;
    public void AddMemorySample(DateTime t, double? mb, int windowSeconds)
    {
        _mem.AddLast((t, mb));
        // prune
        var cutoff = t.AddSeconds(-windowSeconds);
        while (_mem.First != null && _mem.First.Value.t < cutoff)
            _mem.RemoveFirst();
        if (mb.HasValue)
        {
            if (!_peakMb.HasValue || mb > _peakMb) _peakMb = mb;
        }
    }
    public void ClearMemory()
    {
        _mem.Clear();
        _peakMb = null;
    }
    public (double[] x, double[] y) GetSeriesSeconds(int windowSeconds)
    {
        var now = DateTime.UtcNow;
        var start = now.AddSeconds(-windowSeconds);
        var pts = _mem.Where(s => s.t >= start && s.mb.HasValue).Select(s => (x: (s.t - start).TotalSeconds, y: s.mb!.Value)).ToList();
        return (pts.Select(p => p.x).ToArray(), pts.Select(p => p.y).ToArray());
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}

public class RelayCommand : ICommand
{
    private readonly Action<object?> _execute;
    private readonly Predicate<object?>? _canExecute;
    public RelayCommand(Action<object?> execute, Predicate<object?>? canExecute = null)
    {
        _execute = execute;
        _canExecute = canExecute;
    }
    public bool CanExecute(object? parameter) => _canExecute?.Invoke(parameter) ?? true;
    public void Execute(object? parameter) => _execute(parameter);
    public event EventHandler? CanExecuteChanged
    {
        add { CommandManager.RequerySuggested += value; }
        remove { CommandManager.RequerySuggested -= value; }
    }
}
