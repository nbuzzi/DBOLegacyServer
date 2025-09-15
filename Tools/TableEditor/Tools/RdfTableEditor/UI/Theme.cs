using System.Drawing;
using System.Windows.Forms;

namespace RdfTableEditor.UI
{
    internal static class Theme
    {
        public static readonly Color Bg = Color.FromArgb(32, 34, 37);
        public static readonly Color Bg2 = Color.FromArgb(44, 47, 51);
    public static readonly Color PanelBg = Color.FromArgb(54, 57, 63);
        public static readonly Color Border = Color.FromArgb(64, 68, 75);
        public static readonly Color Text = Color.FromArgb(220, 221, 222);
        public static readonly Color SubText = Color.FromArgb(170, 170, 170);
        public static readonly Color Accent = Color.FromArgb(0, 119, 200);

        public static void ApplyDarkTheme(Form form)
        {
            form.BackColor = Bg;
            form.ForeColor = Text;
            form.Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point);
            ApplyToControls(form.Controls);
        }

        private static void ApplyToControls(Control.ControlCollection controls)
        {
            foreach (Control c in controls)
            {
                switch (c)
                {
                    case MenuStrip ms:
                        ms.RenderMode = ToolStripRenderMode.Professional;
                        ms.Renderer = new ToolStripProfessionalRenderer(new DarkColorTable());
                        ms.BackColor = Bg2;
                        ms.ForeColor = Text;
                        break;
                    case StatusStrip ss:
                        ss.RenderMode = ToolStripRenderMode.Professional;
                        ss.Renderer = new ToolStripProfessionalRenderer(new DarkColorTable());
                        ss.BackColor = Bg2;
                        ss.ForeColor = SubText;
                        break;
                    case ToolStrip ts:
                        ts.RenderMode = ToolStripRenderMode.Professional;
                        ts.Renderer = new ToolStripProfessionalRenderer(new DarkColorTable());
                        ts.BackColor = Bg2;
                        ts.ForeColor = Text;
                        break;
                    case DataGridView dgv:
                        StyleGrid(dgv);
                        break;
                    default:
                        c.BackColor = (c is Panel or GroupBox) ? PanelBg : Bg;
                        c.ForeColor = Text;
                        break;
                }
                if (c.HasChildren) ApplyToControls(c.Controls);
            }
        }

        public static void StyleGrid(DataGridView grid)
        {
            grid.BackgroundColor = Bg;
            grid.EnableHeadersVisualStyles = false;
            grid.BorderStyle = BorderStyle.None;
            grid.GridColor = Border;
            grid.ColumnHeadersDefaultCellStyle.BackColor = Bg2;
            grid.ColumnHeadersDefaultCellStyle.ForeColor = Text;
            grid.ColumnHeadersDefaultCellStyle.SelectionBackColor = Bg2;
            grid.ColumnHeadersDefaultCellStyle.SelectionForeColor = Text;
            grid.DefaultCellStyle.BackColor = PanelBg;
            grid.DefaultCellStyle.ForeColor = Text;
            grid.DefaultCellStyle.SelectionBackColor = Color.FromArgb(75, 110, 175);
            grid.DefaultCellStyle.SelectionForeColor = Color.White;
            grid.RowHeadersVisible = false;
            grid.CellBorderStyle = DataGridViewCellBorderStyle.SingleHorizontal;
            grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.None;
            grid.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            grid.RowHeadersWidthSizeMode = DataGridViewRowHeadersWidthSizeMode.DisableResizing;
            EnableDoubleBuffer(grid);
        }

        private static void EnableDoubleBuffer(DataGridView grid)
        {
            typeof(DataGridView).InvokeMember("DoubleBuffered",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.SetProperty,
                null, grid, new object[] { true });
        }

        private sealed class DarkColorTable : ProfessionalColorTable
        {
            public override Color ToolStripDropDownBackground => Bg2;
            public override Color ImageMarginGradientBegin => Bg2;
            public override Color ImageMarginGradientMiddle => Bg2;
            public override Color ImageMarginGradientEnd => Bg2;
            public override Color MenuStripGradientBegin => Bg2;
            public override Color MenuStripGradientEnd => Bg2;
            public override Color MenuItemSelected => Color.FromArgb(60, 60, 60);
            public override Color MenuItemBorder => Border;
            public override Color MenuBorder => Border;
            public override Color SeparatorDark => Border;
            public override Color SeparatorLight => Border;
            public override Color ToolStripBorder => Border;
            public override Color ToolStripGradientBegin => Bg2;
            public override Color ToolStripGradientMiddle => Bg2;
            public override Color ToolStripGradientEnd => Bg2;
        }
    }
}
