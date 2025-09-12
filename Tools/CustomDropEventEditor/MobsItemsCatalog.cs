using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace CustomDropEventEditor
{
    public static class MobsItemsCatalog
    {
        static readonly Regex RxAddMob = new(@"^\s*@?addmob\s+(\d+)\s*[:\-]?\s*(.*)$", RegexOptions.IgnoreCase | RegexOptions.Compiled);
        static readonly Regex RxAddItem = new(@"^\s*@?additem\s+(\d+)\b\s*(.*)$", RegexOptions.IgnoreCase | RegexOptions.Compiled);
        static readonly Regex RxTwoNumsName = new(@"^\s*(\d+)\s*:\s*(\d+)\s*:\s*(.*)$", RegexOptions.Compiled);
        static readonly Regex RxIdDashName = new(@"^\s*(\d+)\s*[:\-]\s*(.*)$", RegexOptions.Compiled);

        public static Dictionary<uint, string> ParseMobs(string path)
        {
            var result = new Dictionary<uint, string>();
            if (!File.Exists(path)) return result;
            foreach (var raw in File.ReadLines(path))
            {
                var line = raw.Trim();
                if (string.IsNullOrWhiteSpace(line) || line.StartsWith("#")) continue;

                // @addmob <id> - name | @addmob <id>: name
                var mAdd = RxAddMob.Match(line);
                if (mAdd.Success)
                {
                    if (uint.TryParse(mAdd.Groups[1].Value, out var id))
                    {
                        var name = mAdd.Groups[2].Value.Trim();
                        SafeAdd(result, id, name);
                        continue;
                    }
                }

                // <num>:<id>:name (take the second number as id)
                var mTwo = RxTwoNumsName.Match(line);
                if (mTwo.Success)
                {
                    if (uint.TryParse(mTwo.Groups[2].Value, out var id))
                    {
                        var name = mTwo.Groups[3].Value.Trim();
                        SafeAdd(result, id, name);
                        continue;
                    }
                }

                // <id> : name  OR  <id> - name
                var mIdName = RxIdDashName.Match(line);
                if (mIdName.Success)
                {
                    if (uint.TryParse(mIdName.Groups[1].Value, out var id))
                    {
                        var name = mIdName.Groups[2].Value.Trim();
                        SafeAdd(result, id, name);
                        continue;
                    }
                }
            }
            return result;
        }

        public static Dictionary<uint, string> ParseItems(string path)
        {
            var result = new Dictionary<uint, string>();
            if (!File.Exists(path)) return result;
            foreach (var raw in File.ReadLines(path))
            {
                var line = raw.Trim();
                if (string.IsNullOrWhiteSpace(line) || line.StartsWith("#")) continue;

                var mAdd = RxAddItem.Match(line);
                if (mAdd.Success)
                {
                    if (uint.TryParse(mAdd.Groups[1].Value, out var id))
                    {
                        var name = mAdd.Groups[2].Value.Trim();
                        SafeAdd(result, id, name);
                        continue;
                    }
                }

                var mIdName = RxIdDashName.Match(line);
                if (mIdName.Success)
                {
                    if (uint.TryParse(mIdName.Groups[1].Value, out var id))
                    {
                        var name = mIdName.Groups[2].Value.Trim();
                        SafeAdd(result, id, name);
                        continue;
                    }
                }
            }
            return result;
        }

        private static void SafeAdd(Dictionary<uint, string> dict, uint id, string name)
        {
            if (!dict.ContainsKey(id))
                dict[id] = name;
            else if (!string.IsNullOrWhiteSpace(name) && dict[id].Length < name.Length)
                dict[id] = name; // keep the longest non-empty name
        }
    }
}
