using System;
using System.Windows.Forms;

namespace RdfTableEditor.UI
{
    // A ToolStripLabel that springs to take up available space (for right-aligning items)
    internal class ToolStripSpringLabel : ToolStripLabel
    {
        public override bool CanSelect => false;
        public override System.Drawing.Size GetPreferredSize(System.Drawing.Size constrainingSize)
        {
            if (Owner is not ToolStrip owner) return base.GetPreferredSize(constrainingSize);
            // Compute available width by subtracting siblings
            int width = owner.DisplayRectangle.Width;
            foreach (ToolStripItem item in owner.Items)
            {
                if (item == this) continue;
                if (item.Alignment == ToolStripItemAlignment.Right) continue;
                width -= item.Margin.Horizontal + item.Width;
            }
            width = Math.Max(0, width - Margin.Horizontal);
            var size = base.GetPreferredSize(constrainingSize);
            return new System.Drawing.Size(width, size.Height);
        }
    }
}
