using System.Text;

namespace RdfTableEditor;

static class Program
{
    /// <summary>
    ///  The main entry point for the application.
    /// </summary>
    [STAThread]
    static void Main()
    {
        Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
        // To customize application configuration such as set high DPI settings or default font,
        // see https://aka.ms/applicationconfiguration.
        ApplicationConfiguration.Initialize();
        // Ensure EDF exporter is registered before UI actions
        Model.Exporters.EdfExportBootstrap.EnsureRegistered();
        Application.Run(new UI.MainForm());
    }
}