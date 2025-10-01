using Google.Apis.Auth.OAuth2;
using Google.Apis.Drive.v3;
using Google.Apis.Services;
using Google.Apis.Util.Store;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace DBOServerMonitor.Services;

public static class DriveOAuthService
{
    private static readonly string[] Scopes = new[] { DriveService.Scope.DriveFile, DriveService.Scope.Drive };

    public static async Task<DriveService> SignInAndCreateServiceAsync(string clientSecretsPath)
    {
        using var stream = new FileStream(clientSecretsPath, FileMode.Open, FileAccess.Read);
        var secrets = GoogleClientSecrets.FromStream(stream).Secrets;

        var credPath = Path.Combine(AppSettings.SettingsDir, "google-oauth-token");
        Directory.CreateDirectory(credPath);
        var dataStore = new FileDataStore(credPath, true);

        var credential = await GoogleWebAuthorizationBroker.AuthorizeAsync(
            secrets,
            Scopes,
            "user",
            CancellationToken.None,
            dataStore
        );

        return new DriveService(new BaseClientService.Initializer
        {
            HttpClientInitializer = credential,
            ApplicationName = "DBO Server Monitor"
        });
    }
}
