using System.Windows;
using ControlzEx.Theming;
using DBOServerMonitor.Services;

namespace DBOServerMonitor;

public partial class App : System.Windows.Application
{
	protected override void OnStartup(StartupEventArgs e)
	{
		base.OnStartup(e);
		var settings = AppSettings.Load();
		var name = $"{settings.ThemeBase}.{settings.ThemeAccent}";
		ThemeManager.Current.ChangeTheme(System.Windows.Application.Current, name);
	}
}
