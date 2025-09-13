using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Xml;

namespace CustomDropEventEditor
{
    public sealed class VisualSkillCatalog
    {
        public sealed class Skill
        {
            public uint Id;
            public string Name = string.Empty;
            public bool KeepFlag;
            public uint KeepTimeMs;
            public List<uint> EffectTblidx = new();
        }

        public static Dictionary<uint, Skill> ParseSkills(string xmlPath)
        {
            var result = new Dictionary<uint, Skill>();
            if (!File.Exists(xmlPath)) return result;

            var doc = new XmlDocument();
            doc.Load(xmlPath);

            // Heuristic: look for rows under any root; rows may be <Table> <Row .../> or <Skill> elements
            foreach (XmlNode node in doc.SelectNodes("//*[self::Row or self::row or self::Skill or self::skill]")!)
            {
                try
                {
                    // Try a few common attribute names for skill id
                    var idAttr = node.Attributes?["tblidx"] ?? node.Attributes?["id"] ?? node.Attributes?["Tblidx"]; 
                    if (idAttr == null) continue;
                    if (!uint.TryParse(idAttr.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var id)) continue;

                    var s = new Skill { Id = id };

                    // Skill name: various field names
                    s.Name = node.Attributes?["wszNameText"]?.Value
                             ?? node.Attributes?["Skill_Name"]?.Value
                             ?? node.Attributes?["Name"]?.Value
                             ?? string.Empty;

                    // Keep flags/time
                    s.KeepFlag = ReadBoolAttr(node, "bKeep_Effect") || ReadBoolAttr(node, "KeepEffect") || ReadBoolAttr(node, "bKeepEffect");
                    s.KeepTimeMs = ReadUIntAttr(node, "dwKeepTimeInMilliSecs");
                    if (s.KeepTimeMs == 0)
                    {
                        var wKeep = ReadUIntAttr(node, "wKeep_Time");
                        if (wKeep > 0) s.KeepTimeMs = wKeep * 1000u;
                    }

                    // Effects: allow multiple naming conventions
                    var effects = new List<uint>();
                    foreach (var key in new[] { "skill_Effect_0", "skill_Effect_1", "skill_Effect_2", "skill_Effect" })
                    {
                        var a = node.Attributes?[key];
                        if (a != null && uint.TryParse(a.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var e))
                            effects.Add(e);
                    }
                    // Also check attributes if present (Attributes can be null)
                    var attrs = node.Attributes;
                    if (attrs != null)
                    {
                        foreach (XmlAttribute a in attrs)
                        {
                            if (a.Name.StartsWith("skill_Effect", StringComparison.OrdinalIgnoreCase)
                                && uint.TryParse(a.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var e))
                                effects.Add(e);
                        }
                    }
                    s.EffectTblidx = effects.Distinct().Where(e => e != 0).ToList();

                    result[id] = s;
                }
                catch { /* skip row on error */ }
            }
            return result;
        }

        public static HashSet<uint> ParseSystemEffectsWithKeep(string xmlPath)
        {
            var set = new HashSet<uint>();
            if (!File.Exists(xmlPath)) return set;
            var doc = new XmlDocument();
            doc.Load(xmlPath);
            foreach (XmlNode node in doc.SelectNodes("//*[self::Row or self::row or self::SystemEffect or self::system_effect]")!)
            {
                try
                {
                    var idAttr = node.Attributes?["tblidx"] ?? node.Attributes?["id"] ?? node.Attributes?["Tblidx"]; 
                    if (idAttr == null) continue;
                    if (!uint.TryParse(idAttr.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var id)) continue;
                    // A keep visual is indicated by a non-zero keep effect name index; different dumps use different field names
                    var keepName = ReadIntAttr(node, "nKeepEffectName");
                    if (keepName == 0)
                    {
                        keepName = ReadIntAttr(node, "KeepEffectName");
                    }
                    if (keepName != 0)
                        set.Add(id);
                }
                catch { }
            }
            return set;
        }

        public static List<Skill> FilterVisualSkills(Dictionary<uint, Skill> skills, HashSet<uint> keepEffects)
        {
            // Prefer skills referencing system effects with keep visuals; fallback to keep flag/time
            return skills.Values.Where(s =>
                (s.EffectTblidx.Any(e => keepEffects.Contains(e))) ||
                (s.KeepFlag && s.KeepTimeMs > 0)).OrderBy(s => s.Id).ToList();
        }

        private static bool ReadBoolAttr(XmlNode n, string name)
        {
            var a = n.Attributes?[name];
            if (a == null) return false;
            if (bool.TryParse(a.Value, out var b)) return b;
            if (int.TryParse(a.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var i)) return i != 0;
            return false;
        }

        private static uint ReadUIntAttr(XmlNode n, string name)
        {
            var a = n.Attributes?[name];
            if (a == null) return 0u;
            if (uint.TryParse(a.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var v)) return v;
            return 0u;
        }

        private static int ReadIntAttr(XmlNode n, string name)
        {
            var a = n.Attributes?[name];
            if (a == null) return 0;
            if (int.TryParse(a.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var v)) return v;
            return 0;
        }
    }
}
