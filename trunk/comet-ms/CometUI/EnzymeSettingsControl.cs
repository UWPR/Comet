using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI
{
    public partial class EnzymeSettingsControl : UserControl
    {
        private new Form  Parent { get; set; }
        private readonly Dictionary<string, int> _enzymeTermini = new Dictionary<string, int>();

        public EnzymeSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            _enzymeTermini.Add("Fully-digested", 2);
            _enzymeTermini.Add("Semi-digested", 1);
            _enzymeTermini.Add("N-term", 8);
            _enzymeTermini.Add("C-term", 9);

            Dictionary<string, int>.KeyCollection enzymeTerminiKeys = _enzymeTermini.Keys;
            foreach (var key in enzymeTerminiKeys)
            {
                enzymeTerminiCombo.Items.Add(key);
            }

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            int enzymeTerminiDefaultIndex = enzymeTerminiCombo.FindString(Settings.Default.EnzymeTermini);
            enzymeTerminiCombo.SelectedIndex = enzymeTerminiDefaultIndex;

            // For this particular combo, index == value of allowed missed cleavages
            missedCleavagesCombo.SelectedIndex = Settings.Default.AllowedMissedCleavages;
        }
    }
}
