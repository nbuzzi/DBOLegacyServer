using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Windows;
using MahApps.Metro.Controls;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace DBOServerMonitor;

public partial class LogsWindow : MetroWindow
{
    private readonly string _logsRoot;
    private string? _currentFile;
    private FileSystemWatcher? _watcher;
    public LogsWindow(string executionEnvPath)
    {
        InitializeComponent();
        _logsRoot = Path.Combine(executionEnvPath, "logs");
        LoadTree();
        this.Closed += (_, __) => { _watcher?.Dispose(); };
    }

    private void LoadTree()
    {
        Tree.Items.Clear();
        if (!Directory.Exists(_logsRoot)) return;

        // Expect structure: logs/ masterserver, gameserver, authserver, charserver, chatserver, queryserver
        foreach (var serverDir in Directory.EnumerateDirectories(_logsRoot).OrderBy(d => d))
        {
            var serverName = System.IO.Path.GetFileName(serverDir);
            if (string.IsNullOrWhiteSpace(serverName)) continue;
            var appNode = new TreeViewItem { Header = serverName };

            if (serverName.Equals("gameserver", StringComparison.OrdinalIgnoreCase))
            {
                // channels: channel0..channel9
                var channels = Directory.EnumerateDirectories(serverDir)
                    .Where(d => System.IO.Path.GetFileName(d).StartsWith("channel", StringComparison.OrdinalIgnoreCase))
                    .OrderBy(d => d, StringComparer.OrdinalIgnoreCase);
                foreach (var chDir in channels)
                {
                    var chName = System.IO.Path.GetFileName(chDir);
                    var chNode = new TreeViewItem { Header = chName };
                    foreach (var f in Directory.EnumerateFiles(chDir, "*", SearchOption.TopDirectoryOnly).OrderBy(f => f))
                    {
                        chNode.Items.Add(new TreeViewItem { Header = System.IO.Path.GetFileName(f), Tag = f });
                    }
                    appNode.Items.Add(chNode);
                }
            }
            else
            {
                // Other servers: list files under their folder (top-level)
                foreach (var f in Directory.EnumerateFiles(serverDir, "*", SearchOption.TopDirectoryOnly).OrderBy(f => f))
                {
                    appNode.Items.Add(new TreeViewItem { Header = System.IO.Path.GetFileName(f), Tag = f });
                }

                // Also include one level deeper subfolders if present
                foreach (var sub in Directory.EnumerateDirectories(serverDir).OrderBy(d => d))
                {
                    var subName = System.IO.Path.GetFileName(sub);
                    var subNode = new TreeViewItem { Header = subName };
                    foreach (var f in Directory.EnumerateFiles(sub, "*", SearchOption.TopDirectoryOnly).OrderBy(f => f))
                    {
                        subNode.Items.Add(new TreeViewItem { Header = System.IO.Path.GetFileName(f), Tag = f });
                    }
                    if (subNode.Items.Count > 0)
                        appNode.Items.Add(subNode);
                }
            }

            Tree.Items.Add(appNode);
        }
    }

    private static string GetDateKey(string path)
    {
        // attempt to parse date from directory structure or file name yyyy-mm-dd or yyyymmdd
        var name = Path.GetFileName(path);
        var dir = Path.GetDirectoryName(path) ?? string.Empty;
        var m1 = Regex.Match(name, @"(20\d{2})[-_]?(\d{2})[-_]?(\d{2})");
        if (m1.Success) return $"{m1.Groups[1].Value}-{m1.Groups[2].Value}-{m1.Groups[3].Value}";
        var m2 = Regex.Match(dir, @"(20\d{2})[-_]?(\d{2})[-_]?(\d{2})");
        if (m2.Success) return $"{m2.Groups[1].Value}-{m2.Groups[2].Value}-{m2.Groups[3].Value}";
        return DateTime.Now.ToString("yyyy-MM-dd");
    }

    private void Tree_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
    {
        if (Tree.SelectedItem is TreeViewItem tvi && tvi.Tag is string file && File.Exists(file))
        {
            _currentFile = file;
            TryLoadLog(file);
            SetupWatcherFor(file);
        }
    }

    private void TryLoadLog(string file)
    {
        try
        {
            // Read with sharing to handle logs being written by servers
            string text;
            using (var fs = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
            using (var sr = new StreamReader(fs, detectEncodingFromByteOrderMarks: true))
            {
                text = sr.ReadToEnd();
            }
            RenderLog(text, SearchBox.Text);
        }
        catch (Exception ex)
        {
            var doc = new FlowDocument();
            doc.Blocks.Add(new Paragraph(new Run($"Failed to load: {ex.Message}")));
            LogRtb.Document = doc;
        }
    }

    private void RenderLog(string text, string? query)
    {
        // basic highlighting: errors and warnings
        var doc = new FlowDocument();
        foreach (var line in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
        {
            // Severity filtering
            var isError = line.Contains("ERROR", StringComparison.OrdinalIgnoreCase);
            var isWarn = line.Contains("WARN", StringComparison.OrdinalIgnoreCase);
            var isInfo = !isError && !isWarn;
            if ((isError && ChkError.IsChecked == false) || (isWarn && ChkWarn.IsChecked == false) || (isInfo && ChkInfo.IsChecked == false))
                continue;
            var p = new Paragraph();
            var brush = System.Windows.Media.Brushes.White;
            if (line.Contains("ERROR", StringComparison.OrdinalIgnoreCase)) brush = System.Windows.Media.Brushes.OrangeRed;
            else if (line.Contains("WARN", StringComparison.OrdinalIgnoreCase)) brush = System.Windows.Media.Brushes.Goldenrod;

            if (!string.IsNullOrWhiteSpace(query) && line.IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                // split into match segments
                var idx = line.IndexOf(query, StringComparison.OrdinalIgnoreCase);
                var before = line.Substring(0, idx);
                var match = line.Substring(idx, query.Length);
                var after = line.Substring(idx + query.Length);
                p.Inlines.Add(new Run(before) { Foreground = brush });
                p.Inlines.Add(new Run(match) { Foreground = System.Windows.Media.Brushes.Black, Background = System.Windows.Media.Brushes.Yellow });
                p.Inlines.Add(new Run(after) { Foreground = brush });
            }
            else
            {
                p.Inlines.Add(new Run(line) { Foreground = brush });
            }
            doc.Blocks.Add(p);
        }
        LogRtb.Document = doc;
        if (ChkFollow.IsChecked == true)
        {
            LogRtb.ScrollToEnd();
        }
    }

    private void Find_Click(object sender, RoutedEventArgs e)
    {
        if (Tree.SelectedItem is TreeViewItem tvi && tvi.Tag is string file && File.Exists(file))
        {
            TryLoadLog(file);
        }
    }

    private void SetupWatcherFor(string file)
    {
        try
        {
            _watcher?.Dispose();
            if (ChkFollow.IsChecked != true) return;
            var dir = Path.GetDirectoryName(file)!;
            var name = Path.GetFileName(file);
            _watcher = new FileSystemWatcher(dir, name)
            {
                NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.Size,
                EnableRaisingEvents = true
            };
            _watcher.Changed += (_, __) => Dispatcher.Invoke(() =>
            {
                if (_currentFile != null && File.Exists(_currentFile))
                {
                    TryLoadLog(_currentFile);
                }
            });
        }
        catch { }
    }
}
