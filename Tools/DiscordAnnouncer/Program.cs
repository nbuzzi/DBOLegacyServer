using System.Net.Http.Json;
using System.Text.Json;

class Program
{
    private const string PATCH_NOTES_WEBHOOK = "https://discord.com/api/webhooks/1423408648763215972/AFW7v9h3xtUS6Y3aqblz1Wp8QEwIC52K-Yw31uq46YvQSkt_-EDdarfpY-Y-MPuY8YQR";
    private const string ANNOUNCEMENTS_WEBHOOK = "https://discord.com/api/webhooks/1423408906578825336/AY_JharAl89hnJsYf9KKrdX3iRK5CnIFOk2y-XZTQyiyZY52_0A6hfsu1u1yY_uc97w0";

    static async Task<int> Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Discord Announcer Tool");
            Console.WriteLine("Usage: DiscordAnnouncer <type> <message|file>");
            Console.WriteLine("  type: 'patch' or 'announcement'");
            Console.WriteLine("  message: The message to send OR path to a text file");
            Console.WriteLine("\nExample: DiscordAnnouncer patch \"Fixed CCBD stage generation bug\"");
            Console.WriteLine("Example: DiscordAnnouncer patch @message.txt");
            return 1;
        }

        string type = args[0].ToLower();
        string message = "";

        if (args.Length > 1)
        {
            string arg = string.Join(" ", args.Skip(1));
            // Check if argument starts with @ (file reference)
            if (arg.StartsWith("@"))
            {
                string filePath = arg.Substring(1);
                if (File.Exists(filePath))
                {
                    message = await File.ReadAllTextAsync(filePath);
                }
                else
                {
                    Console.WriteLine($"Error: File not found: {filePath}");
                    return 1;
                }
            }
            else
            {
                message = arg;
            }
        }
        else
        {
            Console.WriteLine("Error: Message required");
            return 1;
        }

        string webhook = type switch
        {
            "patch" => PATCH_NOTES_WEBHOOK,
            "announcement" => ANNOUNCEMENTS_WEBHOOK,
            _ => null
        };

        if (webhook == null)
        {
            Console.WriteLine($"Error: Invalid type '{type}'. Use 'patch' or 'announcement'");
            return 1;
        }

        try
        {
            using var client = new HttpClient();
            var payload = new
            {
                content = message
            };

            var response = await client.PostAsJsonAsync(webhook, payload);

            if (response.IsSuccessStatusCode)
            {
                Console.WriteLine($"✓ Message sent successfully to {type}!");
                return 0;
            }
            else
            {
                Console.WriteLine($"✗ Error sending message: {response.StatusCode}");
                var error = await response.Content.ReadAsStringAsync();
                Console.WriteLine($"Details: {error}");
                return 1;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Exception: {ex.Message}");
            return 1;
        }
    }
}
