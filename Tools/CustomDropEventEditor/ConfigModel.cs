using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace CustomDropEventEditor
{
    public sealed class ConfigModel
    {
        public Dictionary<uint, List<DropEntry>> Drops { get; } = new();
        public Dictionary<uint, Modifiers> Mods { get; } = new();
        public Dictionary<uint, List<SpawnEntry>> Spawns { get; } = new(); // key 0 = all
        public Dictionary<uint, List<BuffEntry>> Buffs { get; } = new(); // key 0 = all
        public Dictionary<uint, List<uint>> Titles { get; } = new(); // key 0 = all
    public Dictionary<uint, List<VisualEntry>> Visuals { get; } = new(); // key 0 = all

        public static ConfigModel Load(string path)
        {
            var model = new ConfigModel();
            if (!File.Exists(path)) return model;
            foreach (var raw in File.ReadAllLines(path))
            {
                var line = raw.Trim();
                if (string.IsNullOrWhiteSpace(line)) continue;
                // strip comments (#, ;, //) anywhere in the line
                int c1 = line.IndexOf('#');
                int c2 = line.IndexOf(';');
                int c3 = line.IndexOf("//", StringComparison.Ordinal);
                int cut = -1;
                foreach (var c in new[] { c1, c2, c3 })
                {
                    if (c >= 0) cut = cut < 0 ? c : Math.Min(cut, c);
                }
                if (cut >= 0) line = line[..cut].Trim();
                if (string.IsNullOrWhiteSpace(line)) continue;

                // support both ':' and '=' as key/value separator; take the first occurring
                int iColon = line.IndexOf(':');
                int iEq = line.IndexOf('=');
                int sep = -1;
                if (iColon >= 0 && iEq >= 0) sep = Math.Min(iColon, iEq);
                else if (iColon >= 0) sep = iColon; else sep = iEq;
                if (sep < 0) continue;
                var key = line.Substring(0, sep).Trim();
                var value = line[(sep + 1)..].Trim();

                // key could be "<id>", "<id> modifiers", "<id> spawn(s)", "<id> buffs", "<id> titles", or "<id> visuals"
                var parts = key.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                uint id = 0;
                bool isMods = false, isSpawn = false, isBuffs = false, isTitles = false, isVisuals = false;
                if (parts.Length == 1)
                {
                    if (parts[0].Equals("all", StringComparison.OrdinalIgnoreCase))
                        id = 0;
                    else if (!uint.TryParse(parts[0], out id))
                        continue;
                }
                else if (parts.Length >= 2)
                {
                    if (parts[0].Equals("all", StringComparison.OrdinalIgnoreCase))
                        id = 0;
                    else if (!uint.TryParse(parts[0], out id))
                        continue;
                    var tail = parts[1];
                    if (tail.Equals("modifiers", StringComparison.OrdinalIgnoreCase)) isMods = true;
                    else if (tail.Equals("spawn", StringComparison.OrdinalIgnoreCase) || tail.Equals("spawns", StringComparison.OrdinalIgnoreCase)) isSpawn = true;
                    else if (tail.Equals("buffs", StringComparison.OrdinalIgnoreCase)) isBuffs = true;
                    else if (tail.Equals("titles", StringComparison.OrdinalIgnoreCase)) isTitles = true;
                    else if (tail.Equals("visuals", StringComparison.OrdinalIgnoreCase)) isVisuals = true;
                }

                if (isMods)
                {
                    var m = Modifiers.Parse(value);
                    model.Mods[id] = m;
                }
                else if (isSpawn)
                {
                    var list = new List<SpawnEntry>();
                    foreach (var tok in value.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint mob = 0; float rate = 100f; byte count = 1;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out mob)) continue;
                            var rx = t[(at + 1)..].Trim();
                            // allow 'x' or 'X' for count
                            var x = rx.IndexOf('x');
                            if (x < 0) x = rx.IndexOf('X');
                            // allow trailing % in rate (e.g., 50%)
                            if (rx.EndsWith("%", StringComparison.Ordinal)) rx = rx[..^1];
                            if (x >= 0)
                            {
                                var rOnly = rx[..x].Trim();
                                if (rOnly.EndsWith("%", StringComparison.Ordinal)) rOnly = rOnly[..^1];
                                if (!float.TryParse(rOnly, NumberStyles.Float, CultureInfo.InvariantCulture, out rate)) rate = 100f;
                                if (!byte.TryParse(rx[(x + 1)..], out count)) count = 1;
                            }
                            else
                            {
                                if (!float.TryParse(rx, NumberStyles.Float, CultureInfo.InvariantCulture, out rate)) rate = 100f;
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out mob)) continue;
                        }
                        list.Add(new SpawnEntry { MobTblidx = mob, Rate = rate, Count = count });
                    }
                    if (list.Count > 0)
                    {
                        if (model.Spawns.TryGetValue(id, out var existing) && existing != null)
                            existing.AddRange(list);
                        else
                            model.Spawns[id] = list;
                    }
                }
                else if (isBuffs)
                {
                    var list = new List<BuffEntry>();
                    foreach (var tok in value.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint skill = 0; uint durationMs = 0;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out skill)) continue;
                            var dv = t[(at + 1)..].Trim();
                            // allow trailing ms or s suffixes (e.g., 30s, 60000)
                            if (dv.EndsWith("ms", StringComparison.OrdinalIgnoreCase)) dv = dv[..^2];
                            if (dv.EndsWith("s", StringComparison.OrdinalIgnoreCase))
                            {
                                if (uint.TryParse(dv[..^1], out var secs)) durationMs = secs * 1000u; else durationMs = 0;
                            }
                            else
                            {
                                uint.TryParse(dv, out durationMs);
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out skill)) continue;
                        }
                        list.Add(new BuffEntry { SkillTblidx = skill, DurationMs = durationMs });
                    }
                    if (list.Count > 0)
                    {
                        if (model.Buffs.TryGetValue(id, out var existing) && existing != null)
                            existing.AddRange(list);
                        else
                            model.Buffs[id] = list;
                    }
                }
                else if (isTitles)
                {
                    var list = new List<uint>();
                    foreach (var tok in value.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        if (uint.TryParse(t, out var titleId)) list.Add(titleId);
                    }
                    if (list.Count > 0)
                    {
                        if (model.Titles.TryGetValue(id, out var existing) && existing != null)
                            existing.AddRange(list);
                        else
                            model.Titles[id] = list;
                    }
                }
                else if (isVisuals)
                {
                    var list = new List<VisualEntry>();
                    foreach (var tok in value.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint effectTblidx = 0; uint intervalMs = 0;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out effectTblidx)) continue;
                            var dv = t[(at + 1)..].Trim();
                            if (dv.EndsWith("ms", StringComparison.OrdinalIgnoreCase)) dv = dv[..^2];
                            if (dv.EndsWith("s", StringComparison.OrdinalIgnoreCase))
                            {
                                if (uint.TryParse(dv[..^1], out var secs)) intervalMs = secs * 1000u; else intervalMs = 0;
                            }
                            else
                            {
                                uint.TryParse(dv, out intervalMs);
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out effectTblidx)) continue;
                        }
                        list.Add(new VisualEntry { EffectTblidx = effectTblidx, IntervalMs = intervalMs });
                    }
                    if (list.Count > 0)
                    {
                        if (model.Visuals.TryGetValue(id, out var existing) && existing != null)
                            existing.AddRange(list);
                        else
                            model.Visuals[id] = list;
                    }
                }
                else
                {
                    var list = new List<DropEntry>();
                    foreach (var tok in value.Split(','))
                    {
                        var t = tok.Trim();
                        if (string.IsNullOrEmpty(t)) continue;
                        var at = t.IndexOf('@');
                        uint item = 0; float rate = 100f; byte count = 1;
                        if (at >= 0)
                        {
                            if (!uint.TryParse(t[..at].Trim(), out item)) continue;
                            var rv = t[(at + 1)..].Trim();
                            // allow trailing % in rate and optional xcount
                            var x = rv.IndexOf('x');
                            if (x < 0) x = rv.IndexOf('X');
                            if (x >= 0)
                            {
                                var rOnly = rv[..x].Trim();
                                if (rOnly.EndsWith("%", StringComparison.Ordinal)) rOnly = rOnly[..^1];
                                if (!float.TryParse(rOnly, NumberStyles.Float, CultureInfo.InvariantCulture, out rate)) rate = 100f;
                                if (!byte.TryParse(rv[(x + 1)..], out count)) count = 1;
                            }
                            else
                            {
                                if (rv.EndsWith("%", StringComparison.Ordinal)) rv = rv[..^1];
                                if (!float.TryParse(rv, NumberStyles.Float, CultureInfo.InvariantCulture, out rate)) rate = 100f;
                            }
                        }
                        else
                        {
                            if (!uint.TryParse(t, out item)) continue;
                        }
                        list.Add(new DropEntry { ItemTblidx = item, Rate = rate, Count = count });
                    }
                    if (list.Count > 0) model.Drops[id] = list;
                }
            }
            return model;
        }

        public void Save(string path)
        {
            using var sw = new StreamWriter(path);
            sw.WriteLine("# CustomDropEvent configuration");
            sw.WriteLine("# Generated by CustomDropEventEditor");
            // Global spawns first (id 0) — write as a single line with comma-separated entries
            if (Spawns.TryGetValue(0, out var global))
            {
                sw.Write("all spawn: ");
                WriteSpawnList(sw, global);
            }
            // Global buffs next (id 0)
            if (Buffs.TryGetValue(0, out var gBuffs))
            {
                sw.Write("all buffs: ");
                WriteBuffList(sw, gBuffs);
            }
            // Global visuals (id 0)
            if (Visuals.TryGetValue(0, out var gVisuals))
            {
                sw.Write("all visuals: ");
                WriteVisualList(sw, gVisuals);
            }
            // Global titles (id 0)
            if (Titles.TryGetValue(0, out var gTitles))
            {
                sw.Write("all titles: ");
                WriteTitleList(sw, gTitles);
            }
            // Mods
            foreach (var (id, m) in Mods.OrderBy(k => k.Key))
            {
                if (id == 0) continue;
                sw.Write($"{id} modifiers: ");
                sw.WriteLine(m.ToString());
            }
            // Drops
            foreach (var (id, list) in Drops.OrderBy(k => k.Key))
            {
                sw.Write($"{id}: ");
                sw.WriteLine(string.Join(
                    ", ",
                    list.Select(e =>
                    {
                        var needRate = e.Rate < 100f || e.Count > 1;
                        var rate = needRate ? $"@{e.Rate.ToString(CultureInfo.InvariantCulture)}" : string.Empty;
                        var cnt = e.Count > 1 ? $"x{e.Count}" : string.Empty;
                        if (rate.Length == 0 && cnt.Length == 0)
                            rate = "@100"; // explicit default
                        return $"{e.ItemTblidx}{rate}{cnt}";
                    })));
            }
            // Spawns
            foreach (var (id, list) in Spawns.OrderBy(k => k.Key))
            {
                if (id == 0) continue; // already wrote global
                sw.Write($"{id} spawn: ");
                WriteSpawnList(sw, list);
            }
            // Buffs
            foreach (var (id, list) in Buffs.OrderBy(k => k.Key))
            {
                if (id == 0) continue; // already wrote global
                sw.Write($"{id} buffs: ");
                WriteBuffList(sw, list);
            }
            // Visuals
            foreach (var (id, list) in Visuals.OrderBy(k => k.Key))
            {
                if (id == 0) continue; // already wrote global
                sw.Write($"{id} visuals: ");
                WriteVisualList(sw, list);
            }
            // Titles
            foreach (var (id, list) in Titles.OrderBy(k => k.Key))
            {
                if (id == 0) continue; // already wrote global
                sw.Write($"{id} titles: ");
                WriteTitleList(sw, list);
            }
        }

        private static void WriteSpawnList(StreamWriter sw, List<SpawnEntry> list)
        {
            sw.WriteLine(string.Join(
                ", ",
                list.Select(e =>
                {
                    var rate = e.Rate >= 100f ? "" : $"@{e.Rate.ToString(CultureInfo.InvariantCulture)}";
                    var cnt = e.Count > 1 ? $"x{e.Count}" : string.Empty;
                    if (rate.Length == 0 && cnt.Length == 0)
                        rate = "@100"; // explicit
                    return $"{e.MobTblidx}{rate}{cnt}";
                })));
        }

        private static void WriteBuffList(StreamWriter sw, List<BuffEntry> list)
        {
            sw.WriteLine(string.Join(
                ", ",
                list.Select(e => e.DurationMs > 0 ? $"{e.SkillTblidx}@{e.DurationMs}" : $"{e.SkillTblidx}")));
        }

        private static void WriteTitleList(StreamWriter sw, List<uint> list)
        {
            sw.WriteLine(string.Join(
                ", ",
                list.Select(id => id.ToString(CultureInfo.InvariantCulture))));
        }

        private static void WriteVisualList(StreamWriter sw, List<VisualEntry> list)
        {
            sw.WriteLine(string.Join(
                ", ",
                list.Select(e => e.IntervalMs > 0 ? $"{e.EffectTblidx}@{e.IntervalMs}" : $"{e.EffectTblidx}")));
        }
    }

    public sealed class BuffEntry
    {
        public uint SkillTblidx { get; set; }
        public uint DurationMs { get; set; } // 0 = default
    }

    public sealed class DropEntry
    {
        public uint ItemTblidx { get; set; }
        public float Rate { get; set; }
        public byte Count { get; set; }
    }

    public sealed class SpawnEntry
    {
        public uint MobTblidx { get; set; }
        public float Rate { get; set; }
        public byte Count { get; set; }
    }

    public sealed class VisualEntry
    {
        public uint EffectTblidx { get; set; }
        public uint IntervalMs { get; set; } // 0 = on-spawn only
    }

    public sealed class Modifiers
    {
        public float Hp { get; set; } = 1f;
        public float PhysAtk { get; set; } = 1f;
        public float EngAtk { get; set; } = 1f;
        public float PhysDef { get; set; } = 1f;
        public float EngDef { get; set; } = 1f;
        public float AtkSpd { get; set; } = 1f;
        public float RunSpd { get; set; } = 1f;
        public float PhysCrit { get; set; } = 1f;
        public float EngCrit { get; set; } = 1f;
        public float PhysCritDmg { get; set; } = 1f;
        public float EngCritDmg { get; set; } = 1f;
        public float AttackRate { get; set; } = 1f;
        public float DodgeRate { get; set; } = 1f;
        public float BlockRate { get; set; } = 1f;
        public float BlockDmg { get; set; } = 1f;
        public float GuardRate { get; set; } = 1f;
    public int SizeRate { get; set; } = 10;

        public static Modifiers Parse(string value)
        {
            var m = new Modifiers();
            var toks = value.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var tok in toks)
            {
                var kv = tok.Split('=');
                if (kv.Length != 2) continue;
                var key = kv[0];
                var vs = kv[1];
                float vf = 1f; int vi = 0;
                float.TryParse(vs, NumberStyles.Float, CultureInfo.InvariantCulture, out vf);
                int.TryParse(vs, NumberStyles.Integer, CultureInfo.InvariantCulture, out vi);
                switch (key)
                {
                    case "hp": m.Hp = vf; break;
                    case "physAtk": m.PhysAtk = vf; break;
                    case "engAtk": m.EngAtk = vf; break;
                    case "physDef": m.PhysDef = vf; break;
                    case "engDef": m.EngDef = vf; break;
                    case "atkSpd": m.AtkSpd = vf; break;
                    case "runSpd": m.RunSpd = vf; break;
                    case "physCrit": m.PhysCrit = vf; break;
                    case "engCrit": m.EngCrit = vf; break;
                    case "physCritDmg": m.PhysCritDmg = vf; break;
                    case "engCritDmg": m.EngCritDmg = vf; break;
                    case "attackRate": m.AttackRate = vf; break;
                    case "dodgeRate": m.DodgeRate = vf; break;
                    case "blockRate": m.BlockRate = vf; break;
                    case "blockDmg": m.BlockDmg = vf; break;
                    case "guardRate": m.GuardRate = vf; break;
                    case "sizeRate": m.SizeRate = vi; break;
                }
            }
            return m;
        }

        public override string ToString()
        {
            return string.Join(" ", new[]
            {
                $"hp={Hp.ToString(CultureInfo.InvariantCulture)}",
                $"physAtk={PhysAtk.ToString(CultureInfo.InvariantCulture)}",
                $"engAtk={EngAtk.ToString(CultureInfo.InvariantCulture)}",
                $"physDef={PhysDef.ToString(CultureInfo.InvariantCulture)}",
                $"engDef={EngDef.ToString(CultureInfo.InvariantCulture)}",
                $"atkSpd={AtkSpd.ToString(CultureInfo.InvariantCulture)}",
                $"runSpd={RunSpd.ToString(CultureInfo.InvariantCulture)}",
                $"physCrit={PhysCrit.ToString(CultureInfo.InvariantCulture)}",
                $"engCrit={EngCrit.ToString(CultureInfo.InvariantCulture)}",
                $"physCritDmg={PhysCritDmg.ToString(CultureInfo.InvariantCulture)}",
                $"engCritDmg={EngCritDmg.ToString(CultureInfo.InvariantCulture)}",
                $"attackRate={AttackRate.ToString(CultureInfo.InvariantCulture)}",
                $"dodgeRate={DodgeRate.ToString(CultureInfo.InvariantCulture)}",
                $"blockRate={BlockRate.ToString(CultureInfo.InvariantCulture)}",
                $"blockDmg={BlockDmg.ToString(CultureInfo.InvariantCulture)}",
                $"guardRate={GuardRate.ToString(CultureInfo.InvariantCulture)}",
                $"sizeRate={SizeRate.ToString(CultureInfo.InvariantCulture)}",
            });
        }
    }
}
