using Google.Apis.Auth.OAuth2;
using Google.Apis.Drive.v3;
using Google.Apis.Drive.v3.Data;
using Google.Apis.Services;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using DriveFile = Google.Apis.Drive.v3.Data.File;

namespace DBOServerMonitor.Services;

public class GoogleDriveService
{
    private readonly AppSettings _settings;
    public GoogleDriveService(AppSettings settings)
    {
        _settings = settings;
    }

    private async Task<DriveService> CreateServiceAsync()
    {
        if (_settings.UseOAuthForDrive)
        {
            if (string.IsNullOrWhiteSpace(_settings.OAuthClientSecretsJsonPath) || !System.IO.File.Exists(_settings.OAuthClientSecretsJsonPath))
                throw new Exception("Invalid OAuth client secrets JSON path");
            // Use async flow to avoid deadlocks on UI thread
            return await DriveOAuthService.SignInAndCreateServiceAsync(_settings.OAuthClientSecretsJsonPath);
        }
        if (string.IsNullOrWhiteSpace(_settings.GoogleServiceAccountJsonPath) || !System.IO.File.Exists(_settings.GoogleServiceAccountJsonPath))
            throw new Exception("Invalid Google service account JSON path");
        var credential = GoogleCredential.FromFile(_settings.GoogleServiceAccountJsonPath)
            .CreateScoped(DriveService.Scope.DriveFile, DriveService.Scope.Drive);
        return new DriveService(new BaseClientService.Initializer
        {
            HttpClientInitializer = credential,
            ApplicationName = "DBO Server Monitor"
        });
    }

    public async Task<(bool ok, string message)> UploadFolderAsync(string localFolder, string dateFolderName)
    {
        if (string.IsNullOrWhiteSpace(_settings.GoogleDriveParentFolderId))
            return (false, "Missing GoogleDriveParentFolderId");
        if (!System.IO.Directory.Exists(localFolder) && !System.IO.File.Exists(localFolder))
            return (false, "Local backup folder/file not found");
    using var svc = await CreateServiceAsync();
        try
        {
            // ensure date folder exists
            var parent = _settings.GoogleDriveParentFolderId!;
            var folderId = await EnsureFolderAsync(svc, parent, dateFolderName);

            if (System.IO.File.Exists(localFolder))
            {
                // Upload a single file (.zip, .sql, .txt, etc.)
                var ext = System.IO.Path.GetExtension(localFolder);
                var mime = GetMimeType(ext);
                var name = System.IO.Path.GetFileName(localFolder);
                var meta = new DriveFile { Name = name, Parents = new[] { folderId }, MimeType = mime };
                await using var stream = new System.IO.FileStream(localFolder, System.IO.FileMode.Open, System.IO.FileAccess.Read, System.IO.FileShare.Read);
                if (stream.CanSeek) stream.Position = 0;
                var req = svc.Files.Create(meta, stream, mime);
                req.SupportsAllDrives = true;
                req.Fields = "id,name,parents";
                var result = await req.UploadAsync();
                if (result.Status == Google.Apis.Upload.UploadStatus.Failed)
                {
                    var emsg = result.Exception?.Message ?? "Unknown error";
                    return (false, $"Upload failed: {emsg}");
                }
                var id = req.ResponseBody?.Id;
                if (!string.IsNullOrEmpty(id))
                    return (true, $"Uploaded {name} (id={id})");
                return (false, $"Upload finished without file id (status: {result.Status})");
            }
            else
            {
                int uploaded = 0;
                int failed = 0;
                string? lastErr = null;
                foreach (var file in System.IO.Directory.EnumerateFiles(localFolder, "*.sql"))
                {
                    try
                    {
                        var meta = new DriveFile
                        {
                            Name = System.IO.Path.GetFileName(file),
                            Parents = new[] { folderId },
                            MimeType = "application/sql"
                        };
                        await using var stream = new System.IO.FileStream(file, System.IO.FileMode.Open, System.IO.FileAccess.Read, System.IO.FileShare.Read);
                        if (stream.CanSeek) stream.Position = 0;
                        var req = svc.Files.Create(meta, stream, "application/sql");
                        req.SupportsAllDrives = true;
                        req.Fields = "id";
                        var progress = await req.UploadAsync();
                        if (progress.Status == Google.Apis.Upload.UploadStatus.Failed)
                        {
                            failed++;
                            lastErr = progress.Exception?.Message ?? lastErr;
                        }
                        else if (req.ResponseBody != null && !string.IsNullOrEmpty(req.ResponseBody.Id))
                        {
                            uploaded++;
                        }
                    }
                    catch (Exception ex)
                    {
                        failed++;
                        lastErr = ex.Message;
                    }
                }
                if (uploaded > 0 && failed == 0)
                    return (true, $"Uploaded {uploaded} file(s) to Google Drive");
                if (uploaded > 0 && failed > 0)
                    return (true, $"Uploaded {uploaded} file(s), {failed} failed. Last error: {lastErr}");
                return (false, $"No files uploaded. Last error: {lastErr ?? "Unknown"}");
            }
        }
        catch (Google.GoogleApiException gex)
        {
            return (false, $"GoogleApiException: {gex.Error?.Message ?? gex.Message}");
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }

    private static string GetMimeType(string ext)
    {
        if (string.IsNullOrWhiteSpace(ext)) return "application/octet-stream";
        ext = ext.ToLowerInvariant();
        return ext switch
        {
            ".zip" => "application/zip",
            ".sql" => "application/sql",
            ".txt" => "text/plain",
            ".json" => "application/json",
            _ => "application/octet-stream"
        };
    }

    private async Task<string> EnsureFolderAsync(DriveService svc, string parentId, string name)
    {
        // Try find existing
        var listReq = svc.Files.List();
        listReq.Q = $"mimeType='application/vnd.google-apps.folder' and name='{EscapeQuery(name)}' and '{parentId}' in parents and trashed=false";
        listReq.Fields = "files(id,name)";
        listReq.SupportsAllDrives = true;
        listReq.IncludeItemsFromAllDrives = true;
        var list = await listReq.ExecuteAsync();
        var existing = list.Files?.FirstOrDefault();
        if (existing != null) return existing.Id;

        // Create new
        var fileMeta = new DriveFile
        {
            Name = name,
            MimeType = "application/vnd.google-apps.folder",
            Parents = new[] { parentId }
        };
        var create = svc.Files.Create(fileMeta);
        create.SupportsAllDrives = true;
        create.Fields = "id";
        var created = await create.ExecuteAsync();
        if (created == null || string.IsNullOrEmpty(created.Id))
            throw new Exception("Failed to create folder on Drive");
        return created.Id;
    }

    private static string EscapeQuery(string s) => s?.Replace("'", "\\'") ?? string.Empty;
}
