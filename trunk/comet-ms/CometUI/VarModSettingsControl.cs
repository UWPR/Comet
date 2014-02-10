using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace CometUI
{
    public partial class VarModSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        public VarModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {

        }
    }
}
