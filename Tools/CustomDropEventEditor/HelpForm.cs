using System;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace CustomDropEventEditor
{
    public sealed class HelpForm : Form
    {
        private readonly WebBrowser _browser;
        private readonly string _readmePath;

        public HelpForm(string readmePath)
        {
            _readmePath = readmePath;
            Text = "Custom Drop Event - Help";
            StartPosition = FormStartPosition.CenterParent;
            Width = 900;
            Height = 700;
            BackColor = Color.FromArgb(32, 33, 36);
            ForeColor = Color.Gainsboro;
            Font = new Font("Segoe UI", 10f);

            _browser = new WebBrowser
            {
                Dock = DockStyle.Fill,
                AllowWebBrowserDrop = false,
                IsWebBrowserContextMenuEnabled = false,
                WebBrowserShortcutsEnabled = true
            };

            Controls.Add(_browser);

            Load += (s, e) => Render();
            KeyPreview = true;
            KeyDown += (s, e) => { if (e.KeyCode == Keys.Escape) Close(); };
        }

        private void Render()
        {
            try
            {
                if (!File.Exists(_readmePath))
                {
                    _browser.DocumentText = WrapHtml("<h2>README not found</h2><p>Place <code>README_CustomDropEvent.md</code> next to the executable or project root.</p>");
                    return;
                }
                var md = File.ReadAllText(_readmePath, Encoding.UTF8);
                var html = MarkdownToSimpleHtml(md);
                _browser.DocumentText = WrapHtml(html);
            }
            catch (Exception ex)
            {
                _browser.DocumentText = WrapHtml($"<h2>Error</h2><pre>{System.Net.WebUtility.HtmlEncode(ex.Message)}</pre>");
            }
        }

        // Very small Markdown -> HTML conversion for basic docs
        private static string MarkdownToSimpleHtml(string md)
        {
            var sb = new StringBuilder();
            using var reader = new StringReader(md);
            string? line;
            bool inCode = false;
            while ((line = reader.ReadLine()) != null)
            {
                if (line.StartsWith("```"))
                {
                    inCode = !inCode;
                    sb.AppendLine(inCode ? "<pre><code>" : "</code></pre>");
                    continue;
                }
                if (inCode)
                {
                    sb.AppendLine(System.Net.WebUtility.HtmlEncode(line));
                    continue;
                }
                if (line.StartsWith("# ")) sb.AppendLine($"<h1>{Escape(line.Substring(2))}</h1>");
                else if (line.StartsWith("## ")) sb.AppendLine($"<h2>{Escape(line.Substring(3))}</h2>");
                else if (line.StartsWith("### ")) sb.AppendLine($"<h3>{Escape(line.Substring(4))}</h3>");
                else if (line.StartsWith("- ")) sb.AppendLine($"<li>{Escape(line.Substring(2))}</li>");
                else if (string.IsNullOrWhiteSpace(line)) sb.AppendLine("<br/>");
                else sb.AppendLine($"<p>{Escape(line)}</p>");
            }
            var html = sb.ToString();
            // Wrap lone <li> into <ul> blocks (simple pass)
            html = html.Replace("<li>", "<ul><li>").Replace("</li>", "</li></ul>");
            // code spans `text`
            html = System.Text.RegularExpressions.Regex.Replace(html, "`([^`]+)`", m => $"<code>{Escape(m.Groups[1].Value)}</code>");
            // basic links [text](url)
            html = System.Text.RegularExpressions.Regex.Replace(html, "\\[([^\\]]+)\\]\\(([^)]+)\\)", m => $"<a href=\"{Escape(m.Groups[2].Value)}\" target=\"_blank\">{Escape(m.Groups[1].Value)}</a>");
            return html;
        }

        private static string Escape(string s) => System.Net.WebUtility.HtmlEncode(s);

        private static string WrapHtml(string body)
        {
            var css = @"body{background:#202124;color:#e8eaed;font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:16px;}
            h1,h2,h3{color:#e8eaed;margin-top:1.2em}
            code,pre{background:#2d2f34;color:#e8eaed;border-radius:4px}
            pre{padding:12px;overflow:auto}
            p,li{line-height:1.5}
            a{color:#8ab4f8}
            ul{margin:0 0 0 20px;padding:0}
            ";
            return $"<html><head><meta charset='utf-8'><style>{css}</style></head><body>{body}</body></html>";
        }
    }
}
