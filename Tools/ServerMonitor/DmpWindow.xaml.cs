using System.Diagnostics;
using System.IO;
using System.Windows;
using MahApps.Metro.Controls;
using DBOServerMonitor.Services;

namespace DBOServerMonitor;

public partial class DmpWindow : MetroWindow
{
    private readonly DumpService _dumpSvc;
    private readonly string _envPath;
    public DmpWindow(string envPath, AppSettings settings)
    {
        InitializeComponent();
        _envPath = envPath;
        _dumpSvc = new DumpService(settings);
        RefreshList();
    }

    private void RefreshList()
    {
        Grid.ItemsSource = _dumpSvc.ScanDumps(_envPath);
    }

    private void Refresh_Click(object sender, RoutedEventArgs e) => RefreshList();

    private void EnableDumps_Click(object sender, RoutedEventArgs e)
    {
        var (ok, msg) = _dumpSvc.EnableLocalDumps(_envPath);
        System.Windows.MessageBox.Show(msg, ok ? "Enabled" : "Failed");
    }

    private void OpenWinDbg_Click(object sender, RoutedEventArgs e)
    {
        if (Grid.SelectedItem is not DumpInfo dump || !File.Exists(dump.FilePath)) return;
        var windbg = _dumpSvc.TryFindWinDbg();
        if (windbg == null) { System.Windows.MessageBox.Show("WinDbg not found. Install Windows SDK Debugging Tools."); return; }
        Process.Start(new ProcessStartInfo(windbg, $"\"{dump.FilePath}\"") { UseShellExecute = true });
    }

    private void OpenVS_Click(object sender, RoutedEventArgs e)
    {
        if (Grid.SelectedItem is not DumpInfo dump || !File.Exists(dump.FilePath)) return;
        var devenv = _dumpSvc.TryFindDevenv();
        if (devenv == null) { System.Windows.MessageBox.Show("Visual Studio (devenv.exe) not found."); return; }
        Process.Start(new ProcessStartInfo(devenv, $"\"{dump.FilePath}\"") { UseShellExecute = true });
    }

    private async void Analyze_Click(object sender, RoutedEventArgs e)
    {
        if (Grid.SelectedItem is not DumpInfo dump || !File.Exists(dump.FilePath)) return;
        Output.Text = "Running cdb !analyze -v ...\r\n";
        var (ok, text) = await _dumpSvc.AnalyzeWithCdbAsync(dump.FilePath);
        Output.Text = text;
        if (!ok)
            System.Windows.MessageBox.Show("Analysis failed or cdb not found.");
    }
}
