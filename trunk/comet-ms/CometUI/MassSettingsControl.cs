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
    public partial class MassSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        public MassSettingsControl(Form parent)
        {
            InitializeComponent();

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
        }
    }
}
