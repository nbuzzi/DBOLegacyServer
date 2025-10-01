using Microsoft.Win32;
using System.Diagnostics;
using System.IO;

namespace DBOServerMonitor.Services;

public class DumpInfo
{
    public required string FilePath { get; init; }
    public string FileName => Path.GetFileName(FilePath);
    public string App => FileName.Split('.').FirstOrDefault() ?? FileName;
    public long SizeBytes { get; init; }
    public DateTime CreatedUtc { get; init; }
    public double SizeMB => SizeBytes / 1024.0 / 1024.0;
    public DateTime CreatedLocal => CreatedUtc.ToLocalTime();
}

public class DumpService
{
    private readonly AppSettings _settings;
    public DumpService(AppSettings settings) => _settings = settings;

    public List<DumpInfo> ScanDumps(string envPath)
    {
        var list = new List<DumpInfo>();
        try
        {
            if (string.IsNullOrWhiteSpace(envPath) || !Directory.Exists(envPath)) return list;
            var roots = new[]
            {
                envPath,
                Path.Combine(envPath, "logs"),
                Path.Combine(envPath, "logs", "_monitor"),
                Path.Combine(envPath, "dumps"),
            };
            foreach (var root in roots.Distinct().Where(Directory.Exists))
            {
                foreach (var file in Directory.EnumerateFiles(root, "*.dmp", SearchOption.AllDirectories))
                {
                    try
                    {
                        var fi = new FileInfo(file);
                        list.Add(new DumpInfo { FilePath = fi.FullName, SizeBytes = fi.Length, CreatedUtc = fi.CreationTimeUtc });
                    }
                    catch { }
                }
            }
            return list.OrderByDescending(d => d.CreatedUtc).ToList();
        }
        catch { return list; }
    }

    public string? TryFindCdb()
    {
        var candidates = new[]
        {
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Windows Kits", "10", "Debuggers", "x64", "cdb.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Windows Kits", "10", "Debuggers", "x86", "cdb.exe"),
        };
        return candidates.FirstOrDefault(File.Exists);
    }

    public string? TryFindWinDbg()
    {
        var candidates = new[]
        {
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Windows Kits", "10", "Debuggers", "x64", "windbg.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Windows Kits", "10", "Debuggers", "x86", "windbg.exe"),
        };
        return candidates.FirstOrDefault(File.Exists);
    }

    public string? TryFindDevenv()
    {
        // naive search of common VS install roots
        var pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
        var roots = Directory.Exists(Path.Combine(pf, "Microsoft Visual Studio"))
            ? Directory.EnumerateFiles(Path.Combine(pf, "Microsoft Visual Studio"), "devenv.exe", SearchOption.AllDirectories)
            : Enumerable.Empty<string>();
        return roots.FirstOrDefault();
    }

    public async Task<(bool ok, string output)> AnalyzeWithCdbAsync(string dmpPath)
    {
        try
        {
            var cdb = TryFindCdb();
            if (cdb == null) return (false, "cdb.exe not found. Please install Windows SDK Debugging Tools.");
            var psi = new ProcessStartInfo
            {
                FileName = cdb,
                Arguments = $"-z \"{dmpPath}\" -c \"!analyze -v; qd\"",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true,
            };
            using var p = Process.Start(psi)!;
            var stdout = await p.StandardOutput.ReadToEndAsync();
            var stderr = await p.StandardError.ReadToEndAsync();
            p.WaitForExit();
            var output = string.IsNullOrWhiteSpace(stdout) ? stderr : stdout;
            return (p.ExitCode == 0 || output.Length > 0, output);
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }

    public (bool ok, string message) EnableLocalDumps(string envPath)
    {
        try
        {
            var exes = new[] { "MasterServer.exe", "QueryServer.exe", "AuthServer.exe", "CharServer.exe", "ChatServer.exe", "GameServer.exe" };
            var dumpDir = Path.Combine(envPath, "dumps");
            Directory.CreateDirectory(dumpDir);
            const string baseKey = "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps";
            using var root = Registry.LocalMachine.CreateSubKey(baseKey, true) ?? throw new Exception("Cannot open LocalDumps key (admin required)");
            foreach (var exe in exes)
            {
                using var sub = root.CreateSubKey(exe, true) ?? throw new Exception($"Cannot create key for {exe}");
                sub.SetValue("DumpFolder", dumpDir, RegistryValueKind.ExpandString);
                sub.SetValue("DumpCount", 10, RegistryValueKind.DWord);
                sub.SetValue("DumpType", 1, RegistryValueKind.DWord); // 1=MiniDump, 2=Full
            }
            return (true, $"LocalDumps enabled -> {dumpDir}");
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }
}
