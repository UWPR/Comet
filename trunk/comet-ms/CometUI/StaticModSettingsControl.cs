using System;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class StaticModSettingsControl : UserControl
    {
        private new Form Parent { get; set; }
        public StringCollection StaticMods { get; set; }

        public StaticModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            foreach (var row in StaticMods)
            {
                string[] cells = row.Split(',');
                staticModsDataGridView.Rows.Add(cells);
            }
        }

        private void InitializeFromDefaultSettings()
        {
            StaticMods = new StringCollection();
            foreach (var item in Settings.Default.StaticMods)
            {
                StaticMods.Add(item);
            }

            staticNTermPeptideTextBox.Text = Settings.Default.StaticModNTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticCTermPeptideTextBox.Text = Settings.Default.StaticModCTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticNTermProteinTextBox.Text = Settings.Default.StaticModNTermProtein.ToString(CultureInfo.InvariantCulture);
            staticCTermProteinTextBox.Text = Settings.Default.StaticModCTermProtein.ToString(CultureInfo.InvariantCulture);
        }
    }
}
